
test_data=(
"00_352x288_y.yuv"
"bin_352x288_y.yuv"
)

test_gst_ive_case=(
"gst-launch-1.0 -v filesrc location=00_352x288_y.yuv ! videoparse format=gray8 width=352 height=288  ! bmive Type=0 X=19584 Y=45952  ! 'video/x-raw, format=GRAY8, width=352, height=288'  ! filesink location=case0.yuv"
"gst-launch-1.0 -v filesrc location=00_352x288_y.yuv ! videoparse format=gray8 width=352 height=288  ! bmive Type=1 ! 'video/x-raw, format=GRAY8, width=352, height=288'  ! filesink location=case1.yuv"
"gst-launch-1.0 -v filesrc location=00_352x288_y.yuv ! videoparse format=gray8 width=352 height=288  ! bmive Type=2 ! 'video/x-raw, format=GRAY8, width=352, height=288'  ! filesink location=case2.yuv"
"gst-launch-1.0 -v filesrc location=00_352x288_y.yuv ! videoparse format=gray8 width=352 height=288  ! bmive Type=3 ! 'video/x-raw, format=GRAY8, width=352, height=288'  ! filesink location=case3.yuv"
"gst-launch-1.0 -v filesrc location=00_352x288_y.yuv ! videoparse format=gray8 width=352 height=288  ! bmive Type=4 ! 'video/x-raw, format=GRAY8, width=352, height=288'  ! filesink location=case4.yuv"
"gst-launch-1.0 -v filesrc location=00_352x288_y.yuv ! videoparse format=gray8 width=352 height=288  ! bmive Type=5 ! 'video/x-raw, format=GRAY8, width=352, height=288'  ! filesink location=case5.yuv"
"gst-launch-1.0 -v filesrc location=00_352x288_y.yuv ! videoparse format=gray8 width=352 height=288  ! bmive Type=6  ! filesink location=case6.yuv"
"gst-launch-1.0 -v filesrc location=00_352x288_y.yuv ! videoparse format=gray8 width=352 height=288  ! bmive Type=7 Mode1=2 ! 'video/x-raw, format=GRAY8, width=352, height=288'  ! filesink location=case7.yuv"
"gst-launch-1.0 -v filesrc location=00_352x288_y.yuv ! videoparse format=gray8 width=352 height=288  ! bmive Type=8  Mode1=1 InitAreaThr=4 Step=2 ! filesink location=case8.yuv"
"gst-launch-1.0 -v filesrc location=00_352x288_y.yuv ! videoparse format=gray8 width=352 height=288  ! bmive Type=9 Norm=4 ! 'video/x-raw, format=GRAY8, width=352, height=288'  ! filesink location=case9.yuv"
"gst-launch-1.0 -v filesrc location=bin_352x288_y.yuv ! videoparse format=gray8 width=352 height=288  ! bmive Type=10  ! 'video/x-raw, format=GRAY8, width=352, height=288'  ! filesink location=case10.yuv"
"gst-launch-1.0 -v filesrc location=bin_352x288_y.yuv ! videoparse format=gray8 width=352 height=288  ! bmive Type=11  ! 'video/x-raw, format=GRAY8, width=352, height=288'  ! filesink location=case11.yuv"
"gst-launch-1.0 -v filesrc location=00_352x288_y.yuv ! videoparse format=gray8 width=352 height=288  ! bmive Type=12 Mode1=3 LowThr=236 HightThr=249 MinVal=166 MidVal=219 MaxVal=60 ! 'video/x-raw, format=GRAY8, width=352, height=288'  ! filesink location=case12.yuv"
"gst-launch-1.0 -v filesrc location=00_352x288_y.yuv ! videoparse format=gray8 width=352 height=288  ! bmive Type=13 ! 'video/x-raw, format=GRAY8, width=352, height=288'  ! filesink location=case13.yuv"
"gst-launch-1.0 -v filesrc location=00_352x288_y.yuv ! videoparse format=gray8 width=352 height=288  ! bmive Type=14 Mode1=3 LowThr=236 HightThr=249 MinVal=166 MidVal=219 MaxVal=60 ! 'video/x-raw, format=GRAY8, width=352, height=288'  ! filesink location=case14.yuv"
"gst-launch-1.0 -v filesrc location=00_352x288_y.yuv ! videoparse format=gray8 width=352 height=288  ! bmive Type=15 Mode1=3 LowThr=236 HightThr=249 MinVal=166 MidVal=219 MaxVal=60 ! 'video/x-raw, format=GRAY8, width=352, height=288'  ! filesink location=case15.yuv"
"gst-launch-1.0 -v filesrc location=00_352x288_y.yuv ! videoparse format=gray8 width=352 height=288  ! bmive Type=16 Mode1=0 ! 'video/x-raw, format=GRAY8, width=352, height=288'  ! filesink location=case16.yuv"
"gst-launch-1.0 -v filesrc location=00_352x288_y.yuv ! videoparse format=gray8 width=352 height=288  ! bmive Type=17 Mode1=0 ! 'video/x-raw, format=GRAY8, width=352, height=288'  ! filesink location=case17.yuv"
"gst-launch-1.0 -v filesrc location=00_352x288_y.yuv ! videoparse format=gray8 width=352 height=288  ! bmive Type=18 ! 'video/x-raw, format=GRAY8, width=352, height=288'  ! filesink location=case18.yuv"
"gst-launch-1.0 -v filesrc location=00_352x288_y.yuv ! videoparse format=gray8 width=352 height=288  ! bmive Type=19 Mode1=0 ! 'video/x-raw, format=GRAY8, width=352, height=288'  ! filesink location=case19.yuv"
"gst-launch-1.0 -v filesrc location=00_352x288_y.yuv ! videoparse format=gray8 width=352 height=288  ! bmive Type=20 Mode1=0 ThrValue=0 ! 'video/x-raw, format=GRAY8, width=352, height=288'  ! filesink location=case20.yuv"
"gst-launch-1.0 -v filesrc location=00_352x288_y.yuv ! videoparse format=gray8 width=352 height=288  ! bmive Type=21 Mode1=1  ! 'video/x-raw, format=GRAY8, width=352, height=288'  ! filesink location=case21.yuv"
# "gst-launch-1.0 -v filesrc location=00_352x288_y.yuv ! videoparse format=gray8 width=352 height=288  ! bmive Type=22 ! filesink location=case22.yuv ?"
# "gst-launch-1.0 -v filesrc location=00_352x288_y.yuv ! videoparse format=gray8 width=352 height=288  ! bmive Type=23 ! filesink location=case23.yuv ?"
"gst-launch-1.0 -v filesrc location=00_352x288_y.yuv ! videoparse format=gray8 width=352 height=288  ! bmive Type=24 ! filesink location=case24.yuv"
# "gst-launch-1.0 -v filesrc location=00_352x288_y.yuv ! videoparse format=gray8 width=352 height=288  ! bmive Type=25 ! 'video/x-raw, format=GRAY8, width=352, height=288' ! filesink location=case25.yuv ?"
# "gst-launch-1.0 -v filesrc location=00_352x288_y.yuv ! videoparse format=gray8 width=352 height=288  ! bmive Type=26 ! 'video/x-raw, format=GRAY8, width=352, height=288' ! filesink location=case26.yuv ?"
# "gst-launch-1.0 -v filesrc location=00_352x288_y.yuv ! videoparse format=gray8 width=352 height=288  ! bmive Type=27 ThrValue=2048 MinVal=2 MaxVal=30 ! 'video/x-raw, format=GRAY8, width=352, height=288' ! filesink location=case27.yuv ?"
"gst-launch-1.0 -v filesrc location=00_352x288_y.yuv ! videoparse format=gray8 width=352 height=288  ! bmive Type=28 ! 'video/x-raw, format=GRAY8, width=352, height=288' ! filesink location=case28.yuv"
)

install_name=dailytest
install_ip=172.28.141.219
install_pass=dailytest
ref_dest=/data

input_dest="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

timeout_duration=600

install_dir="/home/dailytest/Athena2/Multimedia/bmcv/ive_data"
ref_dir=ive
check_num=0
ref_md5_filename=gstive_ref_md5.txt
golden_dir=gst_ref

# action==0, test case
# action==1, make golden file
action=$1

function scp_file()
{

file_addr=$1
name=$2
ipaddres=$3
dest_path=$4
passwd=$5
to_remote=$6

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

compare(){
    line=$1
    num=$2
    check_num=0

    files=($(find . -maxdepth 1 -type f \( -name "output*.jpg" -o -name "test_gst_jpeg*" -o -name "output*.yuv" -o -name "recycle-output*.yuv" \)))
    if [[ ${#files[@]} -gt 0 ]];then
        arr1=()
        for file in "${files[@]}";do
            arr1+=("$(basename "$file")")
        done

        for file in "${arr1[@]}"; do
            extension="${file##*.}"
            new_filename="${num}_gstjpeg${check_num}.${extension}"
            sudo mv "$file" "$new_filename"

            md5_src=$(md5sum "$new_filename" | awk '{ print $1 }')
            md5_ref=$(grep "$new_filename" ${ref_dest}/${ref_dir}/${ref_md5_filename} | awk '{print $1}' | head -n 1)

            if [ "$md5_src" == "$md5_ref" ]
            then
                echo "success"
            else
                echo "fail case is $num"
                echo ${line} >> gstjpegdiff.txt
            fi
            rm -rf ${new_filename}
            check_num=$((check_num+1))
        done
    else
        echo "no out file to compare"
    fi
}

function make_golden(){
    num=$1

    ${test_gst_ive_case[num]}
    sudo md5sum case${num}* >> ${golden_dir}/case${num}.md5

    find . -name case*
    ret=$?
    if [ ${ret} -ne 0 ]; then
        rm -rf case*
    fi
}

function test_start(){
    num=$1
    line=$2

    timeout ${timeout_duration} ${test_gst_ive_case[num]}
    exit_code=$?
    sleep 1
    if [ $exit_code -eq 0 ]; then
        compare "$line" "$num"

    else
        if [ $exit_code -eq 124 ]; then
            echo "Command timed out.$line"
            echo ${line} >> gstivefail.txt
        else
            echo "Command encountered an error.$line"
            echo ${line} >> gstivefail.txt
            printf "\n" >> gstivefail.txt
        fi
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

    if [ -f "gstivefail.txt" ]; then
         rm -rf gstivefail.txt
    fi
    if [ -f "gstivefail.txt" ]; then
         rm -rf gstivefail.txt
    fi
    # /data/jpeg
    if [ ! -d "${ref_dest}/${ref_dir}" ]; then
        sudo mkdir ${ref_dest}/${ref_dir}
    fi
    find ${ref_dest}/${ref_dir} -name ${ref_md5_filename}
    ret=$?
    if [ ${ret} -eq 0 ]; then
         rm -rf ${ref_md5_filename}
    fi

    #scp_file ${install_dir}/${ref_dir}/gstjpeg/${ref_md5_filename}  $install_name  $install_ip   ${ref_dest}/${ref_dir}    $install_pass 1

    for ((i=0; i<${#test_gst_ive_case[@]}; i++))
    do
        scp_file ${install_dir}/${test_data[i]}  $install_name  $install_ip   $input_dest   $install_pass 1
        if [ ${action} -eq 0 ]; then
            # run case
            test_start "$i" "${test_gst_ive_case[i]}"
        else
            # make golden file
            make_golden ${i}
        fi

       #rm -rf ${input_dest}/${test_data[i]}

    done
    sudo rm -rf ${ref_dest}/${ref_dir}
}

test_all_case
