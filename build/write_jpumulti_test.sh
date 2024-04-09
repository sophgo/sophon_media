# /bin/bash
# README:
#       wtite test statement into temp test script
function writeJpuMultiTest(){
    local nfs_path=$1
    local MODE=$2
    local TAIL_MARK=$3
    local try_n_times=$4
    if [ $MODE = pcie ];then
cat << EOF > $nfs_path/tmp_daily.sh
rm -f out*.png
rm -f out*.jpg
rm -f dump*.BGR
rm -f dump*.NV12
for((j=1;j<=$try_n_times;j++))
do
../samples/bin/test_ocv_jpumulti 1 opencv_input.jpg 1000 1 0 > $dest/sc5_test/daily_jpeglog.txt
cat daily_jpeglog.txt | grep Decoder0 > daily_decjpeglog.txt
cat daily_decjpeglog.txt | awk '{a=\$3}END{printf "%f\n",a}' >> tmp_n_time_dec.txt
done
cat tmp_n_time_dec.txt |  awk '{sum+=\$1;mean=sum/$try_n_times};END{printf "%d\n",mean}' > ocv_jpumulti_decoder_$MODE$TAIL_MARK
rm -f tmp_n_time_dec.txt
for((j=1;j<=$try_n_times;j++))
do
../samples/bin/test_ocv_jpumulti 2 opencv_input.jpg 1000 1 0 > $dest/sc5_test/daily_jpeglog.txt
cat daily_jpeglog.txt | grep Encoder0 > daily_encjpeglog.txt
cat daily_encjpeglog.txt | awk '{a=\$3}END{printf "%f\n",a}' >> tmp_n_time_enc.txt
done
cat tmp_n_time_enc.txt |  awk '{sum+=\$1;mean=sum/$try_n_times};END{printf "%d\n",mean}' > ocv_jpumulti_encoder_$MODE$TAIL_MARK
rm -f tmp_n_time_enc.txt
rm -f daily_jpeglog.txt
rm -f daily_encjpeglog.txt
rm -f daily_decjpeglog.txt
echo "import cv2 as cv" > tmp_python_cv2.py
echo "img = cv.imread(\"opencv_input.jpg\")" >> tmp_python_cv2.py
echo "cv.imwrite(\"out4.jpg\", img)" >> tmp_python_cv2.py
export PYTHONPATH=$dest/sc5_test/libs
python3 tmp_python_cv2.py
EOF
    elif [ $MODE = soc ];then
cat << EOF > $nfs_path/tmp_daily.sh
rm -f /dev/shm/*
# chmod a+x load.sh load_jpu.sh unload_jpu.sh unload.sh
# ./unload.sh
# ./unload_jpu.sh
# ./load.sh
# ./load_jpu.sh
rm -f out*.jpg
rm -f dump*.BGR
rm -f dump*.NV12
for((j=1;j<=$try_n_times;j++))
do
./samples/ocv_jpumulti/test_ocv_jpumulti 1 opencv_input.jpg 1000 1 0 2>&1 | tee daily_jpeglog.txt
cat daily_jpeglog.txt | grep Decoder0 > daily_decjpeglog.txt
cat daily_decjpeglog.txt | awk '{a=\$3}END{printf "%f\n",a}' >> tmp_n_time_dec.txt
done
cat tmp_n_time_dec.txt |  awk '{sum+=\$1;mean=sum/$try_n_times};END{printf "%d\n",mean}' > ocv_jpumulti_decoder_$MODE$TAIL_MARK
rm -f tmp_n_time_dec.txt
for((j=1;j<=$try_n_times;j++))
do
./samples/ocv_jpumulti/test_ocv_jpumulti 2 opencv_input.jpg 1000 1 0 2>&1 | tee daily_jpeglog.txt
cat daily_jpeglog.txt | grep Encoder0 > daily_encjpeglog.txt
cat daily_encjpeglog.txt | awk '{a=\$3}END{printf "%f\n",a}' >> tmp_n_time_enc.txt
done
cat tmp_n_time_enc.txt |  awk '{sum+=\$1;mean=sum/$try_n_times};END{printf "%d\n",mean}' > ocv_jpumulti_encoder_$MODE$TAIL_MARK
rm -f tmp_n_time_enc.txt
rm -f daily_jpeglog.txt
rm -f daily_encjpeglog.txt
rm -f daily_decjpeglog.txt
echo "import cv2 as cv" > tmp_python_cv2.py
echo "img = cv.imread(\"opencv_input.jpg\")" >> tmp_python_cv2.py
echo "cv.imwrite(\"out4.jpg\", img)" >> tmp_python_cv2.py
export PYTHONPATH=$dest/lib
python3 tmp_python_cv2.py
EOF
    fi
}
