import functools
import json
import typing
import typing as t
import warnings
from io import BytesIO

from .._internal import _wsgi_decoding_dance
from ..datastructures import CombinedMultiDict
from ..datastructures import EnvironHeaders
from ..datastructures import FileStorage
from ..datastructures import ImmutableMultiDict
from ..datastructures import iter_multi_items
from ..datastructures import MultiDict
from ..formparser import default_stream_factory
from ..formparser import FormDataParser
from ..sansio.request import Request as _SansIORequest
from ..utils import cached_property
from ..utils import environ_property
from ..wsgi import _get_server
from ..wsgi import get_input_stream
from werkzeug.exceptions import BadRequest

if t.TYPE_CHECKING:
    import typing_extensions as te
    from _typeshed.wsgi import WSGIApplication
    from _typeshed.wsgi import WSGIEnvironment


class Request(_SansIORequest):
    """Represents an incoming WSGI HTTP request, with headers and body
    taken from the WSGI environment. Has properties and methods for
    using the functionality defined by various HTTP specs. The data in
    requests object is read-only.

    Text data is assumed to use UTF-8 encoding, which should be true for
    the vast majority of modern clients. Using an encoding set by the
    client is unsafe in Python due to extra encodings it provides, such
    as ``zip``. To change the assumed encoding, subclass and replace
    :attr:`charset`.

    :param environ: The WSGI environ is generated by the WSGI server and
        contains information about the server configuration and client
        request.
    :param populate_request: Add this request object to the WSGI environ
        as ``environ['werkzeug.request']``. Can be useful when
        debugging.
    :param shallow: Makes reading from :attr:`stream` (and any method
        that would read from it) raise a :exc:`RuntimeError`. Useful to
        prevent consuming the form data in middleware, which would make
        it unavailable to the final application.

    .. versionchanged:: 2.0
        Combine ``BaseRequest`` and mixins into a single ``Request``
        class. Using the old classes is deprecated and will be removed
        in Werkzeug 2.1.

    .. versionchanged:: 0.5
        Read-only mode is enforced with immutable classes for all data.
    """

    #: the maximum content length.  This is forwarded to the form data
    #: parsing function (:func:`parse_form_data`).  When set and the
    #: :attr:`form` or :attr:`files` attribute is accessed and the
    #: parsing fails because more than the specified value is transmitted
    #: a :exc:`~werkzeug.exceptions.RequestEntityTooLarge` exception is raised.
    #:
    #: Have a look at :doc:`/request_data` for more details.
    #:
    #: .. versionadded:: 0.5
    max_content_length: t.Optional[int] = None

    #: the maximum form field size.  This is forwarded to the form data
    #: parsing function (:func:`parse_form_data`).  When set and the
    #: :attr:`form` or :attr:`files` attribute is accessed and the
    #: data in memory for post data is longer than the specified value a
    #: :exc:`~werkzeug.exceptions.RequestEntityTooLarge` exception is raised.
    #:
    #: Have a look at :doc:`/request_data` for more details.
    #:
    #: .. versionadded:: 0.5
    max_form_memory_size: t.Optional[int] = None

    #: The maximum number of multipart parts to parse, passed to
    #: :attr:`form_data_parser_class`. Parsing form data with more than this
    #: many parts will raise :exc:`~.RequestEntityTooLarge`.
    #:
    #: .. versionadded:: 2.2.3
    max_form_parts = 1000

    #: The form data parser that shoud be used.  Can be replaced to customize
    #: the form date parsing.
    form_data_parser_class: t.Type[FormDataParser] = FormDataParser

    #: Disable the :attr:`data` property to avoid reading from the input
    #: stream.
    #:
    #: .. deprecated:: 2.0
    #:     Will be removed in Werkzeug 2.1. Create the request with
    #:     ``shallow=True`` instead.
    #:
    #: .. versionadded:: 0.9
    disable_data_descriptor: t.Optional[bool] = None

    #: The WSGI environment containing HTTP headers and information from
    #: the WSGI server.
    environ: "WSGIEnvironment"

    #: Set when creating the request object. If ``True``, reading from
    #: the request body will cause a ``RuntimeException``. Useful to
    #: prevent modifying the stream from middleware.
    shallow: bool

    def __init__(
        self,
        environ: "WSGIEnvironment",
        populate_request: bool = True,
        shallow: bool = False,
    ) -> None:
        super().__init__(
            method=environ.get("REQUEST_METHOD", "GET"),
            scheme=environ.get("wsgi.url_scheme", "http"),
            server=_get_server(environ),
            root_path=_wsgi_decoding_dance(
                environ.get("SCRIPT_NAME") or "", self.charset, self.encoding_errors
            ),
            path=_wsgi_decoding_dance(
                environ.get("PATH_INFO") or "", self.charset, self.encoding_errors
            ),
            query_string=environ.get("QUERY_STRING", "").encode("latin1"),
            headers=EnvironHeaders(environ),
            remote_addr=environ.get("REMOTE_ADDR"),
        )
        self.environ = environ

        if self.disable_data_descriptor is not None:
            warnings.warn(
                "'disable_data_descriptor' is deprecated and will be"
                " removed in Werkzeug 2.1. Create the request with"
                " 'shallow=True' instead.",
                DeprecationWarning,
                stacklevel=2,
            )
            shallow = shallow or self.disable_data_descriptor

        self.shallow = shallow

        if populate_request and not shallow:
            self.environ["werkzeug.request"] = self

    @classmethod
    def from_values(cls, *args: t.Any, **kwargs: t.Any) -> "Request":
        """Create a new request object based on the values provided.  If
        environ is given missing values are filled from there.  This method is
        useful for small scripts when you need to simulate a request from an URL.
        Do not use this method for unittesting, there is a full featured client
        object (:class:`Client`) that allows to create multipart requests,
        support for cookies etc.

        This accepts the same options as the
        :class:`~werkzeug.test.EnvironBuilder`.

        .. versionchanged:: 0.5
           This method now accepts the same arguments as
           :class:`~werkzeug.test.EnvironBuilder`.  Because of this the
           `environ` parameter is now called `environ_overrides`.

        :return: request object
        """
        from ..test import EnvironBuilder

        charset = kwargs.pop("charset", cls.charset)
        kwargs["charset"] = charset
        builder = EnvironBuilder(*args, **kwargs)
        try:
            return builder.get_request(cls)
        finally:
            builder.close()

    @classmethod
    def application(
        cls, f: t.Callable[["Request"], "WSGIApplication"]
    ) -> "WSGIApplication":
        """Decorate a function as responder that accepts the request as
        the last argument.  This works like the :func:`responder`
        decorator but the function is passed the request object as the
        last argument and the request object will be closed
        automatically::

            @Request.application
            def my_wsgi_app(request):
                return Response('Hello World!')

        As of Werkzeug 0.14 HTTP exceptions are automatically caught and
        converted to responses instead of failing.

        :param f: the WSGI callable to decorate
        :return: a new WSGI callable
        """
        #: return a callable that wraps the -2nd argument with the request
        #: and calls the function with all the arguments up to that one and
        #: the request.  The return value is then called with the latest
        #: two arguments.  This makes it possible to use this decorator for
        #: both standalone WSGI functions as well as bound methods and
        #: partially applied functions.
        from ..exceptions import HTTPException

        @functools.wraps(f)
        def application(*args):  # type: ignore
            request = cls(args[-2])
            with request:
                try:
                    resp = f(*args[:-2] + (request,))
                except HTTPException as e:
                    resp = e.get_response(args[-2])
                return resp(*args[-2:])

        return t.cast("WSGIApplication", application)

    def _get_file_stream(
        self,
        total_content_length: t.Optional[int],
        content_type: t.Optional[str],
        filename: t.Optional[str] = None,
        content_length: t.Optional[int] = None,
    ) -> t.IO[bytes]:
        """Called to get a stream for the file upload.

        This must provide a file-like class with `read()`, `readline()`
        and `seek()` methods that is both writeable and readable.

        The default implementation returns a temporary file if the total
        content length is higher than 500KB.  Because many browsers do not
        provide a content length for the files only the total content
        length matters.

        :param total_content_length: the total content length of all the
                                     data in the request combined.  This value
                                     is guaranteed to be there.
        :param content_type: the mimetype of the uploaded file.
        :param filename: the filename of the uploaded file.  May be `None`.
        :param content_length: the length of this file.  This value is usually
                               not provided because webbrowsers do not provide
                               this value.
        """
        return default_stream_factory(
            total_content_length=total_content_length,
            filename=filename,
            content_type=content_type,
            content_length=content_length,
        )

    @property
    def want_form_data_parsed(self) -> bool:
        """``True`` if the request method carries content. By default
        this is true if a ``Content-Type`` is sent.

        .. versionadded:: 0.8
        """
        return bool(self.environ.get("CONTENT_TYPE"))

    def make_form_data_parser(self) -> FormDataParser:
        """Creates the form data parser. Instantiates the
        :attr:`form_data_parser_class` with some parameters.

        .. versionadded:: 0.8
        """
        return self.form_data_parser_class(
            self._get_file_stream,
            self.charset,
            self.encoding_errors,
            self.max_form_memory_size,
            self.max_content_length,
            self.parameter_storage_class,
            max_form_parts=self.max_form_parts,
        )

    def _load_form_data(self) -> None:
        """Method used internally to retrieve submitted data.  After calling
        this sets `form` and `files` on the request object to multi dicts
        filled with the incoming form data.  As a matter of fact the input
        stream will be empty afterwards.  You can also call this method to
        force the parsing of the form data.

        .. versionadded:: 0.8
        """
        # abort early if we have already consumed the stream
        if "form" in self.__dict__:
            return

        if self.want_form_data_parsed:
            parser = self.make_form_data_parser()
            data = parser.parse(
                self._get_stream_for_parsing(),
                self.mimetype,
                self.content_length,
                self.mimetype_params,
            )
        else:
            data = (
                self.stream,
                self.parameter_storage_class(),
                self.parameter_storage_class(),
            )

        # inject the values into the instance dict so that we bypass
        # our cached_property non-data descriptor.
        d = self.__dict__
        d["stream"], d["form"], d["files"] = data

    def _get_stream_for_parsing(self) -> t.IO[bytes]:
        """This is the same as accessing :attr:`stream` with the difference
        that if it finds cached data from calling :meth:`get_data` first it
        will create a new stream out of the cached data.

        .. versionadded:: 0.9.3
        """
        cached_data = getattr(self, "_cached_data", None)
        if cached_data is not None:
            return BytesIO(cached_data)
        return self.stream

    def close(self) -> None:
        """Closes associated resources of this request object.  This
        closes all file handles explicitly.  You can also use the request
        object in a with statement which will automatically close it.

        .. versionadded:: 0.9
        """
        files = self.__dict__.get("files")
        for _key, value in iter_multi_items(files or ()):
            value.close()

    def __enter__(self) -> "Request":
        return self

    def __exit__(self, exc_type, exc_value, tb) -> None:  # type: ignore
        self.close()

    @cached_property
    def stream(self) -> t.IO[bytes]:
        """
        If the incoming form data was not encoded with a known mimetype
        the data is stored unmodified in this stream for consumption.  Most
        of the time it is a better idea to use :attr:`data` which will give
        you that data as a string.  The stream only returns the data once.

        Unlike :attr:`input_stream` this stream is properly guarded that you
        can't accidentally read past the length of the input.  Werkzeug will
        internally always refer to this stream to read data which makes it
        possible to wrap this object with a stream that does filtering.

        .. versionchanged:: 0.9
           This stream is now always available but might be consumed by the
           form parser later on.  Previously the stream was only set if no
           parsing happened.
        """
        if self.shallow:
            raise RuntimeError(
                "This request was created with 'shallow=True', reading"
                " from the input stream is disabled."
            )

        return get_input_stream(self.environ)

    input_stream = environ_property[t.IO[bytes]](
        "wsgi.input",
        doc="""The WSGI input stream.

        In general it's a bad idea to use this one because you can
        easily read past the boundary.  Use the :attr:`stream`
        instead.""",
    )

    @cached_property
    def data(self) -> bytes:
        """
        Contains the incoming request data as string in case it came with
        a mimetype Werkzeug does not handle.
        """
        return self.get_data(parse_form_data=True)

    @typing.overload
    def get_data(  # type: ignore
        self,
        cache: bool = True,
        as_text: "te.Literal[False]" = False,
        parse_form_data: bool = False,
    ) -> bytes:
        ...

    @typing.overload
    def get_data(
        self,
        cache: bool = True,
        as_text: "te.Literal[True]" = ...,
        parse_form_data: bool = False,
    ) -> str:
        ...

    def get_data(
        self, cache: bool = True, as_text: bool = False, parse_form_data: bool = False
    ) -> t.Union[bytes, str]:
        """This reads the buffered incoming data from the client into one
        bytes object.  By default this is cached but that behavior can be
        changed by setting `cache` to `False`.

        Usually it's a bad idea to call this method without checking the
        content length first as a client could send dozens of megabytes or more
        to cause memory problems on the server.

        Note that if the form data was already parsed this method will not
        return anything as form data parsing does not cache the data like
        this method does.  To implicitly invoke form data parsing function
        set `parse_form_data` to `True`.  When this is done the return value
        of this method will be an empty string if the form parser handles
        the data.  This generally is not necessary as if the whole data is
        cached (which is the default) the form parser will used the cached
        data to parse the form data.  Please be generally aware of checking
        the content length first in any case before calling this method
        to avoid exhausting server memory.

        If `as_text` is set to `True` the return value will be a decoded
        string.

        .. versionadded:: 0.9
        """
        rv = getattr(self, "_cached_data", None)
        if rv is None:
            if parse_form_data:
                self._load_form_data()
            rv = self.stream.read()
            if cache:
                self._cached_data = rv
        if as_text:
            rv = rv.decode(self.charset, self.encoding_errors)
        return rv  # type: ignore

    @cached_property
    def form(self) -> "ImmutableMultiDict[str, str]":
        """The form parameters.  By default an
        :class:`~werkzeug.datastructures.ImmutableMultiDict`
        is returned from this function.  This can be changed by setting
        :attr:`parameter_storage_class` to a different type.  This might
        be necessary if the order of the form data is important.

        Please keep in mind that file uploads will not end up here, but instead
        in the :attr:`files` attribute.

        .. versionchanged:: 0.9

            Previous to Werkzeug 0.9 this would only contain form data for POST
            and PUT requests.
        """
        self._load_form_data()
        return self.form

    @cached_property
    def values(self) -> "CombinedMultiDict[str, str]":
        """A :class:`werkzeug.datastructures.CombinedMultiDict` that
        combines :attr:`args` and :attr:`form`.

        For GET requests, only ``args`` are present, not ``form``.

        .. versionchanged:: 2.0
            For GET requests, only ``args`` are present, not ``form``.
        """
        sources = [self.args]

        if self.method != "GET":
            # GET requests can have a body, and some caching proxies
            # might not treat that differently than a normal GET
            # request, allowing form data to "invisibly" affect the
            # cache without indication in the query string / URL.
            sources.append(self.form)

        args = []

        for d in sources:
            if not isinstance(d, MultiDict):
                d = MultiDict(d)

            args.append(d)

        return CombinedMultiDict(args)

    @cached_property
    def files(self) -> "ImmutableMultiDict[str, FileStorage]":
        """:class:`~werkzeug.datastructures.MultiDict` object containing
        all uploaded files.  Each key in :attr:`files` is the name from the
        ``<input type="file" name="">``.  Each value in :attr:`files` is a
        Werkzeug :class:`~werkzeug.datastructures.FileStorage` object.

        It basically behaves like a standard file object you know from Python,
        with the difference that it also has a
        :meth:`~werkzeug.datastructures.FileStorage.save` function that can
        store the file on the filesystem.

        Note that :attr:`files` will only contain data if the request method was
        POST, PUT or PATCH and the ``<form>`` that posted to the request had
        ``enctype="multipart/form-data"``.  It will be empty otherwise.

        See the :class:`~werkzeug.datastructures.MultiDict` /
        :class:`~werkzeug.datastructures.FileStorage` documentation for
        more details about the used data structure.
        """
        self._load_form_data()
        return self.files

    @property
    def script_root(self) -> str:
        """Alias for :attr:`self.root_path`. ``environ["SCRIPT_ROOT"]``
        without a trailing slash.
        """
        return self.root_path

    @cached_property
    def url_root(self) -> str:
        """Alias for :attr:`root_url`. The URL with scheme, host, and
        root path. For example, ``https://example.com/app/``.
        """
        return self.root_url

    remote_user = environ_property[str](
        "REMOTE_USER",
        doc="""If the server supports user authentication, and the
        script is protected, this attribute contains the username the
        user has authenticated as.""",
    )
    is_multithread = environ_property[bool](
        "wsgi.multithread",
        doc="""boolean that is `True` if the application is served by a
        multithreaded WSGI server.""",
    )
    is_multiprocess = environ_property[bool](
        "wsgi.multiprocess",
        doc="""boolean that is `True` if the application is served by a
        WSGI server that spawns multiple processes.""",
    )
    is_run_once = environ_property[bool](
        "wsgi.run_once",
        doc="""boolean that is `True` if the application will be
        executed only once in a process lifetime.  This is the case for
        CGI for example, but it's not guaranteed that the execution only
        happens one time.""",
    )

    # JSON

    #: A module or other object that has ``dumps`` and ``loads``
    #: functions that match the API of the built-in :mod:`json` module.
    json_module = json

    @property
    def json(self) -> t.Optional[t.Any]:
        """The parsed JSON data if :attr:`mimetype` indicates JSON
        (:mimetype:`application/json`, see :attr:`is_json`).

        Calls :meth:`get_json` with default arguments.
        """
        return self.get_json()

    # Cached values for ``(silent=False, silent=True)``. Initialized
    # with sentinel values.
    _cached_json: t.Tuple[t.Any, t.Any] = (Ellipsis, Ellipsis)

    def get_json(
        self, force: bool = False, silent: bool = False, cache: bool = True
    ) -> t.Optional[t.Any]:
        """Parse :attr:`data` as JSON.

        If the mimetype does not indicate JSON
        (:mimetype:`application/json`, see :attr:`is_json`), this
        returns ``None``.

        If parsing fails, :meth:`on_json_loading_failed` is called and
        its return value is used as the return value.

        :param force: Ignore the mimetype and always try to parse JSON.
        :param silent: Silence parsing errors and return ``None``
            instead.
        :param cache: Store the parsed JSON to return for subsequent
            calls.
        """
        if cache and self._cached_json[silent] is not Ellipsis:
            return self._cached_json[silent]

        if not (force or self.is_json):
            return None

        data = self.get_data(cache=cache)

        try:
            rv = self.json_module.loads(data)
        except ValueError as e:
            if silent:
                rv = None

                if cache:
                    normal_rv, _ = self._cached_json
                    self._cached_json = (normal_rv, rv)
            else:
                rv = self.on_json_loading_failed(e)

                if cache:
                    _, silent_rv = self._cached_json
                    self._cached_json = (rv, silent_rv)
        else:
            if cache:
                self._cached_json = (rv, rv)

        return rv

    def on_json_loading_failed(self, e: ValueError) -> t.Any:
        """Called if :meth:`get_json` parsing fails and isn't silenced.
        If this method returns a value, it is used as the return value
        for :meth:`get_json`. The default implementation raises
        :exc:`~werkzeug.exceptions.BadRequest`.
        """
        raise BadRequest(f"Failed to decode JSON object: {e}")


class StreamOnlyMixin:
    """Mixin to create a ``Request`` that disables the ``data``,
    ``form``, and ``files`` properties. Only ``stream`` is available.

    .. deprecated:: 2.0
        Will be removed in Werkzeug 2.1. Create the request with
        ``shallow=True`` instead.

    .. versionadded:: 0.9
    """

    def __init__(self, *args: t.Any, **kwargs: t.Any) -> None:
        warnings.warn(
            "'StreamOnlyMixin' is deprecated and will be removed in"
            " Werkzeug 2.1. Create the request with 'shallow=True'"
            " instead.",
            DeprecationWarning,
            stacklevel=2,
        )
        kwargs["shallow"] = True
        super().__init__(*args, **kwargs)  # type: ignore


class PlainRequest(StreamOnlyMixin, Request):
    """A request object without ``data``, ``form``, and ``files``.

    .. deprecated:: 2.0
        Will be removed in Werkzeug 2.1. Create the request with
        ``shallow=True`` instead.

    .. versionadded:: 0.9
    """

    def __init__(self, *args: t.Any, **kwargs: t.Any) -> None:
        warnings.warn(
            "'PlainRequest' is deprecated and will be removed in"
            " Werkzeug 2.1. Create the request with 'shallow=True'"
            " instead.",
            DeprecationWarning,
            stacklevel=2,
        )

        # Don't show the DeprecationWarning for StreamOnlyMixin.
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", DeprecationWarning)
            super().__init__(*args, **kwargs)
