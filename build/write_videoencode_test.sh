# /bin/bash
# README:
#       wtite test statement into temp test script
function writeVideoEncodeTest(){
    local nfs_path=$1
    local MODE=$2
    local TAIL_MARK=$3
    if [ $MODE = pcie ];then
cat << EOF > $nfs_path/tmp_daily.sh
rm -f out*.png
../samples/bin/test_ff_video_encode opencv_input.mp4 ff_video_encode.mp4 H264 1920 1080 0 I420 3000 30 > ff_video_encode_$MODE$TAIL_MARK
EOF
    elif [ $MODE = soc ];then
cat << EOF > $nfs_path/tmp_daily.sh
rm -f ff_video_encode.mp4
./samples/ff_video_encode/test_ff_video_encode 1080p_yuv420p.yuv ff_video_encode.mp4 H264 1920 1080 0 I420 3000 30 > ff_video_encode_$MODE$TAIL_MARK
EOF
    fi
}
