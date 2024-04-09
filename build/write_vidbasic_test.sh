# /bin/bash
# README:
#       wtite test statement into temp test script
function writeVidBasicTest(){
    local nfs_path=$1
    local MODE=$2
    local TAIL_MARK=$3
    if [ $MODE = pcie ];then
cat << EOF > $nfs_path/tmp_daily.sh
rm -f out*.png
../samples/ocv_vidbasic/test_ocv_vidbasic opencv_input.mp4 ocv_vidbasic $frames 0 > ocv_vidbasic_$MODE$TAIL_MARK
EOF
     elif [ $MODE = soc ];then
cat << EOF > $nfs_path/tmp_daily.sh
rm -f /dev/shm/*

rm -f ocv_vidbasic*.png
rm -f fbt.txt
rm -f fbt_fps_log.txt
rm -f tmp_n_times.txt
./samples/ocv_vidbasic/test_ocv_vidbasic opencv_input.mp4 ocv_vidbasic $frames 0 2>&1 | tee fbt.txt
cat fbt.txt | grep fps > fbt_fps_log.txt
awk '/fps/{print \$4}' fbt_fps_log.txt > tmp_n_times.txt
awk '{sum+=\$1; count++} END{if (count > 0) printf "%d\n", sum/count; else print "No fps data found"}' tmp_n_times.txt > ocv_vidbasic_$MODE$TAIL_MARK
rm -f ocv_vidbasic*.png
rm -f fbt.txt
rm -f fbt_fps_log.txt
rm -f tmp_n_times.txt
EOF
    fi
}
