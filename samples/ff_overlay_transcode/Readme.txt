Usage:
    test_ff_video_overlay [src_filename] [output_filename] [encoder_name] [zero_copy] [sophon_idx] [overlay_num] [overlay_filepath_1] [x] [y] [overlay_filepath_2] [x] [y]

    [src_filename]            input file name x.mp4 x.ts...
    [output_filename]         encode output file name x.mp4,x.ts...
    [encoder_name]            encode h264_bm,hevc_bm,h265_bm
    [zero_copy ]              0: copy host mem,1: nocopy.
    [sophon_idx]              sophon devices idx
    [overlay_num]             overlay num
    [overlay_filepath_1]      overlay file path 1
    [x]                       x position for overlay1 on src
    [y]                       y position for overlay1 on src
    [overlay_filepath_2]      overlay file path 2
    [x]                       x position for overlay2 on src
    [y]                       y position for overlay2 on src

example:
    test_ff_video_overlay src.mp4 out.ts h264_bm 0 0 2 overlay_1.264 10 10 overlay_2.264 500 500

If the program is multithreaded the Target file will be test0.mp4 test1.mp4 test2.mp4 ...