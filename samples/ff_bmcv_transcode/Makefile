# Makefile for building test_ff_video_transcode on ARM platform

CXX=g++
CXXFLAGS=-std=c++11 -I/opt/sophon/sophon-ffmpeg-latest/include/ -I/opt/sophon/libsophon-current/include/ -I.

# Linker settings
LDFLAGS=-L/opt/sophon/sophon-ffmpeg-latest/lib/ -L/opt/sophon/libsophon-current/lib/
LDLIBS=-lavcodec -lavdevice -lavfilter -lavformat -lavutil -lswresample -lswscale -lbmlib -lbmcv -lbmjpeg -lyuv

# Target executable
TARGET=test_ff_video_transcode

# Source files
SOURCES=$(wildcard *.cpp)

# Build rule
$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET) $(LDFLAGS) $(LDLIBS) -lpthread 

# Clean rule
clean:
	rm -f $(TARGET)

.PHONY: clean
