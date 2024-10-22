foreach plugin : ['bm_allocator', 'bmcodec', 'bmvpss', 'bmv4l2src', 'bmive']
  if not get_option(plugin).disabled()
    subdir(plugin)
  endif
endforeach
