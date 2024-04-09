# /bin/bash
# README:
#       wtite test statement into temp test script
function writeVideoDecodeTest(){
    local nfs_path=$1
    local MODE=$2
    local TAIL_MARK=$3
    local try_n_times=$4
    if [ $MODE = pcie ];then
cat << EOF > $nfs_path/tmp_daily.sh
rm -f out*.png
for((j=1;j<=$try_n_times;j++))
do
timeout 10s ../samples/bin/test_bm_restart 0 1 no 0 1 bilibiligd.264 > fvd.txt
cat fvd.txt | grep avg > fvd_fps_log.txt
cat fvd_fps_log.txt | awk '{a+=\$9+1}END{printf "%d\n",a}' >> tmp_n_times.txt
done
cat tmp_n_times.txt | awk '{sum+=\$1;mean=sum/$try_n_times};END{printf "%d\n",mean}' > ff_video_decode_$MODE$TAIL_MARK
rm -f tmp_n_times.txt
rm -f fvd.txt
rm -f fvd_fps_log.txt
EOF
    elif [ $MODE = soc ];then
cat << EOF > $nfs_path/tmp_daily.sh

rm -f tmp_n_times.txt
rm -f fvd.txt
rm -f fvd_fps_log.txt

for((j=1;j<=$try_n_times;j++))
do
timeout 10s ./samples/ff_video_decode/test_bm_restart 1 0 3 no 0 1 bilibiligd.264 > fvd.txt
cat fvd.txt | grep avg > fvd_fps_log.txt
# cat fvd_fps_log.txt | awk '{a+=\$9+1}END{printf "%d\n",a}' >> tmp_n_times.txt
awk '/avg:/{num=\$(NF-2)} END{print num}' fvd_fps_log.txt >> tmp_n_times.txt
done
cat tmp_n_times.txt | awk '{sum+=\$1;mean=sum/$try_n_times};END{printf "%d\n",mean}' > ff_video_decode_$MODE$TAIL_MARK
rm -f tmp_n_times.txt
rm -f fvd.txt
rm -f fvd_fps_log.txt
EOF
    fi
}
