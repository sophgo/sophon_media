counter=0
check_num=0

timeout_duration=300 #超时时间（单位：秒)

host_name=dailytest
host_ip=172.28.141.219
host_passwd=dailytest
src_dir="/home/dailytest/Athena2/Multimedia/ve"
input_dir="input"
input_file=(
"1080p_nv12.yuv"
"1080p_yuv420p.yuv"
"1080p_yuv422p.yuv"
"1920x1088_420.yuv"
"352x288_I420.yuv"
"352x288_nv12.yuv"
"CBR2925_h265.mp4"
"bilibilibjx.264"
"huangfeihong-h265-1920x1080.mp4"
"station.avi"
"station_1080p.265"
"station_4mb_200.264"
"station_4mb_200.265"

)
ref_dir="ref/ffmpeg"

dest=/data/ffmpeg
golden_dir=ffmpeg_ref
check_num=0

# action==0, test case
# action==1, make golden file
action=$1

test_ffmpeg_ve_case_cmp=(
"test_ff_scale_transcode station.avi case0.ts I420 h265_bm 1280 720 30 8000 2 1 0"
"test_ff_video_xcode huangfeihong-h265-1920x1080.mp4 case1.ts H264 30 3000 1 0 0"
"test_ff_video_xcode CBR2925_h265.mp4 case2.ts H264 30 3000 0 0 0"
"test_ff_video_xcode CBR2925_h265.mp4 case3.ts H265 30 3000 1 1 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case4.ts I420 h264_bm 2048 1080 30 3000 1 1 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case5.ts I420 h264_bm 256 128 30 3000 1 0 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case6.ts I420 h264_bm 352 288 30 3000 1 0 1"
"test_ff_scale_transcode CBR2925_h265.mp4 case7.ts I420 h264_bm 720 480 30 8000 2 0 1"
"test_ff_scale_transcode CBR2925_h265.mp4 case8.ts I420 h264_bm 1280 720 30 8000 2 1 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case9.ts I420 h264_bm 1920 1080 30 3000 2 1 1"
"test_ff_scale_transcode CBR2925_h265.mp4 case10.ts I420 h264_bm 3280 2048 30 3000 2 1 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case11.ts I420 h264_bm 4096 2160 30 3000 2 1 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case12.ts I420 h264_bm 800 400 25 3000 3 0 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case13.ts I420 h264_bm 1920 1080 25 3000 3 1 1"
"test_ff_scale_transcode CBR2925_h265.mp4 case14.ts I420 h264_bm 1200 666 25 3000 2 0 1"
"test_ff_scale_transcode huangfeihong-h265-1920x1080.mp4 case15.ts I420 h265_bm 1368 888 25 3000 2 0 1"
"test_ff_scale_transcode huangfeihong-h265-1920x1080.mp4 case16.ts I420 h265_bm 256 128 30 3000 1 0 0"
"test_ff_scale_transcode huangfeihong-h265-1920x1080.mp4 case17.ts I420 h265_bm 352 288 30 3000 1 0 1"
"test_ff_scale_transcode huangfeihong-h265-1920x1080.mp4 case18.ts I420 h265_bm 720 480 30 8000 2 0 1"
"test_ff_scale_transcode huangfeihong-h265-1920x1080.mp4 case19.ts I420 h265_bm 1280 720 30 8000 2 1 0"
"test_ff_scale_transcode huangfeihong-h265-1920x1080.mp4 case20.ts I420 h265_bm 1920 1080 30 3000 2 1 1"
"test_ff_scale_transcode huangfeihong-h265-1920x1080.mp4 case21.ts I420 h265_bm 2048 1080 30 3000 1 1 0"
"test_ff_scale_transcode huangfeihong-h265-1920x1080.mp4 case22.ts I420 h265_bm 3280 2048 30 3000 2 1 0"
"test_ff_scale_transcode huangfeihong-h265-1920x1080.mp4 case23.ts I420 h265_bm 4096 2160 30 3000 2 1 0"
"test_ff_scale_transcode huangfeihong-h265-1920x1080.mp4 case24.ts I420 h265_bm 800 400 25 3000 3 0 0"
"test_ff_scale_transcode huangfeihong-h265-1920x1080.mp4 case25.ts I420 h265_bm 1920 1080 25 3000 3 1 1"
"test_ff_scale_transcode huangfeihong-h265-1920x1080.mp4 case26.ts I420 h265_bm 1368 888 25 3000 2 0 1"
"test_ff_scale_transcode huangfeihong-h265-1920x1080.mp4 case27.ts I420 h265_bm 256 128 30 3000 1 0 0"
"test_ff_scale_transcode huangfeihong-h265-1920x1080.mp4 case28.ts I420 h265_bm 352 288 30 3000 1 0 1"
"test_ff_scale_transcode huangfeihong-h265-1920x1080.mp4 case29.ts I420 h265_bm 720 480 30 8000 2 0 1"
"test_ff_scale_transcode huangfeihong-h265-1920x1080.mp4 case30.ts I420 h265_bm 1280 720 30 8000 2 1 0"
"test_ff_scale_transcode huangfeihong-h265-1920x1080.mp4 case31.ts I420 h265_bm 1920 1080 30 3000 2 1 1"
"test_ff_scale_transcode huangfeihong-h265-1920x1080.mp4 case32.ts I420 h265_bm 2048 1080 30 3000 1 1 0"
"test_ff_scale_transcode huangfeihong-h265-1920x1080.mp4 case33.ts I420 h265_bm 3280 2048 30 3000 2 1 0"
"test_ff_scale_transcode huangfeihong-h265-1920x1080.mp4 case34.ts I420 h265_bm 4096 2160 30 3000 2 1 0"
"test_ff_scale_transcode huangfeihong-h265-1920x1080.mp4 case35.ts I420 h265_bm 800 400 25 3000 3 0 0"
"test_ff_scale_transcode huangfeihong-h265-1920x1080.mp4 case36.ts I420 h265_bm 1920 1080 25 3000 3 1 1"
"test_ff_scale_transcode CBR2925_h265.mp4 case37.ts I420 h264_bm 256 128 20 1000 1 0 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case38.ts I420 h264_bm 256 128 30 3000 1 0 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case39.ts I420 h264_bm 352 288 30 3000 1 0 1"
"test_ff_scale_transcode CBR2925_h265.mp4 case40.ts I420 h264_bm 720 480 30 8000 2 0 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case41.ts I420 h264_bm 1280 720 30 8000 2 1 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case42.ts I420 h264_bm 1920 1080 30 3000 2 1 1"
"test_ff_scale_transcode CBR2925_h265.mp4 case43.ts I420 h264_bm 2048 1080 30 3000 1 1 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case44.ts I420 h264_bm 3280 2049 30 3000 2 1 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case45.ts I420 h264_bm 4096 2160 30 3000 2 1 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case46.ts I420 h264_bm 800 400 25 3000 3 0 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case47.ts I420 h264_bm 1920 1080 25 3000 3 1 1"
"test_ff_scale_transcode CBR2925_h265.mp4 case48.ts I420 h264_bm 1900 1000 25 3000 2 0 1"
"test_ff_scale_transcode CBR2925_h265.mp4 case49.ts I420 h265_bm 256 128 30 3000 1 0 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case50.ts I420 h265_bm 352 288 30 3000 1 0 1"
"test_ff_scale_transcode CBR2925_h265.mp4 case51.ts I420 h265_bm 720 480 30 8000 2 0 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case52.ts I420 h265_bm 1280 720 30 8000 2 1 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case53.ts I420 h265_bm 1920 1080 30 3000 2 1 1"
"test_ff_scale_transcode CBR2925_h265.mp4 case54.ts I420 h265_bm 2048 1080 30 3000 1 1 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case55.ts I420 h265_bm 3280 2049 30 3000 2 1 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case56.ts I420 h265_bm 4096 2160 30 3000 2 1 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case57.ts I420 h265_bm 800 400 25 3000 3 0 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case58.ts I420 h265_bm 1920 1080 25 3000 3 1 1"
"test_ff_scale_transcode CBR2925_h265.mp4 case59.ts I420 h265_bm 2000 1000 25 3000 2 0 1"
"test_ff_scale_transcode huangfeihong-h265-1920x1080.mp4 case60.ts I420 h265_bm 256 128 30 3000 1 0 0"
"test_ff_scale_transcode huangfeihong-h265-1920x1080.mp4 case61.ts I420 h265_bm 352 288 30 3000 1 0 1"
"test_ff_scale_transcode huangfeihong-h265-1920x1080.mp4 case62.ts I420 h265_bm 720 480 30 8000 2 0 0"
"test_ff_scale_transcode huangfeihong-h265-1920x1080.mp4 case63.ts I420 h265_bm 1280 720 30 8000 2 1 0"
"test_ff_scale_transcode huangfeihong-h265-1920x1080.mp4 case64.ts I420 h265_bm 1920 1080 30 3000 2 1 1"
"test_ff_scale_transcode huangfeihong-h265-1920x1080.mp4 case65.ts I420 h265_bm 2048 1080 30 3000 1 1 0"
"test_ff_scale_transcode huangfeihong-h265-1920x1080.mp4 case66.ts I420 h265_bm 3280 2049 30 3000 2 1 0"
"test_ff_scale_transcode huangfeihong-h265-1920x1080.mp4 case67.ts I420 h265_bm 4096 2160 30 3000 2 1 0"
"test_ff_scale_transcode huangfeihong-h265-1920x1080.mp4 case68.ts I420 h265_bm 800 400 25 3000 3 0 0"
"test_ff_scale_transcode huangfeihong-h265-1920x1080.mp4 case69.ts I420 h265_bm 1920 1080 25 3000 3 1 1"
"test_ff_scale_transcode huangfeihong-h265-1920x1080.mp4 case70.ts I420 h265_bm 2200 990 25 3000 2 0 1"
"test_ff_scale_transcode huangfeihong-h265-1920x1080.mp4 case71.ts I420 h264_bm 256 128 30 3000 1 0 0"
"test_ff_scale_transcode huangfeihong-h265-1920x1080.mp4 case72.ts I420 h264_bm 352 288 30 3000 1 0 1"
"test_ff_scale_transcode huangfeihong-h265-1920x1080.mp4 case73.ts I420 h264_bm 720 480 30 8000 2 0 0"
"test_ff_scale_transcode huangfeihong-h265-1920x1080.mp4 case74.ts I420 h264_bm 1280 720 30 8000 2 1 0"
"test_ff_scale_transcode huangfeihong-h265-1920x1080.mp4 case75.ts I420 h264_bm 1920 1080 30 3000 2 1 1"
"test_ff_scale_transcode huangfeihong-h265-1920x1080.mp4 case76.ts I420 h264_bm 2048 1080 30 3000 1 1 0"
"test_ff_scale_transcode huangfeihong-h265-1920x1080.mp4 case77.ts I420 h264_bm 3280 2049 30 3000 2 1 0"
"test_ff_scale_transcode huangfeihong-h265-1920x1080.mp4 case78.ts I420 h264_bm 4096 2160 30 3000 2 1 0"
"test_ff_scale_transcode huangfeihong-h265-1920x1080.mp4 case79.ts I420 h264_bm 800 400 25 3000 3 0 0"
"test_ff_scale_transcode huangfeihong-h265-1920x1080.mp4 case80.ts I420 h264_bm 1920 1080 25 3000 3 1 1"
"test_ff_scale_transcode huangfeihong-h265-1920x1080.mp4 case81.ts I420 h264_bm 600 400 25 3000 2 0 1"
"test_ff_scale_transcode CBR2925_h265.mp4 case82.ts I420 h265_bm 256 128 30 3000 1 0 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case83.ts I420 h265_bm 352 288 30 3000 1 0 1"
"test_ff_scale_transcode CBR2925_h265.mp4 case84.ts I420 h265_bm 720 480 30 8000 2 0 1"
"test_ff_scale_transcode CBR2925_h265.mp4 case85.ts I420 h265_bm 1280 720 30 8000 2 1 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case86.ts I420 h265_bm 1920 1080 30 3000 2 1 1"
"test_ff_scale_transcode CBR2925_h265.mp4 case87.ts I420 h265_bm 2048 1080 30 3000 1 1 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case88.ts I420 h265_bm 3280 2048 30 3000 2 1 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case89.ts I420 h265_bm 4096 2160 30 3000 2 1 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case90.ts I420 h265_bm 800 400 25 3000 3 0 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case91.ts I420 h265_bm 1920 1080 25 3000 3 1 1"
"test_ff_scale_transcode CBR2925_h265.mp4 case92.ts I420 h265_bm 1230 440 30 3000 2 1 1"
"test_ff_scale_transcode CBR2925_h265.mp4 case93.ts I420 h264_bm 256 128 30 3000 1 0 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case94.ts I420 h264_bm 352 288 30 3000 1 0 1"
"test_ff_scale_transcode CBR2925_h265.mp4 case95.ts I420 h264_bm 720 480 30 8000 2 0 1"
"test_ff_scale_transcode CBR2925_h265.mp4 case96.ts I420 h264_bm 1280 720 30 8000 2 1 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case97.ts I420 h264_bm 1920 1080 30 3000 2 1 1"
"test_ff_scale_transcode CBR2925_h265.mp4 case98.ts I420 h264_bm 2048 1080 30 3000 1 1 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case99.ts I420 h264_bm 3280 2048 30 3000 2 1 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case100.ts I420 h264_bm 4096 2160 30 3000 2 1 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case101.ts I420 h264_bm 800 400 25 3000 3 0 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case102.ts I420 h264_bm 1920 1080 25 3000 3 1 1"
"test_ff_scale_transcode CBR2925_h265.mp4 case103.ts I420 h264_bm 1200 666 25 3000 2 0 1"
"test_ff_scale_transcode CBR2925_h265.mp4 case104.ts I420 h264_bm 256 128 30 3000 1 0 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case105.ts I420 h264_bm 352 288 30 3000 1 0 1"
"test_ff_scale_transcode CBR2925_h265.mp4 case106.ts I420 h264_bm 720 480 30 8000 2 0 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case107.ts I420 h264_bm 1280 720 30 8000 2 1 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case108.ts I420 h264_bm 1920 1080 30 3000 2 1 1"
"test_ff_scale_transcode CBR2925_h265.mp4 case109.ts I420 h264_bm 2048 1080 30 3000 1 1 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case110.ts I420 h264_bm 3280 2049 30 3000 2 1 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case111.ts I420 h264_bm 4096 2160 30 3000 2 1 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case112.ts I420 h264_bm 800 400 30 3000 3 0 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case113.ts I420 h264_bm 1920 1080 30 3000 3 1 1"
"test_ff_scale_transcode CBR2925_h265.mp4 case114.ts I420 h264_bm 1440 880 30 3000 2 0 1"
"test_ff_scale_transcode CBR2925_h265.mp4 case115.ts I420 h265_bm 256 128 30 3000 1 0 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case116.ts I420 h265_bm 352 288 30 3000 1 0 1"
"test_ff_scale_transcode CBR2925_h265.mp4 case117.ts I420 h265_bm 720 480 30 8000 2 0 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case118.ts I420 h265_bm 1280 720 30 8000 2 1 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case119.ts I420 h265_bm 1920 1080 30 3000 2 1 1"
"test_ff_scale_transcode CBR2925_h265.mp4 case120.ts I420 h265_bm 2048 1080 30 3000 1 1 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case121.ts I420 h265_bm 3280 2049 30 3000 2 1 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case122.ts I420 h265_bm 4096 2160 30 3000 2 1 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case123.ts I420 h265_bm 800 400 30 3000 3 0 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case124.ts I420 h265_bm 1920 1080 30 3000 3 1 1"
"test_ff_scale_transcode CBR2925_h265.mp4 case125.ts I420 h265_bm 1780 752 30 3000 2 0 1"
"test_ff_scale_transcode CBR2925_h265.mp4 case126.ts I420 h265_bm 256 128 30 3000 1 0 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case127.ts I420 h265_bm 352 288 30 3000 1 0 1"
"test_ff_scale_transcode CBR2925_h265.mp4 case128.ts I420 h265_bm 720 480 30 8000 2 0 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case129.ts I420 h265_bm 1280 720 30 8000 2 1 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case130.ts I420 h265_bm 1920 1080 30 3000 2 1 1"
"test_ff_scale_transcode CBR2925_h265.mp4 case131.ts I420 h265_bm 2048 1080 30 3000 1 1 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case132.ts I420 h265_bm 3280 2049 30 3000 2 1 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case133.ts I420 h265_bm 4096 2160 30 3000 2 1 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case134.ts I420 h265_bm 800 400 30 3000 3 0 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case135.ts I420 h265_bm 1920 1080 30 3000 3 1 1"
"test_ff_scale_transcode CBR2925_h265.mp4 case136.ts I420 h265_bm 2000 1111 30 3000 2 0 1"
"test_ff_scale_transcode CBR2925_h265.mp4 case137.ts I420 h264_bm 256 128 30 3000 1 0 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case138.ts I420 h264_bm 352 288 30 3000 1 0 1"
"test_ff_scale_transcode CBR2925_h265.mp4 case139.ts I420 h264_bm 720 480 30 8000 2 0 1"
"test_ff_scale_transcode CBR2925_h265.mp4 case140.ts I420 h264_bm 1280 720 30 8000 2 1 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case141.ts I420 h264_bm 1920 1080 30 3000 2 1 1"
"test_ff_scale_transcode CBR2925_h265.mp4 case142.ts I420 h264_bm 2048 1080 30 3000 1 1 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case143.ts I420 h264_bm 3280 2049 30 3000 2 1 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case144.ts I420 h264_bm 4096 2160 30 3000 2 1 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case145.ts I420 h264_bm 800 400 25 3000 3 0 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case146.ts I420 h264_bm 1920 1080 25 3000 3 1 1"
"test_ff_scale_transcode huangfeihong-h265-1920x1080.mp4 case147.ts I420 h264_bm 1444 900 25 3000 2 0 1"
"test_ff_scale_transcode station.avi case148.ts I420 h264_bm 320 240 25 3000 3"
"test_ff_bmcv_transcode soc station_1080p.265 case149.ts I420 h264_bm 800 400 25 3000 1 0 0"
"test_ff_bmcv_transcode soc station_1080p.265 case150.ts I420 h264_bm 1920 1080 25 3000 3 0 0"
"test_ff_bmcv_transcode soc huangfeihong-h265-1920x1080.mp4 case151.ts I420 h264_bm 800 400 25 3000 3 0 0"
"test_ff_bmcv_transcode soc station_1080p.265 case152.ts I420 h264_bm 1901 1080 25 3000 3 0 0"
"test_ff_bmcv_transcode soc station_1080p.265 case153.ts I420 h264_bm 1901 1079 25 3000 3 0 0"
"test_ff_bmcv_transcode soc station_1080p.265 case154.ts I420 h265_bm 800 400 25 3000 5 0 0"
"test_ff_bmcv_transcode soc station_1080p.265 case155.ts I420 h265_bm 1920 1080 25 3000 3 0 0"
"test_ff_bmcv_transcode soc station_1080p.265 case156.ts I420 h265_bm 1901 1080 25 3000 3 0 0"
"test_ff_bmcv_transcode soc station_1080p.265 case157.ts I420 h265_bm 1901 1079 25 3000 3 0 0"
"test_ff_bmcv_transcode soc station.avi case158.ts I420 h265_bm 720 480 25 8000 3 0 0 1 1"
"test_ff_video_encode 1080p_yuv420p.yuv case159.mp4 H264 1920 1080 0 NV12 3000 30 0"
"ffmpeg -s 1920x1080 -pix_fmt yuv420p -i 1080p_yuv420p.yuv -c:v h264_bm -b:v 3M -is_dma_buffer 0 -y case160.264"
"ffmpeg -extra_frame_buffer_num 5 -i bilibilibjx.264 -vf "scale_bm=1920:1080" -is_dma_buffer 1 -y case161.ts"
"ffmpeg -extra_frame_buffer_num 5 -i bilibilibjx.264 -vf "scale_bm=1920:1080" -is_dma_buffer 0  -y case162.ts "
"ffmpeg -extra_frame_buffer_num 5 -i station_4mb_200.264 -is_dma_buffer 1 -c:v h264_bm -b:v 3M -y case163.ts"
"ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v h264_bm -i station_4mb_200.264 -c:v h264_bm -b:v 4000K -y case164.264"
"ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v hevc_bm -i station_1080p.265 -c:v hevc_bm -b:v 4000K -y case165.265"
"ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v h264_bm -output_format 101 -i station_4mb_200.264 -vf "scale_bm=352:288:zero_copy=1" -c:v hevc_bm -b:v 4000K -y case166.265"
"ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v hevc_bm -output_format 101 -i station_4mb_200.265 -vf "scale_bm=352:288" -c:v h264_bm -b:v 4000K -y case167.264"
"ffmpeg -s 1920x1088 -pix_fmt yuv420p -i 1920x1088_420.yuv -c:v h264_bm -b:v 3000 -is_dma_buffer 0 -b_qfactor 4 -y case168.264"
"ffmpeg -s 1920x1080 -pix_fmt yuv420p -i 1080p_yuv420p.yuv -c:v h264_bm -b:v 3M -is_dma_buffer 0 -bf 10 -y case169.264"
"ffmpeg -s 1920x1080 -pix_fmt yuv420p -i 1080p_yuv420p.yuv -c:v h264_bm -b:v 3M -is_dma_buffer 0 -abr 1 -bf 10 -y case170.264"
"ffmpeg -s 1920x1080 -pix_fmt yuv420p -i 1080p_yuv420p.yuv -c:v h264_bm -b:v 3M -is_dma_buffer 0 -abr 1 -bf 10 -q:a 8 -y case171.264"
"test_ff_video_encode 1080p_yuv420p.yuv case172.h264 H264 1920 1080 0 I420 3000 30"
"test_ff_video_encode 1080p_yuv420p.yuv case173.h265 H265 1920 1080 0 I420 3000 30"
"test_ff_video_encode 1080p_nv12.yuv case174.h264 H264 1920 1080 0 NV12 3000 30"
"test_ff_video_encode 1080p_nv12.yuv case175.h265 H265 1920 1080 0 NV12 3000 30"
"test_ff_scale_transcode huangfeihong-h265-1920x1080.mp4 case176.ts I420 h265_bm 256 128 30 3000 1"
"test_ff_scale_transcode huangfeihong-h265-1920x1080.mp4 case177.ts I420 h265_bm 288 352 25 3000 9"
"test_ff_video_encode 1080p_yuv420p.yuv case178.ts H264 352 288 0 I420 3000 30 1"
"test_ff_video_encode 1080p_yuv420p.yuv case179.ts H265 352 288 1 I420 3000 30 0"
"test_ff_video_encode 1080p_nv12.yuv case180.ts H264 352 288 0 nv12 3000 30 1"
"test_ff_video_encode 1080p_nv12.yuv case181.ts H265 352 288 1 nv12 3000 30 0"
"test_ff_video_encode 1080p_yuv420p.yuv case182.ts H264 1920 1080 0 I420 11 30 0"
"test_ff_video_encode 1080p_yuv420p.yuv case183.ts H264 1920 1080 1 I420 300 30 0"
"test_ff_video_encode 1080p_yuv420p.yuv case184.ts H264 1920 1080 0 I420 3000 30 0"
"test_ff_video_encode 1080p_yuv420p.yuv case185.ts H264 1920 1080 0 I420 8000 30 0"
"test_ff_video_encode 1080p_yuv420p.yuv case186.ts H264 1920 1080 0 I420 30000 30 0"
"test_ff_video_encode 1080p_yuv420p.yuv case187.ts H264 1920 1080 0 I420 99999 30 0"
"test_ff_video_encode 1080p_nv12.yuv case188.ts H265 1920 1080 1 nv12 300 30 0"
"test_ff_video_encode 1080p_nv12.yuv case189.ts H265 1920 1080 0 nv12 3000 30 0"
"test_ff_video_encode 1080p_nv12.yuv case190.ts H265 1920 1080 0 nv12 8000 30 0"
"test_ff_video_encode 1080p_nv12.yuv case191.ts H265 1920 1080 0 nv12 30000 30 0"
"test_ff_video_encode 1080p_nv12.yuv case192.ts H265 1920 1080 0 nv12 99999 30 0"
"test_ff_video_encode 1080p_nv12.yuv case193.ts H264 1920 1080 1 NV12 5000 60 0"
"test_ff_video_encode 1080p_nv12.yuv case194.ts H264 1920 1080 0 NV12 5000 120 0"
"test_ff_video_encode 1080p_nv12.yuv case195.ts H264 1920 1080 0 NV12 5000 35.4 0"
"test_ff_video_encode 1080p_nv12.yuv case196.ts H264 1920 1080 0 NV12 5000 15 0"
"test_ff_video_encode 1080p_yuv420p.yuv case197.ts H265 1920 1080 0 I420 5000 30 0"
"test_ff_video_encode 1080p_yuv420p.yuv case198.ts H265 1920 1080 1 I420 5000 60 0"
"test_ff_video_encode 1080p_yuv420p.yuv case199.ts H265 1920 1080 0 I420 5000 120 0"
"test_ff_video_encode 1080p_yuv420p.yuv case200.ts H265 1920 1080 0 I420 5000 35.4 0"
"test_ff_video_encode 1080p_yuv420p.yuv case201.ts H265 1920 1080 0 I420 5000 15 0"
"test_ff_video_encode 1080p_yuv420p.yuv case202.ts H265 1920 1080 0 I420 5000 30 1"
"test_ff_video_encode 1080p_yuv420p.yuv case203.ts H265 1920 1080 1 I420 5000 30 0"
"test_ff_video_encode 1080p_yuv420p.yuv case204.ts H265 1920 1080 1 I420 5000 30 1"
"test_ff_video_encode 1080p_nv12.yuv case205.ts H264 1920 1080 0 NV12 5000 30 0"
"test_ff_video_encode 1080p_nv12.yuv case206.ts H264 1920 1080 0 NV12 5000 30 3"
"test_ff_video_encode 1080p_nv12.yuv case207.ts H264 1920 1080 1 NV12 5000 30 0"
"test_ff_video_encode 1080p_nv12.yuv case208.ts H264 1920 1080 1 NV12 5000 30 3"
"test_ff_video_encode 1080p_yuv420p.yuv case209.mp4 H264 4096 2160 0 I420 3000 30 0"
"test_ff_video_encode 1080p_yuv420p.yuv case210.mp4 H265 4096 2160 0 I420 3000 30 0"
"test_ff_video_encode 1080p_nv12.yuv case211.mp4 H264 4096 2160 0 NV12 3000 30 0"
"test_ff_video_encode 1080p_nv12.yuv case212.mp4 H265 4096 2160 0 NV12 3000 30 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case213.ts I420 h264_bm 2048 1080 30 3000 3 1 0"
"test_ff_scale_transcode huangfeihong-h265-1920x1080.mp4 case214.ts I420 h265_bm 1280 720 30 3000 9 0 0"
"test_ff_scale_transcode CBR2925_h265.mp4 case215.ts I420 h265_bm 1920 1080 60 3000 8 1 1"
"test_ff_scale_transcode huangfeihong-h265-1920x1080.mp4 case216.ts I420 h265_bm 352 288 30 3000 10 1 0"
# "test_ff_scale_transcode rtsp://172.28.141.116:5544/vod/123/station.avi out_0.ts I420 h265_bm 720 480 25 8000 3 1 0"
# "test_ff_scale_transcode rtsp://172.28.141.116:5544/vod/123/station.avi out_0.ts I420 h264_bm 720 480 25 8000 3 1 0"

)

function scp_file()
{

file_addr=$1
name=$2
ipaddres=$3
dest_path=$4
# name=$3
passwd=$5

to_remote=$6 #if 1 localhost copy remote machine if 2 remote machine copy to loaclhost


/usr/bin/expect<<-EOF
set cmp 1
if { $to_remote == "1" } {
    spawn sudo scp -r $name@$ipaddres:$file_addr $dest_path

} else {
    spawn sudo scp -r $file_addr $name@$ipaddres:$dest_path
}

expect {
  "密码："
        {
          send "$passwd\n"
        }
   "pass"
        {
          send "$passwd\n"
        }
   "yes/no"
        {
          sleep 5
        #   send_user "send yes"
          send "yes\n"
        }
   eof
    {
        sleep 5
        send_user "eof\n"
    }
}

# send "exit\r"
expect eof
EOF
}

function make_golden(){
    num=$1

    ${test_ffmpeg_ve_case_cmp[num]}
    sudo md5sum case${num}* >> ${golden_dir}/case${num}.md5

    find . -name case*
    ret=$?
    if [ ${ret} -ne 0 ]; then
        rm -rf case*
    fi
}

function compare_md5(){
    num=$1

    scp_file ${src_dir}/${ref_dir}/case${num}.md5 ${host_name} ${host_ip} . ${host_passwd} 1
    md5sum -c case${num}.md5 >> ffmpeg_ve.log
    echo

    sudo rm case${num}.md5
}

function test_start(){
    compare=$1
    num=$2
    line=$3

    echo $line
    timeout ${timeout_duration} ${line}
    exit_code=$?
    sleep 1
    if [ ${exit_code} -eq 0 ]; then
        echo case${num}: $line success
        if [ ${compare} -eq 1 ]; then
            compare_md5 ${num}
        fi

    else
        if [ ${exit_code} -eq 124 ]; then
            if [ ${compare} -eq 1 ]; then
                echo "Command timed out.$line"
                echo ${line} "Command timed out." >> ffmpeg_ve.log
                printf "\n" >> ffmpeg_ve.log
            else
                echo case${num}: $line success >> ffmpeg_ve.log
                printf "\n" >> ffmpeg_ve.log
            fi
        else
            echo "Command encountered an error.$line"
            echo ${line} "Command Failed" >> ffmpeg_ve.log
            printf "\n" >> ffmpeg_ve.log
        fi

    fi

    find . -name case*
    ret=$?
    if [ ${ret} -ne 0 ]; then
        rm -rf case*
    fi
}

# test_all_case 1个输入参数
#       0 自动化测试
#       1 生成golden文件，进行比较，生成的golden文件传到服务器
function test_all_case(){
    echo action=${action}

    if [ ${action} -eq 1 ]; then
        echo "golden_dir:${golden_dir}"
        if [ -d ${golden_dir} ]; then
            rm -r ${golden_dir}
            mkdir -p ${golden_dir}
        else
            mkdir -p ${golden_dir}
        fi
    fi

    if [ -e "ffmpeg_ve.log" ]; then
        rm -rf ffmpeg_ve.log
    fi

    # sudo  mkdir ${dest}
    # scp_file ${src_dir}/${input_dir}/* ${host_name} ${host_ip} ${dest} ${host_passwd} 1

    # for ((i=0; i<${#input_file[@]}; i++))
    # do
    #     echo ${src_dir}/${input_dir}/${input_file[i]}
    #     scp_file ${src_dir}/${input_dir}/${input_file[i]} ${host_name} ${host_ip} ${dest} ${host_passwd} 1
    # done

    for ((i=0; i<${#test_ffmpeg_ve_case_cmp[@]}; i++))
    do

        if [ ${action} -eq 0 ]; then
            # run case
            test_start 1 ${i} "${test_ffmpeg_ve_case_cmp[i]}"
        else
            # make golden file
            make_golden ${i}
        fi

    done

    if [ ${action} -eq 0 ]; then

        for ((i=0; i<${#test_ffmpeg_ve_case_nocmp[@]}; i++))
        do
            test_start 0 ${i} "${test_ffmpeg_ve_case_nocmp[i]}"
        done
    fi

}

test_all_case
