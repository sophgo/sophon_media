## How to build

### Requirement

1. install official gstreamer devel libraries

```sh
apt-get install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libgstreamer-plugins-bad1.0-dev gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-libav gstreamer1.0-tools gstreamer1.0-gl

export LD_LIBRARY_PATH=/usr/lib/aarch64-linux-gnu/gstreamer-1.0:$LD_LIBRARY_PATH
```

2. put libsophon libs into default library search path, for example:

```sh
cp /opt/sophon/libsophon-current/lib/* /usr/local/lib/

# bmv4l2src need isp-mw lib
cp /opt/sophon/sophon-soc-libisp_1.0.0/lib/* /usr/local/lib/

export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
```

### Build && install

**install path need to be the same with official gstreamer libraries, default is /usr**

```sh
mkdir build && cd build
sudo apt install meson ninja-build

# Actually, you don't need to care about install path, you can manually copy libgstbm*.so to directory in which libgst*.so are stored
meson .. [--prefix=/path/to/install]

ninja
ninja install
```

### Test

```sh
# use the following cmd to test whether environment is configured or not
gst-launch-1.0 -v filesrc location=enc-5.264 ! video/x-h264 ! h264parse ! bmdec ! autovideoconvert ! bmh265enc! filesink location=out.h265

gst-launch-1.0 -v filesrc location=out.h265 !  video/x-h265 ! h265parse ! bmdec ! autovideoconvert ! bmh264enc! filesink location=testtest.h264

gst-launch-1.0 -v filesrc location=test_in.jpg ! image/jpeg ! jpegparse ! bmdec ! videoconvert ! 'video/x-raw,format=I420' ! filesink location=output.yuv
```
