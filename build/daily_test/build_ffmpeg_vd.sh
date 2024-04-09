counter=0
check_num=0

timeout_duration=20 #超时时间（单位：秒)

host_name=dailytest
host_ip=172.28.141.219
host_passwd=dailytest
src_dir="/home/dailytest/Athena2/Multimedia/vd"
input_dir="input/ffmpeg"
ref_dir="ref/ffmpeg"

dest=/data/ffmpeg
golden_dir=ffmpeg_ref
check_num=0

# action==0, test case
# action==1, make golden file
action=$1

test_ffmpeg_vd_case_cmp=(
    "test_ff_bmcv_transcode soc ${dest}/1920x1080.mp4 case0.ts I420 h264_bm 1920 1080 25 3000 1 0 0 0 0 0"
    "test_ff_scale_transcode ${dest}/1920x1080.mp4 case1.ts I420 h264_bm 256 128 30 3000 1"

    "ffmpeg -cbcr_interleave 0 -i ${dest}/1920x1080.mp4 -y case2.yuv"
    "ffmpeg -cbcr_interleave 1 -i ${dest}/1920x1080.mp4 -y case3.yuv"
    "ffmpeg -cbcr_interleave 2 -i ${dest}/1920x1080.mp4 -y case4.yuv"
    "ffmpeg -re -zero_copy 0 -extra_frame_buffer_num 5 -c:v h264_bm -i ${dest}/1920x1080.mp4 -c:v h264_bm -is_dma_buffer 0 -vframes 100 -y case5.mp4"
    "ffmpeg -c:v h264_bm -output_format 101 -i ${dest}/1920x1080.mp4 -vf "scale_bm=352:288:zero_copy=1:opt=pad" -c:v h264_bm -g 256 -b:v 256K -y case6.264"

    "ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v h264_bm -cbcr_interleave 0 -i ${dest}/1920x1080.mp4 -vf "hwdownload,format=yuv420p\|bmcodec" -y case7.yuv"
    #"ffmpeg -cbcr_interleave 0 -mode_bitstream 0 -i ${dest}/1920x1080.mp4 -y case8.yuv"
)

test_ffmpeg_vd_case_nocmp=(
    "test_bm_restart 1 0 2 no 0 1 ${dest}/1920x1080.mp4 ${dest}/1920x1080.mp4 ${dest}/1920x1080.mp4"
    "test_bm_restart 1 0 2 hevc_bm 0 1 ${dest}/station_4mb_200.265 ${dest}/station_4mb_200.265 ${dest}/station_4mb_200.265"
    "test_bm_restart 1 0 1 no 0 0 rtsp://172.28.9.17:5544/live/123/CBR2925_h265.mp4"
)

function scp_file()
{

file_addr=$1
name=$2
ipaddres=$3
dest_path=$4
# name=$3
passwd=$5

to_remote=$6 #if 1 localhost copy remote machine if 2 remote machine copy to loaclhost


/usr/bin/expect<<-EOF
set cmp 1
if { $to_remote == "1" } {
    spawn sudo scp -r $name@$ipaddres:$file_addr $dest_path

} else {
    spawn sudo scp -r $file_addr $name@$ipaddres:$dest_path
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
          sleep 5
        #   send_user "send yes"
          send "yes\n"
        }
   eof
    {
        sleep 5
        send_user "eof\n"
    }
}

# send "exit\r"
expect eof
EOF
}

function make_golden(){
    num=$1

    ${test_ffmpeg_vd_case_cmp[num]}
    sudo md5sum case${num}* >> ${golden_dir}/case${num}.md5

    find . -name case*
    ret=$?
    if [ ${ret} -ne 0 ]; then
        rm -rf case*
    fi
}

function compare_md5(){
    num=$1

    scp_file ${src_dir}/${ref_dir}/case${num}.md5 ${host_name} ${host_ip} . ${host_passwd} 1
    md5sum -c case${num}.md5 >> ffmpeg_vd.log
    echo

    sudo rm case${num}.md5
}

function test_start(){
    compare=$1
    num=$2
    line=$3

    echo $line
    timeout ${timeout_duration} ${line}
    exit_code=$?
    sleep 1
    if [ ${exit_code} -eq 0 ]; then
        echo case${num}: $line success
        if [ ${compare} -eq 1 ]; then
            compare_md5 ${num}
        fi

    else
        if [ ${exit_code} -eq 124 ]; then
            if [ ${compare} -eq 1 ]; then
                echo "Command timed out.$line"
                echo ${line} "Command timed out." >> ffmpeg_vd.log
                printf "\n" >> ffmpeg_vd.log
            else
                echo case${num}: $line success >> ffmpeg_vd.log
                printf "\n" >> ffmpeg_vd.log
            fi
        else
            echo "Command encountered an error.$line"
            echo ${line} "Command Failed" >> ffmpeg_vd.log
            printf "\n" >> ffmpeg_vd.log
        fi

    fi

    find . -name case*
    ret=$?
    if [ ${ret} -ne 0 ]; then
        rm -rf case*
    fi
}

function test_all_case(){
    echo action=${action}

    if [ ${action} -eq 1 ]; then
        if [ -d ${golden_dir} ]; then
            rm -r ${golden_dir}
            mkdir -p ${golden_dir}
        else
            mkdir -p ${golden_dir}
        fi
    fi

    if [ -e "ffmpeg_vd.log" ]; then
        rm -rf ffmpeg_vd.log
    fi

    sudo  mkdir ${dest}
    scp_file ${src_dir}/${input_dir}/* ${host_name} ${host_ip} ${dest} ${host_passwd} 1

    for ((i=0; i<${#test_ffmpeg_vd_case_cmp[@]}; i++))
    do

        if [ ${action} -eq 0 ]; then
            # run case
            test_start 1 ${i} "${test_ffmpeg_vd_case_cmp[i]}"
        else
            # make golden file
            make_golden ${i}
        fi

    done

    if [ ${action} -eq 0 ]; then

        for ((i=0; i<${#test_ffmpeg_vd_case_nocmp[@]}; i++))
        do
            test_start 0 ${i} "${test_ffmpeg_vd_case_nocmp[i]}"
        done
    fi

    sudo rm -r ${dest}
}

test_all_case
