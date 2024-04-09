
pic_folder="car"
resolutions="3840x2160 1920x1080 1280x720 854x480 640x360 426x240 256x144"
pix_fmts1="yuvj444 yuvj422 yuvj420 rgb24 gray"

echo > ${pic_folder}.log
for size in ${resolutions};
do
    echo "size: ${size}" >> ${pic_folder}.log
    for fmt in ${pix_fmts1};
    do
        echo "pixel format: ${fmt}" >> ${pic_folder}.log
        echo "jpubasic src/${pic_folder}/${size}_${fmt}.jpg 5" >> ${pic_folder}.log
        jpubasic src/${pic_folder}/${size}_${fmt}.jpg 5 >> ${pic_folder}.log
        echo >> ${pic_folder}.log
        echo >> ${pic_folder}.log
        echo >> ${pic_folder}.log
    done
done
