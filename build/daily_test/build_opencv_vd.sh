counter=0
check_num=0

timeout_duration=30 #超时时间（单位：秒)

host_name=dailytest
host_ip=172.28.141.219
host_passwd=dailytest
src_dir="/home/dailytest/Athena2/Multimedia/vd"
input_dir="input/opencv"
ref_dir="ref/opencv"

dest=/data/opencv
golden_dir=opencv_ref
check_num=0

# action==0, test case
# action==1, make golden file
action=$1

test_opencv_vd_case_cmp=(
    "test_ocv_vidbasic ${dest}/3840x2160.mp4 case0 5 1"
    "test_ocv_vidbasic ${dest}/3840x2160.mp4 case1 5 0"

    "vidbasic ${dest}/1280x720.mp4 case2 10 1"
    "vidbasic ${dest}/1280x720.mp4 case3 10 0"
    "vidbasic ${dest}/1920x1080.mp4 case4 10 1"
    "vidbasic ${dest}/1920x1080.mp4 case5 10 0"
    "vidbasic ${dest}/1440x1080.mp4 case6 10 0"
    "vidbasic ${dest}/1440x1080.mp4 case7 10 1"
    "vidbasic ${dest}/352x288.mp4 case8 10 0"
    "vidbasic ${dest}/352x288.mp4 case9 10 1"
    "vidbasic ${dest}/3840x2160.mp4 case10 5 0"
    "vidbasic ${dest}/3840x2160.mp4 case11 5 1"
    "vidbasic ${dest}/480x360.mp4 case12 10 1"
    "vidbasic ${dest}/480x360.mp4 case13 10 1"
    "vidbasic ${dest}/640x360.mp4 case14 10 0"
    "vidbasic ${dest}/640x360.mp4 case15 10 1"
    "vidbasic ${dest}/768x432.mp4 case16 10 0"
    "vidbasic ${dest}/768x432.mp4 case17 10 1"
    "vidbasic ${dest}/960x720.mp4 case18 10 0"
    "vidbasic ${dest}/960x720.mp4 case19 10 1"

    "test_ocv_video_xcode ${dest}/1920x1080.mp4 H265enc 100 case20.ts 1 0 0 bitrate=3000"
    "test_ocv_video_xcode ${dest}/1920x1080.mp4 H265enc 100 case21.ts 0 0 0 bitrate=3000"
    "test_ocv_video_xcode ${dest}/1920x1080.mp4 H264enc 100 case22.ts 1 0 0 bitrate=3000"
    "test_ocv_video_xcode ${dest}/1920x1080.mp4 H264enc 100 case23.ts 0 0 0 bitrate=3000"

    "test_ocv_video_xcode ${dest}/352x288.mp4 H265enc 100 case24.ts 1 0 0 bitrate=3000"
    "test_ocv_video_xcode ${dest}/352x288.mp4 H265enc 100 case25.ts 0 0 0 bitrate=3000"
    "test_ocv_video_xcode ${dest}/352x288.mp4 H264enc 100 case26.ts 1 0 0 bitrate=3000"
    "test_ocv_video_xcode ${dest}/352x288.mp4 H264enc 100 case27.ts 0 0 0 bitrate=3000"

    "test_ocv_video_xcode ${dest}/station.avi H265enc 1000 case28.ts 1 0 0 bitrate=3000"
    "test_ocv_video_xcode ${dest}/station.avi H264enc 1000 case29.ts 1 0 0 bitrate=3000"
)

test_opencv_vd_case_nocmp=(
    "test_ocv_vidmulti 1 ${dest}/1920x1080.mp4 0 0"
    "test_ocv_vidmulti 16 ${dest}/1920x1080.mp4 0 0 ${dest}/1920x1080.mp4 0 0 ${dest}/1920x1080.mp4 0 0 ${dest}/1920x1080.mp4 0 0 ${dest}/1920x1080.mp4 0 0 ${dest}/1920x1080.mp4 0 0 ${dest}/1920x1080.mp4 0 0 ${dest}/1920x1080.mp4 0 0 ${dest}/1920x1080.mp4 0 0 ${dest}/1920x1080.mp4 0 0 ${dest}/1920x1080.mp4 0 0 ${dest}/1920x1080.mp4 0 0 ${dest}/1920x1080.mp4 0 0 ${dest}/1920x1080.mp4 0 0 ${dest}/1920x1080.mp4 0 0 ${dest}/1920x1080.mp4 0 0"
    "vidmulti 1 ${dest}/352x288.mp4 0"
    "vidmulti 25  ${dest}/352x288.mp4 0 ${dest}/352x288.mp4 0 ${dest}/352x288.mp4 0 ${dest}/352x288.mp4 0 ${dest}/352x288.mp4 0 ${dest}/352x288.mp4 0 ${dest}/352x288.mp4 0 ${dest}/352x288.mp4 0 ${dest}/352x288.mp4 0 ${dest}/352x288.mp4 0 ${dest}/352x288.mp4 0 ${dest}/352x288.mp4 0 ${dest}/352x288.mp4 0 ${dest}/352x288.mp4 0 ${dest}/352x288.mp4 0 ${dest}/352x288.mp4 0 ${dest}/352x288.mp4 0 ${dest}/352x288.mp4 0 ${dest}/352x288.mp4 0 ${dest}/352x288.mp4 0 ${dest}/352x288.mp4 0 ${dest}/352x288.mp4 0 ${dest}/352x288.mp4 0 ${dest}/352x288.mp4 0 ${dest}/352x288.mp4 0"
    "vidmulti 1 ${dest}/1280x720.mp4 0 "
    "vidmulti 16 ${dest}/1280x720.mp4 0 ${dest}/1280x720.mp4 0 ${dest}/1280x720.mp4 0 ${dest}/1280x720.mp4 0 ${dest}/1280x720.mp4 0 ${dest}/1280x720.mp4 0 ${dest}/1280x720.mp4 0 ${dest}/1280x720.mp4 0 ${dest}/1280x720.mp4 0 ${dest}/1280x720.mp4 0 ${dest}/1280x720.mp4 0 ${dest}/1280x720.mp4 0 ${dest}/1280x720.mp4 0 ${dest}/1280x720.mp4 0 ${dest}/1280x720.mp4 0 ${dest}/1280x720.mp4 0"
    "vidmulti 1 ${dest}/1920x1080.mp4 0 "
    "vidmulti 16 ${dest}/1920x1080.mp4 0 ${dest}/1920x1080.mp4 0 ${dest}/1920x1080.mp4 0 ${dest}/1920x1080.mp4 0 ${dest}/1920x1080.mp4 0 ${dest}/1920x1080.mp4 0 ${dest}/1920x1080.mp4 0 ${dest}/1920x1080.mp4 0 ${dest}/1920x1080.mp4 0 ${dest}/1920x1080.mp4 0 ${dest}/1920x1080.mp4 0 ${dest}/1920x1080.mp4 0 ${dest}/1920x1080.mp4 0 ${dest}/1920x1080.mp4 0 ${dest}/1920x1080.mp4 0 ${dest}/1920x1080.mp4 0"
    "vidmulti 1 ${dest}/2560x2048.mp4 0"
    "vidmulti 4 ${dest}/2560x2048.mp4 0 ${dest}/2560x2048.mp4 0 ${dest}/2560x2048.mp4 0 ${dest}/2560x2048.mp4 0"
    "vidmulti 1 ${dest}/4096x2160.mp4 0"
    "vidmulti 4 ${dest}/4096x2160.mp4 0 ${dest}/4096x2160.mp4 0 ${dest}/4096x2160.mp4 0 ${dest}/4096x2160.mp4 0"
    "vidmulti 1 ${dest}/3840x2160.mp4 0"
    "vidmulti 4 ${dest}/3840x2160.mp4 0 ${dest}/3840x2160.mp4 0 ${dest}/3840x2160.mp4 0 ${dest}/3840x2160.mp4 0"
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

    ${test_opencv_vd_case_cmp[num]}
    md5sum case${num}* >> ${golden_dir}/case${num}.md5

    find . -name case*
    ret=$?
    if [ ${ret} -ne 0 ]; then
        rm -rf case*
    fi
}

function compare_md5(){
    num=$1

    scp_file ${src_dir}/${ref_dir}/case${num}.md5 ${host_name} ${host_ip} . ${host_passwd} 1
    md5sum -c case${num}.md5 >> opencv_vd.log
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
                echo ${line} "Command timed out." >> opencv_vd.log
                printf "\n" >> opencv_vd.log
            else
                echo case${num}: $line success >> opencv_vd.log
                printf "\n" >> opencv_vd.log
            fi
        else
            echo "Command encountered an error.$line"
            echo ${line} "Command Failed" >> opencv_vd.log
            printf "\n" >> opencv_vd.log
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

    if [ -e "opencv_vd.log" ]; then
        rm -rf opencv_vd.log
    fi

    sudo  mkdir ${dest}
    scp_file ${src_dir}/${input_dir}/* ${host_name} ${host_ip} ${dest} ${host_passwd} 1

    for ((i=0; i<${#test_opencv_vd_case_cmp[@]}; i++))
    do

        if [ ${action} -eq 0 ]; then
            # run case
            test_start 1 ${i} "${test_opencv_vd_case_cmp[i]}"
        else
            # make golden file
            make_golden ${i}
        fi

    done

    state=0
    if [ ${action} -eq 0 ]; then

        for ((i=0; i<${#test_opencv_vd_case_nocmp[@]}; i++))
        do
            test_start 0 ${i} "${test_opencv_vd_case_nocmp[i]}"
        done
    fi

    sudo rm -r ${dest}
}

test_all_case
