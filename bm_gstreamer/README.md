## How to build

### Requirement

1. install official gstreamer devel libraries
```
apt-get install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libgstreamer-plugins-bad1.0-dev gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-libav gstreamer1.0-tools gstreamer1.0-gl
```

2. put libsophon libs into default library search path, for example:
```
cp /opt/sophon/libsophon-current/lib/* /usr/local/lib/
```

### Build && install

**install path need to be the same with official gstreamer libraries, default is /usr**

```
mkdir build && cd build
meson .. --prefix=/path/to/install
ninja
ninja install
```
