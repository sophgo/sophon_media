counter=0
check_num=0

timeout_duration=30 #超时时间（单位：秒)

host_name=dailytest
host_ip=172.28.141.219
host_passwd=dailytest
dest=/data

src_dir="/home/dailytest/Athena2/Multimedia/vd"
input_dir="${src_dir}/input/bmapi"
ref_dir="ref/bmapi"
check_num=0

bmapivd_case=(
"bm_test -c 0 -n 1 -m 2 --input ${dest}/bilibiligd_5.264  --instance 4"
"bm_test -c 0 -n 1 -m 0 --input ${dest}/bilibiligd_5.264 --instance 4"
"bm_test -c 0 -n 1 -m 0 --input ${dest}/bilibiligd_5.264 --wtl-format 101 --instance 1"
"bm_test -c 0 -n 1 --output_format 1 --input ${dest}/bilibiligd_5.264 --instance 1 "
"bm_test -c 1 --ref-yuv ${dest}/${ref_dir}/264_1080p_nv21.yuv -n 1 --output_format 2 --input ${dest}/bilibiligd_5.264 --instance 1 "
"bm_test -c 0 -n 1 --input ${dest}/2560x1440_intree100.264 --output_format 1 --instance 1"
"bm_test -c 0 -n 1 --input ${dest}/test_h264_4k.264 --output_format 1 --instance 1"
"bm_test -c 0 -n 1 --input ${dest}/test_h264_7680_4320.264 --output_format 1 --instance 1"

"bm_test -c 0 -m 2 --stream-type 12 --input ${dest}/test_h265_hd_20m.265 --instance 4"
"bm_test -c 0 -n 1 -m 0 --stream-type 12 --input ${dest}/test_h265_hd_20m.265 --instance 4"
"bm_test -c 0 -n 1 --stream-type 12 --input ${dest}/test_h265_hd_20m.265 --wtl-format 101 --instance 1"
"bm_test -c 0 -n 1 --output_format 1 --stream-type 12 --input ${dest}/test_h265_hd_20m.265 --instance 1"
"bm_test -c 0 --output_format 2 --stream-type 12 --input ${dest}/test_h265_hd_20m.265 --instance 1"
"bm_test -c 0 -n 1 --stream-type 12 --input ${dest}/2560x1440_intree100.265 --output_format 1 --instance 1"
"bm_test -c 0 -n 1 --stream-type 12 --input ${dest}/test_h265_main10_4k_20m.265 --output_format 1 --instance 1"
"bm_test -c 0 -n 1 --stream-type 12 --input ${dest}/test_h265main10_7680_4320.265 --output_format 1 --instance 1"
)

input_data=(
    "bilibiligd_5.264"
    "bilibiligd_5.264"
    "bilibiligd_5.264"
    "bilibiligd_5.264"
    "bilibiligd_5.264"
    "2560x1440_intree100.264"
    "test_h264_4k.264"
    "test_h264_7680_4320.264"
    "test_h265_hd_20m.265"
    "test_h265_hd_20m.265"
    "test_h265_hd_20m.265"
    "test_h265_hd_20m.265"
    "test_h265_hd_20m.265"
    "test_h265_hd_20m.265"
    "2560x1440_intree100.265"
    "test_h265_main10_4k_20m.265"
    "test_h265main10_7680_4320.265"
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
    spawn sudo scp -r $filename linaro@$ipaddres:$dest_path
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

function test_start(){
    num=$1
    line=$2

    timeout ${timeout_duration} ${bmapivd_case[num]}
    exit_code=$?
    sleep 1
    if [ $exit_code -eq 0 ]; then
        echo $line

    else
        if [ $exit_code -eq 124 ]; then
            echo "Command timed out.$line"
            echo ${line} "Command timed out." >> bmapi_vd.log
            printf "\n" >> bmapi_vd.log
        else
            echo "Command encountered an error.$line"
            echo ${line} "Command Failed" >> bmapi_vd.log
            printf "\n" >> bmapi_vd.log
        fi

    fi
}

function test_all_case(){
    find . -name bmapi_vd.log
    ret=$?
    if [ ${ret} -ne 0 ]; then
        rm -rf bmapi_vd.log
    fi

    for ((i=0; i<${#bmapivd_case[@]}; i++))
    do

        # download input data
        scp_file ${input_dir}/${input_data[i]} ${host_name} ${host_ip} ${dest} ${host_passwd} 1

        # run case
        test_start "$i" "${bmapivd_case[i]}"

        # remove input
        sudo rm -rf ${dest}/${input_data[i]}
        echo "rm -rf ${dest}/${input_data[i]}"

    done

    # sudo rm -r ${dest}/${ref_dir}
}

test_all_case