# /bin/bash
# README:
#       wtite test statement into temp test script
function writeVidMultiTest(){
    local nfs_path=$1
    local MODE=$2
    local TAIL_MARK=$3
    local try_n_times=$4
    if [ $MODE = pcie ];then
cat << EOF > $nfs_path/tmp_daily.sh
rm -f out*.png
for((j=1;j<=$try_n_times;j++))
do
timeout -s SIGINT 10s ../samples/bin/test_ocv_vidmulti 1 bilibiligd.264 0 0 > ovm.txt
cat ovm.txt | tr -s "\r\n" "\n" > ovm_new.txt
cat ovm_new.txt | grep fps > ovm_fps_log.txt
sed -i '1d' ovm_fps_log.txt
cat ovm_fps_log.txt | awk '{sum+=\$6}END{printf "%d\n", sum/NR}' >> tmp_n_times.txt
done
cat tmp_n_times.txt | awk '{sum+=\$1;mean=sum/$try_n_times};END{printf "%d\n",mean}' > ocv_vidmulti_$MODE$TAIL_MARK
rm -f tmp_n_times.txt
rm -f ovm.txt
rm -f ovm_new.txt
rm -f ovm_fps_log.txt
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
for((j=1;j<=$try_n_times;j++))
do
timeout -s SIGINT 10s ./samples/ocv_vidmulti/test_ocv_vidmulti 1 bilibiligd.264 0 0 2>&1 | tee ovm.txt
cat ovm.txt | tr -s "\r\n" "\n" > ovm_new.txt
cat ovm_new.txt | grep fps > ovm_fps_log.txt
sed -i '1d' ovm_fps_log.txt
cat ovm_fps_log.txt | awk '{sum+=\$6}END{printf "%d\n", sum/NR}' >> tmp_n_times.txt
done
cat tmp_n_times.txt | awk '{sum+=\$1;mean=sum/$try_n_times};END{printf "%d\n",mean}' > ocv_vidmulti_$MODE$TAIL_MARK
rm -f tmp_n_times.txt
rm -f ovm.txt
rm -f ovm_new.txt
rm -f ovm_fps_log.txt
EOF
    fi
}
