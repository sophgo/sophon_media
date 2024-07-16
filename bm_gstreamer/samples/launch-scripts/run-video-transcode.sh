#!/bin/bash
# $1: debug level
CURRENT_DIR=$(readlink -f $(dirname $0))

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/lib/aarch64-linux-gnu/

export GST_DEBUG=$1
export GST_PLUGIN_PATH=$GST_PLUGIN_PATH:$CURRENT_DIR/../../lib
gst-launch-1.0 uridecodebin uri="file://$(pwd)/../data/videos/cat.mp4" caps=video/x-h264 ! \
	h264parse ! bmdec ! bmh264enc ! filesink location="./out.h264"
