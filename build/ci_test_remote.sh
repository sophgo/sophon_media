#!/bin/bash

# README
# Function: This script defines the ci test functions for 8 modules.
# USAGE（Interface）: bash ci_test.sh soc


if [ $# != 1 ] || [ "$1" != "soc" ]; then
    echo -e "\e[31m Error: Parameter not support \e[0m"
    echo -e "\e[36m USAGE: $0 soc \e[0m"
    echo -e "\e[36m e.g.: $0 soc \e[0m"
    return 1
fi

# The two paths SCRIPT_DIR and MM_TOP_DIR need to be set to the actual path of sophon_media when the script is executed.
SCRIPT_DIR=/home/fj/sophon_media/build
MM_TOP_DIR=/home/fj/sophon_media

# Destination path on remote A2 evb board
dest=/home/linaro/test

# Only one valid parameter "soc" is supported.
SOC_IP_ADDR=172.28.3.232
SOC_PASS_WORD=linaro123456
mkdir -p /tmp/test_soc/
chmod u+x /tmp/test_soc/
nfs_path=/tmp/test_soc/

function scp_file() {
    filename=$1
    dest_path=$2
    passwd=$3
    ipaddres=$4
    to_remote=$5 # 1: localhost to remote, 2: remote to localhost

    /usr/bin/expect <<-EOF
        set cmp 1
        if { $to_remote == "1" } {
            spawn scp -r $filename linaro@$ipaddres:$dest_path
        } else {
            spawn scp -r linaro@$ipaddres:$dest_path $filename
        }

        expect {
            "密码：" {
                send "$passwd\n"
            }
            "pass" {
                send "$passwd\n"
            }
            "yes/no" {
                sleep 3
                send "yes\n"
                exp_continue
            }
            eof {
                sleep 3
                send_user "eof\n"
            }
        }

        set timeout 15
        send "exit\r"
        expect eof
	EOF
}

function soc_file_copy() {
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

function run_on_target() {
    local RUN_SCRIPT=$1
    local INPUT=$2
    local INPUT_COPY=$3
    local MODE=$4
    if [ $MODE = pcie ]; then
        :
    elif [ $MODE = soc ];then
        scp_file $RUN_SCRIPT $dest/run.sh $SOC_PASS_WORD $SOC_IP_ADDR 1 > /dev/null
        scp_file $INPUT $dest/$INPUT_COPY $SOC_PASS_WORD $SOC_IP_ADDR 1 > /dev/null
        cmd="export LD_LIBRARY_PATH=$dest/lib; \
            cd $dest; \
            chmod +x run.sh; \
            ./run.sh;"
        formatted_cmd=$(echo "$cmd" | tr -s ' ' | tr ';' '\n')
        echo -e "<SOC CMD>\n$formatted_cmd\n"

        cat <<-EOF > /tmp/tmp_soc.py
		import paramiko
		ssh = paramiko.SSHClient()
		ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
		ssh.connect('$SOC_IP_ADDR', 22, 'linaro', 'linaro123456')
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

    fi
        (
            python3 /tmp/tmp_soc.py
        ) || return $?
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
    echo -e "\e[36m Run ff_bmcv_transcode \e[0m"
    local INPUT_VIDEO=$1
    local REF_FIG=$2
    local MODE=$3
    local TAIL_MARK=$4
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
        run_on_target $nfs_path/tmp_daily.sh $INPUT_VIDEO bilibiligd.264 $MODE || return $?
        rm -f $nfs_path/tmp_daily.sh

        # scp_file $nfs_path/ff_bmcv_transcode0.mp4 $dest/ff_bmcv_transcode0.mp4 $SOC_PASS_WORD $SOC_IP_ADDR 2 >/dev/null
        # cmp $nfs_path/ff_bmcv_transcode0.mp4 $REF_FIG || return $?

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
            return 1
        elif [[ $speed_diff -lt -95 ]] || [[ $speed_diff -gt 95 ]]; then
            echo "ff_bmcv_transcode test pass (This should be your first time to create a log)" >> $logfile
        elif [[ $speed_diff -lt -10 ]] || [[ $speed_diff -gt 10 ]]; then
            echo -e "\e[31m test_ff_bmcv_transcode : Speed Abnormal \e[0m"
            return 1
        else
            echo -e "\e[32m test_ff_bmcv_transcode : Speed Normal \e[0m"
            echo "ff_bmcv_transcode test pass (Speed_Diff : $speed_diff%)" >> $logfile
        fi

    fi
}

function run_ff_video_decode() {
    echo -e "\e[36m Run ff_video_decode \e[0m"
    local INPUT_VIDEO=$1
    local REF_FIG=$2
    local MODE=$3
    local TAIL_MARK=$4
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
        run_on_target $nfs_path/tmp_daily.sh $INPUT_VIDEO bilibiligd.264 $MODE || return $?
        rm -f out.png
        rm -f $nfs_path/tmp_daily.sh
        scp_file $nfs_path/ff_video_decode_$MODE$TAIL_MARK $dest/ff_video_decode_$MODE$TAIL_MARK $SOC_PASS_WORD $SOC_IP_ADDR 2 > /dev/null
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
            echo -e "\e[31m test_ff_video_decode : Speed Abnormal \e[0m"
            return 1
        else
            echo -e "\e[32m test_ff_video_decode : Speed Normal \e[0m"
            echo "ff_video_decode test pass (Speed_Diff : $speed_diff%)" >> $logfile
        fi
    fi
}

function run_videotranscode() {
    echo -e "\e[36m Run videotranscode \e[0m"
    local INPUT_VIDEO=$1
    local REF_FIG=$2
    local MODE=$3
    local TAIL_MARK=$4
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
        run_on_target $nfs_path/tmp_daily.sh $INPUT_VIDEO bilibiligd.264 $MODE || return $?
        rm -f out.png
        rm -f $nfs_path/tmp_daily.sh

        scp_file $nfs_path/md5_videotranscode.txt $dest/md5_videotranscode.txt $SOC_PASS_WORD $SOC_IP_ADDR 2 >/dev/null
        cmp $nfs_path/md5_videotranscode.txt $2 || return $?
        echo -e "\e[32mvideotranscode : Comparing output and reference successfully\e[0m"

        scp_file $nfs_path/videotranscode_$MODE$TAIL_MARK $dest/videotranscode_$MODE$TAIL_MARK $SOC_PASS_WORD $SOC_IP_ADDR 2 >/dev/null
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
    echo -e "\e[36m Run ocv_vidbasic \e[0m"
    local INPUT_VIDEO=$1
    local REF_FIG=$2
    local MODE=$3
    local TAIL_MARK=$4
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
        run_on_target $nfs_path/tmp_daily.sh $INPUT_VIDEO opencv_input.mp4 $MODE || return $?
        rm -f $nfs_path/tmp_daily.sh

        scp_file $nfs_path/md5_ocv_vidbasic.txt $dest/md5_ocv_vidbasic.txt $SOC_PASS_WORD $SOC_IP_ADDR 2 >/dev/null
        cmp $nfs_path/md5_ocv_vidbasic.txt $2 || return $?
        echo -e "\e[32mvidbasic : Comparing output and reference successfully\e[0m"
        echo "ocv_vidbasic test pass" >> $logfile

        # scp_file $nfs_path/ocv_vidbasic_$MODE$TAIL_MARK $dest/ocv_vidbasic_$MODE$TAIL_MARK $SOC_PASS_WORD $SOC_IP_ADDR 2 >/dev/null
        # var_fps=$(cat $nfs_path/ocv_vidbasic_$MODE$TAIL_MARK)
        # echo "var_fps: $var_fps"
        # if [ $old_var_fps -eq 1 ];then
        #     speed_diff=100
        # else
        #     speed_diff=`echo "$old_var_fps,$var_fps" | awk -F "," '{old_fps_float=$1+0.0;fps_float=$2+0.0;percent=(fps_float-old_fps_float)/old_fps_float}END{printf "%d",percent*100}'`
        # fi
        # # speed_diff is calculated using fps here
        # echo "Speed_Diff : $speed_diff%"
        # if [[ $speed_diff -lt -100 ]] || [[ $speed_diff -gt 100 ]]; then
        #     echo -e "\e[31m ocv_vidbasic : Log informations have not be recorded \e[0m"
        #     return 1
        # elif [[ $speed_diff -lt -95 ]] || [[ $speed_diff -gt 95 ]]; then
        #     echo "ocv_vidbasic test pass (This should be your first time to create a log)" >> $logfile
        # elif [[ $speed_diff -lt -10 ]] || [[ $speed_diff -gt 10 ]]; then
        #     echo -e "\e[31m ocv_vidbasic : Speed Abnormal \e[0m"
        #     return 1
        # else
        #     # echo $var_fps > $nfs_path/ocv_vidbasic_$MODE$TAIL_MARK
        #     echo -e "\e[32m ocv_vidbasic : Speed Normal \e[0m"
        #     echo "ocv_vidbasic test pass (Speed_Diff : $speed_diff%)" >> $logfile
        # fi
    fi
}

function run_ocv_jpubasic() {
    echo -e "\e[36m Run ocv_jpubasic \e[0m"
    local INPUT_FIG=$1
    local REF_FIG=$2
    local MODE=$3
    local TAIL_MARK=$4
    local try_n_times=100
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
        run_on_target $nfs_path/tmp_daily.sh $INPUT_FIG opencv_input.jpg $MODE || return $?
        rm -f out.png
        rm -f $nfs_path/tmp_daily.sh

        scp_file $nfs_path/out1.jpg $dest/out1.jpg $SOC_PASS_WORD $SOC_IP_ADDR 2 > /dev/null
        scp_file $nfs_path/out2.jpg $dest/out2.jpg $SOC_PASS_WORD $SOC_IP_ADDR 2 > /dev/null
        scp_file $nfs_path/out3.jpg $dest/out3.jpg $SOC_PASS_WORD $SOC_IP_ADDR 2 > /dev/null

        cmp $nfs_path/out1.jpg $2 || return $?
        cmp $nfs_path/out1.jpg $nfs_path/out2.jpg || return $?
        cmp $nfs_path/out1.jpg $nfs_path/out3.jpg || return $?
        echo -e "\e[32mjpubasic : Comparing output and reference successfully\e[0m"

        scp_file $nfs_path/ocv_jpubasic_decoder_$MODE$TAIL_MARK $dest/ocv_jpubasic_decoder_$MODE$TAIL_MARK $SOC_PASS_WORD $SOC_IP_ADDR 2 > /dev/null
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
            echo -e "\e[31m test_ocv_jpubasic(decode) : Speed Abnormal \e[0m"
            return 1
        else
            echo $var_time_dec > $nfs_path/ocv_jpubasic_decoder_$MODE$TAIL_MARK
            echo -e "\e[32m test_ocv_jpubasic(decode) : Speed Normal \e[0m"
            echo "ocv_jpubasic(decode) test pass (Speed_Diff : $speed_diff%)" >> $logfile
        fi
        scp_file $nfs_path/ocv_jpubasic_encoder_$MODE$TAIL_MARK $dest/ocv_jpubasic_encoder_$MODE$TAIL_MARK $SOC_PASS_WORD $SOC_IP_ADDR 2 > /dev/null
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
    echo -e "\e[36m Run ocv_drawing \e[0m"
    local INPUT_VIDEO=$1
    local REF_FIG=$2
    local MODE=$3
    local TAIL_MARK=$4
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
        run_on_target $nfs_path/tmp_daily.sh $INPUT_VIDEO bilibiligd.264 $MODE || return $?
        rm -f out.png
        rm -f $nfs_path/tmp_daily.sh

        scp_file $nfs_path/dump_0_out.jpg $dest/dump_0_out.jpg $SOC_PASS_WORD $SOC_IP_ADDR 2 >/dev/null
        cmp $nfs_path/dump_0_out.jpg $2 || return $?
        echo -e "\e[32mocv_drawing : Comparing output and reference successfully\e[0m"

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
            return 1
        elif [[ $speed_diff -lt -95 ]] || [[ $speed_diff -gt 95 ]]; then
            echo "ocv_drawing test pass (This should be your first time to create a log)" >> $logfile
        elif [[ $speed_diff -lt -10 ]] || [[ $speed_diff -gt 10 ]]; then
            echo -e "\e[31m ocv_drawing : Speed Abnormal \e[0m"
            return 1
        else
            echo -e "\e[32m ocv_drawing : Speed Normal \e[0m"
            echo "ocv_drawing test pass (Speed_Diff : $speed_diff%)" >> $logfile
        fi
    fi
}

function run_vpss_convert() {
    echo -e "\e[36m Run vpss_convert \e[0m"
    local INPUT_PICTURE=$1
    local REF_FIG=$2
    local MODE=$3
    local TAIL_MARK=$4
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
        run_on_target $nfs_path/tmp_daily.sh $INPUT_PICTURE 1920x1080_rgb.bin $MODE || return $?
        rm -f $nfs_path/tmp_daily.sh

        scp_file $nfs_path/md5_vpss_convert.txt $dest/md5_vpss_convert.txt $SOC_PASS_WORD $SOC_IP_ADDR 2 >/dev/null
        cmp $nfs_path/md5_vpss_convert.txt $2 || return $?
        echo -e "\e[32mvpss_convert : Comparing output and reference successfully\e[0m"

        scp_file $nfs_path/vpss_convert_$MODE$TAIL_MARK $dest/vpss_convert_$MODE$TAIL_MARK $SOC_PASS_WORD $SOC_IP_ADDR 2 >/dev/null
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
            echo -e "\e[31m vpss_convert : Speed Abnormal \e[0m"
            return 1
        else
            echo -e "\e[32m vpss_convert : Speed Normal \e[0m"
            echo "vpss_convert test pass (Speed_Diff : $speed_diff%)" >> $logfile
        fi
    fi
}

function run_tpu_gaussian_blur() {
    echo -e "\e[36m Run tpu_gaussian_blur \e[0m"
    local INPUT_PICTURE=$1
    local REF_FIG=$2
    local MODE=$3
    local TAIL_MARK=$4
    local try_n_times=100
    if [ $MODE = pcie ];then
        :
    elif [ $MODE = soc ]; then
        if [[ ! -f $nfs_path/tpu_gaussian_blur_$MODE$TAIL_MARK ]]; then
            echo 1 > $nfs_path/tpu_gaussian_blur_$MODE$TAIL_MARK
        else
            chmod 777 $nfs_path/tpu_gaussian_blur_$MODE$TAIL_MARK
        fi
        old_var_fps=$(cat $nfs_path/tpu_gaussian_blur_$MODE$TAIL_MARK)
        echo "old_var_fps: $old_var_fps"
        writeGaussianBlur $nfs_path $MODE $TAIL_MARK $try_n_times
        run_on_target $nfs_path/tmp_daily.sh $INPUT_PICTURE 1920x1080_rgb.bin $MODE || return $?
        rm -f $nfs_path/tmp_daily.sh

        scp_file $nfs_path/md5_tpu_gaussian_blur.txt $dest/md5_tpu_gaussian_blur.txt $SOC_PASS_WORD $SOC_IP_ADDR 2 >/dev/null
        cmp $nfs_path/md5_tpu_gaussian_blur.txt $2 || return $?
        echo -e "\e[32mtpu_gaussian_blur : Comparing output and reference successfully\e[0m"

        scp_file $nfs_path/tpu_gaussian_blur_$MODE$TAIL_MARK $dest/tpu_gaussian_blur_$MODE$TAIL_MARK $SOC_PASS_WORD $SOC_IP_ADDR 2 >/dev/null
        var_fps=$(cat $nfs_path/tpu_gaussian_blur_$MODE$TAIL_MARK)
        echo "var_fps: $var_fps"
        if [ $old_var_fps -eq 1 ];then
            speed_diff=100
        else
            speed_diff=`echo "$old_var_fps,$var_fps" | awk -F "," '{old_fps_float=$1+0.0;fps_float=$2+0.0;percent=(fps_float-old_fps_float)/old_fps_float}END{printf "%d",percent*100}'`
        fi
        # speed_diff is calculated using fps here
        echo "Speed_Diff : $speed_diff%"
        if [[ $speed_diff -lt -100 ]] || [[ $speed_diff -gt 100 ]]; then
            echo -e "\e[31m tpu_gaussian_blur : Log informations have not be recorded \e[0m"
            return 1
        elif [[ $speed_diff -lt -95 ]] || [[ $speed_diff -gt 95 ]]; then
            echo "tpu_gaussian_blur test pass (This should be your first time to create a log)" >> $logfile
        elif [[ $speed_diff -lt -10 ]] || [[ $speed_diff -gt 10 ]]; then
            echo -e "\e[31m tpu_gaussian_blur : Speed Abnormal \e[0m"
            return 1
        else
            echo -e "\e[32m tpu_gaussian_blur : Speed Normal \e[0m"
            echo "tpu_gaussian_blur test pass (Speed_Diff : $speed_diff%)" >> $logfile
        fi
    fi
}

function run_ive_stcandicorner() {
    echo -e "\e[36m Run ive_stcandicorner \e[0m"
    local INPUT_PICTURE=$1
    local REF_PICTURE=$2
    local MODE=$3
    local TAIL_MARK=$4
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
        scp_file $REF_PICTURE $dest/sample_tile_Shitomasi_sky_640x480.yuv $SOC_PASS_WORD $SOC_IP_ADDR 1 > /dev/null
        run_on_target $nfs_path/tmp_daily.sh $INPUT_PICTURE sky_640x480.yuv $MODE || return $?
        rm -f $nfs_path/tmp_daily.sh

        scp_file $nfs_path/ive_stcandicorner_$MODE$TAIL_MARK $dest/ive_stcandicorner_$MODE$TAIL_MARK $SOC_PASS_WORD $SOC_IP_ADDR 2 >/dev/null
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
            echo -e "\e[31m ive_stcandicorner : Speed Abnormal \e[0m"
            return 1
        else
            echo -e "\e[32m ive_stcandicorner : Speed Normal \e[0m"
            echo "ive_stcandicorner test pass (Speed_Diff : $speed_diff%)" >> $logfile
        fi
    fi
}

function run_dpu_online() {
    echo -e "\e[36m Run dpu_online \e[0m"
    local INPUT_PICTURE=$1
    local REF_PICTURE=$2
    local MODE=$3
    local TAIL_MARK=$4
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
        scp_file $REF_PICTURE $dest/sofa_right_img_512x284.bin $SOC_PASS_WORD $SOC_IP_ADDR 1 > /dev/null
        scp_file ${MM_TOP_DIR}/stream/fgs_512x284_res.bin $dest/fgs_512x284_res.bin $SOC_PASS_WORD $SOC_IP_ADDR 1 > /dev/null
        run_on_target $nfs_path/tmp_daily.sh $INPUT_PICTURE sofa_left_img_512x284.bin $MODE || return $?
        rm -f $nfs_path/tmp_daily.sh

        scp_file $nfs_path/dpu_online_$MODE$TAIL_MARK $dest/dpu_online_$MODE$TAIL_MARK $SOC_PASS_WORD $SOC_IP_ADDR 2 >/dev/null
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
            echo -e "\e[31m dpu_online : Speed Abnormal \e[0m"
            return 1
        else
            echo -e "\e[32m dpu_online : Speed Normal \e[0m"
            echo "dpu_online test pass (Speed_Diff : $speed_diff%)" >> $logfile
        fi
    fi
}

function run_ldc_rot() {
    echo -e "\e[36m Run ldc_rot \e[0m"
    local INPUT_PICTURE=$1
    local REF_FIG=$2
    local MODE=$3
    local TAIL_MARK=$4
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
        run_on_target $nfs_path/tmp_daily.sh $INPUT_PICTURE 1920x1088_nv21.bin $MODE || return $?
        rm -f $nfs_path/tmp_daily.sh

        scp_file $nfs_path/md5_ldc_rot.txt $dest/md5_ldc_rot.txt $SOC_PASS_WORD $SOC_IP_ADDR 2 >/dev/null
        cmp $nfs_path/md5_ldc_rot.txt $2 || return $?
        echo -e "\e[32mldc_rot : Comparing output and reference successfully\e[0m"

        scp_file $nfs_path/ldc_rot_$MODE$TAIL_MARK $dest/ldc_rot_$MODE$TAIL_MARK $SOC_PASS_WORD $SOC_IP_ADDR 2 >/dev/null
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
            echo -e "\e[31m ldc_rot : Speed Abnormal \e[0m"
            return 1
        else
            echo -e "\e[32m ldc_rot : Speed Normal \e[0m"
            echo "ldc_rot test pass (Speed_Diff : $speed_diff%)" >> $logfile
        fi
    fi
}

function run_dwa_fisheye() {
    echo -e "\e[36m Run dwa_fisheye \e[0m"
    local INPUT_PICTURE=$1
    local REF_FIG=$2
    local MODE=$3
    local TAIL_MARK=$4
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
        run_on_target $nfs_path/tmp_daily.sh $INPUT_PICTURE fisheye_floor_1024x1024.yuv $MODE || return $?
        rm -f $nfs_path/tmp_daily.sh

        scp_file $nfs_path/md5_dwa_fisheye.txt $dest/md5_dwa_fisheye.txt $SOC_PASS_WORD $SOC_IP_ADDR 2 >/dev/null
        cmp $nfs_path/md5_dwa_fisheye.txt $2 || return $?
        echo -e "\e[32mdwa_fisheye : Comparing output and reference successfully\e[0m"

        scp_file $nfs_path/dwa_fisheye_$MODE$TAIL_MARK $dest/dwa_fisheye_$MODE$TAIL_MARK $SOC_PASS_WORD $SOC_IP_ADDR 2 >/dev/null
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
            echo -e "\e[31m dwa_fisheye : Speed Abnormal \e[0m"
            return 1
        else
            echo -e "\e[32m dwa_fisheye : Speed Normal \e[0m"
            echo "dwa_fisheye test pass (Speed_Diff : $speed_diff%)" >> $logfile
        fi
    fi
}

function run_blend_4way() {
    echo -e "\e[36m Run blend_4way \e[0m"
    local INPUT_PICTURE=$1
    local REF_FIG=$2
    local MODE=$3
    local TAIL_MARK=$4
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

        scp_file ${MM_TOP_DIR}/stream/c01_result_420p_c2_3840x1080_none_ovlp.yuv $dest/c01_result_420p_c2_3840x1080_none_ovlp.yuv $SOC_PASS_WORD $SOC_IP_ADDR 1 > /dev/null
        run_on_target $nfs_path/tmp_daily.sh $INPUT_PICTURE 1920x1080.yuv $MODE || return $?
        rm -f $nfs_path/tmp_daily.sh

        scp_file $nfs_path/md5_blend_4way.txt $dest/md5_blend_4way.txt $SOC_PASS_WORD $SOC_IP_ADDR 2 >/dev/null
        cmp $nfs_path/md5_blend_4way.txt $2 || return $?
        echo -e "\e[32mblend_4way : Comparing output and reference successfully\e[0m"

        scp_file $nfs_path/blend_4way_$MODE$TAIL_MARK $dest/blend_4way_$MODE$TAIL_MARK $SOC_PASS_WORD $SOC_IP_ADDR 2 >/dev/null
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

    run_ff_bmcv_transcode ${MM_TOP_DIR}/stream/bilibiligd.264 ${MM_TOP_DIR}/stream/${chip_name}/ref_bmcv_transcode.mp4 $RUN_MODE _daily.txt || return $?  # 138
    # run_videotranscode ${MM_TOP_DIR}/stream/bilibiligd.264 ${MM_TOP_DIR}/stream/${chip_name}/ref_md5_videotranscode.txt $RUN_MODE _daily.txt || return $?
    run_ocv_vidbasic ${MM_TOP_DIR}/stream/opencv_input.mp4 ${MM_TOP_DIR}/stream/${chip_name}/ref_md5_ocv_vidbasic.txt $RUN_MODE _daily.txt || return $? # 11
    # run_ocv_jpubasic ${MM_TOP_DIR}/stream/opencv_nv12_4k_test.jpg ${MM_TOP_DIR}/stream/${chip_name}/opencv_nv12_4k_test_yuvref.jpg $RUN_MODE _daily.txt || return $?
    run_ff_video_decode ${MM_TOP_DIR}/stream/bilibiligd.264 ${MM_TOP_DIR}/stream/${chip_name}/opencv_ref.png $RUN_MODE _daily.txt || return $?
    # run_ocv_drawing ${MM_TOP_DIR}/stream/bilibiligd.264 ${MM_TOP_DIR}/stream/${chip_name}/ref_ocv_drawing.jpg $RUN_MODE _daily.txt|| return $?
    # run_vpss_convert ${MM_TOP_DIR}/stream/1920x1080_rgb.bin ${MM_TOP_DIR}/stream/${chip_name}/ref_md5_vpss_convert.txt $RUN_MODE _daily.txt|| return $?
    run_tpu_gaussian_blur ${MM_TOP_DIR}/stream/1920x1080_rgb.bin ${MM_TOP_DIR}/stream/${chip_name}/ref_md5_tpu_gaussian_blur.txt $RUN_MODE _daily.txt || return $?
    run_ive_stcandicorner ${MM_TOP_DIR}/stream/sky_640x480.yuv ${MM_TOP_DIR}/stream/${chip_name}/sample_tile_Shitomasi_sky_640x480.yuv $RUN_MODE _daily.txt || return $?
    run_dpu_online ${MM_TOP_DIR}/stream/sofa_left_img_512x284.bin ${MM_TOP_DIR}/stream/sofa_right_img_512x284.bin $RUN_MODE _daily.txt || return $?
    run_ldc_rot ${MM_TOP_DIR}/stream/1920x1088_nv21.bin  ${MM_TOP_DIR}/stream/${chip_name}/ref_md5_ldc_rot.txt $RUN_MODE _daily.txt || return $?
    run_dwa_fisheye ${MM_TOP_DIR}/stream/fisheye_floor_1024x1024.yuv ${MM_TOP_DIR}/stream/${chip_name}/ref_md5_dwa_fisheye.txt $RUN_MODE _daily.txt || return $?
    # run_blend_4way ${MM_TOP_DIR}/stream/1920x1080.yuv ${MM_TOP_DIR}/stream/${chip_name}/ref_md5_blend_4way.txt $RUN_MODE _daily.txt || return $?
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
    source ${SCRIPT_DIR}/write_tpu_gaussian_blur.sh
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

if [[ $1 == soc ]]; then
    soc_file_copy
    ci_test soc
fi




