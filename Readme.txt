soc CI

1: 编译sophon_media需要libsophav、host-tools仓库，可以在github上下载。
sophon_media:https://github.com/sophgo/sophon_media
libsophav:https://github.com/sophgo/libsophav
host-tools:https://github.com/sophgo/host-tools
2: 拉取libsophav至sophon_media子目录，host-tools至sophon_media同级目录
3: 在sophon_media下执行 source build/build_cmake.sh $GCC_V 即可以完成编译
GCC_V可供指定的选项有 930 、1131
930 对应 unbntu 20.04 GLIBC 2.31 的交叉编译链，1131 对应 unbntu 22.04 GLIBC 2.35 的交叉编译链




