test_data=(
"1920x1080_444p.yuv"
"1920x1080_422p.yuv"
"1920x1088_420p.yuv"
"640x480_444p.yuv"
"640x480_422p.yuv"
"640x480_420p.yuv"
"1920x1080_444p.yuv"
"1920x1080_422p.yuv"
"1920x1088_420p.yuv"
"4k_444p_20_frames.yuv"
"4k_422p_20_frames.yuv"
"4k_420p_20_frames.yuv"
"200test1_422.yuv"
"100test1_420.jpg"
"100test1_420.yuv"
"1024test3_422.jpg"
"100test1_444.jpg"
"200test1_444.yuv"
"CBR2925_h265.mp4"
"1080p_yuv420p.yuv"
"1080p_yuv420p.yuv"
"1080p_yuv420p.yuv"
"1080p_yuv420p.yuv"
"1080p_yuv420p.yuv"
"1920x1080.mp4"
"1920x1080.mp4"
"640x480_444p.mov"
"640x480_422p.mov"
"640x480_420p.mov"
"1920x1080_444p.mov"
"1920x1080_422p.mov"
"1920x1080_420p.mov"
"4k_444p_200_frames.mov"
"4k_422p_200_frames.mov"
"4k_420p_200_frames.mov"
"station_1080p.265"
"CBR2925_h265.mp4"
"CBR2925_h265.mp4"
"CBR2925_h265.mp4"
"CBR2925_h265.mp4"
"CBR2925_h265.mp4"
"CBR2925_h265.mp4"
"CBR2925_h265.mp4"
"CBR2925_h265.mp4"
"1080p_yuv420p.yuv"
"1920x1080.mp4"
"1920x1080.mp4"
"1920x1080_444p.yuv"
"1920x1080_422p.yuv"
)

test_ffmpeg_hw_case=(
"ffmpeg -init_hw_device bmcodec=foo:1 -s 1920x1080 -pix_fmt yuvj444p -i 1920x1080_444p.yuv -filter_hw_device foo -vf "format=yuvj444p\|bmcodec,hwupload" -c:v jpeg_bm -vframes 1 -y test_ffmpeg_hw_out2.jpg"
"ffmpeg -init_hw_device bmcodec=foo:1 -s 1920x1080 -pix_fmt yuvj444p -i 1920x1080_422p.yuv -filter_hw_device foo -vf "format=yuvj422p\|bmcodec,hwupload" -c:v jpeg_bm -vframes 1 -y test_ffmpeg_hw_out3.jpg"
"ffmpeg -init_hw_device bmcodec=foo:1 -s 1920x1088 -pix_fmt yuvj420p -i 1920x1088_420p.yuv -filter_hw_device foo -vf "format=yuvj420p\|bmcodec,hwupload" -c:v jpeg_bm -vframes 1 -y test_ffmpeg_hw_out4.jpg"
"ffmpeg -init_hw_device bmcodec=foo:1 -s 640x480 -pix_fmt yuvj444p -i 640x480_444p.yuv -filter_hw_device foo -vf "format=yuvj444p\|bmcodec,hwupload" -c:v jpeg_bm -y test_ffmpeg_hw_out5.mov"
"ffmpeg -init_hw_device bmcodec=foo:1 -s 640x480 -pix_fmt yuvj422p -i 640x480_422p.yuv -filter_hw_device foo -vf "format=yuvj422p\|bmcodec,hwupload" -c:v jpeg_bm -y test_ffmpeg_hw_out6.mov"
"ffmpeg -init_hw_device bmcodec=foo:1 -s 640x480 -pix_fmt yuvj420p -i 640x480_420p.yuv -filter_hw_device foo -vf "format=yuvj420p\|bmcodec,hwupload" -c:v jpeg_bm -y test_ffmpeg_hw_out7.mov"
"ffmpeg -init_hw_device bmcodec=foo:1 -s 1920x1080 -pix_fmt yuvj444p -i 1920x1080_444p.yuv -filter_hw_device foo -vf "format=yuvj444p\|bmcodec,hwupload" -c:v jpeg_bm -y test_ffmpeg_hw_out8.mov"
"ffmpeg -init_hw_device bmcodec=foo:1 -s 1920x1080 -pix_fmt yuvj422p -i 1920x1080_422p.yuv -filter_hw_device foo -vf "format=yuvj422p\|bmcodec,hwupload" -c:v jpeg_bm -y test_ffmpeg_hw_out9.mov"
"ffmpeg -init_hw_device bmcodec=foo:1 -s 1920x1088 -pix_fmt yuvj420p -i 1920x1088_420p.yuv -filter_hw_device foo -vf "format=yuvj420p\|bmcodec,hwupload" -c:v jpeg_bm -y test_ffmpeg_hw_out10.mov"
"ffmpeg -init_hw_device bmcodec=foo:1 -s 4000x4000 -pix_fmt yuvj444p -i 4k_444p_20_frames.yuv -filter_hw_device foo -vf "format=yuvj444p\|bmcodec,hwupload" -c:v jpeg_bm -y test_ffmpeg_hw_out11.mov"
"ffmpeg -init_hw_device bmcodec=foo:1 -s 4000x4000 -pix_fmt yuvj422p -i 4k_422p_20_frames.yuv -filter_hw_device foo -vf "format=yuvj422p\|bmcodec,hwupload" -c:v jpeg_bm -y test_ffmpeg_hw_out12.mov"
"ffmpeg -init_hw_device bmcodec=foo:1 -s 4000x4000 -pix_fmt yuvj420p -i 4k_420p_20_frames.yuv -filter_hw_device foo -vf "format=yuvj420p\|bmcodec,hwupload" -c:v jpeg_bm -y test_ffmpeg_hw_out13.mov"
"ffmpeg -init_hw_device bmcodec=foo:0 -s 200x200 -pix_fmt yuvj422p -i 200test1_422.yuv -filter_hw_device foo -vf "format=yuvj422p\|bmcodec,hwupload" -c:v jpeg_bm -y test_ffmpeg_hw_out23.jpg"
"ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v jpeg_bm -i 100test1_420.jpg -vf "hwdownload,format=yuvj420p\|bmcodec" -y test_ffmpeg_hw_out25.yuv"
"ffmpeg -init_hw_device bmcodec=foo:0 -s 100x100 -pix_fmt yuvj420p -i 100test1_420.yuv -filter_hw_device foo -vf "format=yuvj420p\|bmcodec,hwupload" -c:v jpeg_bm -y test_ffmpeg_hw_out26.jpg"
"ffmpeg -hwaccel bmcodec -hwaccel_device 1 -c:v jpeg_bm -i 1024test3_422.jpg -vf "hwdownload,format=yuvj422p\|bmcodec" -y test_ffmpeg_hw_out27.yuv"
"ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v jpeg_bm -i 100test1_444.jpg -vf "hwdownload,format=yuvj444p\|bmcodec" -y test_ffmpeg_hw_out28.yuv"
"ffmpeg -init_hw_device bmcodec=foo:0 -s 200x200 -pix_fmt yuvj444p -i 200test1_444.yuv -filter_hw_device foo -vf "format=yuvj444p\|bmcodec,hwupload" -c:v jpeg_bm -y test_ffmpeg_hw_out29.jpg"
"ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v hevc_bm -cbcr_interleave 0 -i CBR2925_h265.mp4 -filter_complex "[0:v]scale_bm=352:288[img1]\;[0:v]scale_bm=1280:720:format=yuvj420p[img2]" -map "[img1]" -c:v h264_bm -b:v 256K -y img1.mp4 -map "[img2]" -c:v jpeg_bm -vframes 1 -y test_ffmpeg_hw_out35.jpeg"
"ffmpeg -init_hw_device bmcodec=foo:0 -s 1920x1080 -i 1080p_yuv420p.yuv -filter_hw_device foo -vf "format=yuv420p\|bmcodec,hwupload" -c:v h264_bm -gop_preset 1 -g 32 -b:v 32M -y test_ffmpeg_hw_out36.mp4"
"ffmpeg -init_hw_device bmcodec=foo:0 -s 1920x1080 -i 1080p_yuv420p.yuv -filter_hw_device foo -vf "format=yuv420p\|bmcodec,hwupload" -c:v h264_bm -gop_preset 7 -g 32 -b:v 6M test_ffmpeg_hw_out37.mp4"
"ffmpeg -init_hw_device bmcodec=foo:0 -s 1920x1080 -i 1080p_yuv420p.yuv -filter_hw_device foo -vf "format=yuv420p\|bmcodec,hwupload" -c:v h264_bm -gop_preset 8 -g 32 -b:v 6M test_ffmpeg_hw_out38.mp4"
"ffmpeg -init_hw_device bmcodec=foo:0 -s 1920x1080 -i 1080p_yuv420p.yuv -filter_hw_device foo -vf "format=yuv420p\|bmcodec,hwupload" -c:v h265_bm -gop_preset 7 -g 32 -b:v 3M test_ffmpeg_hw_out40.mp4"
"ffmpeg -init_hw_device bmcodec=foo:0 -s 1920x1080 -i 1080p_yuv420p.yuv -filter_hw_device foo -vf "format=yuv420p\|bmcodec,hwupload" -c:v h265_bm -gop_preset 8 -g 32 -b:v 3M test_ffmpeg_hw_out41.mp4"
"ffmpeg -init_hw_device bmcodec=foo:0 -c:v h264 -i 1920x1080.mp4 -filter_hw_device foo -vf "format=yuv420p\|bmcodec,hwupload" -c:v h264_bm -perf 1 -gop_preset 8 -g 32 -b:v 10M -y test_ffmpeg_hw_out44.mp4"
"ffmpeg -init_hw_device bmcodec=foo:0 -c:v h264 -i 1920x1080.mp4 -filter_hw_device foo -vf "format=yuv420p\|bmcodec,hwupload" -c:v h265_bm -perf 1 -gop_preset 8 -g 32 -b:v 5M -y test_ffmpeg_hw_out45.mp4"
"ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v jpeg_bm -i 640x480_444p.mov -c:v jpeg_bm -y test_ffmpeg_hw_out14.mov"
"ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v jpeg_bm -i 640x480_422p.mov -c:v jpeg_bm -y test_ffmpeg_hw_out15.mov"
"ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v jpeg_bm -i 640x480_420p.mov -c:v jpeg_bm -y test_ffmpeg_hw_out16.mov"
"ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v jpeg_bm -i 1920x1080_444p.mov -c:v jpeg_bm -y test_ffmpeg_hw_out17.mov"
"ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v jpeg_bm -i 1920x1080_422p.mov -c:v jpeg_bm -y test_ffmpeg_hw_out18.mov"
"ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v jpeg_bm -i 1920x1088_420p.mov -c:v jpeg_bm -y test_ffmpeg_hw_out19.mov"
"ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v jpeg_bm -i 4k_444p_200_frames.mov -c:v jpeg_bm -y test_ffmpeg_hw_out20.mov"
"ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v jpeg_bm -i 4k_422p_200_frames.mov -c:v jpeg_bm -y test_ffmpeg_hw_out21.mov"
"ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v jpeg_bm -i 4k_420p_200_frames.mov -c:v jpeg_bm -y test_ffmpeg_hw_out22.mov"
"ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v hevc_bm -cbcr_interleave 0 -i station_1080p.265 -vf "scale_bm=352:288" -c:v h264_bm -g 256 -b:v 256K -y test_ffmpeg_hw_out30.mp4"
"ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v hevc_bm -cbcr_interleave 0 -i CBR2925_h265.mp4 -vf "scale_bm=352:288:opt=crop" -c:v h264_bm -g 256 -b:v 256K -y test_ffmpeg_hw_out31.mp4"
"ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v hevc_bm -cbcr_interleave 0 -i CBR2925_h265.mp4 -vf "scale_bm=1280:760:opt=pad" -c:v h264_bm -g 256 -b:v 800K -y test_ffmpeg_hw_out32.mp4"
"ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v hevc_bm -cbcr_interleave 0 -i CBR2925_h265.mp4 -vf "scale_bm=352:288,fps=fps=25" -c:v h264_bm -g 256 -b:v 256K -y test_ffmpeg_hw_out33.mp4"
"ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v hevc_bm -cbcr_interleave 0 -i CBR2925_h265.mp4 -vf "scale_bm=352:288:opt=pad:format=yuvj420p" -c:v jpeg_bm -vframes 1 -y test_ffmpeg_hw_out34.jpg"
"ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v hevc_bm -cbcr_interleave 0 -i CBR2925_h265.mp4 -vf "scale_bm=352:288" -c:v h264_bm -g 256 -b:v 256K -y test_ffmpeg_hw_out49.mp4"
"ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v hevc_bm -cbcr_interleave 0 -i CBR2925_h265.mp4 -vf "scale_bm=352:288:opt=crop" -c:v h264_bm -g 256 -b:v 256K -y test_ffmpeg_hw_out50.mp4"
"ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v hevc_bm -cbcr_interleave 0 -i CBR2925_h265.mp4 -vf "scale_bm=1280:760:opt=pad" -c:v h264_bm -g 256 -b:v 800K -y test_ffmpeg_hw_out51.mp4"
"ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v hevc_bm -cbcr_interleave 0 -i CBR2925_h265.mp4 -vf "scale_bm=352:288,fps=fps=25" -c:v h264_bm -g 256 -b:v 256K -y test_ffmpeg_hw_out52.mp4"
"ffmpeg -init_hw_device bmcodec=foo:0 -s 1920x1080 -i 1080p_yuv420p.yuv -filter_hw_device foo -vf "format=yuv420p\|bmcodec,hwupload" -c:v h264_bm -gop_preset 7 -g 32 -b:v 6M test_ffmpeg_hw_out53.mp4"
"ffmpeg -c:v h264_bm -output_format 101 -i 1920x1080.mp4 -vf "scale_bm=352:288:zero_copy=1" -c:v h264_bm -g 50 -b:v 32K -enc-params "gop_preset=2:mb_rc=1:delta_qp=3:min_qp=20:max_qp=40" -y test_ffmpeg_hw_out54.264"
"ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v h264_bm -output_format 101 -i 1920x1080.mp4 -vf "scale_bm=352:288" -c:v h264_bm -g 50 -b:v 32K -enc-params "gop_preset=2:mb_rc=1:delta_qp=3:min_qp=20:max_qp=40" -y test_ffmpeg_hw_out55.264"
"ffmpeg -init_hw_device bmcodec=foo:1 -s 1920x1080 -pix_fmt yuvj444p -i 1920x1080_444p.yuv -filter_hw_device foo -vf "format=yuvj444p\|bmcodec,hwupload" -c:v jpeg_bm -vframes 1 -y test_ffmpeg_hw_out57.jpg"
"ffmpeg -init_hw_device bmcodec=foo:1 -s 1920x1080 -pix_fmt yuvj422p -i 1920x1080_422p.yuv -filter_hw_device foo -vf "format=yuvj422p\|bmcodec,hwupload" -c:v jpeg_bm -vframes 1 -y test_ffmpeg_hw_out58.jpg"
# "bmvpuenc -w 1920 -h 1080 -i 1920x1080_444p.yuv -n 10 -o out1.265"
# "test_ff_video_encode 1920x1080_444p.yuv out2.264 H264 1920 1080 0 I420"
# "ffmpeg -i CBR2925_h265.mp4 -vframes 5 out3.yuv"
# "ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v h264_bm -cbcr_interleave 0 -extra_frame_buffer_num 5 -i 1920x1080.mp4 -c:v h264_bm -perf 1 -gop_preset 7 -g 32 -b:v 6M -f null -"
# "ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v h264_bm -cbcr_interleave 0 -extra_frame_buffer_num 16 -i 1920x1080.mp4 -c:v h264_bm -perf 1 -gop_preset 8 -g 32 -b:v 6M -f null -"
# "ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v h264_bm -cbcr_interleave 0 -extra_frame_buffer_num 5 -i 1920x1080.mp4 -c:v h265_bm -perf 1 -gop_preset 1 -g 32 -b:v 20M -f null -"
# "ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v h264_bm -cbcr_interleave 0 -extra_frame_buffer_num 5 -i 1920x1080.mp4 -c:v h265_bm -perf 1 -gop_preset 7 -g 32 -b:v 3M -f null -"
# "ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v h264_bm -cbcr_interleave 0 -extra_frame_buffer_num 16 -i 1920x1080.mp4 -c:v h265_bm -perf 1 -gop_preset 8 -g 32 -b:v 3M -f null -"
# "ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v h264_bm -cbcr_interleave 0 -extra_frame_buffer_num 16 -i 1920x1080.mp4 -c:v h265_bm -perf 1 -gop_preset 8 -g 32 -b:v 3M -f null -"
# "ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v h264_bm -cbcr_interleave 0 -extra_frame_buffer_num 16 -i 1920x1080.mp4 -c:v hevc_bm -perf 1 -gop_preset 8 -g 32 -b:v 3M -f null -"
# "ffmpeg -hwaccel bmcodec -hwaccel_device 0 -c:v h264_bm -cbcr_interleave 0 -extra_frame_buffer_num 5 -i 1920x1080.mp4 -c:v h264_bm -perf 1 -gop_preset 1 -g 32 -b:v 32M -f null -"
# "ffmpeg -hwaccel bmcodec -hwaccel_device 7 -c:v h264_bm -cbcr_interleave 0 -extra_frame_buffer_num 5 -i 1920x1080.mp4 -c:v h265_bm -perf 1 -gop_preset 1 -g 32 -b:v 20M -f null -"
# "test_ff_video_encode 1080p_yuv420p.yuv test_ffmpeg_hw_out47.ts H264 1920 1080 0 I420 3000 30 2"
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
ref_md5_filename=ffmpeghw_ref_md5.txt


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

    files=($(find . -maxdepth 1 -type f \( -name "out*.yuv" -o -name "test_ffmpeg_hw*" -o -name "out*.265" -o -name "out*.264" -o -name "img*.mp4" \)))
    if [[ ${#files[@]} -gt 0 ]];then
        arr1=()
        for file in "${files[@]}";do
            arr1+=("$(basename "$file")")
        done

        for file in "${arr1[@]}"; do
            extension="${file##*.}"
            new_filename="${num}_ffmpeghw${check_num}.${extension}"
            sudo mv "$file" "$new_filename"

            md5_src=$(md5sum "$new_filename" | awk '{ print $1 }')
            md5_ref=$(grep "$new_filename" ${ref_dest}/${ref_dir}/${ref_md5_filename} | awk '{print $1}' | head -n 1)

            if [ "$md5_src" == "$md5_ref" ]
            then
                echo "success compare"
            else
                echo "fail case is $num"
                echo ${line} >> ffmpeghwdiff.txt
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

    timeout ${timeout_duration} ${test_ffmpeg_hw_case[num]}
    exit_code=$?
    sleep 1
    if [ $exit_code -eq 0 ]; then
        compare "$line" "$num"

    else
        if [ $exit_code -eq 124 ]; then
            echo "Command timed out.$line"
            echo ${line} >> ffmpeghwfail.txt
        else
            echo "Command encountered an error.$line"
            echo ${line} >> ffmpeghwfail.txt
            printf "\n" >> ffmpeghwfail.txt
        fi
    fi
}

function test_all_case(){

    if [ -f "ffmpeghwfail.txt" ]; then
         rm -rf ffmpeghwfail.txt
    fi
    if [ -f "ffmpeghwdiff.txt" ]; then
         rm -rf ffmpeghwdiff.txt
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

    scp_file ${install_dir}/${ref_dir}/ffmpeghw/${ref_md5_filename}  $install_name  $install_ip   ${ref_dest}/${ref_dir}    $install_pass 1

    for ((i=0; i<${#test_ffmpeg_hw_case[@]}; i++))
    do
        scp_file ${install_dir}/all_data_opencv_ffmpeg/${test_data[i]}  $install_name  $install_ip   $input_dest   $install_pass 1

        test_start "$i" "${test_ffmpeg_hw_case[i]}"
        rm -rf ${input_dest}/${test_data[i]}

    done
    sudo rm -rf ${ref_dest}/${ref_dir}
}

test_all_case




















