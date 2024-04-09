#!/bin/bash

# README
# Function: This script defines the daily test functions for 8 test cases.
# Note: The script only defines each test function without setting a new calling interface. Two new parameters are added to the "regression.sh" script to call the functions defined #       in this script. The added parameters are "pcie_daily_test" and "soc_daily_test".
# USAGE（Interface）: bash regression.sh pcie_daily_test
#                    bash regression.sh soc_daily_test



if [ $# != 1 ] ; then
    echo -e "\e[31m Error : Parameter not support \e[0m"
    echo -e "\e[36m USAGE: $0 soc_daily_test \e[0m"
    echo -e "\e[36m e.g.: $0 soc_daily_test \e[0m"
    return -1
fi


SCRIPT_DIR=/home/fj/sophon_media/build
MM_TOP_DIR=/home/fj/sophon_media
dest=/home/linaro/test

if [ $1 = soc_daily_test ]; then
    #run_regression_mw_bm1684
    # export PRODUCTFORM=soc
    SOC_IP_ADDR=172.28.3.232
    SOC_PASS_WORD=linaro
    mkdir -p /tmp/test_soc/
    chmod u+x /tmp/test_soc/
    nfs_path=/tmp/test_soc/

else
    echo -e "\e[31m Error: Parameter not support \e[0m"
    return -1
fi

function scp_file()
{

filename=$1
dest_path=$2
passwd=$3
ipaddres=$4
to_remote=$5 #if 1 localhost copy remote machine if 2 remote machine copy to loaclhost
#echo $filename
#echo $dest_path
/usr/bin/expect<<-EOF
set cmp 1
if { $to_remote == "1" } {
    spawn scp -r $filename linaro@$ipaddres:$dest_path
} else {
    spawn scp -r linaro@$ipaddres:$dest_path $filename
}

expect {
  "密码："
        {
          send "$passwd\n"
        }
   "pass"
        {
          send "$passwd\n"
        }
   "yes/no"
        {
          sleep 3
          send "yes\n"
          exp_continue
        }
   eof
    {
        sleep 3
        send_user "eof\n"
    }
}

set timeout 15
send "exit\r"
expect eof
EOF
}

function soc_file_copy()
{
    install_dir=${MM_TOP_DIR}
    scp_file ${install_dir}/install/ffmpeg/usr/local/bin/                        $dest/              $SOC_PASS_WORD $SOC_IP_ADDR 1 > /dev/null
    scp_file ${install_dir}/buildit/release/opencv/bin/                          $dest/              $SOC_PASS_WORD $SOC_IP_ADDR 1 > /dev/null
    scp_file ${install_dir}/buildit/samples/                                     $dest/              $SOC_PASS_WORD $SOC_IP_ADDR 1 > /dev/null
    scp_file ${install_dir}/buildit/libsophav/bmcv/                              $dest/              $SOC_PASS_WORD $SOC_IP_ADDR 1 > /dev/null
    scp_file ${install_dir}/buildit/libsophav/libyuv/lib/                        $dest/              $SOC_PASS_WORD $SOC_IP_ADDR 1 > /dev/null
    scp_file ${install_dir}/buildit/libsophav/bmcv/libbmcv.so                    $dest/lib/          $SOC_PASS_WORD $SOC_IP_ADDR 1 > /dev/null
    scp_file ${install_dir}/buildit/libsophav/jpeg/libbmjpeg.so                  $dest/lib/          $SOC_PASS_WORD $SOC_IP_ADDR 1 > /dev/null
    scp_file ${install_dir}/buildit/libsophav/video/dec/libbmvd.so               $dest/lib/          $SOC_PASS_WORD $SOC_IP_ADDR 1 > /dev/null
    scp_file ${install_dir}/buildit/libsophav/video/enc/libbmvenc.so             $dest/lib/          $SOC_PASS_WORD $SOC_IP_ADDR 1 > /dev/null
    scp_file ${install_dir}/buildit/release/opencv/lib/                          $dest/              $SOC_PASS_WORD $SOC_IP_ADDR 1 > /dev/null
    scp_file ${install_dir}/buildit/release/opencv/opencv-python/python2         $dest/lib/          $SOC_PASS_WORD $SOC_IP_ADDR 1 > /dev/null
    scp_file ${install_dir}/buildit/release/opencv/opencv-python/cv2.so          $dest/lib/          $SOC_PASS_WORD $SOC_IP_ADDR 1 > /dev/null
    scp_file ${install_dir}/install/ffmpeg/usr/local/lib/                        $dest/              $SOC_PASS_WORD $SOC_IP_ADDR 1 > /dev/null
    scp_file ${install_dir}/res                                                  $dest/              $SOC_PASS_WORD $SOC_IP_ADDR 1 > /dev/null
    scp_file ${MM_TOP_DIR}/libsophav/3rdparty/libbmcv/lib/soc/                   $dest/lib/          $SOC_PASS_WORD $SOC_IP_ADDR 1 > /dev/null
}

function run_on_target(){
    local RUN_SCRIPT=$1
    local INPUT=$2
    local INPUT_COPY=$3
    local MODE=$4
    if [ $MODE = pcie ]; then
        scp_file $RUN_SCRIPT $dest/sc5_test/run.sh linaro123456 172.28.3.197 1 > /dev/null
        scp_file $INPUT $dest/sc5_test/$INPUT_COPY linaro123456 172.28.3.197 1 > /dev/null
        cmd="cd $dest/sc5_test;cp -r $dest/ffmpeglib/lib/* $dest/sc5_test/libs/ffmpeg;cp -r $dest/opencv-python/opencv-python/* $dest/sc5_test/libs/;cp -r $dest/opencvlib/lib/* $dest/sc5_test/libs/; cp -r $dest/opencvbin/bin/* $dest/sc5_test/bin ; cp -r $dest/decodelib/lib/* $dest/sc5_test/libs/decode/;cp -r $dest/bmcvbmlib/pcie/* $dest/sc5_test/libs;source envsetup.sh; ./run.sh;"
        echo "<SOC CMD> $cmd"
        writeRemoteOperation $PCIE_IP_ADDR "$cmd"

    elif [ $MODE = soc ];then
        scp_file $RUN_SCRIPT $dest/run.sh $SOC_PASS_WORD $SOC_IP_ADDR 1 > /dev/null
        scp_file $INPUT $dest/$INPUT_COPY $SOC_PASS_WORD $SOC_IP_ADDR 1 > /dev/null
        cmd="export LD_LIBRARY_PATH=$dest/lib; \
            cd $dest; \
            chmod +x run.sh; \
            ./run.sh;"
        formatted_cmd=$(echo "$cmd" | tr -s ' ' | tr ';' '\n')
        echo "<SOC CMD> $formatted_cmd"
        writeRemoteOperation $SOC_IP_ADDR "$cmd"
    fi
        (
            python3 /tmp/tmp_soc.py
        ) || return $?
}

function run_ff_bmcv_transcode(){
    echo -e "\e[36m Run ff_bmcv_transcode \e[0m"
    local INPUT_VIDEO=$1
    local REF_FIG=$2
    local MODE=$3
    local TAIL_MARK=$4
    # local try_n_times=8
    local try_n_times=3
    # frames=10
    if [ $MODE = pcie ];then
        if [[ ! -f $nfs_path/ff_bmcv_transcode_$MODE$TAIL_MARK ]]; then
            echo 1 > $nfs_path/ff_bmcv_transcode_$MODE$TAIL_MARK
        else
            chmod 777 $nfs_path/ff_bmcv_transcode_$MODE$TAIL_MARK
        fi
        old_var_fps=$(cat $nfs_path/ff_bmcv_transcode_$MODE$TAIL_MARK)
        writeVideoTranscodeTest $nfs_path $MODE $TAIL_MARK $try_n_times
        chmod u+x $nfs_path/tmp_daily.sh
        run_on_target $nfs_path/tmp_daily.sh $INPUT_VIDEO opencv_input.mp4 $MODE || return $?
        scp_file $nfs_path/ff_bmcv_transcode_$MODE$TAIL_MARK /data/jenkins/usr/local/vid_test/sc5_test/ff_bmcv_transcode_$MODE$TAIL_MARK linaro123456 172.28.3.197 2 > /dev/null
        var_fps=$(cat $nfs_path/ff_bmcv_transcode_$MODE$TAIL_MARK)
        rm -f out.png
        rm -f $nfs_path/tmp_daily.sh
        if [ $old_var_fps -eq 1 ];then
            speed_diff=100
        else
            speed_diff=`echo "$old_var_fps,$var_fps" | awk -F "," '{old_fps_float=$1+0.0;fps_float=$2+0.0;percent=(fps_float-old_fps_float)/old_fps_float}END{printf "%d",percent*100}'`
        fi
        # speed_diff is calculated using fps here
        echo "Speed_Diff : $speed_diff%"
        if [[ $speed_diff -lt -100 ]] || [[ $speed_diff -gt 100 ]]; then
            echo -e "\e[31m test_ff_bmcv_transcode : Log informations have not be recorded \e[0m"
            return -1
        elif [[ $speed_diff -lt -95 ]] || [[ $speed_diff -gt 95 ]]; then
            echo "ff_bmcv_transcode test pass (This should be your first time to create a log)" >> $nfs_path/daily_test_log.txt
        elif [[ $speed_diff -lt -10 ]] || [[ $speed_diff -gt 10 ]]; then
            echo -e "\e[31m test_ff_bmcv_transcode : Speed Abnormal \e[0m"
            return -1
        else
            echo $var_fps > $nfs_path/ff_bmcv_transcode_$MODE$TAIL_MARK
            echo -e "\e[32m test_ff_bmcv_transcode : Speed Normal \e[0m"
            echo "ff_bmcv_transcode test pass (Speed_Diff : $speed_diff%)" >> $nfs_path/daily_test_log.txt
        fi

    elif [ $MODE = soc ]; then
        if [[ ! -f $nfs_path/ff_bmcv_transcode_$MODE$TAIL_MARK ]]; then
            echo 1 > $nfs_path/ff_bmcv_transcode_$MODE$TAIL_MARK
        else
            chmod 777 $nfs_path/ff_bmcv_transcode_$MODE$TAIL_MARK
        fi
        old_var_fps=$(cat $nfs_path/ff_bmcv_transcode_$MODE$TAIL_MARK)
        echo "old_var_fps: $old_var_fps"
        writeVideoTranscodeTest $nfs_path $MODE $TAIL_MARK $try_n_times
        run_on_target $nfs_path/tmp_daily.sh $INPUT_VIDEO bilibiligd.264 $MODE || return $?
        rm -f $nfs_path/tmp_daily.sh

        scp_file $nfs_path/ff_bmcv_transcode0.mp4 $dest/ff_bmcv_transcode0.mp4 $SOC_PASS_WORD $SOC_IP_ADDR 2 >/dev/null
        cmp $nfs_path/ff_bmcv_transcode0.mp4 $REF_FIG || return ?

        scp_file $nfs_path/ff_bmcv_transcode_$MODE$TAIL_MARK $dest/ff_bmcv_transcode_$MODE$TAIL_MARK $SOC_PASS_WORD $SOC_IP_ADDR 2 >/dev/null
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
            return -1
        elif [[ $speed_diff -lt -95 ]] || [[ $speed_diff -gt 95 ]]; then
            echo "ff_bmcv_transcode test pass (This should be your first time to create a log)" >> $nfs_path/daily_test_log.txt
        elif [[ $speed_diff -lt -10 ]] || [[ $speed_diff -gt 10 ]]; then
            echo -e "\e[31m test_ff_bmcv_transcode : Speed Abnormal \e[0m"
            return -1
        else
            echo -e "\e[32m test_ff_bmcv_transcode : Speed Normal \e[0m"
            echo "ff_bmcv_transcode test pass (Speed_Diff : $speed_diff%)" >> $nfs_path/daily_test_log.txt
        fi

    fi
}

function run_ff_video_encode(){
    echo -e "\e[36m Run ff_video_encode \e[0m"
    local INPUT_VIDEO=$1
    local REF_FIG=$2
    local MODE=$3
    local TAIL_MARK=$4
    if [ $MODE = pcie ];then
        if [[ ! -f $nfs_path/ff_video_encode_$MODE$TAIL_MARK ]]; then
            echo 1 > $nfs_path/ff_video_encode_$MODE$TAIL_MARK
        else
            chmod 777 $nfs_path/ff_video_encode_$MODE$TAIL_MARK
        fi
        old_log=$(cat $nfs_path/ff_video_encode_$MODE$TAIL_MARK)
        writeVideoEncodeTest $nfs_path $MODE $TAIL_MARK
        chmod u+x $nfs_path/tmp_daily.sh
        run_on_target $nfs_path/tmp_daily.sh $INPUT_VIDEO opencv_input.mp4 $MODE || return $?
        scp_file $nfs_path/ff_video_encode_$MODE$TAIL_MARK /data/jenkins/usr/local/vid_test/sc5_test/ff_video_encode_$MODE$TAIL_MARK linaro123456 172.28.3.197 2 > /dev/null
        cur_log=$(cat $nfs_path/ff_video_encode_$MODE$TAIL_MARK)
        rm -f out.png
        rm -f $nfs_path/tmp_daily.sh
        if [ "$old_log" = "1" ];then
            echo "test_ff_video_encode test pass (This should be your first time to create a log)" >> $nfs_path/daily_test_log.txt
        elif [ "$old_log" = "$cur_log" ];then
            echo "ff_video_encode test pass" >> $nfs_path/daily_test_log.txt
        else
            echo -e "\e[31m test_ff_video_encode : Execution exception \e[0m"
            return -1
        fi

    elif [ $MODE = soc ]; then
        # if [[ ! -f $nfs_path/ff_video_encode_$MODE$TAIL_MARK ]]; then
        #     echo 1 > $nfs_path/ff_video_encode_$MODE$TAIL_MARK
        # else
        #     chmod 777 $nfs_path/ff_video_encode_$MODE$TAIL_MARK
        # fi
        # old_log=$(cat $nfs_path/ff_video_encode_$MODE$TAIL_MARK)
        # writeVideoEncodeTest $nfs_path $MODE $TAIL_MARK
        # chmod u+x $nfs_path/tmp_daily.sh
        # run_on_target $nfs_path/tmp_daily.sh $INPUT_VIDEO 1080p_yuv420p.yuv $MODE || return $?
        # rm -f out.png
        # rm -f $nfs_path/tmp_daily.sh
        # scp_file $nfs_path/ff_video_encode_$MODE$TAIL_MARK $dest/ff_video_encode_$MODE$TAIL_MARK $SOC_PASS_WORD $SOC_IP_ADDR 2 > /dev/null
        # cur_log=$(cat $nfs_path/ff_video_encode_$MODE$TAIL_MARK)
        # if [ "$old_log" = "1" ];then
        #     echo "test_ff_video_encode test pass (This should be your first time to create a log)" >> $nfs_path/daily_test_log.txt
        # elif [ "$old_log" = "$cur_log" ];then
        #     echo "ff_video_encode test pass" >> $nfs_path/daily_test_log.txt
        # else
        #     echo -e "\e[31m test_ff_video_encode : Execution exception \e[0m"
        #     return -1
        # fi

        writeVideoEncodeTest $nfs_path $MODE $TAIL_MARK
        chmod u+x $nfs_path/tmp_daily.sh
        run_on_target $nfs_path/tmp_daily.sh $INPUT_VIDEO 1080p_yuv420p.yuv $MODE || return $?
        rm -f $nfs_path/tmp_daily.sh
        scp_file $nfs_path/ff_video_encode.mp4 $dest/ff_video_encode.mp4 $SOC_PASS_WORD $SOC_IP_ADDR 2 > /dev/null
        if ! cmp $REF_FIG $nfs_path/ff_video_encode.mp4; then
            echo -e "\e[31m test_ff_video_encode : Execution exception \e[0m"
            return 255
        fi
        echo "ff_video_encode test pass" >> $nfs_path/daily_test_log.txt
    fi
}

function run_ff_video_decode(){
    echo -e "\e[36m Run ff_video_decode \e[0m"
    local INPUT_VIDEO=$1
    local REF_FIG=$2
    local MODE=$3
    local TAIL_MARK=$4
    local try_n_times=1
    # Note : The executable program of this example is named as test_bm_restart
    if [ $MODE = pcie ];then
        if [[ ! -f $nfs_path/ff_video_decode_$MODE$TAIL_MARK ]]; then
            echo 1 > $nfs_path/ff_video_decode_$MODE$TAIL_MARK
        else
            chmod 777 $nfs_path/ff_video_decode_$MODE$TAIL_MARK
        fi
        old_var_fps=$(cat $nfs_path/ff_video_decode_$MODE$TAIL_MARK)
        writeVideoDecodeTest $nfs_path $MODE $TAIL_MARK $try_n_times
        chmod u+x $nfs_path/tmp_daily.sh
        run_on_target $nfs_path/tmp_daily.sh $INPUT_VIDEO opencv_input.mp4 $MODE || return $?
        scp_file $nfs_path/ff_video_decode_$MODE$TAIL_MARK /data/jenkins/usr/local/vid_test/sc5_test/ff_video_decode_$MODE$TAIL_MARK linaro123456 172.28.3.197 2 > /dev/null
        var_fps=$(cat $nfs_path/ff_video_decode_$MODE$TAIL_MARK)
        rm -f out.png
        rm -f $nfs_path/tmp_daily.sh
        if [ $old_var_fps -eq 1 ];then
            speed_diff=100
        else
            speed_diff=`echo "$old_var_fps,$var_fps" | awk -F "," '{old_fps_float=$1+0.0;fps_float=$2+0.0;percent=(fps_float-old_fps_float)/old_fps_float}END{printf "%d",percent*100}'`
        fi
        # speed_diff is calculated using fps here
        echo "Speed_Diff : $speed_diff%"
        if [[ $speed_diff -lt -100 ]] || [[ $speed_diff -gt 100 ]]; then
            echo -e "\e[31m test_ff_video_decode : Log informations have not be recorded \e[0m"
            return -1
        elif [[ $speed_diff -lt -95 ]] || [[ $speed_diff -gt 95 ]]; then
            echo "test_ff_video_decode test pass (This should be your first time to create a log)" >> $nfs_path/daily_test_log.txt
        elif [[ $speed_diff -lt -10 ]]; then
            echo -e "\e[31m test_ff_video_decode : Speed Abnormal \e[0m"
            return -1
        else
            echo $var_fps > $nfs_path/ff_bmcv_transcode_$MODE$TAIL_MARK
            echo -e "\e[32m test_ff_video_decode : Speed Normal \e[0m"
            echo "ff_video_decode test pass (Speed_Diff : $speed_diff%)" >> $nfs_path/daily_test_log.txt
        fi

    elif [ $MODE = soc ]; then
        if [[ ! -f $nfs_path/ff_video_decode_$MODE$TAIL_MARK ]]; then
            echo 1 > $nfs_path/ff_video_decode_$MODE$TAIL_MARK
        else
            chmod 777 $nfs_path/ff_video_decode_$MODE$TAIL_MARK
        fi
        old_var_fps=$(cat $nfs_path/ff_video_decode_$MODE$TAIL_MARK)
        writeVideoDecodeTest $nfs_path $MODE $TAIL_MARK $try_n_times
        run_on_target $nfs_path/tmp_daily.sh $INPUT_VIDEO bilibiligd.264 $MODE || return $?
        rm -f out.png
        rm -f $nfs_path/tmp_daily.sh
        scp_file $nfs_path/ff_video_decode_$MODE$TAIL_MARK $dest/ff_video_decode_$MODE$TAIL_MARK $SOC_PASS_WORD $SOC_IP_ADDR 2 > /dev/null
        var_fps=$(cat $nfs_path/ff_video_decode_$MODE$TAIL_MARK)
        if [ $old_var_fps -eq 1 ];then
            speed_diff=100
        else
            speed_diff=`echo "$old_var_fps,$var_fps" | awk -F "," '{old_fps_float=$1+0.0;fps_float=$2+0.0;percent=(fps_float-old_fps_float)/old_fps_float}END{printf "%d",percent*100}'`
        fi
        # speed_diff is calculated using fps here
        echo "Speed_Diff : $speed_diff%"
        if [[ $speed_diff -lt -100 ]] || [[ $speed_diff -gt 100 ]]; then
            echo -e "\e[31m test_ff_video_decode : Log informations have not be recorded \e[0m"
            return -1
        elif [[ $speed_diff -lt -95 ]] || [[ $speed_diff -gt 95 ]]; then
            echo "test_ff_video_decode test pass (This should be your first time to create a log)" >> $nfs_path/daily_test_log.txt
        elif [[ $speed_diff -lt -10 ]] || [[ $speed_diff -gt 10 ]]; then
            echo "old_var_fps: $old_var_fps"
            echo "var_fps: $var_fps"
            echo -e "\e[31m test_ff_video_decode : Speed Abnormal \e[0m"
            return -1
        else
            echo -e "\e[32m test_ff_video_decode : Speed Normal \e[0m"
            echo "ff_video_decode test pass (Speed_Diff : $speed_diff%)" >> $nfs_path/daily_test_log.txt
        fi
    fi
}

function run_ocv_vidmulti(){
    echo -e "\e[36m Run ocv_vidmulti \e[0m"
    local INPUT_VIDEO=$1
    local REF_FIG=$2
    local MODE=$3
    local TAIL_MARK=$4
    local try_n_times=1
    # try_n_times=10
    if [ $MODE = pcie ];then
        if [[ ! -f $nfs_path/ocv_vidmulti_$MODE$TAIL_MARK ]]; then
            echo 1 > $nfs_path/ocv_vidmulti_$MODE$TAIL_MARK
        else
            chmod 777 $nfs_path/ocv_vidmulti_$MODE$TAIL_MARK
        fi
        old_var_fps=$(cat $nfs_path/ocv_vidmulti_$MODE$TAIL_MARK)
        writeVidMultiTest $nfs_path $MODE $TAIL_MARK $try_n_times
        chmod u+x $nfs_path/tmp_daily.sh
        run_on_target $nfs_path/tmp_daily.sh $INPUT_VIDEO a.264 $MODE || return $?
        scp_file $nfs_path/ocv_vidmulti_$MODE$TAIL_MARK /data/jenkins/usr/local/vid_test/sc5_test/ocv_vidmulti_$MODE$TAIL_MARK linaro123456 172.28.3.197 2 > /dev/null
        var_fps=$(cat $nfs_path/ocv_vidmulti_$MODE$TAIL_MARK)
        rm -f out.png
        rm -f $nfs_path/tmp_daily.sh
        if [ $old_var_fps -eq 1 ];then
            speed_diff=100
        else
            speed_diff=`echo "$old_var_fps,$var_fps" | awk -F "," '{old_fps_float=$1+0.0;fps_float=$2+0.0;percent=(fps_float-old_fps_float)/old_fps_float}END{printf "%d",percent*100}'`
        fi
        # speed_diff is calculated using fps here
        echo "Speed_Diff : $speed_diff"
        if [[ $speed_diff -lt -100 ]] || [[ $speed_diff -gt 100 ]]; then
            echo -e "\e[31m test_ocv_vidmulti : Log informations have not be recorded \e[0m"
            return -1
        elif [[ $speed_diff -lt -95 ]] || [[ $speed_diff -gt 95 ]]; then
            echo "test_ocv_vidmulti test pass (This should be your first time to create a log)" >> $nfs_path/daily_test_log.txt
        elif [[ $speed_diff -lt -10 ]]; then
            echo -e "\e[31m test_ocv_vidmulti : Speed Abnormal \e[0m"
            return -1
        else
            echo $var_fps > $nfs_path/ocv_vidmulti_$MODE$TAIL_MARK
            echo -e "\e[32m test_ocv_vidmulti : Speed Normal \e[0m"
            echo "ocv_vidmulti test pass (Speed_Diff : $speed_diff%) " >> $nfs_path/daily_test_log.txt
        fi

    elif [ $MODE = soc ]; then
        if [[ ! -f $nfs_path/ocv_vidmulti_$MODE$TAIL_MARK ]]; then
            echo 1 > $nfs_path/ocv_vidmulti_$MODE$TAIL_MARK
        else
            chmod 777 $nfs_path/ocv_vidmulti_$MODE$TAIL_MARK
        fi
        old_var_fps=$(cat $nfs_path/ocv_vidmulti_$MODE$TAIL_MARK)
        echo "old_var_fps: $old_var_fps"
        writeVidMultiTest $nfs_path $MODE $TAIL_MARK $try_n_times
        run_on_target $nfs_path/tmp_daily.sh $INPUT_VIDEO bilibiligd.264 $MODE || return $?
        rm -f out.png
        rm -f $nfs_path/tmp_daily.sh
        scp_file $nfs_path/ocv_vidmulti_$MODE$TAIL_MARK $dest/ocv_vidmulti_$MODE$TAIL_MARK $SOC_PASS_WORD $SOC_IP_ADDR 2 > /dev/null
        var_fps=$(cat $nfs_path/ocv_vidmulti_$MODE$TAIL_MARK)
        echo "var_fps: $var_fps"
        if [ $old_var_fps -eq 1 ];then
            speed_diff=100
        else
            speed_diff=`echo "$old_var_fps,$var_fps" | awk -F "," '{old_fps_float=$1+0.0;fps_float=$2+0.0;percent=(fps_float-old_fps_float)/old_fps_float}END{printf "%d",percent*100}'`
        fi
        # speed_diff is calculated using fps here
        echo "Speed_Diff : $speed_diff"
        if [[ $speed_diff -lt -100 ]] || [[ $speed_diff -gt 100 ]]; then
            echo -e "\e[31m test_ocv_vidmulti : Log informations have not be recorded \e[0m"
            return -1
        elif [[ $speed_diff -lt -95 ]] || [[ $speed_diff -gt 95 ]]; then
            echo "test_ocv_vidmulti test pass (This should be your first time to create a log)" >> $nfs_path/daily_test_log.txt
        elif [[ $speed_diff -lt -10 ]] || [[ $speed_diff -gt 10 ]]; then
            echo -e "\e[31m test_ocv_vidmulti : Speed Abnormal \e[0m"
            return -1
        else
            # echo $var_fps > $nfs_path/ocv_vidmulti_$MODE$TAIL_MARK
            echo -e "\e[32m test_ocv_vidmulti : Speed Normal \e[0m"
            echo "ocv_vidmulti test pass (Speed_Diff : $speed_diff%) " >> $nfs_path/daily_test_log.txt
        fi
    fi
}

function run_ocv_vidbasic(){
    echo -e "\e[36m Run ocv_vidbasic \e[0m"
    local INPUT_VIDEO=$1
    local REF_FIG=$2
    local MODE=$3
    local TAIL_MARK=$4
    frames=1000
    if [ $MODE = pcie ];then
        if [[ ! -f $nfs_path/ocv_vidbasic_$MODE$TAIL_MARK ]]; then
            echo 1 > $nfs_path/ocv_vidbasic_$MODE$TAIL_MARK
        else
            chmod 777 $nfs_path/ocv_vidbasic_$MODE$TAIL_MARK
        fi
        old_log=$(cat $nfs_path/ocv_vidbasic_$MODE$TAIL_MARK)
        writeVidBasicTest $nfs_path $MODE $TAIL_MARK $try_n_times
        chmod u+x $nfs_path/tmp_daily.sh
        run_on_target $nfs_path/tmp_daily.sh $INPUT_VIDEO opencv_input.mp4 $MODE || return $?
        scp_file $nfs_path/ocv_vidbasic_$MODE$TAIL_MARK /data/jenkins/usr/local/vid_test/sc5_test/ocv_vidbasic_$MODE$TAIL_MARK linaro123456 172.28.3.197 2 > /dev/null
        rm -f out.png
        rm -f $nfs_path/tmp_daily.sh
        cur_log=$(cat $nfs_path/ocv_vidbasic_$MODE$TAIL_MARK)
        if [ "$old_log" = "1" ];then
            echo "ocv_vidbasic test pass (This should be your first time to create a log)" >> $nfs_path/daily_test_log.txt
        elif [ "$old_log" = "$cur_log" ];then
            echo "ocv_vidbasic test pass" >> $nfs_path/daily_test_log.txt
        else
            echo -e "\e[31m test_ocv_vidbasic : Execution exception \e[0m"
            return -1
        fi

    elif [ $MODE = soc ]; then
        if [[ ! -f $nfs_path/ocv_vidbasic_$MODE$TAIL_MARK ]]; then
            echo 1 > $nfs_path/ocv_vidbasic_$MODE$TAIL_MARK
        else
            chmod 777 $nfs_path/ocv_vidbasic_$MODE$TAIL_MARK
        fi
        old_var_fps=$(cat $nfs_path/ocv_vidbasic_$MODE$TAIL_MARK)
        echo "old_var_fps: $old_var_fps"
        writeVidBasicTest $nfs_path $MODE $TAIL_MARK $try_n_times
        run_on_target $nfs_path/tmp_daily.sh $INPUT_VIDEO opencv_input.mp4 $MODE || return $?
        rm -f $nfs_path/tmp_daily.sh
        scp_file $nfs_path/ocv_vidbasic_$MODE$TAIL_MARK $dest/ocv_vidbasic_$MODE$TAIL_MARK $SOC_PASS_WORD $SOC_IP_ADDR 2 >/dev/null

        var_fps=$(cat $nfs_path/ocv_vidbasic_$MODE$TAIL_MARK)
        echo "var_fps: $var_fps"
        if [ $old_var_fps -eq 1 ];then
            speed_diff=100
        else
            speed_diff=`echo "$old_var_fps,$var_fps" | awk -F "," '{old_fps_float=$1+0.0;fps_float=$2+0.0;percent=(fps_float-old_fps_float)/old_fps_float}END{printf "%d",percent*100}'`
        fi
        # speed_diff is calculated using fps here
        echo "Speed_Diff : $speed_diff%"
        if [[ $speed_diff -lt -100 ]] || [[ $speed_diff -gt 100 ]]; then
            echo -e "\e[31m ocv_vidbasic : Log informations have not be recorded \e[0m"
            return -1
        elif [[ $speed_diff -lt -95 ]] || [[ $speed_diff -gt 95 ]]; then
            echo "ocv_vidbasic test pass (This should be your first time to create a log)" >> $nfs_path/daily_test_log.txt
        elif [[ $speed_diff -lt -10 ]] || [[ $speed_diff -gt 10 ]]; then
            echo -e "\e[31m ocv_vidbasic : Speed Abnormal \e[0m"
            return -1
        else
            # echo $var_fps > $nfs_path/ocv_vidbasic_$MODE$TAIL_MARK
            echo -e "\e[32m ocv_vidbasic : Speed Normal \e[0m"
            echo "ocv_vidbasic test pass (Speed_Diff : $speed_diff%)" >> $nfs_path/daily_test_log.txt
        fi
    fi
}


function run_ocv_video_xcode(){
    echo -e "\e[36m Run ocv_video_xcode \e[0m"
    local INPUT_VIDEO=$1
    local REF_FIG=$2
    local MODE=$3
    local TAIL_MARK=$4
    if [ $MODE = pcie ];then
        if [[ ! -f $nfs_path/ocv_video_xcode_$MODE$TAIL_MARK ]]; then
            echo 1 > $nfs_path/ocv_video_xcode_$MODE$TAIL_MARK
        else
            chmod 777 $nfs_path/ocv_video_xcode_$MODE$TAIL_MARK
        fi
        old_log=$(cat $nfs_path/ocv_video_xcode_$MODE$TAIL_MARK)
        writeVideoXcodeTest $nfs_path $MODE $TAIL_MARK $try_n_times
        chmod u+x $nfs_path/tmp_daily.sh
        run_on_target $nfs_path/tmp_daily.sh $INPUT_VIDEO opencv_input.mp4 $MODE || return $?
        scp_file $nfs_path/ocv_video_xcode_$MODE$TAIL_MARK /data/jenkins/usr/local/vid_test/sc5_test/ocv_video_xcode_$MODE$TAIL_MARK linaro123456 172.28.3.197 2 > /dev/null
        rm -f out.png
        rm -f $nfs_path/tmp_daily.sh
        cur_log=$(cat $nfs_path/ocv_video_xcode_$MODE$TAIL_MARK)
        if [ "$old_log" = "1" ];then
            echo "ocv_video_xcode test pass (This should be your first time to create a log)" >> $nfs_path/daily_test_log.txt
        elif [ "$old_log" = "$cur_log" ];then
            echo "ocv_video_xcode test pass" >> $nfs_path/daily_test_log.txt
        else
            echo -e "\e[31m test_ocv_video_xcode : Execution exception \e[0m"
            return -1
        fi

    elif [ $MODE = soc ]; then
        if [[ ! -f $nfs_path/ocv_video_xcode_$MODE$TAIL_MARK ]]; then
            echo 1 > $nfs_path/ocv_video_xcode_$MODE$TAIL_MARK
        else
            chmod 777 $nfs_path/ocv_video_xcode_$MODE$TAIL_MARK
        fi
        old_var_fps=$(cat $nfs_path/ocv_video_xcode_$MODE$TAIL_MARK)
        echo "old_var_fps: $old_var_fps"
        writeVideoXcodeTest $nfs_path $MODE $TAIL_MARK $try_n_times
        run_on_target $nfs_path/tmp_daily.sh $INPUT_VIDEO bilibiligd.264 $MODE || return $?
        rm -f $nfs_path/tmp_daily.sh
        scp_file $nfs_path/ocv_video_xcode_$MODE$TAIL_MARK $dest/ocv_video_xcode_$MODE$TAIL_MARK $SOC_PASS_WORD $SOC_IP_ADDR 2 > /dev/null

        var_fps=$(cat $nfs_path/ocv_video_xcode_$MODE$TAIL_MARK)
        echo "var_fps: $var_fps"
        if [ $old_var_fps -eq 1 ];then
            speed_diff=100
        else
            speed_diff=`echo "$old_var_fps,$var_fps" | awk -F "," '{old_fps_float=$1+0.0;fps_float=$2+0.0;percent=(fps_float-old_fps_float)/old_fps_float}END{printf "%d",percent*100}'`
        fi
        # speed_diff is calculated using fps here
        echo "Speed_Diff : $speed_diff%"
        if [[ $speed_diff -lt -100 ]] || [[ $speed_diff -gt 100 ]]; then
            echo -e "\e[31m ocv_video_xcode : Log informations have not be recorded \e[0m"
            return -1
        elif [[ $speed_diff -lt -95 ]] || [[ $speed_diff -gt 95 ]]; then
            echo "ocv_video_xcode test pass (This should be your first time to create a log)" >> $nfs_path/daily_test_log.txt
        elif [[ $speed_diff -lt -10 ]]|| [[ $speed_diff -gt 10 ]]; then
            echo -e "\e[31m ocv_video_xcode : Speed Abnormal \e[0m"
            return -1
        else
            echo -e "\e[32m ocv_video_xcode : Speed Normal \e[0m"
            echo "ocv_video_xcode test pass (Speed_Diff : $speed_diff%)" >> $nfs_path/daily_test_log.txt
        fi
    fi
}

function run_ocv_jpubasic(){
    echo -e "\e[36m Run ocv_jpubasic \e[0m"
    local INPUT_FIG=$1
    local REF_FIG=$2
    local MODE=$3
    local TAIL_MARK=$4
    local try_n_times=3
    if [ $MODE = pcie ];then
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
        old_var_fps_dec=$(cat $nfs_path/ocv_jpubasic_decoder_$MODE$TAIL_MARK)
        old_var_fps_enc=$(cat $nfs_path/ocv_jpubasic_encoder_$MODE$TAIL_MARK)
        writeJpuBasicTest $nfs_path $MODE $TAIL_MARK $try_n_times
        chmod u+x $nfs_path/tmp_daily.sh
        run_on_target $nfs_path/tmp_daily.sh $INPUT_FIG opencv_input.jpg $MODE || return $?
        rm -f out.png
        rm -f $nfs_path/tmp_daily.sh
        scp_file $nfs_path/ocv_jpubasic_decoder_$MODE$TAIL_MARK /data/jenkins/usr/local/vid_test/sc5_test/ocv_jpubasic_decoder_$MODE$TAIL_MARK linaro123456 172.28.3.197 2 > /dev/null
        var_fps_dec=$(cat $nfs_path/ocv_jpumulti_decoder_$MODE$TAIL_MARK)
        if [ $old_var_fps_dec -eq 1 ];then
            speed_diff=100
        else
            speed_diff=`echo "$old_var_fps_dec,$var_fps_dec"|awk -F "," '{old_fps_float=$1+0.0;fps_float=$2+0.0;percent=(fps_float-old_fps_float)/old_fps_float}END{printf "%d",percent*100}'`
        fi
        # speed_diff is calculated using time here
        echo "Decode_Speed_Diff : $speed_diff%"
        if [[ $speed_diff -lt -100 ]] || [[ $speed_diff -gt 100 ]]; then
            echo -e "\e[31m test_ocv_jpubasic(decode) : Log informations have not be recorded \e[0m"
            return -1
        elif [[ $speed_diff -lt -95 ]] || [[ $speed_diff -gt 95 ]]; then
            echo "ocv_jpubasic(decode) test pass (This should be your first time to create a log)" >> $nfs_path/daily_test_log.txt
        elif [[ $speed_diff -lt -10 ]] || [[ $speed_diff -gt 10 ]]; then
            echo -e "\e[31m test_ocv_jpubasic(decode) : Speed Abnormal \e[0m"
            return -1
        else
            echo $var_fps_dec > $nfs_path/ocv_jpubasic_decoder_$MODE$TAIL_MARK
            echo -e "\e[32m test_ocv_jpubasic(decode) : Speed Normal \e[0m"
            echo "ocv_jpubasic(decode) test pass (Speed_Diff : $speed_diff%)" >> $nfs_path/daily_test_log.txt
        fi
        scp_file $nfs_path/ocv_jpubasic_encoder_$MODE$TAIL_MARK /data/jenkins/usr/local/vid_test/sc5_test/ocv_jpubasic_encoder_$MODE$TAIL_MARK linaro123456 172.28.3.197 2 > /dev/null
        var_fps_enc=$(cat $nfs_path/ocv_jpumulti_encoder_$MODE$TAIL_MARK)
        if [ $old_var_fps_enc -eq 1 ];then
            speed_diff=100
        else
            speed_diff=`echo "$old_var_fps_enc,$var_fps_enc"|awk -F "," '{old_fps_float=$1+0.0;fps_float=$2+0.0;percent=(fps_float-old_fps_float)/old_fps_float}END{printf "%d",percent*100}'`
        fi
        # speed_diff is calculated using time here
        echo "Encode_Speed_Diff : $speed_diff%"
        if [[ $speed_diff -lt -100 ]] || [[ $speed_diff -gt 100 ]]; then
            echo -e "\e[31m test_ocv_jpubasic(encode) : Log informations have not be recorded \e[0m"
            return -1
        elif [[ $speed_diff -lt -95 ]] || [[ $speed_diff -gt 95 ]]; then
            echo "ocv_jpubasic(encode) test pass (This should be your first time to create a log)" >> $nfs_path/daily_test_log.txt
        elif [[ $speed_diff -lt -10 ]] || [[ $speed_diff -gt 10 ]]; then
            echo -e "\e[31m test_ocv_jpubasic(encode) : Speed Abnormal \e[0m"
            return -1
        else
            echo $var_fps_enc > $nfs_path/ocv_jpubasic_encoder_$MODE$TAIL_MARK
            echo -e "\e[32m test_ocv_jpubasic(encode) : Speed Normal \e[0m"
            echo "ocv_jpubasic(encode) test pass (Speed_Diff : $speed_diff%)" >> $nfs_path/daily_test_log.txt
        fi

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
        old_var_fps_dec=$(cat $nfs_path/ocv_jpubasic_decoder_$MODE$TAIL_MARK)
        old_var_fps_enc=$(cat $nfs_path/ocv_jpubasic_encoder_$MODE$TAIL_MARK)
        echo "old_var_fps_dec: $old_var_fps_dec"
        echo "old_var_fps_enc: $old_var_fps_enc"
        writeJpuBasicTest $nfs_path $MODE $TAIL_MARK $try_n_times
        run_on_target $nfs_path/tmp_daily.sh $INPUT_FIG opencv_input.jpg $MODE || return $?
        rm -f out.png
        rm -f $nfs_path/tmp_daily.sh

        scp_file $nfs_path/out1.jpg $dest/out1.jpg $SOC_PASS_WORD $SOC_IP_ADDR 2 > /dev/null
        scp_file $nfs_path/out2.jpg $dest/out2.jpg $SOC_PASS_WORD $SOC_IP_ADDR 2 > /dev/null
        scp_file $nfs_path/out3.jpg $dest/out3.jpg $SOC_PASS_WORD $SOC_IP_ADDR 2 > /dev/null

        cmp $nfs_path/out1.jpg $2 || return $?
        cmp $nfs_path/out1.jpg $nfs_path/out2.jpg || return $?
        cmp $nfs_path/out1.jpg $nfs_path/out3.jpg || return $?

        scp_file $nfs_path/ocv_jpubasic_decoder_$MODE$TAIL_MARK $dest/ocv_jpubasic_decoder_$MODE$TAIL_MARK $SOC_PASS_WORD $SOC_IP_ADDR 2 > /dev/null
        var_fps_dec=$(cat $nfs_path/ocv_jpubasic_decoder_$MODE$TAIL_MARK)
        echo "var_fps_dec: $var_fps_dec"
        if [ $old_var_fps_dec -eq 1 ];then
            speed_diff=100
        else
            speed_diff=`echo "$old_var_fps_dec,$var_fps_dec"|awk -F "," '{old_fps_float=$1+0.0;fps_float=$2+0.0;percent=(fps_float-old_fps_float)/old_fps_float}END{printf "%d",percent*100}'`
        fi
        # speed_diff is calculated using time here
        echo "Decode_Speed_Diff : $speed_diff%"
        if [[ $speed_diff -lt -100 ]] || [[ $speed_diff -gt 100 ]]; then
            echo -e "\e[31m test_ocv_jpubasic(decode) : Log informations have not be recorded \e[0m"
            return -1
        elif [[ $speed_diff -lt -95 ]] || [[ $speed_diff -gt 95 ]]; then
            echo "ocv_jpubasic(decode) test pass (This should be your first time to create a log)" >> $nfs_path/daily_test_log.txt
        elif [[ $speed_diff -lt -10 ]] || [[ $speed_diff -gt 10 ]]; then
            echo -e "\e[31m test_ocv_jpubasic(decode) : Speed Abnormal \e[0m"
            return -1
        else
            echo $var_fps_dec > $nfs_path/ocv_jpubasic_decoder_$MODE$TAIL_MARK
            echo -e "\e[32m test_ocv_jpubasic(decode) : Speed Normal \e[0m"
            echo "ocv_jpubasic(decode) test pass (Speed_Diff : $speed_diff%)" >> $nfs_path/daily_test_log.txt
        fi
        scp_file $nfs_path/ocv_jpubasic_encoder_$MODE$TAIL_MARK $dest/ocv_jpubasic_encoder_$MODE$TAIL_MARK $SOC_PASS_WORD $SOC_IP_ADDR 2 > /dev/null
        var_fps_enc=$(cat $nfs_path/ocv_jpubasic_encoder_$MODE$TAIL_MARK)
        echo "var_fps_enc: $var_fps_enc"
        if [ $old_var_fps_enc -eq 1 ];then
            speed_diff=100
        else
            speed_diff=`echo "$old_var_fps_enc,$var_fps_enc"|awk -F "," '{old_fps_float=$1+0.0;fps_float=$2+0.0;percent=(fps_float-old_fps_float)/old_fps_float}END{printf "%d",percent*100}'`
        fi
        # speed_diff is calculated using time here
        echo "Encode_Speed_Diff : $speed_diff%"
        if [[ $speed_diff -lt -100 ]] || [[ $speed_diff -gt 100 ]]; then
            echo -e "\e[31m test_ocv_jpubasic(encode) : Log informations have not be recorded \e[0m"
            return -1
        elif [[ $speed_diff -lt -95 ]] || [[ $speed_diff -gt 95 ]]; then
            echo "ocv_jpubasic(encode) test pass (This should be your first time to create a log)" >> $nfs_path/daily_test_log.txt
        elif [[ $speed_diff -lt -10 ]] || [[ $speed_diff -gt 10 ]]; then
            echo -e "\e[31m test_ocv_jpubasic(encode) : Speed Abnormal \e[0m"
            return -1
        else
            echo $var_fps_enc > $nfs_path/ocv_jpubasic_encoder_$MODE$TAIL_MARK
            echo -e "\e[32m test_ocv_jpubasic(encode) : Speed Normal \e[0m"
            echo "ocv_jpubasic(encode) test pass (Speed_Diff : $speed_diff%)" >> $nfs_path/daily_test_log.txt
        fi
    fi
}

function run_ocv_jpumulti()
{
    echo -e "\e[36m Run ocv_jpumulti \e[0m"
    local INPUT_FIG=$1
    local REF_FIG=$2
    local MODE=$3
    local TAIL_MARK=$4
    local try_n_times=3
    if [ $MODE = pcie ]; then
        if [[ ! -f $nfs_path/ocv_jpumulti_decoder_$MODE$TAIL_MARK ]]; then
            echo 1 > $nfs_path/ocv_jpumulti_decoder_$MODE$TAIL_MARK
        else
            chmod 777 $nfs_path/ocv_jpumulti_decoder_$MODE$TAIL_MARK
        fi
        if [[ ! -f $nfs_path/ocv_jpumulti_encoder_$MODE$TAIL_MARK ]]; then
            echo 1 > $nfs_path/ocv_jpumulti_encoder_$MODE$TAIL_MARK
        else
            chmod 777 $nfs_path/ocv_jpumulti_encoder_$MODE$TAIL_MARK
        fi
        old_var_fps_dec=$(cat $nfs_path/ocv_jpumulti_decoder_$MODE$TAIL_MARK)
        old_var_fps_enc=$(cat $nfs_path/ocv_jpumulti_encoder_$MODE$TAIL_MARK)
        writeJpuMultiTest $nfs_path $MODE $TAIL_MARK $try_n_times
        chmod u+x $nfs_path/tmp_daily.sh
        run_on_target $nfs_path/tmp_daily.sh $INPUT_FIG opencv_input.jpg $MODE || return $?
        rm -f $nfs_path/tmp_daily.sh
        scp_file $nfs_path/ocv_jpumulti_decoder_$MODE$TAIL_MARK /data/jenkins/usr/local/vid_test/sc5_test/ocv_jpumulti_decoder_$MODE$TAIL_MARK linaro123456 172.28.3.197 2 > /dev/null
        var_fps_dec=$(cat $nfs_path/ocv_jpumulti_decoder_$MODE$TAIL_MARK)
        if [ $old_var_fps_dec -eq 1 ];then
            speed_diff=100
        else
            speed_diff=`echo "$old_var_fps_dec,$var_fps_dec"|awk -F "," '{old_fps_float=$1+0.0;fps_float=$2+0.0;percent=(fps_float-old_fps_float)/old_fps_float}END{printf "%d",percent*100}'`
        fi
        # speed_diff is calculated using time here
        echo "Decode_Speed_Diff : $speed_diff%"
        if [[ $speed_diff -lt -100 ]] || [[ $speed_diff -gt 100 ]]; then
            echo -e "\e[31m test_ocv_jpumulti(decode) : Log informations have not be recorded \e[0m"
            return -1
        elif [[ $speed_diff -lt -95 ]] || [[ $speed_diff -gt 95 ]]; then
            echo "ocv_jpumulti(decode) test pass (This should be your first time to create a log)" >> $nfs_path/daily_test_log.txt
        elif [[ $speed_diff -gt 10 ]]; then
            echo -e "\e[31m test_ocv_jpumulti(decode) : Speed Abnormal \e[0m"
            return -1
        else
            echo $var_fps_dec > $nfs_path/ocv_jpumulti_decoder_$MODE$TAIL_MARK
            echo -e "\e[32m test_ocv_jpumulti(decode) : Speed Normal \e[0m"
            echo "ocv_jpumulti(decode) test pass (Speed_Diff : $speed_diff%)" >> $nfs_path/daily_test_log.txt
        fi
        scp_file $nfs_path/ocv_jpumulti_encoder_$MODE$TAIL_MARK /data/jenkins/usr/local/vid_test/sc5_test/ocv_jpumulti_encoder_$MODE$TAIL_MARK linaro123456 172.28.3.197 2 > /dev/null
        var_fps_enc=$(cat $nfs_path/ocv_jpumulti_encoder_$MODE$TAIL_MARK)
        if [ $old_var_fps_enc -eq 1 ];then
            speed_diff=100
        else
            speed_diff=`echo "$old_var_fps_enc,$var_fps_enc"|awk -F "," '{old_fps_float=$1+0.0;fps_float=$2+0.0;percent=(fps_float-old_fps_float)/old_fps_float}END{printf "%d",percent*100}'`
        fi
        # speed_diff is calculated using time here
        echo "Encode_Speed_Diff : $speed_diff%"
        if [[ $speed_diff -lt -100 ]] || [[ $speed_diff -gt 100 ]]; then
            echo -e "\e[31m test_ocv_jpumulti(encode) : Log informations have not be recorded \e[0m"
            return -1
        elif [[ $speed_diff -lt -95 ]] || [[ $speed_diff -gt 95 ]]; then
            echo "ocv_jpumulti(encode) test pass (This should be your first time to create a log)" >> $nfs_path/daily_test_log.txt
        elif [[ $speed_diff -gt 10 ]]; then
            echo -e "\e[31m test_ocv_jpumulti(encode) : Speed Abnormal \e[0m"
            return -1
        else
            echo $var_fps_enc > $nfs_path/ocv_jpumulti_encoder_$MODE$TAIL_MARK
            echo -e "\e[32m test_ocv_jpumulti(encode) : Speed Normal \e[0m"
            echo "ocv_jpumulti(encode) test pass (Speed_Diff : $speed_diff%)" >> $nfs_path/daily_test_log.txt
        fi

    elif [ $MODE = soc ]; then
        if [[ ! -f $nfs_path/ocv_jpumulti_decoder_$MODE$TAIL_MARK ]]; then
            echo 1 > $nfs_path/ocv_jpumulti_decoder_$MODE$TAIL_MARK
        else
            chmod 777 $nfs_path/ocv_jpumulti_decoder_$MODE$TAIL_MARK
        fi
        if [[ ! -f $nfs_path/ocv_jpumulti_encoder_$MODE$TAIL_MARK ]]; then
            echo 1 > $nfs_path/ocv_jpumulti_encoder_$MODE$TAIL_MARK
        else
            chmod 777 $nfs_path/ocv_jpumulti_encoder_$MODE$TAIL_MARK
        fi
        old_var_fps_dec=$(cat $nfs_path/ocv_jpumulti_decoder_$MODE$TAIL_MARK)
        old_var_fps_enc=$(cat $nfs_path/ocv_jpumulti_encoder_$MODE$TAIL_MARK)
        echo "old_var_fps_dec: $old_var_fps_dec"
        echo "old_var_fps_enc: $old_var_fps_enc"
        writeJpuMultiTest $nfs_path $MODE $TAIL_MARK $try_n_times
        chmod u+x $nfs_path/tmp_daily.sh
        run_on_target $nfs_path/tmp_daily.sh $INPUT_FIG opencv_input.jpg $MODE || return $?
        rm -f $nfs_path/tmp_daily.sh
        scp_file $nfs_path/ocv_jpumulti_decoder_$MODE$TAIL_MARK $dest/ocv_jpumulti_decoder_$MODE$TAIL_MARK $SOC_PASS_WORD $SOC_IP_ADDR 2 > /dev/null
        var_fps_dec=$(cat $nfs_path/ocv_jpumulti_decoder_$MODE$TAIL_MARK)
        echo "var_fps_dec: $var_fps_dec"
        if [ $old_var_fps_dec -eq 1 ];then
            speed_diff=100
        else
            speed_diff=`echo "$old_var_fps_dec,$var_fps_dec"|awk -F "," '{old_fps_float=$1+0.0;fps_float=$2+0.0;percent=(fps_float-old_fps_float)/old_fps_float}END{printf "%d",percent*100}'`
        fi
        # speed_diff is calculated using time here
        echo "Decode_Speed_Diff : $speed_diff%"
        if [[ $speed_diff -lt -100 ]] || [[ $speed_diff -gt 100 ]]; then
            echo -e "\e[31m test_ocv_jpumulti(decode) : Log informations have not be recorded \e[0m"
            return -1
        elif [[ $speed_diff -lt -95 ]] || [[ $speed_diff -gt 95 ]]; then
            echo "ocv_jpumulti(decode) test pass (This should be your first time to create a log)" >> $nfs_path/daily_test_log.txt
        elif [[ $speed_diff -gt 10 ]] || [[ $speed_diff -gt 10 ]]; then
            echo -e "\e[31m test_ocv_jpumulti(decode) : Speed Abnormal \e[0m"
            return -1
        else
            echo $var_fps_dec > $nfs_path/ocv_jpumulti_decoder_$MODE$TAIL_MARK
            echo -e "\e[32m test_ocv_jpumulti(decode) : Speed Normal \e[0m"
            echo "ocv_jpumulti(decode) test pass (Speed_Diff : $speed_diff%)" >> $nfs_path/daily_test_log.txt
        fi
        scp_file $nfs_path/ocv_jpumulti_encoder_$MODE$TAIL_MARK $dest/ocv_jpumulti_encoder_$MODE$TAIL_MARK $SOC_PASS_WORD $SOC_IP_ADDR 2 > /dev/null
        var_fps_enc=$(cat $nfs_path/ocv_jpumulti_encoder_$MODE$TAIL_MARK)
        echo "var_fps_enc: $var_fps_enc"
        if [ $old_var_fps_enc -eq 1 ];then
            speed_diff=100
        else
            speed_diff=`echo "$old_var_fps_enc,$var_fps_enc"|awk -F "," '{old_fps_float=$1+0.0;fps_float=$2+0.0;percent=(fps_float-old_fps_float)/old_fps_float}END{printf "%d",percent*100}'`
        fi
        # speed_diff is calculated using time here
        echo "Encode_Speed_Diff : $speed_diff%"
        if [[ $speed_diff -lt -100 ]] || [[ $speed_diff -gt 100 ]]; then
            echo -e "\e[31m test_ocv_jpumulti(encode) : Log informations have not be recorded \e[0m"
            return -1
        elif [[ $speed_diff -lt -95 ]] || [[ $speed_diff -gt 95 ]]; then
            echo "ocv_jpumulti(encode) test pass (This should be your first time to create a log)" >> $nfs_path/daily_test_log.txt
        elif [[ $speed_diff -gt 10 ]]; then
            echo -e "\e[31m test_ocv_jpumulti(encode) : Speed Abnormal \e[0m"
            return -1
        else
            echo $var_fps_enc > $nfs_path/ocv_jpumulti_encoder_$MODE$TAIL_MARK
            echo -e "\e[32m test_ocv_jpumulti(encode) : Speed Normal \e[0m"
            echo "ocv_jpumulti(encode) test pass (Speed_Diff : $speed_diff%)" >> $nfs_path/daily_test_log.txt
        fi
    fi
}

function run_ocv_drawing(){
    echo -e "\e[36m Run ocv_drawing \e[0m"
    local INPUT_VIDEO=$1
    local REF_FIG=$2
    local MODE=$3
    local TAIL_MARK=$4
    if [ $MODE = pcie ];then
        if [[ ! -f $nfs_path/ocv_drawing_$MODE$TAIL_MARK ]]; then
            echo 1 > $nfs_path/ocv_drawing_$MODE$TAIL_MARK
        else
            chmod 777 $nfs_path/ocv_drawing_$MODE$TAIL_MARK
        fi
        old_log=$(cat $nfs_path/ocv_drawing_$MODE$TAIL_MARK)
        writeOcvDrawing $nfs_path $MODE $TAIL_MARK $try_n_times
        chmod u+x $nfs_path/tmp_daily.sh
        run_on_target $nfs_path/tmp_daily.sh $INPUT_VIDEO opencv_input.mp4 $MODE || return $?
        scp_file $nfs_path/ocv_drawing_$MODE$TAIL_MARK /data/jenkins/usr/local/vid_test/sc5_test/ocv_drawing_$MODE$TAIL_MARK linaro123456 172.28.3.197 2 > /dev/null
        rm -f out.png
        rm -f $nfs_path/tmp_daily.sh
        cur_log=$(cat $nfs_path/ocv_drawing_$MODE$TAIL_MARK)
        if [ "$old_log" = "1" ];then
            echo "ocv_drawing test pass (This should be your first time to create a log)" >> $nfs_path/daily_test_log.txt
        elif [ "$old_log" = "$cur_log" ];then
            echo "ocv_drawing test pass" >> $nfs_path/daily_test_log.txt
        else
            echo -e "\e[31m test_ocv_drawing : Execution exception \e[0m"
            return -1
        fi

    elif [ $MODE = soc ]; then
        if [[ ! -f $nfs_path/ocv_drawing_$MODE$TAIL_MARK ]]; then
            echo 1 > $nfs_path/ocv_drawing_$MODE$TAIL_MARK
        else
            chmod 777 $nfs_path/ocv_drawing_$MODE$TAIL_MARK
        fi
        old_var_fps=$(cat $nfs_path/ocv_drawing_$MODE$TAIL_MARK)
        echo "old_var_fps: $old_var_fps"
        writeOcvDrawing $nfs_path $MODE $TAIL_MARK $try_n_times
        run_on_target $nfs_path/tmp_daily.sh $INPUT_VIDEO bilibiligd.264 $MODE || return $?
        rm -f out.png
        rm -f $nfs_path/tmp_daily.sh
        scp_file $nfs_path/ocv_drawing_$MODE$TAIL_MARK $dest/ocv_drawing_$MODE$TAIL_MARK $SOC_PASS_WORD $SOC_IP_ADDR 2 >/dev/null
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
            return -1
        elif [[ $speed_diff -lt -95 ]] || [[ $speed_diff -gt 95 ]]; then
            echo "ocv_drawing test pass (This should be your first time to create a log)" >> $nfs_path/daily_test_log.txt
        elif [[ $speed_diff -lt -10 ]] || [[ $speed_diff -gt 10 ]]; then
            echo -e "\e[31m ocv_drawing : Speed Abnormal \e[0m"
            return -1
        else
            # echo $var_fps > $nfs_path/ocv_drawing_$MODE$TAIL_MARK
            echo -e "\e[32m ocv_drawing : Speed Normal \e[0m"
            echo "ocv_drawing test pass (Speed_Diff : $speed_diff%)" >> $nfs_path/daily_test_log.txt
        fi
    fi
}

function run_vpss()
{

cat << EOF > /tmp/tmp_vpss.sh
set -eux
rm -f out*.jpg
rm -f dump*.BGR
rm -f dump*.NV12

bmcv/test_vpss_convert_thread
bmcv/test_vpss_convert_to_thread
bmcv/test_vpss_copy_to_thread
bmcv/test_vpss_draw_rectangle_thread
bmcv/test_vpss_fbd_thread
bmcv/test_vpss_fill_rectangle_thread
bmcv/test_vpss_mosaic_thread
bmcv/test_vpss_padding_thread
bmcv/test_vpss_stitch_thread
bmcv/test_vpss_water_thread

EOF

    chmod u+x /tmp/tmp_vpss.sh

    scp_file /tmp/tmp_vpss.sh $dest/run.sh $SOC_PASS_WORD $SOC_IP_ADDR 1 > /dev/null
    cmd="export LD_LIBRARY_PATH=$dest/lib; \
         cd $dest; \
         chmod +x run.sh; \
         ./run.sh;"
    formatted_cmd=$(echo "$cmd" | tr -s ' ' | tr ';' '\n')
    echo "<SOC CMD> $formatted_cmd"

cat << EOF > /tmp/tmp_soc.py
import paramiko
ssh = paramiko.SSHClient()
ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
ssh.connect('$SOC_IP_ADDR', 22, 'linaro', '$SOC_PASS_WORD')
stdin, stdout, stderr = ssh.exec_command('$cmd', get_pty=True)

for line in stdout.readlines():
    print(line.strip())
for line in stderr.readlines():
    print(line.strip())

exit_status = stdout.channel.recv_exit_status()  # 获取命令的退出状态
stdin.close()
ssh.close()

if exit_status != 0:
    print(f"Error occurred. Exit status: {exit_status}")
    exit(1)
else:
    exit(0)
EOF
    python3 /tmp/tmp_soc.py
    exit_status=$?

    if [ $exit_status -eq 0 ]; then
        echo -e "\e[32m vpss test passed \e[0m\n"
        echo "vpss test pass" >> $nfs_path/daily_test_log.txt
    else
        echo -e "\e[31m vpss test failed \e[0m"
    fi

    rm -f /tmp/tmp_vpss.sh
    return $exit_status
}

function run_daily_regression_mw_bm1686(){
    local RUN_MODE=$1
    chip_name=1686

    logfile="$nfs_path/daily_test_log.txt"
    if [ -f "$logfile" ]; then
        > "$logfile"
    else
        touch "$logfile"
    fi

    run_ff_bmcv_transcode ${MM_TOP_DIR}/stream/bilibiligd.264 ${MM_TOP_DIR}/stream/${chip_name}/ref_transcode.mp4 $RUN_MODE _daily.txt || return $?  # 138
    run_ocv_vidbasic ${MM_TOP_DIR}/stream/opencv_input.mp4 ${MM_TOP_DIR}/stream/${chip_name}/opencv_ref.png $RUN_MODE _daily.txt || return $? # 11
    # run_ocv_video_xcode ${MM_TOP_DIR}/stream/bilibiligd.264 ${MM_TOP_DIR}/stream/${chip_name}/opencv_ref.png $RUN_MODE _daily.txt || return $? # 216
    run_ocv_jpubasic ${MM_TOP_DIR}/stream/opencv_nv12_4k_test.jpg ${MM_TOP_DIR}/stream/${chip_name}/opencv_nv12_4k_test_yuvref.jpg $RUN_MODE _daily.txt|| return $?

    run_ff_video_encode ${MM_TOP_DIR}/stream/1080p_yuv420p.yuv ${MM_TOP_DIR}/stream/${chip_name}/ref_video_encode.mp4 $RUN_MODE _daily.txt || return $?
    run_ff_video_decode ${MM_TOP_DIR}/stream/bilibiligd.264 ${MM_TOP_DIR}/stream/${chip_name}/opencv_ref.png $RUN_MODE _daily.txt || return $?
    # run_ocv_vidmulti ${MM_TOP_DIR}/stream/bilibiligd.264 ${MM_TOP_DIR}/stream/${chip_name}/opencv_ref.png $RUN_MODE _daily.txt || return $?
    # run_ocv_jpumulti ${MM_TOP_DIR}/stream/opencv_nv12_4k_test.jpg ${MM_TOP_DIR}/stream/${chip_name}/opencv_nv12_4k_test_yuvref.jpg $RUN_MODE _daily.txt|| return $?
    run_ocv_drawing ${MM_TOP_DIR}/stream/bilibiligd.264 ${MM_TOP_DIR}/stream/${chip_name}/opencv_ref.png $RUN_MODE _daily.txt|| return $?
    # run_vpss || return $?
    daily_log=$(cat $nfs_path/daily_test_log.txt)
    echo -e "\e[32m---- All Test Complished ---- \e[0m"
    echo -e "\e[32m$daily_log\e[0m"
}

function ci_test(){
    source ${SCRIPT_DIR}/write_remote_operation.sh
    source ${SCRIPT_DIR}/write_videotranscode_test.sh
    source ${SCRIPT_DIR}/write_videoencode_test.sh
    source ${SCRIPT_DIR}/write_videodecode_test.sh
    source ${SCRIPT_DIR}/write_video_xcode_test.sh
    source ${SCRIPT_DIR}/write_vidbasic_test.sh
    source ${SCRIPT_DIR}/write_vidmulti_test.sh
    source ${SCRIPT_DIR}/write_jpubasic_test.sh
    source ${SCRIPT_DIR}/write_jpumulti_test.sh
    source ${SCRIPT_DIR}/write_ocv_drawing_test.sh
    local RUN_MODE=$1
    if [ $RUN_MODE = pcie ];then
        run_daily_regression_mw_bm1686 $RUN_MODE
    elif [ $RUN_MODE = soc ];then
        run_daily_regression_mw_bm1686 $RUN_MODE
    else
        echo -e "\e[31m Error \e[0m: Parameter not support."
        return -1
    fi
}

if [[ $1 == soc_daily_test ]]; then
    soc_file_copy
    ci_test soc
fi




