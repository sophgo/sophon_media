counter=0
check_num=0

timeout_duration=300 #超时时间（单位：秒)

host_name=dailytest
host_ip=172.28.141.219
host_passwd=dailytest
src_dir="/home/dailytest/Athena2/Multimedia/ocv-drawing"
input_dir="input/"
ref_dir="ref/"

# dest=/data/ocv-drawing
dest=/home/linaro
golden_dir=ocv-drawing_ref
check_num=0

# action==0, test case
# action==1, make golden file
action=0 #默认为0 保留test case功能 更新golden的MD5 需要手动进入脚本 修改action参数为1

test_ocv_drawing_cmp=(
        "ocv-drawing station.avi 1 0 0 0 out.png line=100_100_200_200_255_0_0_1"
        "ocv-drawing station.avi 1 0 0 0 out.png line=100_100_200_200_255_0_0_5"
        "ocv-drawing station.avi 1 0 0 0 out.png rectangle=300_200_500_400_0_0_255_5"
        "ocv-drawing station.avi 1 0 0 0 out.png circle=400_100_20_0_0_0_5"
        "ocv-drawing station.avi 1 0 0 0 out.png ellipse=500_400_100_40_160_0_255_255__5"
        "ocv-drawing station.avi 1 0 0 0 out.png polylines=4_400_100_900_150_900_300_4400_300_255_255_255_2"
        "ocv-drawing station.avi 1 0 0 0 out.png fillPoly=4_450_150_850_100_850_250_3550_250_255_0_0"
        "ocv-drawing station.avi 1 0 0 0 out.png putText=text.txt_200_200_3_2.5_0_255_0_6"
        "ocv-drawing station.avi 1 0 0 0 out.png freetype=text.txt_Arial-CE-Italic.ttf_100_100_50_0_0_250_-1_8"
        "ocv-drawing station.avi 1 0 0 0 out.png circle=500_600_100_0_0_0_1:ellipse=500_600_100_40_0_255_255_255_1:fillPoly=4_400_100_900_150_900_300_400_300_255_0_0:polylines=4_500_200_1000_250_1000_400_500_400_0_255_0_2:line=0_0_500_500_0_0_0_5:rectangle=0_0_100_100_255_255_255_20:putText=text.txt_100_500_4_3_0_0_0_5:freetype=text.txt_Arial-CE-Italic.ttf_100_100_80_0_0_0_-1_16"

        "ocv-drawing station.avi 50 1 0 0 out.jpg line=100_100_200_200_255_0_0_1"
        "ocv-drawing station.avi 50 1 0 0 out.jpg line=100_100_200_200_255_0_0_5"
        "ocv-drawing station.avi 50 1 0 0 out.jpg rectangle=300_200_500_400_0_0_255_5"
        "ocv-drawing station.avi 50 1 0 0 out.jpg circle=400_100_20_0_0_0_5"
        "ocv-drawing station.avi 50 1 0 0 out.jpg ellipse=500_400_100_40_160_0_255_255__5"
        "ocv-drawing station.avi 50 1 0 0 out.jpg polylines=4_400_100_900_150_900_300_4400_300_255_255_255_2"
        "ocv-drawing station.avi 50 1 0 0 out.jpg fillPoly=4_450_150_850_100_850_250_3550_250_255_0_0"
        "ocv-drawing station.avi 50 1 0 0 out.jpg putText=text.txt_200_200_3_2.5_0_255_0_6"
        "ocv-drawing station.avi 50 1 0 0 out.jpg freetype=text.txt_Arial-CE-Italic.ttf_100_100_50_0_0_250_-1_8"
        "ocv-drawing station.avi 300 1 0 0 out.jpg circle=500_600_100_0_0_0_1:ellipse=500_600_100_40_0_255_255_255_1:fillPoly=4_400_100_900_150_900_300_400_300_255_0_0:polylines=4_500_200_1000_250_1000_400_500_400_0_255_0_2:line=0_0_500_500_0_0_0_5:rectangle=0_0_100_100_255_255_255_20:putText=text.txt_100_500_4_3_0_0_0_5:freetype=text.txt_Arial-CE-Italic.ttf_100_100_80_0_0_0_-1_16"

        "ocv-drawing station.avi 50 1 1 0 out.jpg line=100_100_200_200_255_0_0_1"
        "ocv-drawing station.avi 50 1 1 0 out.jpg line=100_100_200_200_255_0_0_5"
        "ocv-drawing station.avi 50 1 1 0 out.jpg rectangle=300_200_500_400_0_0_255_5"
        "ocv-drawing station.avi 50 1 1 0 out.jpg circle=400_100_20_0_0_0_5"
        "ocv-drawing station.avi 50 1 1 0 out.jpg ellipse=500_400_100_40_160_0_255_255__5"
        "ocv-drawing station.avi 50 1 1 0 out.jpg polylines=4_400_100_900_150_900_300_4400_300_255_255_255_2"
        "ocv-drawing station.avi 50 1 1 0 out.jpg fillPoly=4_450_150_850_100_850_250_3550_250_255_0_0"
        "ocv-drawing station.avi 50 1 1 0 out.jpg putText=text.txt_200_200_3_2.5_0_255_0_6"
        "ocv-drawing station.avi 50 1 1 0 out.jpg freetype=text.txt_Arial-CE-Italic.ttf_100_100_50_0_0_250_-1_8"
        "ocv-drawing station.avi 300 1 1 0 out.jpg circle=500_600_100_0_0_0_1:ellipse=500_600_100_40_0_255_255_255_1:fillPoly=4_400_100_900_150_900_300_400_300_255_0_0:polylines=4_500_200_1000_250_1000_400_500_400_0_255_0_2:line=0_0_500_500_0_0_0_5:rectangle=0_0_100_100_255_255_255_20:putText=text.txt_100_500_4_3_0_0_0_5:freetype=text.txt_Arial-CE-Italic.ttf_100_100_80_0_0_0_-1_16"

        "ocv-drawing station.avi 50 1 0 0 video line=100_100_200_200_255_0_0_1"
        "ocv-drawing station.avi 50 1 0 0 video line=100_100_200_200_255_0_0_5"
        "ocv-drawing station.avi 50 1 0 0 video rectangle=300_200_500_400_0_0_255_5"
        "ocv-drawing station.avi 50 1 0 0 video circle=400_100_20_0_0_0_5"
        "ocv-drawing station.avi 50 1 0 0 video ellipse=500_400_100_40_160_0_255_255__5"
        "ocv-drawing station.avi 50 1 0 0 video polylines=4_400_100_900_150_900_300_4400_300_255_255_255_2"
        "ocv-drawing station.avi 50 1 0 0 video fillPoly=4_450_150_850_100_850_250_3550_250_255_0_0"
        "ocv-drawing station.avi 50 1 0 0 video putText=text.txt_200_200_3_2.5_0_255_0_6"
        "ocv-drawing station.avi 50 1 0 0 video freetype=text.txt_Arial-CE-Italic.ttf_100_100_50_0_0_250_-1_8"
        "ocv-drawing station.avi 300 1 0 0 video circle=500_600_100_0_0_0_1:ellipse=500_600_100_40_0_255_255_255_1:fillPoly=4_400_100_900_150_900_300_400_300_255_0_0:polylines=4_500_200_1000_250_1000_400_500_400_0_255_0_2:line=0_0_500_500_0_0_0_5:rectangle=0_0_100_100_255_255_255_20:putText=text.txt_100_500_4_3_0_0_0_5:freetype=text.txt_Arial-CE-Italic.ttf_100_100_80_0_0_0_-1_16"

        "ocv-drawing station.avi 50 1 1 0 video line=100_100_200_200_255_0_0_1"
        "ocv-drawing station.avi 50 1 1 0 video line=100_100_200_200_255_0_0_5"
        "ocv-drawing station.avi 50 1 1 0 video rectangle=300_200_500_400_0_0_255_5"
        "ocv-drawing station.avi 50 1 1 0 video circle=400_100_20_0_0_0_5"
        "ocv-drawing station.avi 50 1 1 0 video ellipse=500_400_100_40_160_0_255_255__5"
        "ocv-drawing station.avi 50 1 1 0 video polylines=4_400_100_900_150_900_300_4400_300_255_255_255_2"
        "ocv-drawing station.avi 50 1 1 0 video fillPoly=4_450_150_850_100_850_250_3550_250_255_0_0"
        "ocv-drawing station.avi 50 1 1 0 video putText=text.txt_200_200_3_2.5_0_255_0_6"
        "ocv-drawing station.avi 50 1 1 0 video freetype=text.txt_Arial-CE-Italic.ttf_100_100_50_0_0_250_-1_8"
        "ocv-drawing station.avi 300 1 1 0 video circle=500_600_100_0_0_0_1:ellipse=500_600_100_40_0_255_255_255_1:fillPoly=4_400_100_900_150_900_300_400_300_255_0_0:polylines=4_500_200_1000_250_1000_400_500_400_0_255_0_2:line=0_0_500_500_0_0_0_5:rectangle=0_0_100_100_255_255_255_20:putText=text.txt_100_500_4_3_0_0_0_5:freetype=text.txt_Arial-CE-Italic.ttf_100_100_80_0_0_0_-1_16"
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

    ${test_ocv_drawing_cmp[num]}
    sudo md5sum dump* >> ${golden_dir}/case${num}.md5
    sudo md5sum out*.mp4 >> ${golden_dir}/case${num}.md5

    sudo rm -rf dump*
    sudo rm -rf output*.mp4
}

function compare_md5(){
    num=$1

    scp_file ${src_dir}/${ref_dir}/case${num}.md5 ${host_name} ${host_ip} . ${host_passwd} 1
    echo case${num}: $line >> ocv-drawing.log
    md5sum -c case${num}.md5 >> ocv-drawing.log
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
                echo ${line} "Command timed out." >> ocv-drawing.log
                printf "\n" >> ocv-drawing.log
            else
                echo case${num}: $line success >> ocv-drawing.log
                printf "\n" >> ocv-drawing.log
            fi
        else
            echo "Command encountered an error.$line"
            echo ${line} "Command Failed" >> ocv-drawing.log
            printf "\n" >> ocv-drawing.log
        fi

    fi

    sudo rm -rf dump*
    sudo rm -rf output*.mp4
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

    if [ -e "ocv-drawing.log" ]; then
        rm -rf ocv-drawing.log
    fi

    # sudo  mkdir ${dest}
    scp_file ${src_dir}/${input_dir}/* ${host_name} ${host_ip} ${dest} ${host_passwd} 1

    for ((i=0; i<${#test_ocv_drawing_cmp[@]}; i++))
    do

        if [ ${action} -eq 0 ]; then
            # run case
            test_start 1 ${i} "${test_ocv_drawing_cmp[i]}"
        else
            # make golden file
            make_golden ${i}
        fi

    done

    # sudo rm -r ${dest}
    sudo rm station.avi text.txt Arial-CE-Italic.ttf
}

test_all_case


