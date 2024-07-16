counter=0
check_num=0

timeout_duration=300 #超时时间（单位：秒)

host_name=dailytest
host_ip=172.28.141.219
host_passwd=dailytest
src_dir="/home/dailytest/Athena2/Multimedia/ve"
input_dir="input"
input_file=(
"1080p_nv12.yuv"
"1080p_yuv420p.yuv"
"1080p_yuv422p.yuv"
"1920x1088_420.yuv"
"352x288_I420.yuv"
"352x288_nv12.yuv"
)
ref_dir="ref/gst"

dest=/data/gst
golden_dir=gst_ref
check_num=0

# action==0, test case
# action==1, make golden file
action=$1

test_gst_ve_case_cmp=(
"gst-launch-1.0 filesrc location=${dest}/1080p_nv12.yuv ! rawvideoparse format=nv12 width=1920 height=1080 ! bmh265enc gop=50 ! filesink location=case0.h265"
"gst-launch-1.0 filesrc location=${dest}/352x288_nv12.yuv ! rawvideoparse format=nv12 width=352 height=288 ! bmh265enc gop=50 ! filesink location=case1.h265"
"gst-launch-1.0 filesrc location=${dest}/1080p_yuv420p.yuv ! rawvideoparse format=i420 width=1920 height=1080 ! bmh265enc gop=50 ! filesink location=case2.h265"
"gst-launch-1.0 filesrc location=${dest}/1920x1088_420.yuv ! rawvideoparse format=i420 width=1920 height=1088 ! bmh265enc gop=50 ! filesink location=case3.h265"
"gst-launch-1.0 filesrc location=${dest}/352x288_I420.yuv ! rawvideoparse format=i420 width=352 height=288 ! bmh265enc gop=50 ! filesink location=case4.h265"
"gst-launch-1.0 filesrc location=${dest}/1080p_nv12.yuv ! rawvideoparse format=nv12 width=1920 height=1080 ! bmh264enc gop=50 ! filesink location=case5.h264"
"gst-launch-1.0 filesrc location=${dest}/352x288_nv12.yuv ! rawvideoparse format=nv12 width=352 height=288 ! bmh264enc gop=50 ! filesink location=case6.h264"
"gst-launch-1.0 filesrc location=${dest}/1080p_yuv420p.yuv ! rawvideoparse format=i420 width=1920 height=1080 ! bmh264enc gop=50 ! filesink location=case7.h264"
"gst-launch-1.0 filesrc location=${dest}/1920x1088_420.yuv ! rawvideoparse format=i420 width=1920 height=1088 ! bmh264enc gop=50 ! filesink location=case8.h264"
"gst-launch-1.0 filesrc location=${dest}/352x288_I420.yuv ! rawvideoparse format=i420 width=352 height=288 ! bmh264enc gop=50 ! filesink location=case9.h264"
"gst-launch-1.0 filesrc location=${dest}/352x288_nv12.yuv ! rawvideoparse format=nv12 width=352 height=288 ! bmh265enc ! filesink location=case10.h265"
"gst-launch-1.0 filesrc location=${dest}/352x288_nv12.yuv ! rawvideoparse format=nv12 width=352 height=288 ! bmh264enc ! filesink location=case11.h264"
"gst-launch-1.0 filesrc location=${dest}/1080p_nv12.yuv ! rawvideoparse format=nv12 width=1920 height=1080 ! bmh265enc gop=50 bps=2000000 ! filesink location=case12.h265"
"gst-launch-1.0 filesrc location=${dest}/1080p_nv12.yuv ! rawvideoparse format=nv12 width=1920 height=1080 ! bmh264enc gop=50 bps=2000000 ! filesink location=case13.h264"
"gst-launch-1.0 filesrc location=${dest}/1080p_nv12.yuv ! rawvideoparse format=nv12 width=1920 height=1080 ! bmh264enc gop=50 cqp=32 ! filesink location=case14.h264"
"gst-launch-1.0 filesrc location=${dest}/1080p_nv12.yuv ! rawvideoparse format=nv12 width=1920 height=1080 ! bmh265enc gop=50 cqp=32 ! filesink location=case15.h265"
"gst-launch-1.0 filesrc location=${dest}/1080p_nv12.yuv ! rawvideoparse format=nv12 width=1920 height=1080 ! bmh265enc gop=50 bps=2000000 qp-min=24 qp-max=45 ! filesink location=case16.h265"
"gst-launch-1.0 filesrc location=${dest}/1080p_nv12.yuv ! rawvideoparse format=nv12 width=1920 height=1080 ! bmh264enc gop=50 bps=2000000 qp-min=24 qp-max=45 ! filesink location=case17.h264"
"gst-launch-1.0 filesrc location=${dest}/352x288_nv12.yuv ! rawvideoparse format=nv12 width=352 height=288 ! bmh265enc gop=50 bps=300000 ! filesink location=case18.h265"
"gst-launch-1.0 filesrc location=${dest}/352x288_nv12.yuv ! rawvideoparse format=nv12 width=352 height=288 ! bmh264enc gop=50 bps=300000 ! filesink location=case19.h264"
"gst-launch-1.0 filesrc location=${dest}/352x288_nv12.yuv ! rawvideoparse format=nv12 width=352 height=288 ! bmh265enc gop=50 bps=300000 gop-preset=5 ! filesink location=case20.h265"
"gst-launch-1.0 filesrc location=${dest}/352x288_nv12.yuv ! rawvideoparse format=nv12 width=352 height=288 ! bmh264enc gop=50 bps=300000 gop-preset=5 ! filesink location=case21.h264"
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

    ${test_gst_ve_case_cmp[num]}
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
    md5sum -c case${num}.md5 >> gst_ve.log
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
                echo ${line} "Command timed out." >> gst_ve.log
                printf "\n" >> gst_ve.log
            else
                echo case${num}: $line success >> gst_ve.log
                printf "\n" >> gst_ve.log
            fi
        else
            echo "Command encountered an error.$line"
            echo ${line} "Command Failed" >> gst_ve.log
            printf "\n" >> gst_ve.log
        fi

    fi

    find . -name case*
    ret=$?
    if [ ${ret} -ne 0 ]; then
        rm -rf case*
    fi
}

# test_all_case 1个输入参数
#       0 自动化测试
#       1 生成golden文件，进行比较，生成的golden文件传到服务器
function test_all_case(){
    echo action=${action}

    if [ ${action} -eq 1 ]; then
        echo "golden_dir:${golden_dir}"
        if [ -d ${golden_dir} ]; then
            rm -r ${golden_dir}
            mkdir -p ${golden_dir}
        else
            mkdir -p ${golden_dir}
        fi
    fi

    if [ -e "gst_ve.log" ]; then
        rm -rf gst_ve.log
    fi

    sudo  mkdir ${dest}
    scp_file ${src_dir}/${input_dir}/* ${host_name} ${host_ip} ${dest} ${host_passwd} 1

     for ((i=0; i<${#input_file[@]}; i++))
     do
         echo ${src_dir}/${input_dir}/${input_file[i]}
         scp_file ${src_dir}/${input_dir}/${input_file[i]} ${host_name} ${host_ip} ${dest} ${host_passwd} 1
     done

    for ((i=0; i<${#test_gst_ve_case_cmp[@]}; i++))
    do

        if [ ${action} -eq 0 ]; then
            # run case
            test_start 1 ${i} "${test_gst_ve_case_cmp[i]}"
        else
            # make golden file
            make_golden ${i}
        fi

    done
    sudo rm -rf ${dest}
}

test_all_case
