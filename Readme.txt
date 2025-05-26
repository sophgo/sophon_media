# README

Welcome to the Sophgo Media Repository for the BM1688. This repository will host ongoing development for the following modules:

- FFMPEG
- OpenCV
- GStreamer
- Sophon Sample

**Note:** This repository requires the `libsophav` submodule.

## Requirements

### 1. Obtain the Source Code

You can clone the source code from either Gerrit or GitHub.

#### From Gerrit

```bash
git clone ssh://your.name@gerrit-ai.sophgo.vip:29418/sophon_media
cd sophon_media
git submodule update --init
```

#### From GitHub

```bash
# For sophon_media
git clone https://github.com/sophgo/sophon_media.git

# For libsophav
git clone https://github.com/sophgo/libsophav.git
```

### 2. Obtain the GCC Compiler

You can also clone the GCC compiler from Gerrit or GitHub.

#### From Gerrit

```bash
git clone ssh://your.name@gerrit-ai.sophgo.vip:29418/host-tools
```

#### From GitHub

```bash
# For host-tools
git clone https://github.com/sophgo/host-tools.git
```

#### Note on GCC Versions

You will need to set the `PATH` variable according to the GCC version you are using:

- **GCC 6.3.1** (GNU libc 2.23)
  ```bash
  export PATH=/yourpath/host-tools/gcc/gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu/bin:$PATH
  ```

- **GCC 9.3.0** (GNU libc 2.31)
  ```bash
  export PATH=/yourpath/host-tools/gcc/gcc-buildroot-9.3.0-aarch64-linux-gnu/bin:$PATH
  ```

- **GCC 11.3.1** (GNU libc 2.35)
  ```bash
  export PATH=/yourpath/host-tools/gcc/arm-gnu-toolchain-11.3.rel1-x86_64-aarch64-none-linux-gnu/bin:$PATH
  ```

### 3. Build the Project

To build the project, run the following command:

```bash
source build/build_cmake.sh ${GCC_VERSION} ${PRODUCTFORM}  # (optional, default is soc)
```

- **GCC_VERSION:** 630, 930, or 1131
- **PRODUCTFORM:** soc, pcie, or pcie_arm64

**Example:**

```bash
source build/build_cmake.sh 1131
```

---

Feel free to reach out if you have any questions or need further assistance!




