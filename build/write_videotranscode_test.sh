# /bin/bash
# README:
#       wtite test statement into temp test script

function writeVideoTranscodeTest(){
    local nfs_path=$1
    local MODE=$2
    local TAIL_MARK=$3
    local try_n_times=$4
    if [ $MODE = pcie ];then
cat << EOF > $nfs_path/tmp_daily.sh
rm -f out*.png
for((j=1;j<=$try_n_times;j++))
do
../samples/bin/test_ff_bmcv_transcode opencv_input.mp4 ff_bmcv_transcode.mp4 I420 h264_bm 416 240 25 3000 1 0 0 > fbt.txt
cat fbt.txt | grep fps > fbt_fps_log.txt
cat fbt_fps_log.txt | awk '{a+=\$6+1}END{printf "%d\n",a}' >> tmp_n_times.txt
done
cat tmp_n_times.txt | awk '{sum+=\$1;mean=sum/$try_n_times};END{printf "%d\n",mean}' > ff_bmcv_transcode_$MODE$TAIL_MARK
rm -f tmp_n_times.txt
rm -f fbt.txt
rm -f fbt_fps_log.txt
EOF
    elif [ $MODE = soc ];then
cat << EOF > $nfs_path/tmp_daily.sh
rm -f /dev/shm/*
# chmod a+x load.sh load_jpu.sh unload_jpu.sh unload.sh
# ./unload.sh
# ./unload_jpu.sh
# ./load.sh
# ./load_jpu.sh

rm -f ff_bmcv_transcode*.mp4
rm -f tmp_n_times.txt
rm -f fbt.txt
rm -f fbt_fps_log.txt

for((j=1;j<=$try_n_times;j++))
do
./samples/ff_bmcv_transcode/test_ff_bmcv_transcode soc bilibiligd.264 ff_bmcv_transcode.mp4 I420 h264_bm 416 240 25 3000 1 0 0 2>&1 | tee fbt.txt
cat fbt.txt | grep fps > fbt_fps_log.txt
cat fbt_fps_log.txt | awk 'NR==1{a+=\$6}END{printf "%d\n",a}' >> tmp_n_times.txt
done
awk '{if(\$1 != 0){sum+=\$1; n++}} END{if(n>0){mean=sum/n; printf "%d\n",mean} else {print "No non-zero numbers found"}}' tmp_n_times.txt > ff_bmcv_transcode_$MODE$TAIL_MARK
# rm -f ff_bmcv_transcode*.mp4
# rm -f tmp_n_times.txt
# rm -f fbt.txt
# rm -f fbt_fps_log.txt
EOF
    fi
}
