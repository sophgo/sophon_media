
test_data=(
    "JPEG_1920x1088_yuv420_planar.jpg"
    "JPEG_1920x1088_yuv420_planar.jpg"
    "JPEG_16x16_yuv420_planar.jpg "
    "JPEG_32768x48_yuv420_planar.jpg"
    "JPEG_64x32768_yuv420_planar.jpg"
    "JPEG_1920x1088_yuv420_planar.yuv"
    "JPEG_1920x1088_yuv420_planar.yuv"
    "JPEG_1920x1088_yuv420_planar.yuv"
    "JPEG_1920x1088_yuv420_planar.yuv"
    "JPEG_16x16_yuv420_planar.yuv"
    "JPEG_32768x48_yuv420_planar.yuv"
    "JPEG_64x32768_yuv420_planar.yuv"
    "JPEG_1920x1088_yuv400_planar.yuv"
    "JPEG_1920x1088_yuv420_planar.yuv"

)

bmapijpeg_case=(
    "bmjpegdec -i JPEG_1920x1088_yuv420_planar.jpg -o out_1920x1088_yuv420_planar.yuv -n 1"
"bmjpegdec -i JPEG_1920x1088_yuv420_planar.jpg -o out_1920x1088_yuv420_planar.yuv -n 10"
"bmjpegdec -i JPEG_16x16_yuv420_planar.jpg -o outJPEG_16x16_yuv420_planar.yuv"
"bmjpegdec -i JPEG_32768x48_yuv420_planar.jpg -o outJPEG_32768x48_yuv420_planar.yuv"
"bmjpegdec -i JPEG_64x32768_yuv420_planar.jpg -o outJPEG_64x32768_yuv420_planar.yuv"
"bmjpegenc -f 0 -w 1920 -h 1088 -i JPEG_1920x1088_yuv420_planar.yuv -o outJPEG_1920x1088_yuv420_planar.jpg -n 1"
"bmjpegenc -f 0 -w 1920 -h 1088 -i JPEG_1920x1088_yuv420_planar.yuv -o outJPEG_1920x1088_yuv420_planar.jpg -n 10000"
"bmjpegenc -f 0 -w 1920 -h 1088 -i JPEG_1920x1088_yuv420_planar.yuv -o outJPEG_1920x1088_yuv420_planar.jpg -n 1 -g 1"
"bmjpegenc -f 0 -w 1920 -h 1088 -i JPEG_1920x1088_yuv420_planar.yuv -o outJPEG_1920x1088_yuv420_planar.jpg -n 1 -g 4"
"bmjpegenc -f 0 -w 16 -h 16 -i JPEG_16x16_yuv420_planar.yuv -o outJPEG_16x16_yuv420_planar.jpg"
"bmjpegenc -f 0 -i JPEG_32768x48_yuv420_planar.yuv -w 32768 -h 48 -o outJPEG_32768x48_yuv420_planar.jpg"
"bmjpegenc -f 0 -i JPEG_64x32768_yuv420_planar.yuv -w 64 -h 32768 -o outJPEG_64x32768_yuv420_planar.jpg"
"bmjpegmulti -t 1 -i JPEG_1920x1088_yuv400_planar.yuv -o out1920x1088_400.jpg -w 1920 -h 1088 -s 4 -f 0 -n 4"
"bmjpegmulti -t 1 -i JPEG_1920x1088_yuv420_planar.yuv -o outJPEG_1920x1088_yuv420_planar.jpg -w 1920 -h 1088 -s 0 -f 0 -n 4"

)

install_name=dailytest
install_ip=172.28.141.219
install_pass=dailytest
ref_dest=/data

timeout_duration=300
input_dest="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

install_dir="/home/dailytest/Athena2/Multimedia"
ref_dir=jpeg
check_num=0

function scp_file()
{

file_addr=$1
name=$2
ipaddres=$3
dest_path=$4
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
          send "yes\n"
        }
   eof
    {
        sleep 5
        send_user "eof\n"
    }
}

expect eof
EOF
}

compare(){
    line=$1

    files=($(find .  -maxdepth 1 -type f \( -name "out*.jpg" -o -name "out*.yuv" \)))

    if [[ ${#files[@]} -gt 0 ]];then
        arr1=()
        for file in "${files[@]}";do
            arr1+=("$(basename "$file")")
        done
        for file in "${arr1[@]}"; do
            extension="${file##*.}"

            diff "$file" "${ref_dest}/${ref_dir}/out_bmapijpeg/bmapijpeg${check_num}.${extension}"
            if [ $? -ne 0 ]
            then
                echo ${line} >> bmapijpegdiff.txt
            else
                
                echo "success"
            fi
            rm -rf $file
            check_num=$((check_num+1))
        done
    else
        echo "no out file to compare"
    fi
}

function test_start(){
    num=$1
    line=$2

    timeout "$timeout_duration" ${bmapijpeg_case[num]} 
    exit_code=$?
    sleep 1
    if [ $exit_code -eq 0 ]; then
        echo $line
        compare "$line"
    else
        if [ $exit_code -eq 124 ]; then
            echo "Command timed out.$line"
            echo ${line} >> bmapijpegfail.txt
        else
            echo "Command encountered an error.$line"
            echo ${line} >> bmapijpegfail.txt
            printf "\n" >> bmapijpegfail.txt
        fi
    fi
}

function test_all_case(){
    find . -name bmapijpegfail.txt
    ret=$?
    if [ ${ret} -ne 0 ]; then
         rm -rf bmapijpegfail.txt
    fi
    find . -name bmapijpegdiff.txt
    ret=$?
    if [ ${ret} -ne 0 ]; then
         rm -rf bmapijpegdiff.txt
    fi
    if [ ! -d "${ref_dest}/${ref_dir}" ]; then
        sudo mkdir ${ref_dest}/${ref_dir}
    fi

    scp_file ${install_dir}/${ref_dir}/out_bmapijpeg  $install_name  $install_ip   ${ref_dest}/${ref_dir}   $install_pass 1

    for ((i=0; i<${#bmapijpeg_case[@]}; i++))
    do

        scp_file ${install_dir}/all_data_opencv_ffmpeg/${test_data[i]}  $install_name  $install_ip   $input_dest   $install_pass 1

        test_start "$i" "${bmapijpeg_case[i]}"
        rm -rf ${input_dest}/${test_data[i]}

    done
    sudo rm -rf ${ref_dest}/${ref_dir}
}

test_all_case






