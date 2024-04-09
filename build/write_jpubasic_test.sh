# /bin/bash
# README:
#       wtite test statement into temp test script
function writeJpuBasicTest(){
    local nfs_path=$1
    local MODE=$2
    local TAIL_MARK=$3
    local try_n_times=$4
    if [ $MODE = pcie ];then
cat << EOF > $nfs_path/tmp_daily.sh
rm -f out*.png
for((j=1;j<=$try_n_times;j++))
do
../samples/bin/test_ocv_jpubasic opencv_input.jpg 1 1 0 > ojb.txt
cat ojb.txt | grep decoder > daily_decjpeglog.txt
cat ojb.txt | grep encoder > daily_encjpeglog.txt
cat daily_decjpeglog.txt | awk '{a=\$3}END{printf "%f\n",a}' >> tmp_n_time_dec.txt
cat daily_encjpeglog.txt | awk '{a=\$3}END{printf "%f\n",a}' >> tmp_n_time_enc.txt
done
cat tmp_n_time_dec.txt |  awk '{sum+=\$1;mean=sum/$try_n_times};END{printf "%d\n",mean}' > ocv_jpubasic_decoder_$MODE$TAIL_MARK
cat tmp_n_time_enc.txt |  awk '{sum+=\$1;mean=sum/$try_n_times};END{printf "%d\n",mean}' > ocv_jpubasic_encoder_$MODE$TAIL_MARK
rm -f tmp_n_time_dec.txt
rm -f tmp_n_time_enc.txt
rm -f ojb.txt
rm -f daily_encjpeglog.txt
rm -f daily_decjpeglog.txt
EOF
    elif [ $MODE = soc ];then
cat << EOF > $nfs_path/tmp_daily.sh
rm -f /dev/shm/*
rm -f out*.jpg
for((j=1;j<=$try_n_times;j++))
do
./samples/ocv_jpubasic/test_ocv_jpubasic opencv_input.jpg 1 1 0 2>&1 | tee ojb.txt
cat ojb.txt | grep decoder > daily_decjpeglog.txt
cat ojb.txt | grep encoder > daily_encjpeglog.txt
cat daily_decjpeglog.txt | awk '{a=\$3}END{printf "%f\n",a}' >> tmp_n_time_dec.txt
cat daily_encjpeglog.txt | awk '{a=\$3}END{printf "%f\n",a}' >> tmp_n_time_enc.txt
done
cat tmp_n_time_dec.txt |  awk '{sum+=\$1;mean=sum/$try_n_times};END{printf "%d\n",mean}' > ocv_jpubasic_decoder_$MODE$TAIL_MARK
cat tmp_n_time_enc.txt |  awk '{sum+=\$1;mean=sum/$try_n_times};END{printf "%d\n",mean}' > ocv_jpubasic_encoder_$MODE$TAIL_MARK
rm -f tmp_n_time_dec.txt
rm -f tmp_n_time_enc.txt
rm -f obj.txt
rm -f daily_encjpeglog.txt
rm -f daily_decjpeglog.txt
EOF
    fi
}
