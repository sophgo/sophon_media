#!/bin/bash

# options
TIMEOUT_DURATION=${TIMEOUT_DURATION:-300}
WORK_PATH=${WORK_PATH:-$PWD}
DATA_PATH=${DATA_PATH:-/data/perf}
LOG_NAME=${LOG_NAME:-perf.log}


test_case=(
    "jpumulti 3 $DATA_PATH/JPEG_1920x1088_yuv420_planar.jpg 1000 4 1"  # jpu
    "jpumulti 3 $DATA_PATH/JPEG_1920x1088_yuv420_nv21.jpg 1000 4 1"
    "jpumulti 3 $DATA_PATH/JPEG_1920x1088_yuv422_planar.jpg 1000 4 1"
    "jpumulti 3 $DATA_PATH/JPEG_1920x1088_yuv422sp_nv16.jpg 1000 4 1"
    "jpumulti 3 $DATA_PATH/JPEG_1920x1088_yuv400_planar.jpg 1000 4 1"
    "jpumulti 3 $DATA_PATH/JPEG_1920x1088_yuv444_planar.jpg 1000 4 1"
    "ffmpeg -i $DATA_PATH/station_1088p.h264 -is_dma_buffer 1 -c:v h265_bm -b:v 4M -y out_station_dma1_outh265.h265"  # vpu
    "ffmpeg -i $DATA_PATH/station_1088p.h264 -is_dma_buffer 1 -c:v h264_bm -b:v 4M -y out_station_dma1_outh264.h264"
    "ffmpeg -i $DATA_PATH/station_1088p.h265 -is_dma_buffer 0 -c:v h265_bm -b:v 4M -y out_station_dma0_outh265.ts"
    "ffmpeg -i $DATA_PATH/station_1088p.h265 -is_dma_buffer 0 -c:v h264_bm -b:v 4M -y out_station_dma0_outh264.mp4"
    "ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v h264_bm -cbcr_interleave 0 -extra_frame_buffer_num 16 -i $DATA_PATH/station_1088p_264.mp4 -c:v h265_bm -perf 1 -gop_preset 2 -g 29 -b:v 3M -y out_station_yuv420p_outh265.h265"
    "ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v h264_bm -cbcr_interleave 0 -extra_frame_buffer_num 16 -i $DATA_PATH/station_1088p_264.mp4 -c:v h265_bm -perf 1 -gop_preset 4 -g 30 -b:v 3M -y out_station_yuv420p_outh265.h265"
    "ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v h264_bm -cbcr_interleave 1 -extra_frame_buffer_num 16 -i $DATA_PATH/station_1088p_264.mp4 -c:v h265_bm -perf 1 -gop_preset 6 -g 32 -b:v 3M -y out_station_nv12_outh265.ts"
    "ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v h264_bm -cbcr_interleave 2 -extra_frame_buffer_num 16 -i $DATA_PATH/station_1088p_264.mp4 -c:v h265_bm -perf 1 -gop_preset 8 -g 32 -b:v 3M -y out_station_nv21_outh265.mp4"
    "test_ff_bmcv_transcode soc $DATA_PATH/station_1088p.h265 out_test_ff_bmcv_transcode.mp4 I420 h264_bm 1920 1088 25 3000 10 0 0"
    "test_ff_scale_transcode $DATA_PATH/station_1088p.h264 out_test_ff_scale_transcode.avi I420 h264_bm 1280 992 30 8000 16 0 1"
    "ffmpeg -c:v hevc_bm -output_format 101 -i $DATA_PATH/station_1088p_265.mp4 -vf "scale_bm=352:288:zero_copy=1" -c:v h264_bm -g 256 -b:v 256K -y out_cif_scale.264"
)

refer_time=(
    4140  # jpu
    4157
    5449
    5345
    3774
    7512
    20646  # vpu
    17747
    188720
    188987
    19964
    20148
    20246
    19619
    171095
    215308
    25269
)

refer_md5=(
    "b65d90b987068a6f72cb9361e439e00a"  #jpu
    "b65d90b987068a6f72cb9361e439e00a"
    "9472e3a2d3076224c68ecf87886b227e"
    "8157d9d39a08a75ea0f7558530808016"
    "501e79ba251960a4e6e8b9d4e1e76324"
    "217175b2d05983aa62d5798e1e241095"
    "ef7534ae313c70b13d2dc6d788330184"  # vpu
    "c97df9cfed3967e90e38130adfef2fe5"
    "9abdbb1869b904f943e929c5f834a642"
    "81f04e48219f021071a277f91d023bac"
    "67b4c07c4ed71225988c5e3f514d9660"
    "7d14cf81b60afd6bde50c45e0a5d0384"
    "bc3075f6de59553e7becb64e6411ea80"
    "299dca40fd3f016c39b567e620226872"
    "4715961c851d29aa06a1a89a50c5263c"
    "3d6e6aaf1d81d519de273a7e899494b4"
    "1ec84906e404dc65ee8f09c89e2771aa"
)


function run_cmd() {
    local CASE=$1
    local TIME=$2
    local MD5=$3

    echo -e "\e[36m\n*********************************************************\e[0m" | tee -a $LOG_NAME
    echo -e "\e[36m test case: $CASE \e[0m" | tee -a $LOG_NAME
    start_time=$(date +%s%3N)
    timeout $TIMEOUT_DURATION bash -c "$($CASE >> $LOG_NAME 2>&1)"
    exit_code=$?
    if [ $exit_code -eq 124 ]; then
        echo -e "\e[31m timeout!!! \e[0m" | tee -a $LOG_NAME
        return -1
    else
        end_time=$(date +%s%3N)
        sum_time=$[ $end_time - $start_time ]
        echo -e "\e[32m $start_time ---> $end_time" "Total: $sum_time ms, exit_code: $exit_code \e[0m" | tee -a $LOG_NAME

        diff_time=`echo "$TIME,$sum_time" | awk -F "," '{percent=($2-$1)/$1} END {printf "%d", percent*100}'`
        echo -e "\e[36m diff_time: $diff_time% \e[0m" | tee -a $LOG_NAME

        if [[ $diff_time -lt -10 ]] || [[ $diff_time -gt 10 ]]; then
            echo -e "\e[31m compare time failed!!! \e[0m" | tee -a $LOG_NAME
            return -1
        else
            echo -e "\e[32m compare time pass \e[0m" | tee -a $LOG_NAME
        fi

        md5_vals=($(md5sum out* | awk '{print $1}'))
        for md5 in "${md5_vals[@]}"; do
            if [ "$md5" != "$MD5" ]; then
                echo -e "\e[31m compare md5 failed!!! \e[0m" | tee -a $LOG_NAME
                return -1
            else
                echo -e "\e[32m compare md5 pass \e[0m" | tee -a $LOG_NAME
            fi
        done
    fi
    echo -e "\e[36m*********************************************************\n\e[0m" | tee -a $LOG_NAME
    rm -rf out*

    return 0
}

function perfmance_test() {
    echo "options:" | tee -a $LOG_NAME
    echo "  TIMEOUT_DURATION: ${TIMEOUT_DURATION}" | tee -a $LOG_NAME
    echo "  WORK_PATH: ${WORK_PATH}" | tee -a $LOG_NAME
    echo "  DATA_PATH: ${DATA_PATH}" | tee -a $LOG_NAME
    echo "  LOG_NAME: ${LOG_NAME}" | tee -a $LOG_NAME

    pushd $WORK_PATH
    rm -f $LOG_NAME
    rm -rf out*

    # run test case
    for (( i=0; i<${#test_case[@]}; i++ )); do
        run_cmd "${test_case[$i]}" ${refer_time[$i]} ${refer_md5[$i]} || return $?
    done

    popd

    echo -e "\e[32m---- All Test Complished ---- \e[0m"
    return 0
}

perfmance_test
