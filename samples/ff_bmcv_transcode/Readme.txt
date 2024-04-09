Function Description：
    ffmpeg video transcode.
    And if Select I420 encode mode in the command line, User can add a mosaic in the upper left corner of the
    video, and a watermark in the full screen.

Usage:
in pcie crad
    ./test_ff_bmcv_transcode pcie input.mp4 test.mp4 I420 h265_bm 1280 720 25 3000 1 1 0
    pcie: PCie platform
    I420: NV12 transcode to yuv420
    h265_bm: Hardware speedup encode
    1280*720: resize para
    25:fps
    3000: bitrate
    1:thread num
    1:YUV data does not copy to host memory
    0:card 0
    ./test_ff_video_transcode input.mp4 test.mp4 NV12 h265_bm 1280 720 25 3000 1 1 0
    NV12: no pixel format transcode
    h265_bm: Hardware speedup encode
    1280*720: If it is nv12, this parameter is invalid.The image size is generated according to the original image size
    25:fps
    3000: bitrate
    1:thread num
    1:YUV data does not copy to host memory
    0:card 0


in soc
    ./test_ff_bmcv_transcode soc input.mp4 test.mp4 I420 h265_bm 1280 720 25 3000 1
    soc: SoC platform
    I420: NV12 transcode to yuv420
    h265_bm: Hardware speedup encode
    1280*720: resize para
    25:fps
    3000: bitrate
    1:thread num

    ./test_ff_video_transcode input.mp4 test.mp4 NV12 h265_bm 1280 720 25 3000 1
    NV12: no pixel format transcode
    h265_bm: Hardware speedup encode
    1280*720: If it is nv12, this parameter is invalid.The image size is generated according to the original image size
    25:fps
    3000: bitrate
    1:thread num


If the program is multithreaded the Target file will be test0.mp4 test1.mp4 test2.mp4 ...

