Usage:
    ./test_ff_hw_bmcv_transcode [platform] [src_filename] [output_filename] [codecer_name] [width] [height] [frame_rate] [bitrate] [thread_num] [en_bmx264] [zero_copy] [sophon_idx] <optional: enable_mosaic> <optional: enable_watermark>

    [platform]:        pcie or soc
    [src_filename]:    input file
    [output_filename]: output file
    [codecer_name]:    h264_bm or h265_bm
    [width]:           encode parameter video width
    [height]:          encode parameter video height
    [frame_rate]:      encode parameter frame_rate
    [bitrate]:         bitrate(bps)
    [thread_num]:      thread number
    [en_bmx264]:       pcie mode: 1 enable bmx264, 0 disable bmx264
                       soc mode: 0
    [zero_copy]:       pcie mode: 0: copy host mem, 1: nocopy
                       soc mode:  0~N any number is acceptable, but it is invalid
    [sophon_idx]:      pcie mode: sophon devices idx
                       soc mode:  0, other number is invalid
    <enable_mosaic>:    Optional, 1: enable mosaix process,    0: disable mosaix process
    <enable_watermark>: Optional, 1: enable watermark process, 0: disable watermark process

example:
    ./test_ff_hw_bmcv_transcode pcie INPUT.264 out.264 h264_bm 1920 1080 25 3000 1 0 1 0

If the program is multithreaded the Target file will be test0.mp4 test1.mp4 test2.mp4 ...

