
DEBUG        ?= 0
CHIP         ?= bm1684
SUBTYPE      ?= asic
PRODUCTFORM  ?= soc

INSTALL_DIR    ?= release

NATIVE_SDK_DIR ?= ../../install/${PRODUCTFORM}_${CHIP}_${SUBTYPE}/decode
VPU_SDK_DIR    ?= $(NATIVE_SDK_DIR)
JPU_SDK_DIR    ?= $(NATIVE_SDK_DIR)
YUV_SDK_DIR    ?= $(NATIVE_SDK_DIR)
VPP_SDK_DIR    ?= $(NATIVE_SDK_DIR)
ION_SDK_DIR    ?= $(NATIVE_SDK_DIR)

FF_SDK_DIR     ?= ../../install/${PRODUCTFORM}_${CHIP}_${SUBTYPE}/ffmpeg/usr/local
OCV_SDK_DIR    ?= ../../install/${PRODUCTFORM}_${CHIP}_${SUBTYPE}/opencv
#TODO
BMCV_SDK_DIR   ?= ../../install/${PRODUCTFORM}_${CHIP}_${SUBTYPE}/bmcv
BMLIB_DIR      ?= ../../bm_opencv/3rdparty/libbmcv/

ifeq ($(PRODUCTFORM),pcie) # pcie mode
    CROSS_CC_PREFIX =
    CPPFLAGS += -D_GLIBCXX_USE_CXX11_ABI=${ABI_FLAG}
endif

ifeq ($(PRODUCTFORM),soc)
    CROSS_CC_PREFIX = aarch64-linux-gnu-
endif

ifeq ($(PRODUCTFORM),pcie_arm64)
    CROSS_CC_PREFIX = aarch64-linux-gnu-
    CPPFLAGS += -D_GLIBCXX_USE_CXX11_ABI=${ABI_FLAG}
endif

ifeq ($(PRODUCTFORM),pcie_loongarch64)
    CROSS_CC_PREFIX = loongarch64-linux-gnu-
endif

ifeq ($(PRODUCTFORM),pcie_mips64)
    CROSS_CC_PREFIX = mips-linux-gnu-
    CPPFLAGS += -mips64r2 -mabi=64 -march=gs464e -D_GLIBCXX_USE_CXX11_ABI=0
endif

ifeq ($(PRODUCTFORM),pcie_sw64)
    CROSS_CC_PREFIX = sw_64-sunway-linux-gnu-
endif

CC  = $(CROSS_CC_PREFIX)gcc
CXX = $(CROSS_CC_PREFIX)g++

CPPFLAGS += -std=c++11 -fPIC -Wall -Wl,--fatal-warning
ifeq ($(DEBUG), 0)
    CPPFLAGS += -O2
else
    CPPFLAGS += -g
endif

# NATIVE API SDK
NATIVE_SDK_HEADERS:=-I$(JPU_SDK_DIR)/include/ -I$(VPU_SDK_DIR)/include/ -I$(YUV_SDK_DIR)/include/ -I$(VPP_SDK_DIR)/include/ -I$(ION_SDK_DIR)/include/ -I$(NATIVE_SDK_DIR)/include/
NATIVE_SDK_LDFLAGS:=-L$(JPU_SDK_DIR)/lib/ -L$(VPU_SDK_DIR)/lib/ -L$(YUV_SDK_DIR)/lib/ -L$(VPP_SDK_DIR)/lib/ -L$(ION_SDK_DIR)/lib/ -L$(NATIVE_SDK_DIR)/lib/
NATIVE_SDK_LDLIBS :=-lbmion -lbmjpulite -lbmjpuapi -lbmvpulite -lbmvpuapi -lbmvideo  -lyuv

# FFMPEG SDK
FF_SDK_HEADERS := -I$(FF_SDK_DIR)/include/
FF_SDK_LDFLAGS := -L$(FF_SDK_DIR)/lib/
FF_SDK_LDLIBS  := -lavcodec -lavformat -lavutil -lswresample -lswscale

# OpenCV SDK
OCV_SDK_HEADERS := -I$(OCV_SDK_DIR)/include/opencv4
OCV_SDK_LDFLAGS := -L$(OCV_SDK_DIR)/lib/
OCV_SDK_LDLIBS  := -lopencv_core -lopencv_imgcodecs -lopencv_imgproc -lopencv_videoio

# BMCV SDK
BMCV_SDK_HEADERS := -I$(BMLIB_DIR)/include/ -I$(BMCV_SDK_DIR)/include/
BMCV_SDK_LDFLAGS := -L$(BMCV_SDK_DIR)/lib/ -L$(BMLIB_DIR)/lib/$(PRODUCTFORM)
BMCV_SDK_LDLIBS  := -lbmcv -lbmlib -lvpp_cmodel

CPPFLAGS += $(NATIVE_SDK_HEADERS) $(FF_SDK_HEADERS) $(OCV_SDK_HEADERS) $(BMCV_SDK_HEADERS)
LDFLAGS  := $(NATIVE_SDK_LDFLAGS) $(FF_SDK_LDFLAGS) $(OCV_SDK_LDFLAGS) $(BMCV_SDK_LDFLAGS)
LDLIBS   := $(NATIVE_SDK_LDLIBS) $(FF_SDK_LDLIBS) $(OCV_SDK_LDLIBS) $(BMCV_SDK_LDLIBS) -lpthread -lstdc++

ifeq ($(PRODUCTFORM),pcie_loongarch64)
LDLIBS   += -lm   -lresolv  -lrt  -ldl
endif

TARGET=test_ocv_vidmulti
MAKEFILE=Makefile
ALLOBJS=*.o
ALLDEPS=*.dep
RM=rm -rf
CP=cp -f

SOURCES := vidmulti.cpp

OBJECTPATHS:=$(patsubst %.cpp,%.o,$(SOURCES))

.phony: all clean

all: $(TARGET)

$(TARGET): $(OBJECTPATHS)
	$(CC) -o $@ $(OBJECTPATHS) $(LDFLAGS) $(LDLIBS) $(CPPFLAGS)

install: $(TARGET)
	install -d $(INSTALL_DIR)/bin
	install $(TARGET) $(INSTALL_DIR)/bin

uninstall:
	$(RM) $(INSTALL_DIR)/bin/$(TARGET)

clean:
	$(RM) $(TARGET)
	$(RM) $(ALLDEPS)
	$(RM) $(ALLOBJS)

ocv_video_xcode.o : ocv_video_xcode.cpp $(MAKEFILE)
	$(CXX) $(CPPFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)
