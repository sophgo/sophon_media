# /bin/bash
# README:
#       wtite test statement into temp test script
function writeVideoXcodeTest(){
    local nfs_path=$1
    local MODE=$2
    local TAIL_MARK=$3
    if [ $MODE = pcie ];then
cat << EOF > $nfs_path/tmp_daily.sh
rm -f out*.png
../samples/bin/test_ocv_video_xcode opencv_input.mp4 H264enc 30 ocv_video_xcode.mp4 1 0 > ocv_video_xcode_$MODE$TAIL_MARK
EOF
    elif [ $MODE = soc ];then
cat << EOF > $nfs_path/tmp_daily.sh
rm -f /dev/shm/*
# chmod a+x load.sh load_jpu.sh unload_jpu.sh unload.sh
# ./unload.sh
# ./unload_jpu.sh
# ./load.sh
# ./load_jpu.sh
rm -f out*.png
./samples/ocv_video_xcode/test_ocv_video_xcode bilibiligd.264 H264enc 3000 ocv_video_xcode.mp4 1 0 2>&1 | tee fbt.txt
cat fbt.txt | grep fps > fbt_fps_log.txt
awk '/fps/{print \$4}' fbt_fps_log.txt > tmp_n_times.txt
awk '{sum+=\$1; count++} END{if (count > 0) printf "%d\n", sum/count; else print "No fps data found"}' tmp_n_times.txt > ocv_video_xcode_$MODE$TAIL_MARK
# rm -f fbt.txt
# rm -f fbt_fps_log.txt
# rm -f tmp_n_times.txt

EOF
    fi
}
