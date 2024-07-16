#!/bin/bash

# README
# Function: This script defines the ci test functions for 8 modules.
# USAGE（Interface）: bash ci_test.sh soc


if [ $# != 3 ] || [ "$1" != "soc" ]; then
    echo -e "\e[31m Error: Parameter not support \e[0m"
    echo -e "\e[36m USAGE: $0 mode dest_path function\e[0m"
    echo -e "\e[36m e.g.: $0 soc /home/fj/sophon_media/build/test soc_file_copy\e[0m"
    echo -e "\e[36m e.g.: $0 soc /home/linaro/test ci_test\e[0m"
    return 1
fi

# The two paths SCRIPT_DIR and MM_TOP_DIR need to be set to the actual path of sophon_media when the script is executed.
SCRIPT_DIR=$(dirname $(realpath "${BASH_SOURCE}"))
MM_TOP_DIR=$(dirname $(dirname "$SCRIPT_DIR"))
echo "SCRIPT_DIR: $SCRIPT_DIR"
echo "MM_TOP_DIR: $MM_TOP_DIR"

# Destination path on remote A2 evb board
dest=$2
echo "dest is set to: $dest"
echo "Command to execute: $3"

# Only one valid parameter "soc" is supported.
# SOC_IP_ADDR and SOC_PASS_WORD are not used
SOC_IP_ADDR=172.28.3.232
SOC_PASS_WORD=linaro123456
mkdir -p /tmp/test_soc/
chmod u+x /tmp/test_soc/
nfs_path=/tmp/test_soc/


function cp_file() {
    local filename=$1
    local dest_path=$2
    # The 'passwd' and 'ipaddress' parameters are not used in this local copy function, but are retained for compatibility
    # local passwd=$3
    # local ipaddress=$4
    local to_remote=$5 # 1: Local to local, 2: Local to local (keeping original parameter interface)

    if [ "$to_remote" = "1" ]; then
        # Direction 1: Copy from source to destination
        cp -r $filename "$dest_path"
    elif [ "$to_remote" = "2" ]; then
        # Direction 2: Copy from destination to source
        cp -r "$dest_path" "$filename"
    fi
}


function soc_file_copy() {
    cp_file ${MM_TOP_DIR}/install/ffmpeg/usr/local/bin/                        $dest/              $SOC_PASS_WORD $SOC_IP_ADDR 1 > /dev/null
    cp_file ${MM_TOP_DIR}/buildit/release/opencv/bin/                          $dest/              $SOC_PASS_WORD $SOC_IP_ADDR 1 > /dev/null
    cp_file ${MM_TOP_DIR}/buildit/samples/                                     $dest/              $SOC_PASS_WORD $SOC_IP_ADDR 1 > /dev/null
    cp_file ${MM_TOP_DIR}/buildit/libsophav/bmcv/                              $dest/              $SOC_PASS_WORD $SOC_IP_ADDR 1 > /dev/null
    cp_file ${MM_TOP_DIR}/buildit/libsophav/libyuv/lib/                        $dest/              $SOC_PASS_WORD $SOC_IP_ADDR 1 > /dev/null
    cp_file ${MM_TOP_DIR}/buildit/libsophav/bmcv/libbmcv.so                    $dest/lib/          $SOC_PASS_WORD $SOC_IP_ADDR 1 > /dev/null
    cp_file ${MM_TOP_DIR}/buildit/libsophav/jpeg/libbmjpeg.so                  $dest/lib/          $SOC_PASS_WORD $SOC_IP_ADDR 1 > /dev/null
    cp_file ${MM_TOP_DIR}/buildit/libsophav/video/dec/libbmvd.so               $dest/lib/          $SOC_PASS_WORD $SOC_IP_ADDR 1 > /dev/null
    cp_file ${MM_TOP_DIR}/buildit/libsophav/video/enc/libbmvenc.so             $dest/lib/          $SOC_PASS_WORD $SOC_IP_ADDR 1 > /dev/null
    cp_file ${MM_TOP_DIR}/buildit/release/opencv/lib/\*.so\*                   $dest/lib           $SOC_PASS_WORD $SOC_IP_ADDR 1 > /dev/null
    cp_file ${MM_TOP_DIR}/buildit/release/opencv/opencv-python/python2         $dest/lib/          $SOC_PASS_WORD $SOC_IP_ADDR 1 > /dev/null
    cp_file ${MM_TOP_DIR}/buildit/release/opencv/opencv-python/cv2.so          $dest/lib/          $SOC_PASS_WORD $SOC_IP_ADDR 1 > /dev/null
    cp_file ${MM_TOP_DIR}/install/ffmpeg/usr/local/lib/\*.so\*                 $dest/lib           $SOC_PASS_WORD $SOC_IP_ADDR 1 > /dev/null
    cp_file ${MM_TOP_DIR}/libsophav/3rdparty/libbmcv/lib/soc/libcmodel.so      $dest/lib/          $SOC_PASS_WORD $SOC_IP_ADDR 1 > /dev/null
    cp_file ${MM_TOP_DIR}/build/ci_test/                                       $dest/              $SOC_PASS_WORD $SOC_IP_ADDR 1 > /dev/null
    # cp_file ${MM_TOP_DIR}/stream/                                              $dest/              $SOC_PASS_WORD $SOC_IP_ADDR 1 > /dev/null
}

function run_on_target() {
    local RUN_SCRIPT=$1
    local MODE=$2
    if [ $MODE = pcie ]; then
        :
    elif [ $MODE = soc ];then
        cp_file $RUN_SCRIPT $dest/run.sh $SOC_PASS_WORD $SOC_IP_ADDR 1 > /dev/null
        # cp_file $INPUT $dest/$INPUT_COPY $SOC_PASS_WORD $SOC_IP_ADDR 1 > /dev/null
        cmd="export LD_LIBRARY_PATH=$dest/lib; \
            cd $dest; \
            chmod +x run.sh; \
            ./run.sh;"
        formatted_cmd=$(echo "$cmd" | tr -s ' ' | tr ';' '\n')
        echo -e "<SOC CMD>\n$formatted_cmd"

        eval "$cmd" || return $?
    fi
}

# refer_time=(
#     137    # bmcvtranscode
#     214    # videotranscode
#     14039  # jpubasic
#     15517  # jpubasic
#     382    # video_decode
#     191    # ocv_drawing
# )

function run_ff_bmcv_transcode() {
    echo -e "\e[36m \nRun ff_bmcv_transcode \e[0m"
    local MODE=$1
    local TAIL_MARK=$2
    local try_n_times=1
    if [ $MODE = pcie ];then
        :
    elif [ $MODE = soc ]; then
        if [[ ! -f $nfs_path/ff_bmcv_transcode_$MODE$TAIL_MARK ]]; then
            echo 1 > $nfs_path/ff_bmcv_transcode_$MODE$TAIL_MARK
        else
            chmod 777 $nfs_path/ff_bmcv_transcode_$MODE$TAIL_MARK
        fi
        old_var_fps=$(cat $nfs_path/ff_bmcv_transcode_$MODE$TAIL_MARK)
        echo "old_var_fps: $old_var_fps"
        writeBmcvTranscodeTest $nfs_path $MODE $TAIL_MARK $try_n_times
        run_on_target $nfs_path/tmp_daily.sh $MODE || return $?
        rm -f $nfs_path/tmp_daily.sh

        # cp_file $nfs_path/ff_bmcv_transcode0.mp4 $dest/ff_bmcv_transcode0.mp4 $SOC_PASS_WORD $SOC_IP_ADDR 2 >/dev/null
        # cmp $nfs_path/ff_bmcv_transcode0.mp4 $REF_FIG || return $?

        cp_file $nfs_path/ff_bmcv_transcode_$MODE$TAIL_MARK $dest/ff_bmcv_transcode_$MODE$TAIL_MARK $SOC_PASS_WORD $SOC_IP_ADDR 2 >/dev/null
        var_fps=$(cat $nfs_path/ff_bmcv_transcode_$MODE$TAIL_MARK)
        echo "var_fps: $var_fps"
        if [ $old_var_fps -eq 1 ];then
            speed_diff=100
        else
            speed_diff=`echo "$old_var_fps,$var_fps" | awk -F "," '{old_fps_float=$1+0.0;fps_float=$2+0.0;percent=(fps_float-old_fps_float)/old_fps_float}END{printf "%d",percent*100}'`
        fi
        # speed_diff is calculated using fps here
        echo "Speed_Diff : $speed_diff%"
        if [[ $speed_diff -lt -100 ]] || [[ $speed_diff -gt 100 ]]; then
            echo -e "\e[31m test_ff_bmcv_transcode : Log informations have not be recorded \e[0m"
            return 1
        elif [[ $speed_diff -lt -95 ]] || [[ $speed_diff -gt 95 ]]; then
            echo "ff_bmcv_transcode test pass (This should be your first time to create a log)" >> $logfile
        elif [[ $speed_diff -lt -10 ]] || [[ $speed_diff -gt 10 ]]; then
            echo -e "\e[31m test_ff_bmcv_transcode : Speed Abnormal \e[0m"
            echo $old_var_fps > $nfs_path/ff_bmcv_transcode_$MODE$TAIL_MARK
            return 1
        else
            echo -e "\e[32m test_ff_bmcv_transcode : Speed Normal \e[0m"
            echo "ff_bmcv_transcode test pass (Speed_Diff : $speed_diff%)" >> $logfile
        fi

    fi
}

function run_ff_video_decode() {
    echo -e "\e[36m \nRun ff_video_decode \e[0m"
    local MODE=$1
    local TAIL_MARK=$2
    local try_n_times=1
    # Note : The executable program of this example is named as test_bm_restart
    if [ $MODE = pcie ];then
        :
    elif [ $MODE = soc ]; then
        if [[ ! -f $nfs_path/ff_video_decode_$MODE$TAIL_MARK ]]; then
            echo 1 > $nfs_path/ff_video_decode_$MODE$TAIL_MARK
        else
            chmod 777 $nfs_path/ff_video_decode_$MODE$TAIL_MARK
        fi
        old_var_fps=$(cat $nfs_path/ff_video_decode_$MODE$TAIL_MARK)
        echo "old_var_fps: $old_var_fps"
        writeVideoDecodeTest $nfs_path $MODE $TAIL_MARK $try_n_times
        run_on_target $nfs_path/tmp_daily.sh $MODE || return $?
        rm -f out.png
        rm -f $nfs_path/tmp_daily.sh
        cp_file $nfs_path/ff_video_decode_$MODE$TAIL_MARK $dest/ff_video_decode_$MODE$TAIL_MARK $SOC_PASS_WORD $SOC_IP_ADDR 2 > /dev/null
        var_fps=$(cat $nfs_path/ff_video_decode_$MODE$TAIL_MARK)
        echo "var_fps: $var_fps"
        if [ $old_var_fps -eq 1 ];then
            speed_diff=100
        else
            speed_diff=`echo "$old_var_fps,$var_fps" | awk -F "," '{old_fps_float=$1+0.0;fps_float=$2+0.0;percent=(fps_float-old_fps_float)/old_fps_float}END{printf "%d",percent*100}'`
        fi
        # speed_diff is calculated using fps here
        echo "Speed_Diff : $speed_diff%"
        if [[ $speed_diff -lt -100 ]] || [[ $speed_diff -gt 100 ]]; then
            echo -e "\e[31m test_ff_video_decode : Log informations have not be recorded \e[0m"
            return 1
        elif [[ $speed_diff -lt -95 ]] || [[ $speed_diff -gt 95 ]]; then
            echo "test_ff_video_decode test pass (This should be your first time to create a log)" >> $logfile
        elif [[ $speed_diff -lt -10 ]] || [[ $speed_diff -gt 10 ]]; then
            echo "old_var_fps: $old_var_fps"
            echo "var_fps: $var_fps"
            # echo $old_var_fps > $nfs_path/ff_video_decode_$MODE$TAIL_MARK
            echo -e "\e[31m test_ff_video_decode : Speed Abnormal \e[0m"
            return 1
        else
            echo -e "\e[32m test_ff_video_decode : Speed Normal \e[0m"
            echo "ff_video_decode test pass (Speed_Diff : $speed_diff%)" >> $logfile
        fi
    fi
}

function run_videotranscode() {
    echo -e "\e[36m \nRun videotranscode \e[0m"
    local MODE=$1
    local TAIL_MARK=$2
    if [ $MODE = pcie ];then
        :
    elif [ $MODE = soc ]; then
        if [[ ! -f $nfs_path/videotranscode_$MODE$TAIL_MARK ]]; then
            echo 1 > $nfs_path/videotranscode_$MODE$TAIL_MARK
        else
            chmod 777 $nfs_path/videotranscode_$MODE$TAIL_MARK
        fi
        old_var_fps=$(cat $nfs_path/videotranscode_$MODE$TAIL_MARK)
        echo "old_var_fps: $old_var_fps"
        writeVideoTranscode $nfs_path $MODE $TAIL_MARK
        run_on_target $nfs_path/tmp_daily.sh $MODE || return $?
        rm -f $nfs_path/tmp_daily.sh

        cp_file $nfs_path/md5_videotranscode.txt $dest/md5_videotranscode.txt $SOC_PASS_WORD $SOC_IP_ADDR 2 >/dev/null
        cmp $nfs_path/md5_videotranscode.txt $dest/ci_test/ref/ref_md5_videotranscode.txt || return $?
        echo -e "\e[32mvideotranscode : Comparing output and reference successfully\e[0m"

        cp_file $nfs_path/videotranscode_$MODE$TAIL_MARK $dest/videotranscode_$MODE$TAIL_MARK $SOC_PASS_WORD $SOC_IP_ADDR 2 >/dev/null
        var_fps=$(cat $nfs_path/videotranscode_$MODE$TAIL_MARK)
        echo "var_fps: $var_fps"
        if [ $old_var_fps -eq 1 ];then
            speed_diff=100
        else
            speed_diff=`echo "$old_var_fps,$var_fps" | awk -F "," '{old_fps_float=$1+0.0;fps_float=$2+0.0;percent=(fps_float-old_fps_float)/old_fps_float}END{printf "%d",percent*100}'`
        fi
        # speed_diff is calculated using fps here
        echo "Speed_Diff : $speed_diff%"
        if [[ $speed_diff -lt -100 ]] || [[ $speed_diff -gt 100 ]]; then
            echo -e "\e[31m videotranscode : Log informations have not be recorded \e[0m"
            return 1
        elif [[ $speed_diff -lt -95 ]] || [[ $speed_diff -gt 95 ]]; then
            echo "videotranscode test pass (This should be your first time to create a log)" >> $logfile
        elif [[ $speed_diff -lt -10 ]] || [[ $speed_diff -gt 10 ]]; then
            # echo $old_var_fps > $nfs_path/videotranscode_$MODE$TAIL_MARK
            echo -e "\e[31m videotranscode : Speed Abnormal \e[0m"
            return 1
        else
            # echo $var_fps > $nfs_path/videotranscode_$MODE$TAIL_MARK
            echo -e "\e[32m videotranscode : Speed Normal \e[0m"
            echo "videotranscode test pass (Speed_Diff : $speed_diff%)" >> $logfile
        fi
    fi
}

function run_ocv_vidbasic() {
    echo -e "\e[36m \nRun ocv_vidbasic \e[0m"
    local MODE=$1
    local TAIL_MARK=$2
    if [ $MODE = pcie ];then
        :
    elif [ $MODE = soc ]; then
        if [[ ! -f $nfs_path/ocv_vidbasic_$MODE$TAIL_MARK ]]; then
            echo 1 > $nfs_path/ocv_vidbasic_$MODE$TAIL_MARK
        else
            chmod 777 $nfs_path/ocv_vidbasic_$MODE$TAIL_MARK
        fi
        # old_var_fps=$(cat $nfs_path/ocv_vidbasic_$MODE$TAIL_MARK)
        # echo "old_var_fps: $old_var_fps"
        writeVidBasicTest $nfs_path $MODE $TAIL_MARK
        run_on_target $nfs_path/tmp_daily.sh $MODE || return $?
        rm -f $nfs_path/tmp_daily.sh

        cp_file $nfs_path/md5_ocv_vidbasic.txt $dest/md5_ocv_vidbasic.txt $SOC_PASS_WORD $SOC_IP_ADDR 2 >/dev/null
        cmp $nfs_path/md5_ocv_vidbasic.txt $dest/ci_test/ref/ref_md5_ocv_vidbasic.txt || return $?
        echo -e "\e[32mvidbasic : Comparing output and reference successfully\e[0m"
        echo "ocv_vidbasic test pass" >> $logfile

    fi
}

function run_ocv_jpubasic() {
    echo -e "\e[36m \nRun ocv_jpubasic \e[0m"
    local MODE=$1
    local TAIL_MARK=$2
    local try_n_times=3
    if [ $MODE = pcie ];then
        :
    elif [ $MODE = soc ]; then
        if [[ ! -f $nfs_path/ocv_jpubasic_decoder_$MODE$TAIL_MARK ]]; then
            echo 1 > $nfs_path/ocv_jpubasic_decoder_$MODE$TAIL_MARK
        else
            chmod 777 $nfs_path/ocv_jpubasic_decoder_$MODE$TAIL_MARK
        fi
        if [[ ! -f $nfs_path/ocv_jpubasic_encoder_$MODE$TAIL_MARK ]]; then
            echo 1 > $nfs_path/ocv_jpubasic_encoder_$MODE$TAIL_MARK
        else
            chmod 777 $nfs_path/ocv_jpubasic_encoder_$MODE$TAIL_MARK
        fi
        old_var_time_dec=$(cat $nfs_path/ocv_jpubasic_decoder_$MODE$TAIL_MARK)
        old_var_time_enc=$(cat $nfs_path/ocv_jpubasic_encoder_$MODE$TAIL_MARK)
        echo "old_var_time_dec: $old_var_time_dec µs"
        echo "old_var_time_enc: $old_var_time_enc µs"
        writeJpuBasicTest $nfs_path $MODE $TAIL_MARK $try_n_times
        run_on_target $nfs_path/tmp_daily.sh $MODE || return $?
        rm -f out.png
        rm -f $nfs_path/tmp_daily.sh

        cp_file $nfs_path/out1.jpg $dest/out1.jpg $SOC_PASS_WORD $SOC_IP_ADDR 2 > /dev/null
        cp_file $nfs_path/out2.jpg $dest/out2.jpg $SOC_PASS_WORD $SOC_IP_ADDR 2 > /dev/null
        cp_file $nfs_path/out3.jpg $dest/out3.jpg $SOC_PASS_WORD $SOC_IP_ADDR 2 > /dev/null

        cmp $nfs_path/out1.jpg $dest/ci_test/ref/opencv_nv12_4k_test_yuvref.jpg || return $?
        cmp $nfs_path/out1.jpg $nfs_path/out2.jpg || return $?
        cmp $nfs_path/out1.jpg $nfs_path/out3.jpg || return $?
        echo -e "\e[32mjpubasic : Comparing output and reference successfully\e[0m"

        cp_file $nfs_path/ocv_jpubasic_decoder_$MODE$TAIL_MARK $dest/ocv_jpubasic_decoder_$MODE$TAIL_MARK $SOC_PASS_WORD $SOC_IP_ADDR 2 > /dev/null
        var_time_dec=$(cat $nfs_path/ocv_jpubasic_decoder_$MODE$TAIL_MARK)
        echo "var_time_dec: $var_time_dec µs"
        if [ $old_var_time_dec -eq 1 ];then
            speed_diff=100
        else
            speed_diff=`echo "$old_var_time_dec,$var_time_dec"|awk -F "," '{old_time_float=$1+0.0;time_float=$2+0.0;percent=(time_float-old_time_float)/old_time_float}END{printf "%d",percent*100}'`
        fi
        # speed_diff is calculated using time here
        echo "Decode_Speed_Diff : $speed_diff%"
        if [[ $speed_diff -lt -100 ]] || [[ $speed_diff -gt 100 ]]; then
            echo -e "\e[31m test_ocv_jpubasic(decode) : Log informations have not be recorded \e[0m"
            return 1
        elif [[ $speed_diff -lt -95 ]] || [[ $speed_diff -gt 95 ]]; then
            echo "ocv_jpubasic(decode) test pass (This should be your first time to create a log)" >> $logfile
        elif [[ $speed_diff -lt -10 ]] || [[ $speed_diff -gt 10 ]]; then
            # echo $old_var_time_dec > $nfs_path/ocv_jpubasic_decoder_$MODE$TAIL_MARK
            echo -e "\e[31m test_ocv_jpubasic(decode) : Speed Abnormal \e[0m"
            return 1
        else
            echo $var_time_dec > $nfs_path/ocv_jpubasic_decoder_$MODE$TAIL_MARK
            echo -e "\e[32m test_ocv_jpubasic(decode) : Speed Normal \e[0m"
            echo "ocv_jpubasic(decode) test pass (Speed_Diff : $speed_diff%)" >> $logfile
        fi
        cp_file $nfs_path/ocv_jpubasic_encoder_$MODE$TAIL_MARK $dest/ocv_jpubasic_encoder_$MODE$TAIL_MARK $SOC_PASS_WORD $SOC_IP_ADDR 2 > /dev/null
        var_time_enc=$(cat $nfs_path/ocv_jpubasic_encoder_$MODE$TAIL_MARK)
        echo "var_time_enc: $var_time_enc µs"
        if [ $old_var_time_enc -eq 1 ];then
            speed_diff=100
        else
            speed_diff=`echo "$old_var_time_enc,$var_time_enc"|awk -F "," '{old_time_float=$1+0.0;time_float=$2+0.0;percent=(time_float-old_time_float)/old_time_float}END{printf "%d",percent*100}'`
        fi
        # speed_diff is calculated using time here
        echo "Encode_Speed_Diff : $speed_diff%"
        if [[ $speed_diff -lt -100 ]] || [[ $speed_diff -gt 100 ]]; then
            echo -e "\e[31m test_ocv_jpubasic(encode) : Log informations have not be recorded \e[0m"
            return 1
        elif [[ $speed_diff -lt -95 ]] || [[ $speed_diff -gt 95 ]]; then
            echo "ocv_jpubasic(encode) test pass (This should be your first time to create a log)" >> $logfile
        elif [[ $speed_diff -lt -10 ]] || [[ $speed_diff -gt 10 ]]; then
            # echo $old_var_time_enc > $nfs_path/ocv_jpubasic_encoder_$MODE$TAIL_MARK
            echo -e "\e[31m test_ocv_jpubasic(encode) : Speed Abnormal \e[0m"
            return 1
        else
            echo $var_time_enc > $nfs_path/ocv_jpubasic_encoder_$MODE$TAIL_MARK
            echo -e "\e[32m test_ocv_jpubasic(encode) : Speed Normal \e[0m"
            echo "ocv_jpubasic(encode) test pass (Speed_Diff : $speed_diff%)" >> $logfile
        fi
    fi
}

function run_ocv_drawing() {
    echo -e "\e[36m \nRun ocv_drawing \e[0m"
    local MODE=$1
    local TAIL_MARK=$2
    if [ $MODE = pcie ];then
        :
    elif [ $MODE = soc ]; then
        if [[ ! -f $nfs_path/ocv_drawing_$MODE$TAIL_MARK ]]; then
            echo 1 > $nfs_path/ocv_drawing_$MODE$TAIL_MARK
        else
            chmod 777 $nfs_path/ocv_drawing_$MODE$TAIL_MARK
        fi
        old_var_fps=$(cat $nfs_path/ocv_drawing_$MODE$TAIL_MARK)
        echo "old_var_fps: $old_var_fps"
        writeOcvDrawing $nfs_path $MODE $TAIL_MARK
        run_on_target $nfs_path/tmp_daily.sh $MODE || return $?
        rm -f $nfs_path/tmp_daily.sh

        cp_file $nfs_path/md5_ocv_drawing.txt $dest/md5_ocv_drawing.txt $SOC_PASS_WORD $SOC_IP_ADDR 2 >/dev/null
        cmp $nfs_path/md5_ocv_drawing.txt $dest/ci_test/ref/ref_md5_ocv_drawing.txt || return $?
        echo -e "\e[32mocv_drawing : Comparing output and reference successfully\e[0m"

        cp_file $nfs_path/ocv_drawing_$MODE$TAIL_MARK $dest/ocv_drawing_$MODE$TAIL_MARK $SOC_PASS_WORD $SOC_IP_ADDR 2 >/dev/null
        var_fps=$(cat $nfs_path/ocv_drawing_$MODE$TAIL_MARK)
        echo "var_fps: $var_fps"
        if [ $old_var_fps -eq 1 ];then
            speed_diff=100
        else
            speed_diff=`echo "$old_var_fps,$var_fps" | awk -F "," '{old_fps_float=$1+0.0;fps_float=$2+0.0;percent=(fps_float-old_fps_float)/old_fps_float}END{printf "%d",percent*100}'`
        fi
        # speed_diff is calculated using fps here
        echo "Speed_Diff : $speed_diff%"
        if [[ $speed_diff -lt -100 ]] || [[ $speed_diff -gt 100 ]]; then
            echo -e "\e[31m ocv_drawing : Log informations have not be recorded \e[0m"
            return 1
        elif [[ $speed_diff -lt -95 ]] || [[ $speed_diff -gt 95 ]]; then
            echo "ocv_drawing test pass (This should be your first time to create a log)" >> $logfile
        elif [[ $speed_diff -lt -10 ]] || [[ $speed_diff -gt 10 ]]; then
            # echo $old_var_fps > $nfs_path/ocv_drawing_$MODE$TAIL_MARK
            echo -e "\e[31m ocv_drawing : Speed Abnormal \e[0m"
            return 1
        else
            echo -e "\e[32m ocv_drawing : Speed Normal \e[0m"
            echo "ocv_drawing test pass (Speed_Diff : $speed_diff%)" >> $logfile
        fi
    fi
}

function run_vpss_convert() {
    echo -e "\e[36m \nRun vpss_convert \e[0m"
    local MODE=$1
    local TAIL_MARK=$2
    local try_n_times=100
    if [ $MODE = pcie ];then
        :
    elif [ $MODE = soc ]; then
        if [[ ! -f $nfs_path/vpss_convert_$MODE$TAIL_MARK ]]; then
            echo 1 > $nfs_path/vpss_convert_$MODE$TAIL_MARK
        else
            chmod 777 $nfs_path/vpss_convert_$MODE$TAIL_MARK
        fi
        old_var_fps=$(cat $nfs_path/vpss_convert_$MODE$TAIL_MARK)
        echo "old_var_fps: $old_var_fps"
        writeVpssConvert $nfs_path $MODE $TAIL_MARK $try_n_times
        run_on_target $nfs_path/tmp_daily.sh $MODE || return $?
        rm -f $nfs_path/tmp_daily.sh

        cp_file $nfs_path/md5_vpss_convert.txt $dest/md5_vpss_convert.txt $SOC_PASS_WORD $SOC_IP_ADDR 2 >/dev/null
        cmp $nfs_path/md5_vpss_convert.txt $dest/ci_test/ref/ref_md5_vpss_convert.txt || return $?
        echo -e "\e[32mvpss_convert : Comparing output and reference successfully\e[0m"

        cp_file $nfs_path/vpss_convert_$MODE$TAIL_MARK $dest/vpss_convert_$MODE$TAIL_MARK $SOC_PASS_WORD $SOC_IP_ADDR 2 >/dev/null
        var_fps=$(cat $nfs_path/vpss_convert_$MODE$TAIL_MARK)
        echo "var_fps: $var_fps"
        if [ $old_var_fps -eq 1 ];then
            speed_diff=100
        else
            speed_diff=`echo "$old_var_fps,$var_fps" | awk -F "," '{old_fps_float=$1+0.0;fps_float=$2+0.0;percent=(fps_float-old_fps_float)/old_fps_float}END{printf "%d",percent*100}'`
        fi
        # speed_diff is calculated using fps here
        echo "Speed_Diff : $speed_diff%"
        if [[ $speed_diff -lt -100 ]] || [[ $speed_diff -gt 100 ]]; then
            echo -e "\e[31m vpss_convert : Log informations have not be recorded \e[0m"
            return 1
        elif [[ $speed_diff -lt -95 ]] || [[ $speed_diff -gt 95 ]]; then
            echo "vpss_convert test pass (This should be your first time to create a log)" >> $logfile
        elif [[ $speed_diff -lt -10 ]] || [[ $speed_diff -gt 10 ]]; then
            # echo $old_var_fps > $nfs_path/vpss_convert_$MODE$TAIL_MARK
            echo -e "\e[31m vpss_convert : Speed Abnormal \e[0m"
            return 1
        else
            echo -e "\e[32m vpss_convert : Speed Normal \e[0m"
            echo "vpss_convert test pass (Speed_Diff : $speed_diff%)" >> $logfile
        fi
    fi
}

function run_tpu_add_weight() {
    echo -e "\e[36m \nRun tpu_add_weight \e[0m"
    local MODE=$1
    local TAIL_MARK=$2
    local try_n_times=3
    if [ $MODE = pcie ];then
        :
    elif [ $MODE = soc ]; then
        if [[ ! -f $nfs_path/tpu_add_weight_$MODE$TAIL_MARK ]]; then
            echo 1 > $nfs_path/tpu_add_weight_$MODE$TAIL_MARK
        else
            chmod 777 $nfs_path/tpu_add_weight_$MODE$TAIL_MARK
        fi
        old_var_fps=$(cat $nfs_path/tpu_add_weight_$MODE$TAIL_MARK)
        echo "old_var_fps: $old_var_fps"
        writeTpuAddWeight $nfs_path $MODE $TAIL_MARK $try_n_times
        run_on_target $nfs_path/tmp_daily.sh $MODE || return $?
        rm -f $nfs_path/tmp_daily.sh

        cp_file $nfs_path/md5_tpu_add_weight.txt $dest/md5_tpu_add_weight.txt $SOC_PASS_WORD $SOC_IP_ADDR 2 >/dev/null
        cmp $nfs_path/md5_tpu_add_weight.txt $dest/ci_test/ref/ref_md5_tpu_add_weight.txt || return $?
        echo -e "\e[32mtpu_add_weight : Comparing output and reference successfully\e[0m"

        cp_file $nfs_path/tpu_add_weight_$MODE$TAIL_MARK $dest/tpu_add_weight_$MODE$TAIL_MARK $SOC_PASS_WORD $SOC_IP_ADDR 2 >/dev/null
        var_fps=$(cat $nfs_path/tpu_add_weight_$MODE$TAIL_MARK)
        echo "var_fps: $var_fps"
        if [ $old_var_fps -eq 1 ];then
            speed_diff=100
        else
            speed_diff=`echo "$old_var_fps,$var_fps" | awk -F "," '{old_fps_float=$1+0.0;fps_float=$2+0.0;percent=(fps_float-old_fps_float)/old_fps_float}END{printf "%d",percent*100}'`
        fi
        # speed_diff is calculated using fps here
        echo "Speed_Diff : $speed_diff%"
        if [[ $speed_diff -lt -100 ]] || [[ $speed_diff -gt 100 ]]; then
            echo -e "\e[31m tpu_add_weight : Log informations have not be recorded \e[0m"
            return 1
        elif [[ $speed_diff -lt -95 ]] || [[ $speed_diff -gt 95 ]]; then
            echo "tpu_add_weight test pass (This should be your first time to create a log)" >> $logfile
        elif [[ $speed_diff -lt -10 ]] || [[ $speed_diff -gt 10 ]]; then
            # echo $old_var_fps > $nfs_path/tpu_add_weight_$MODE$TAIL_MARK
            echo -e "\e[31m tpu_add_weight : Speed Abnormal \e[0m"
            return 1
        else
            echo -e "\e[32m tpu_add_weight : Speed Normal \e[0m"
            echo "tpu_add_weight test pass (Speed_Diff : $speed_diff%)" >> $logfile
        fi
    fi
}

function run_ive_stcandicorner() {
    echo -e "\e[36m \nRun ive_stcandicorner \e[0m"
    local MODE=$1
    local TAIL_MARK=$2
    local try_n_times=100
    if [ $MODE = pcie ];then
        :
    elif [ $MODE = soc ]; then
        if [[ ! -f $nfs_path/ive_stcandicorner_$MODE$TAIL_MARK ]]; then
            echo 1 > $nfs_path/ive_stcandicorner_$MODE$TAIL_MARK
        else
            chmod 777 $nfs_path/ive_stcandicorner_$MODE$TAIL_MARK
        fi
        old_var_fps=$(cat $nfs_path/ive_stcandicorner_$MODE$TAIL_MARK)
        echo "old_var_fps: $old_var_fps"
        writeIveStcandicorner $nfs_path $MODE $TAIL_MARK $try_n_times
        run_on_target $nfs_path/tmp_daily.sh $MODE || return $?
        rm -f $nfs_path/tmp_daily.sh

        cp_file $nfs_path/ive_stcandicorner_$MODE$TAIL_MARK $dest/ive_stcandicorner_$MODE$TAIL_MARK $SOC_PASS_WORD $SOC_IP_ADDR 2 >/dev/null
        var_fps=$(cat $nfs_path/ive_stcandicorner_$MODE$TAIL_MARK)
        echo "var_fps: $var_fps"
        if [ $old_var_fps -eq 1 ];then
            speed_diff=100
        else
            speed_diff=`echo "$old_var_fps,$var_fps" | awk -F "," '{old_fps_float=$1+0.0;fps_float=$2+0.0;percent=(fps_float-old_fps_float)/old_fps_float}END{printf "%d",percent*100}'`
        fi
        # speed_diff is calculated using fps here
        echo "Speed_Diff : $speed_diff%"
        if [[ $speed_diff -lt -100 ]] || [[ $speed_diff -gt 100 ]]; then
            echo -e "\e[31m ive_stcandicorner : Log informations have not be recorded \e[0m"
            return 1
        elif [[ $speed_diff -lt -95 ]] || [[ $speed_diff -gt 95 ]]; then
            echo "ive_stcandicorner test pass (This should be your first time to create a log)" >> $logfile
        elif [[ $speed_diff -lt -10 ]] || [[ $speed_diff -gt 10 ]]; then
            # echo $old_var_fps > $nfs_path/ive_stcandicorner_$MODE$TAIL_MARK
            echo -e "\e[31m ive_stcandicorner : Speed Abnormal \e[0m"
            return 1
        else
            echo -e "\e[32m ive_stcandicorner : Speed Normal \e[0m"
            echo "ive_stcandicorner test pass (Speed_Diff : $speed_diff%)" >> $logfile
        fi
    fi
}

function run_dpu_online() {
    echo -e "\e[36m \nRun dpu_online \e[0m"
    local MODE=$1
    local TAIL_MARK=$2
    local try_n_times=100
    if [ $MODE = pcie ];then
        :
    elif [ $MODE = soc ]; then
        if [[ ! -f $nfs_path/dpu_online_$MODE$TAIL_MARK ]]; then
            echo 1 > $nfs_path/dpu_online_$MODE$TAIL_MARK
        else
            chmod 777 $nfs_path/dpu_online_$MODE$TAIL_MARK
        fi
        old_var_fps=$(cat $nfs_path/dpu_online_$MODE$TAIL_MARK)
        echo "old_var_fps: $old_var_fps"
        writeDpuOnline $nfs_path $MODE $TAIL_MARK $try_n_times
        run_on_target $nfs_path/tmp_daily.sh $MODE || return $?
        rm -f $nfs_path/tmp_daily.sh

        cp_file $nfs_path/dpu_online_$MODE$TAIL_MARK $dest/dpu_online_$MODE$TAIL_MARK $SOC_PASS_WORD $SOC_IP_ADDR 2 >/dev/null
        var_fps=$(cat $nfs_path/dpu_online_$MODE$TAIL_MARK)
        echo "var_fps: $var_fps"
        if [ $old_var_fps -eq 1 ];then
            speed_diff=100
        else
            speed_diff=`echo "$old_var_fps,$var_fps" | awk -F "," '{old_fps_float=$1+0.0;fps_float=$2+0.0;percent=(fps_float-old_fps_float)/old_fps_float}END{printf "%d",percent*100}'`
        fi
        # speed_diff is calculated using fps here
        echo "Speed_Diff : $speed_diff%"
        if [[ $speed_diff -lt -100 ]] || [[ $speed_diff -gt 100 ]]; then
            echo -e "\e[31m dpu_online : Log informations have not be recorded \e[0m"
            return 1
        elif [[ $speed_diff -lt -95 ]] || [[ $speed_diff -gt 95 ]]; then
            echo "dpu_online test pass (This should be your first time to create a log)" >> $logfile
        elif [[ $speed_diff -lt -10 ]] || [[ $speed_diff -gt 10 ]]; then
            # echo $old_var_fps > $nfs_path/dpu_online_$MODE$TAIL_MARK
            echo -e "\e[31m dpu_online : Speed Abnormal \e[0m"
            return 1
        else
            echo -e "\e[32m dpu_online : Speed Normal \e[0m"
            echo "dpu_online test pass (Speed_Diff : $speed_diff%)" >> $logfile
        fi
    fi
}

function run_ldc_rot() {
    echo -e "\e[36m \nRun ldc_rot \e[0m"
    local MODE=$1
    local TAIL_MARK=$2
    local try_n_times=100
    if [ $MODE = pcie ];then
        :
    elif [ $MODE = soc ]; then
        if [[ ! -f $nfs_path/ldc_rot_$MODE$TAIL_MARK ]]; then
            echo 1 > $nfs_path/ldc_rot_$MODE$TAIL_MARK
        else
            chmod 777 $nfs_path/ldc_rot_$MODE$TAIL_MARK
        fi
        old_var_fps=$(cat $nfs_path/ldc_rot_$MODE$TAIL_MARK)
        echo "old_var_fps: $old_var_fps"
        writeLdcRot $nfs_path $MODE $TAIL_MARK $try_n_times
        run_on_target $nfs_path/tmp_daily.sh  $MODE || return $?
        rm -f $nfs_path/tmp_daily.sh

        cp_file $nfs_path/md5_ldc_rot.txt $dest/md5_ldc_rot.txt $SOC_PASS_WORD $SOC_IP_ADDR 2 >/dev/null
        cmp $nfs_path/md5_ldc_rot.txt $dest/ci_test/ref/ref_md5_ldc_rot.txt || return $?
        echo -e "\e[32mldc_rot : Comparing output and reference successfully\e[0m"

        cp_file $nfs_path/ldc_rot_$MODE$TAIL_MARK $dest/ldc_rot_$MODE$TAIL_MARK $SOC_PASS_WORD $SOC_IP_ADDR 2 >/dev/null
        var_fps=$(cat $nfs_path/ldc_rot_$MODE$TAIL_MARK)
        echo "var_fps: $var_fps"
        if [ $old_var_fps -eq 1 ];then
            speed_diff=100
        else
            speed_diff=`echo "$old_var_fps,$var_fps" | awk -F "," '{old_fps_float=$1+0.0;fps_float=$2+0.0;percent=(fps_float-old_fps_float)/old_fps_float}END{printf "%d",percent*100}'`
        fi
        # speed_diff is calculated using fps here
        echo "Speed_Diff : $speed_diff%"
        if [[ $speed_diff -lt -100 ]] || [[ $speed_diff -gt 100 ]]; then
            echo -e "\e[31m ldc_rot : Log informations have not be recorded \e[0m"
            return 1
        elif [[ $speed_diff -lt -95 ]] || [[ $speed_diff -gt 95 ]]; then
            echo "ldc_rot test pass (This should be your first time to create a log)" >> $logfile
        elif [[ $speed_diff -lt -10 ]] || [[ $speed_diff -gt 10 ]]; then
            # echo $old_var_fps > $nfs_path/ldc_rot_$MODE$TAIL_MARK
            echo -e "\e[31m ldc_rot : Speed Abnormal \e[0m"
            return 1
        else
            echo -e "\e[32m ldc_rot : Speed Normal \e[0m"
            echo "ldc_rot test pass (Speed_Diff : $speed_diff%)" >> $logfile
        fi
    fi
}

function run_dwa_fisheye() {
    echo -e "\e[36m \nRun dwa_fisheye \e[0m"
    local MODE=$1
    local TAIL_MARK=$2
    local try_n_times=100
    if [ $MODE = pcie ];then
        :
    elif [ $MODE = soc ]; then
        if [[ ! -f $nfs_path/dwa_fisheye_$MODE$TAIL_MARK ]]; then
            echo 1 > $nfs_path/dwa_fisheye_$MODE$TAIL_MARK
        else
            chmod 777 $nfs_path/dwa_fisheye_$MODE$TAIL_MARK
        fi
        old_var_fps=$(cat $nfs_path/dwa_fisheye_$MODE$TAIL_MARK)
        echo "old_var_fps: $old_var_fps"
        writeDwaFisheye $nfs_path $MODE $TAIL_MARK $try_n_times
        run_on_target $nfs_path/tmp_daily.sh $MODE || return $?
        rm -f $nfs_path/tmp_daily.sh

        cp_file $nfs_path/md5_dwa_fisheye.txt $dest/md5_dwa_fisheye.txt $SOC_PASS_WORD $SOC_IP_ADDR 2 >/dev/null
        cmp $nfs_path/md5_dwa_fisheye.txt $dest/ci_test/ref/ref_md5_dwa_fisheye.txt || return $?
        echo -e "\e[32mdwa_fisheye : Comparing output and reference successfully\e[0m"

        cp_file $nfs_path/dwa_fisheye_$MODE$TAIL_MARK $dest/dwa_fisheye_$MODE$TAIL_MARK $SOC_PASS_WORD $SOC_IP_ADDR 2 >/dev/null
        var_fps=$(cat $nfs_path/dwa_fisheye_$MODE$TAIL_MARK)
        echo "var_fps: $var_fps"
        if [ $old_var_fps -eq 1 ];then
            speed_diff=100
        else
            speed_diff=`echo "$old_var_fps,$var_fps" | awk -F "," '{old_fps_float=$1+0.0;fps_float=$2+0.0;percent=(fps_float-old_fps_float)/old_fps_float}END{printf "%d",percent*100}'`
        fi
        # speed_diff is calculated using fps here
        echo "Speed_Diff : $speed_diff%"
        if [[ $speed_diff -lt -100 ]] || [[ $speed_diff -gt 100 ]]; then
            echo -e "\e[31m dwa_fisheye : Log informations have not be recorded \e[0m"
            return 1
        elif [[ $speed_diff -lt -95 ]] || [[ $speed_diff -gt 95 ]]; then
            echo "dwa_fisheye test pass (This should be your first time to create a log)" >> $logfile
        elif [[ $speed_diff -lt -10 ]] || [[ $speed_diff -gt 10 ]]; then
            # echo $old_var_fps > $nfs_path/dwa_fisheye_$MODE$TAIL_MARK
            echo -e "\e[31m dwa_fisheye : Speed Abnormal \e[0m"
            return 1
        else
            echo -e "\e[32m dwa_fisheye : Speed Normal \e[0m"
            echo "dwa_fisheye test pass (Speed_Diff : $speed_diff%)" >> $logfile
        fi
    fi
}

function run_blend_4way() {
    echo -e "\e[36m \nRun blend_4way \e[0m"
    local MODE=$1
    local TAIL_MARK=$2
    local try_n_times=100
    if [ $MODE = pcie ];then
        :
    elif [ $MODE = soc ]; then
        if [[ ! -f $nfs_path/blend_4way_$MODE$TAIL_MARK ]]; then
            echo 1 > $nfs_path/blend_4way_$MODE$TAIL_MARK
        else
            chmod 777 $nfs_path/blend_4way_$MODE$TAIL_MARK
        fi
        old_var_fps=$(cat $nfs_path/blend_4way_$MODE$TAIL_MARK)
        echo "old_var_fps: $old_var_fps"
        writeBlend4way $nfs_path $MODE $TAIL_MARK $try_n_times

        run_on_target $nfs_path/tmp_daily.sh $MODE || return $?
        rm -f $nfs_path/tmp_daily.sh

        cp_file $nfs_path/md5_blend_4way.txt $dest/md5_blend_4way.txt $SOC_PASS_WORD $SOC_IP_ADDR 2 >/dev/null
        cmp $nfs_path/md5_blend_4way.txt $dest/ci_test/ref/ref_md5_blend_4way.txt || return $?
        echo -e "\e[32mblend_4way : Comparing output and reference successfully\e[0m"

        cp_file $nfs_path/blend_4way_$MODE$TAIL_MARK $dest/blend_4way_$MODE$TAIL_MARK $SOC_PASS_WORD $SOC_IP_ADDR 2 >/dev/null
        var_fps=$(cat $nfs_path/blend_4way_$MODE$TAIL_MARK)
        echo "var_fps: $var_fps"
        if [ $old_var_fps -eq 1 ];then
            speed_diff=100
        else
            speed_diff=`echo "$old_var_fps,$var_fps" | awk -F "," '{old_fps_float=$1+0.0;fps_float=$2+0.0;percent=(fps_float-old_fps_float)/old_fps_float}END{printf "%d",percent*100}'`
        fi
        # speed_diff is calculated using fps here
        echo "Speed_Diff : $speed_diff%"
        if [[ $speed_diff -lt -100 ]] || [[ $speed_diff -gt 100 ]]; then
            echo -e "\e[31m blend_4way : Log informations have not be recorded \e[0m"
            return 1
        elif [[ $speed_diff -lt -95 ]] || [[ $speed_diff -gt 95 ]]; then
            echo "blend_4way test pass (This should be your first time to create a log)" >> $logfile
        elif [[ $speed_diff -lt -10 ]] || [[ $speed_diff -gt 10 ]]; then
            # echo $old_var_fps > $nfs_path/blend_4way_$MODE$TAIL_MARK
            echo -e "\e[31m blend_4way : Speed Abnormal \e[0m"
            return 1
        else
            echo -e "\e[32m blend_4way : Speed Normal \e[0m"
            echo "blend_4way test pass (Speed_Diff : $speed_diff%)" >> $logfile
        fi
    fi
}

function run_daily_regression_mw_bm1686() {
    local RUN_MODE=$1
    chip_name=1686

    logfile="$nfs_path/ci_test_log.txt"
    if [ -f "$logfile" ]; then
        > "$logfile"
    else
        touch "$logfile"
    fi

    wget -nv -N -P $dest ftp://dailytest:dailytest@172.28.141.219/Athena2/Multimedia/ci_data_edge/stream.tar.gz
    tar -xzf $dest/stream.tar.gz -C $dest

    run_ff_bmcv_transcode $RUN_MODE _daily.txt || return $?  # 138
    # run_videotranscode $RUN_MODE _daily.txt || return $?
    run_ocv_vidbasic $RUN_MODE _daily.txt || return $? # 11
    # run_ocv_jpubasic $RUN_MODE _daily.txt || return $?
    run_ff_video_decode $RUN_MODE _daily.txt || return $?
    run_ocv_drawing $RUN_MODE _daily.txt|| return $?
    run_vpss_convert $RUN_MODE _daily.txt|| return $?
    run_tpu_add_weight $RUN_MODE _daily.txt || return $?
    # run_ive_stcandicorner $RUN_MODE _daily.txt || return $?
    run_dpu_online $RUN_MODE _daily.txt || return $?
    run_ldc_rot $RUN_MODE _daily.txt || return $?
    run_dwa_fisheye $RUN_MODE _daily.txt || return $?
    run_blend_4way $RUN_MODE _daily.txt || return $?
    ci_log=$(cat $logfile)
    echo -e "\e[32m---- All Test Complished ---- \e[0m"
    echo -e "\e[32m$ci_log\e[0m"
}

function ci_test() {
    source ${SCRIPT_DIR}/write_videotranscode_test.sh
    source ${SCRIPT_DIR}/write_bmcv_transcode_test.sh
    source ${SCRIPT_DIR}/write_videodecode_test.sh
    source ${SCRIPT_DIR}/write_vidbasic_test.sh
    source ${SCRIPT_DIR}/write_jpubasic_test.sh
    source ${SCRIPT_DIR}/write_ocv_drawing_test.sh
    source ${SCRIPT_DIR}/write_vpss_convert_test.sh
    source ${SCRIPT_DIR}/write_tpu_add_weight.sh
    source ${SCRIPT_DIR}/write_ive_stcandicorner_test.sh
    source ${SCRIPT_DIR}/write_dpu_online_test.sh
    source ${SCRIPT_DIR}/write_ldc_rot_test.sh
    source ${SCRIPT_DIR}/write_dwa_fisheye_test.sh
    source ${SCRIPT_DIR}/write_blend_4way_test.sh
    local RUN_MODE=$1
    if [ $RUN_MODE = pcie ];then
        run_daily_regression_mw_bm1686 $RUN_MODE
    elif [ $RUN_MODE = soc ];then
        run_daily_regression_mw_bm1686 $RUN_MODE
    else
        echo -e "\e[31m Error \e[0m: Parameter not support."
        return 1
    fi
}


# Execute different functions based on the third parameter
if [[ $3 == "soc_file_copy" ]]; then
    soc_file_copy
elif [[ $3 == "ci_test" ]]; then
    ci_test soc
else
    echo "Invalid command: $command"
    exit 1
fi




