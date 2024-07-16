
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
"1024test3_422.jpg"
"200test1_422.yuv"
"100test1_444.jpg"
"200test1_444.yuv"
"JPEG_1920x1088_yuv420_planar.jpg"
"JPEG_1920x1088_yuv420_planar.yuv"
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

test_gst_jpeg_case=(
"gst-launch-1.0 filesrc location=640x480_420p.yuv ! rawvideoparse format=i420 width=640 height=480 ! bmjpegenc ! filesink location=case0.jpg"
"gst-launch-1.0 filesrc location=JPEG_1920x1088_yuv444_planar.yuv ! rawvideoparse format=y444 width=1920 height=1088 ! bmjpegenc ! filesink location=case1.jpg"
"gst-launch-1.0 filesrc location=JPEG_1920x1088_yuv420_planar.yuv ! rawvideoparse format=i420 width=1920 height=1088 ! bmjpegenc ! filesink location=case2.jpg"
"gst-launch-1.0 filesrc location=640x480_444p.yuv ! rawvideoparse format=y444 width=640 height=480 ! bmjpegenc ! avimux ! filesink location=case3.avi"
"gst-launch-1.0 filesrc location=640x480_422p.yuv ! rawvideoparse format=y42b width=640 height=480 ! bmjpegenc ! avimux ! filesink location=case4.avi"
"gst-launch-1.0 filesrc location=640x480_420p.yuv ! rawvideoparse format=i420 width=640 height=480 ! bmjpegenc ! avimux ! filesink location=case5.avi"
"gst-launch-1.0 filesrc location=1920x1080_444p.yuv ! rawvideoparse format=y444 width=1920 height=1080 ! bmjpegenc ! avimux ! filesink location=case6.avi"
"gst-launch-1.0 filesrc location=1920x1080_422p.yuv ! rawvideoparse format=y42b width=1920 height=1080 ! bmjpegenc ! avimux ! filesink location=case7.avi"
"gst-launch-1.0 filesrc location=1920x1088_420p.yuv ! rawvideoparse format=i420 width=1920 height=1080 ! bmjpegenc ! avimux ! filesink location=case8.avi"
"gst-launch-1.0 filesrc location=4k_444p_20_frames.yuv ! rawvideoparse format=y444 width=4000 height=4000 ! bmjpegenc ! avimux ! filesink location=case9.avi"
"gst-launch-1.0 filesrc location=4k_422p_20_frames.yuv ! rawvideoparse format=y42b width=4000 height=4000 ! bmjpegenc ! avimux ! filesink location=case10.avi"
"gst-launch-1.0 filesrc location=4k_420p_20_frames.yuv ! rawvideoparse format=i420 width=4000 height=4000 ! bmjpegenc ! avimux ! filesink location=case11.avi"
"gst-launch-1.0 filesrc location=JPEG_1920x1088_yuv420_planar.jpg ! jpegparse ! bmdec ! filesink location=case12.yuv"
"gst-launch-1.0 filesrc location=1024test3_422.jpg ! jpegparse ! bmdec ! filesink location=case13.yuv"
"gst-launch-1.0 filesrc location=200test1_422.yuv ! rawvideoparse format=y42b width=200 height=200 ! bmjpegenc ! filesink location=case14.jpg"
"gst-launch-1.0 filesrc location=100test1_444.jpg ! jpegparse ! bmdec ! filesink location=case15.yuv"
"gst-launch-1.0 filesrc location=200test1_444.yuv ! rawvideoparse format=y444 width=200 height=200 ! bmjpegenc ! filesink location=case16.jpg"
"gst-launch-1.0 filesrc location=JPEG_1920x1088_yuv420_planar.jpg ! jpegparse ! bmdec ! filesink location=case17.yuv"
"gst-launch-1.0 filesrc location=JPEG_1920x1088_yuv420_planar.yuv ! rawvideoparse format=i420 width=1920 height=1088 ! bmjpegenc ! filesink location=case18.jpg"
"gst-launch-1.0 filesrc location=JPEG_1920x1088_yuv420_planar.jpg ! jpegparse ! bmdec ! filesink location=case19.yuv" #bs_buffer_size=3072
"gst-launch-1.0 filesrc location=JPEG_1920x1088_yuv420_planar.yuv ! rawvideoparse format=i420 width=1920 height=1088 ! bmjpegenc q-factor=95 ! filesink location=case20.jpg"
"gst-launch-1.0 filesrc location=640x480_444p.mov ! qtdemux ! jpegparse ! bmdec ! bmjpegenc ! avimux ! filesink location=case21.avi"
"gst-launch-1.0 filesrc location=640x480_422p.mov  ! qtdemux ! jpegparse ! bmdec ! bmjpegenc ! avimux ! filesink location=case22.avi"
"gst-launch-1.0 filesrc location=640x480_420p.mov  ! qtdemux ! jpegparse ! bmdec ! bmjpegenc ! avimux ! filesink location=case23.avi"
"gst-launch-1.0 filesrc location=1920x1080_444p.mov ! qtdemux ! jpegparse ! bmdec ! bmjpegenc ! avimux ! filesink location=case24.avi"
"gst-launch-1.0 filesrc location=1920x1080_422p.mov ! qtdemux ! jpegparse ! bmdec ! bmjpegenc ! avimux ! filesink location=case25.avi"
"gst-launch-1.0 filesrc location=1920x1088_420p.mov ! qtdemux ! jpegparse ! bmdec ! bmjpegenc ! avimux ! filesink location=case26.avi"
"gst-launch-1.0 filesrc location=4k_444p_200_frames.mov ! qtdemux ! jpegparse ! bmdec ! bmjpegenc ! avimux ! filesink location=case27.avi"
"gst-launch-1.0 filesrc location=4k_422p_200_frames.mov ! qtdemux ! jpegparse ! bmdec ! bmjpegenc ! avimux ! filesink location=case28.avi"
"gst-launch-1.0 filesrc location=4k_420p_200_frames.mov ! qtdemux ! jpegparse ! bmdec ! bmjpegenc ! avimux ! filesink location=case29.avi"
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
ref_md5_filename=gstjpeg_ref_md5.txt
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

    ${test_gst_jpeg_case[num]}
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

    timeout ${timeout_duration} ${test_gst_jpeg_case[num]}
    exit_code=$?
    sleep 1
    if [ $exit_code -eq 0 ]; then
        compare "$line" "$num"

    else
        if [ $exit_code -eq 124 ]; then
            echo "Command timed out.$line"
            echo ${line} >> gstjpegfail.txt
        else
            echo "Command encountered an error.$line"
            echo ${line} >> gstjpegfail.txt
            printf "\n" >> gstjpegfail.txt
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

    if [ -f "gstjpegfail.txt" ]; then
         rm -rf gstjpegfail.txt
    fi
    if [ -f "gstjpegfail.txt" ]; then
         rm -rf gstjpegfail.txt
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

    for ((i=0; i<${#test_gst_jpeg_case[@]}; i++))
    do
        scp_file ${install_dir}/all_data_opencv_ffmpeg/${test_data[i]}  $install_name  $install_ip   $input_dest   $install_pass 1
        if [ ${action} -eq 0 ]; then
            # run case
            test_start "$i" "${test_gst_jpeg_case[i]}"
        else
            # make golden file
            make_golden ${i}
        fi

       #rm -rf ${input_dest}/${test_data[i]}

    done
    sudo rm -rf ${ref_dest}/${ref_dir}
}

test_all_case
