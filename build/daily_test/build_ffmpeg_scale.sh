test_data=(
"1920x1080.mp4"
"1920x1080.mp4"
"station_1080p.265"
"station_1080p.265"
"station_1080p.265"
"station_1080p.265"
"station_1080p.265"
"station_1080p.265"
"station_1080p.265"
"station_1080p.265"
"station_1080p.265"
"1920x1080.mp4"
"bilibiligd_5_nv12.yuv"
"bilibiligd_5_nv12.yuv"
"bilibiligd_5_nv12.yuv"
"bilibiligd_5_nv12.yuv"
"bilibiligd_5_nv12.yuv"
"1920x1080_444p.yuv"
"1920x1080_444p.yuv"
"1920x1080_444p.yuv"
"1920x1080_444p.yuv"
"1920x1080_444p.yuv"
"1920x1080_444p.yuv"
"1920x1080_444p.yuv"
"1920x1080_444p.yuv"
"1920x1080_444p.yuv"
"bgr-org-opencv.rgb"
"bgr-org-opencv.rgb"
"bgr-org-opencv.rgb"
"bgr-org-opencv.rgb"
"bgr-org-opencv.rgb"
"station_4mb_200.264"
"station_4mb_200.264"
"station_4mb_200.264"
"bilibiligd_5.264"
"bilibiligd_5.264"
"station_1080p.265"
"station_1080p.265"
"bilibiligd_5.264"
"bilibiligd_5.264"
"bilibiligd_5.264"
"bilibiligd_5.264"
"bilibiligd_5.264"
)

test_ffmpeg_scale_case=(
"ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v h264_bm -output_format 101 -i 1920x1080.mp4 -vf "scale_bm=352:288" -c:v h264_bm -g 50 -b:v 32K -enc-params "gop_preset=2:mb_rc=1:delta_qp=3:min_qp=20:max_qp=40" -y test_ffmpeg_scale_out1.264"
"test_ff_bmcv_transcode soc 1920x1080.mp4 test_ffmpeg_scale_out2.ts I420 h264_bm 800 400 25 3000 3 0 0"
"ffmpeg -c:v hevc_bm -output_format 101 -i station_1080p.265 -vf "scale_bm=352:288:opt=crop:zero_copy=1" -c:v h264_bm -g 256 -b:v 256K -y test_ffmpeg_scale_out3.264"
"ffmpeg -c:v hevc_bm -i station_1080p.265 -vf "scale_bm=2560:1440:opt=crop:zero_copy=1" -c:v h264_bm -g 256 -b:v 8M -y test_ffmpeg_scale_out4.264"
"ffmpeg -c:v hevc_bm -i station_1080p.265 -vf "scale_bm=352:288:opt=pad:zero_copy=1" -c:v h264_bm -g 256 -b:v 256K -y test_ffmpeg_scale_out5.264"
"ffmpeg -c:v hevc_bm -i station_1080p.265 -vf "scale_bm=1920:1080:opt=pad:zero_copy=1" -c:v h264_bm -g 256 -b:v 2M -y test_ffmpeg_scale_out6.264"
"ffmpeg -c:v hevc_bm -i station_1080p.265 -vf "scale_bm=2560:1440:opt=pad:format=yuvj420p:zero_copy=1" -c:v jpeg_bm -vframes 1 -y test_ffmpeg_scale_out7.jpeg"
"ffmpeg -c:v hevc_bm -i station_1080p.265 -filter_complex "[0:v]scale_bm=352:288:zero_copy=1[img1]\;[0:v]scale_bm=1280:720:format=yuvj420p:zero_copy=1[img2]" -map "[img1]" -c:v h264_bm -b:v 2M -y img1.264 -map "[img2]" -c:v jpeg_bm -vframes 1 -y test_ffmpeg_scale_out8.jpeg"
"ffmpeg -c:v hevc_bm -cbcr_interleave 0 -zero_copy 0 -i station_1080p.265 -y test_ffmpeg_scale_out9.yuv"
"ffmpeg -c:v hevc_bm -i station_1080p.265 -vf "scale_bm=352:288,format=yuv420p" -y test_ffmpeg_scale_out10.yuv"
"ffmpeg -c:v hevc_bm -i station_1080p.265 -vf "scale_bm=352:288:opt=pad:flags=bilinear:zero_copy=1" -c:v h264_bm -g 256 -b:v 256K -y test_ffmpeg_scale_out11.264"
"ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v h264_bm -output_format 101 -i 1920x1080.mp4 -vf "scale_bm=352:288" -c:v h264_bm -g 265 -b:v 256K -y test_ffmpeg_scale_out12.265"
"ffmpeg -s 1920X1080 -pix_fmt nv12 -i bilibiligd_5_nv12.yuv -vf "scale_bm=700:600:opt=1:flags=1:format=rgb24" -y test_ffmpeg_scale_out13.rgb -loglevel trace"
"ffmpeg -s 1920X1080 -pix_fmt nv12 -i bilibiligd_5_nv12.yuv -vf "scale_bm=900:900:opt=0:flags=0:format=yuv444p" -y test_ffmpeg_scale_out14.yuv -loglevel trace"
"ffmpeg -s 1920X1080 -pix_fmt nv12 -i bilibiligd_5_nv12.yuv -vf "scale_bm=1920:1080:opt=1:flags=2:format=yuv422p" -y test_ffmpeg_scale_out15.yuv -loglevel trace"
"ffmpeg -s 1920X1080 -pix_fmt nv12 -i bilibiligd_5_nv12.yuv -vf "scale_bm=1900:1300:opt=2:flags=2:format=bgr24" -y test_ffmpeg_scale_out16.rgb -loglevel trace"
"ffmpeg -s 1920X1080 -pix_fmt nv12 -i bilibiligd_5_nv12.yuv -vf "scale_bm=700:600:opt=1:flags=1:format=rgb24" -y test_ffmpeg_scale_out17.rgb -loglevel trace"
"ffmpeg -s 1920x1080 -pix_fmt yuv444p -i 1920x1080_444p.yuv -vf "scale_bm=550:660:opt=0:flags=0" -y test_ffmpeg_scale_out18.rgb -loglevel trace"
"ffmpeg -s 1920x1080 -pix_fmt yuv444p -i 1920x1080_444p.yuv -vf "scale_bm=700:800:opt=1:flags=0" -y test_ffmpeg_scale_out19.rgb -loglevel trace"
"ffmpeg -s 1920x1080 -pix_fmt yuv444p -i 1920x1080_444p.yuv -vf "scale_bm=900:800:opt=2:flags=0" -y test_ffmpeg_scale_out20.rgb -loglevel trace"
"ffmpeg -s 1920x1080 -pix_fmt yuv444p -i 1920x1080_444p.yuv -vf "scale_bm=1200:800:opt=2:flags=1" -y test_ffmpeg_scale_out21.rgb -loglevel trace"
"ffmpeg -s 1920x1080 -pix_fmt yuv444p -i 1920x1080_444p.yuv -vf "scale_bm=1100:600:opt=1:flags=2" -y test_ffmpeg_scale_out22.rgb -loglevel trace"
"ffmpeg -s 1920x1080 -pix_fmt yuv444p -i 1920x1080_444p.yuv -vf "scale_bm=1600:1300:opt=2:flags=2:format=yuv420p" -y test_ffmpeg_scale_out24.yuv"
"ffmpeg -s 1920x1080 -pix_fmt yuv444p -i 1920x1080_444p.yuv -vf "scale_bm=900:1300:opt=2:flags=2:format=yuv422p" -y test_ffmpeg_scale_out25.yuv"
"ffmpeg -s 1920x1080 -pix_fmt yuv444p -i 1920x1080_444p.yuv -vf "scale_bm=900:1300:opt=2:flags=2:format=rgb24" -y test_ffmpeg_scale_out26.rgb"
"ffmpeg -s 1920x1080 -pix_fmt yuv444p -i 1920x1080_444p.yuv -vf "scale_bm=1900:1300:opt=2:flags=2:format=bgr24" -y test_ffmpeg_scale_out27.rgb"
"ffmpeg -s 1920x1080 -pix_fmt bgr24 -i bgr-org-opencv.rgb -vf "scale_bm=1900:1300:opt=2:flags=2:format=bgr24" -y test_ffmpeg_scale_out28.rgb"
"ffmpeg -s 1920x1080 -pix_fmt bgr24 -i bgr-org-opencv.rgb -vf "scale_bm=768:666:opt=1:flags=1:format=rgb24" -y test_ffmpeg_scale_out29.rgb"
"ffmpeg -s 1920x1080 -pix_fmt bgr24 -i bgr-org-opencv.rgb -vf "scale_bm=568:866:opt=1:flags=1:format=yuv420p" -y test_ffmpeg_scale_out30.yuv"
"ffmpeg -s 1920x1080 -pix_fmt bgr24 -i bgr-org-opencv.rgb -vf "scale_bm=1168:1266:opt=0:flags=1:format=yuv422p" -y test_ffmpeg_scale_out31.yuv"
"ffmpeg -s 1920x1080 -pix_fmt bgr24 -i bgr-org-opencv.rgb -vf "scale_bm=8192:8192:opt=1:flags=0:format=yuv444p" -y test_ffmpeg_scale_out32.yuv"
"ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v h264_bm -i station_4mb_200.264 -vf "scale_bm=1920:1080:opt=2:flags=2,hwdownload,format=yuv420p\|bmcodec" -y test_ffmpeg_scale_out33.yuv -loglevel trace"
"ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v h264_bm -i station_4mb_200.264 -frames:v 1 -vf "scale_bm=1920:1080:opt=0:flags=0,hwdownload,format=yuv420p\|bmcodec" -y test_ffmpeg_scale_out34.yuv -loglevel trace"
"ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v h264_bm -i station_4mb_200.264 -frames:v 1 -vf "scale_bm=1600:900:opt=0:flags=0,hwdownload,format=yuv420p\|bmcodec" -y test_ffmpeg_scale_out35.yuv -loglevel trace"
"ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v h264_bm -i bilibiligd_5.264 -vf "scale_bm=1920:1080:opt=1:flags=1,hwdownload,format=yuv420p\|bmcodec" -y test_ffmpeg_scale_out36.yuv -loglevel trace"
"ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v h264_bm -i bilibiligd_5.264 -vf "scale_bm=1920:1080:opt=1:flags=2,hwdownload,format=yuv420p\|bmcodec" -y test_ffmpeg_scale_out37.yuv -loglevel trace"
"ffmpeg -c:v hevc_bm -output_format 0 -i station_1080p.265 -vf "scale_bm=352:288:zero_copy=1" -c:v h264_bm -g 256 -b:v 256K -y test_ffmpeg_scale_out38.264"
"ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v hevc_bm -output_format 0 -i station_1080p.265 -vf "scale_bm=352:288" -c:v h264_bm -g 256 -b:v 256K -y test_ffmpeg_scale_out39.264"
"ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v h264_bm -i bilibiligd_5.264 -frames:v 1 -vf "scale_bm=640:640:opt=1:flags=0,hwdownload,format=yuv420p\|bmcodec" -y test_ffmpeg_scale_out40.yuv -loglevel trace"
"ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v h264_bm -i bilibiligd_5.264 -frames:v 1 -vf "scale_bm=1024:1024:opt=2:flags=2,hwdownload,format=yuv420p\|bmcodec" -y test_ffmpeg_scale_out41.yuv -loglevel trace"
"ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v h264_bm -i bilibiligd_5.264 -frames:v 1 -vf "scale_bm=512:384:opt=0:flags=1,hwdownload,format=yuv420p\|bmcodec" -y test_ffmpeg_scale_out42.yuv -loglevel trace"
"ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v h264_bm -i bilibiligd_5.264 -frames:v 1 -vf "scale_bm=1600:900:opt=0:flags=0,hwdownload,format=yuv420p\|bmcodec" -y test_ffmpeg_scale_out43.yuv -loglevel trace"
"ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v h264_bm -i bilibiligd_5.264 -frames:v 1 -vf "scale_bm=366:1000:opt=2:flags=1,hwdownload,format=yuv420p\|bmcodec" -y test_ffmpeg_scale_out44.yuv -loglevel trace"
)

install_name=dailytest
install_ip=172.28.141.219
install_pass=dailytest
ref_dest=/data

input_dest="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

timeout_duration=600

install_dir="/home/dailytest/Athena2/Multimedia"
ref_dir=ffmpeg
check_num=0
ref_md5_filename=ffmpegscale_ref_md5.txt


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

    files=($(find . -maxdepth 1 -type f \( -name "test_ffmpeg_scale*" -o -name "img*.264" \)))
    if [[ ${#files[@]} -gt 0 ]];then
        arr1=()
        for file in "${files[@]}";do
            arr1+=("$(basename "$file")")
        done
        for file in "${arr1[@]}"; do
            extension="${file##*.}"
            new_filename="${num}_ffmpegscale${check_num}.${extension}"
            sudo mv "$file" "$new_filename"

            md5_src=$(md5sum "$new_filename" | awk '{ print $1 }')
            md5_ref=$(grep "$new_filename" ${ref_dest}/${ref_dir}/${ref_md5_filename} | awk '{print $1}' | head -n 1)

            if [ "$md5_src" == "$md5_ref" ]
            then
                echo "success"
            else
                echo "fail case is $num"
                echo ${line} >> ffmpegscalediff.txt
            fi
            rm -rf ${new_filename}
            check_num=$((check_num+1))
        done
    else
        echo "no out file to compare"
    fi
}

function test_start(){
    num=$1
    line=$2

    timeout ${timeout_duration} ${test_ffmpeg_scale_case[num]}
    exit_code=$?
    sleep 1
    if [ $exit_code -eq 0 ]; then
        compare "$line" "$num"
    else
        if [ $exit_code -eq 124 ]; then
            echo "Command timed out.$line"
            echo ${line} >> ffmpegscalefail.txt
        else
            echo "Command encountered an error.$line"
            echo ${line} >> ffmpegscalefail.txt
            printf "\n" >> ffmpegscalefail.txt
        fi
    fi
}

function test_all_case(){

    if [ -f "ffmpegscalefail.txt" ]; then
         rm -rf ffmpegscalefail.txt
    fi
    if [ -f "ffmpegscalediff.txt" ]; then
         rm -rf ffmpegscalediff.txt
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

    scp_file ${install_dir}/${ref_dir}/ffmpegscale/${ref_md5_filename}  $install_name  $install_ip   ${ref_dest}/${ref_dir}    $install_pass 1

    for ((i=0; i<${#test_ffmpeg_scale_case[@]}; i++))
    do
        scp_file ${install_dir}/all_data_opencv_ffmpeg/${test_data[i]}  $install_name  $install_ip   $input_dest   $install_pass 1
        test_start "$i" "${test_ffmpeg_scale_case[i]}"
        rm -rf ${input_dest}/${test_data[i]}

    done
    sudo rm -rf ${ref_dest}/${ref_dir}
}

test_all_case