
test_data=(
"640x480_420p.yuv"
"JPEG_1920x1088_yuv444_planar.yuv"
"JPEG_1920x1088_yuv420_planar.yuv"
"640x480_444p.yuv"
"640x480_422p.yuv"
"640x480_420p.yuv"
"1920x1080_444p.yuv"
"1920x1080_422p.yuv"
"1920x1088_420p.yuv"
"4k_444p_20_frames.yuv"
"4k_422p_20_frames.yuv"
"4k_420p_20_frames.yuv"
"JPEG_1920x1088_yuv420_planar.jpg"
"100test1_420.yuv"
"1024test3_422.jpg"
"200test1_422.yuv"
"100test1_444.jpg"
"200test1_444.yuv"
"JPEG_1920x1088_yuv420_planar.jpg"
"JPEG_1920x1088_yuv420_planar.yuv"
"station.avi"
"JPEG_1920x1088_yuv420_planar.jpg"
"JPEG_1920x1088_yuv420_planar.yuv"
"640x480_444p.mov"
"640x480_422p.mov"
"640x480_420p.mov"
"1920x1080_444p.mov"
"1920x1080_422p.mov"
"1920x1088_420p.mov"
"4k_444p_200_frames.mov"
"4k_422p_200_frames.mov"
"4k_420p_200_frames.mov"
"JPEG_1920x1088_yuv420_planar.jpg"
"JPEG_1920x1088_yuv420_planar.jpg"
"JPEG_16x16_yuv420_planar.jpg JPEG_720x480_yuv422_planar.jpg"
)

test_ffmpeg_jpeg_case=(
"ffmpeg -init_hw_device bmcodec=foo:0 -s 640x480 -pix_fmt gray -i 640x480_420p.yuv -filter_hw_device foo -vf "format=gray\|bmcodec,hwupload" -c:v jpeg_bm -vframes 1 -y test_ffmpeg_jpeg_out1.jpg"
"ffmpeg -init_hw_device bmcodec=foo:1 -s 1920x1088 -pix_fmt yuvj444p -i JPEG_1920x1088_yuv444_planar.yuv -filter_hw_device foo -vf "format=yuvj444p\|bmcodec,hwupload" -c:v jpeg_bm -y test_ffmpeg_jpeg_out2.jpg"
"ffmpeg -init_hw_device bmcodec=foo:1 -s 1920x1088 -pix_fmt yuvj420p -i JPEG_1920x1088_yuv420_planar.yuv -filter_hw_device foo -vf "format=yuvj420p\|bmcodec,hwupload" -c:v jpeg_bm -y test_ffmpeg_jpeg_out3.jpg"
"ffmpeg -init_hw_device bmcodec=foo:1 -s 640x480 -pix_fmt yuvj444p -i 640x480_444p.yuv -filter_hw_device foo -vf "format=yuvj444p\|bmcodec,hwupload" -c:v jpeg_bm -y test_ffmpeg_jpeg_out4.mov"
"ffmpeg -init_hw_device bmcodec=foo:1 -s 640x480 -pix_fmt yuvj422p -i 640x480_422p.yuv -filter_hw_device foo -vf "format=yuvj422p\|bmcodec,hwupload" -c:v jpeg_bm -y test_ffmpeg_jpeg_out5.mov"
"ffmpeg -init_hw_device bmcodec=foo:1 -s 640x480 -pix_fmt yuvj420p -i 640x480_420p.yuv -filter_hw_device foo -vf "format=yuvj420p\|bmcodec,hwupload" -c:v jpeg_bm -y test_ffmpeg_jpeg_out6.mov"
"ffmpeg -init_hw_device bmcodec=foo:1 -s 1920x1080 -pix_fmt yuvj444p -i 1920x1080_444p.yuv -filter_hw_device foo -vf "format=yuvj444p\|bmcodec,hwupload" -c:v jpeg_bm -y test_ffmpeg_jpeg_out7.mov"
"ffmpeg -init_hw_device bmcodec=foo:1 -s 1920x1080 -pix_fmt yuvj422p -i 1920x1080_422p.yuv -filter_hw_device foo -vf "format=yuvj422p\|bmcodec,hwupload" -c:v jpeg_bm -y test_ffmpeg_jpeg_out8.mov"
"ffmpeg -init_hw_device bmcodec=foo:1 -s 1920x1088 -pix_fmt yuvj420p -i 1920x1088_420p.yuv -filter_hw_device foo -vf "format=yuvj420p\|bmcodec,hwupload" -c:v jpeg_bm -y test_ffmpeg_jpeg_out9.mov"
"ffmpeg -init_hw_device bmcodec=foo:1 -s 4000x4000 -pix_fmt yuvj444p -i 4k_444p_20_frames.yuv -filter_hw_device foo -vf "format=yuvj444p\|bmcodec,hwupload" -c:v jpeg_bm -y test_ffmpeg_jpeg_out10.mov"
"ffmpeg -init_hw_device bmcodec=foo:1 -s 4000x4000 -pix_fmt yuvj422p -i 4k_422p_20_frames.yuv -filter_hw_device foo -vf "format=yuvj422p\|bmcodec,hwupload" -c:v jpeg_bm -y test_ffmpeg_jpeg_out11.mov"
"ffmpeg -init_hw_device bmcodec=foo:1 -s 4000x4000 -pix_fmt yuvj420p -i 4k_420p_20_frames.yuv -filter_hw_device foo -vf "format=yuvj420p\|bmcodec,hwupload" -c:v jpeg_bm -y test_ffmpeg_jpeg_out12.mov"
"ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v jpeg_bm -i JPEG_1920x1088_yuv420_planar.jpg -vf "hwdownload,format=yuvj420p\|bmcodec" -y test_ffmpeg_jpeg_out13.yuv"
"ffmpeg -init_hw_device bmcodec=foo:0 -s 100x100 -pix_fmt yuvj420p -i 100test1_420.yuv -filter_hw_device foo -vf "format=yuvj420p\|bmcodec,hwupload" -c:v jpeg_bm -y test_ffmpeg_jpeg_out14.jpg"
"ffmpeg -hwaccel bmcodec -hwaccel_device 1 -c:v jpeg_bm -i 1024test3_422.jpg -vf "hwdownload,format=yuvj422p\|bmcodec" -y test_ffmpeg_jpeg_out15.yuv"
"ffmpeg -init_hw_device bmcodec=foo:0 -s 200x200 -pix_fmt yuvj422p -i 200test1_422.yuv -filter_hw_device foo -vf "format=yuvj422p\|bmcodec,hwupload" -c:v jpeg_bm -y test_ffmpeg_jpeg_out16.jpg"
"ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v jpeg_bm -i 100test1_444.jpg -vf "hwdownload,format=yuvj444p\|bmcodec" -y test_ffmpeg_jpeg_out17.yuv"
"ffmpeg -init_hw_device bmcodec=foo:0 -s 200x200 -pix_fmt yuvj444p -i 200test1_444.yuv -filter_hw_device foo -vf "format=yuvj444p\|bmcodec,hwupload" -c:v jpeg_bm -y test_ffmpeg_jpeg_out18.jpg"
"ffmpeg -c:v jpeg_bm -i JPEG_1920x1088_yuv420_planar.jpg -s 1920x1088 -y test_ffmpeg_jpeg_out19.yuv"
"ffmpeg -s 1920x1088 -pix_fmt yuvj420p -i JPEG_1920x1088_yuv420_planar.yuv -c:v jpeg_bm -is_dma_buffer 0 -y test_ffmpeg_jpeg_out20.jpg"
"ffmpeg -i station.avi -vf fps=1/60 -vframes 1 -y test_ffmpeg_jpeg_out21.jpg"
"ffmpeg -c:v jpeg_bm -bs_buffer_size 3072 -i JPEG_1920x1088_yuv420_planar.jpg -y test_ffmpeg_jpeg_out22.yuv"
"ffmpeg -s 1920x1088 -pix_fmt yuvj420p -i JPEG_1920x1088_yuv420_planar.yuv -c:v jpeg_bm -is_dma_buffer 0 -qf 95 -y test_ffmpeg_jpeg_out23.jpg"
"ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v jpeg_bm -i 640x480_444p.mov -c:v jpeg_bm -y test_ffmpeg_jpeg_out24.mov"
"ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v jpeg_bm -i 640x480_422p.mov -c:v jpeg_bm -y test_ffmpeg_jpeg_out25.mov"
"ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v jpeg_bm -i 640x480_420p.mov -c:v jpeg_bm -y test_ffmpeg_jpeg_out26.mov"
"ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v jpeg_bm -i 1920x1080_444p.mov -c:v jpeg_bm -y test_ffmpeg_jpeg_out27.mov"
"ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v jpeg_bm -i 1920x1080_422p.mov -c:v jpeg_bm -y test_ffmpeg_jpeg_out28.mov"
"ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v jpeg_bm -i 1920x1088_420p.mov -c:v jpeg_bm -y test_ffmpeg_jpeg_out29.mov"
"ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v jpeg_bm -i 4k_444p_200_frames.mov -c:v jpeg_bm -y test_ffmpeg_jpeg_out30.mov"
"ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v jpeg_bm -i 4k_422p_200_frames.mov -c:v jpeg_bm -y test_ffmpeg_jpeg_out31.mov"
"ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v jpeg_bm -i 4k_420p_200_frames.mov -c:v jpeg_bm -y test_ffmpeg_jpeg_out32.mov"
# "test_ff_bmjpeg 1 JPEG_1920x1088_yuv420_planar.jpg"
# "test_ff_bmjpeg 10 JPEG_1920x1088_yuv420_planar.jpg"
# "test_ff_bmjpeg_dec_recycle 655360 3133440 2 JPEG_16x16_yuv420_planar.jpg JPEG_720x480_yuv422_planar.jpg"

)

install_name=dailytest
install_ip=172.28.141.219
install_pass=dailytest
ref_dest=/data

input_dest="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

timeout_duration=600

install_dir="/home/dailytest/Athena2/Multimedia"
ref_dir=jpeg
check_num=0
ref_md5_filename=ffmpegjpeg_ref_md5.txt


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

    files=($(find . -maxdepth 1 -type f \( -name "output*.jpg" -o -name "test_ffmpeg_jpeg*" -o -name "output*.yuv" -o -name "recycle-output*.yuv" \)))
    if [[ ${#files[@]} -gt 0 ]];then
        arr1=()
        for file in "${files[@]}";do
            arr1+=("$(basename "$file")")
        done

        for file in "${arr1[@]}"; do
            extension="${file##*.}"
            new_filename="${num}_ffmpegjpeg${check_num}.${extension}"
            sudo mv "$file" "$new_filename"

            md5_src=$(md5sum "$new_filename" | awk '{ print $1 }')
            md5_ref=$(grep "$new_filename" ${ref_dest}/${ref_dir}/${ref_md5_filename} | awk '{print $1}' | head -n 1)

            if [ "$md5_src" == "$md5_ref" ]
            then
                echo "success"
            else
                echo "fail case is $num"
                echo ${line} >> ffmpegjpegdiff.txt
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

    timeout ${timeout_duration} ${test_ffmpeg_jpeg_case[num]}
    exit_code=$?
    sleep 1
    if [ $exit_code -eq 0 ]; then
        compare "$line" "$num"

    else
        if [ $exit_code -eq 124 ]; then
            echo "Command timed out.$line"
            echo ${line} >> ffmpegjpegfail.txt
        else
            echo "Command encountered an error.$line"
            echo ${line} >> ffmpegjpegfail.txt
            printf "\n" >> ffmpegjpegfail.txt
        fi
    fi
}

function test_all_case(){

    if [ -f "ffmpegjpegfail.txt" ]; then
         rm -rf ffmpegjpegfail.txt
    fi
    if [ -f "ffmpegjpegdiff.txt" ]; then
         rm -rf ffmpegjpegdiff.txt
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

    scp_file ${install_dir}/${ref_dir}/ffmpegjpeg/${ref_md5_filename}  $install_name  $install_ip   ${ref_dest}/${ref_dir}    $install_pass 1

    for ((i=0; i<${#test_ffmpeg_jpeg_case[@]}; i++))
    do
        scp_file ${install_dir}/all_data_opencv_ffmpeg/${test_data[i]}  $install_name  $install_ip   $input_dest   $install_pass 1
        test_start "$i" "${test_ffmpeg_jpeg_case[i]}"
        rm -rf ${input_dest}/${test_data[i]}

    done
    sudo rm -rf ${ref_dest}/${ref_dir}
}

test_all_case
