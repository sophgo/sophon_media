#!/bin/bash
CURRENT_DIR=$(readlink -f $(dirname $0))

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/lib/aarch64-linux-gnu/

export GST_DEBUG=1
#bilibilibjx.264
#station_1080p.265
/usr/bin/test_gst_transcode \
  --video_path=../../huangfeihong-h265-1920x1080.mp4 \
  --output_path=output1.h265 \
  --dec_type=4 \
  --enc_type=2 \
  --bps=1000000 \
  --gop=50 \
  --gop_preset=5 \
  --qp_min=20 \
  --qp_max=45 \
  --cqp=25 \

 /usr/bin/test_gst_transcode \
  --video_path=../../JPEG_1920x1088_yuv420_planar.jpg \
  --output_path=out.jpeg \
  --dec_type=3 \
  --enc_type=3 \
  --q_factor=80

   /usr/bin/test_gst_transcode \
  --video_path=../../JPEG_1920x1088_yuv420_planar.jpg \
  --output_path=out2.jpeg \
  --dec_type=4 \
  --enc_type=3 \
  --q_factor=40