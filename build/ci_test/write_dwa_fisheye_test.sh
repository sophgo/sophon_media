# /bin/bash
# README:
#       wtite test statement into temp test script
function writeDwaFisheye(){
    local nfs_path=$1
    local MODE=$2
    local TAIL_MARK=$3
    if [ $MODE = pcie ];then
        :
    elif [ $MODE = soc ];then
		cat <<-EOF > $nfs_path/tmp_daily.sh
		for((i=1; i<=$try_n_times; i++))
		do
		    ./bmcv/test_dwa_fisheye_thread 1024 1024 1280 720 0 ./stream/fisheye_floor_1024x1024.yuv out_fisheye_PANORAMA_360.yuv 1 1 0 128 128 512 512 0 0 0 1 0 1 1 1 2>&1 | tee dwa_fisheye_log.txt
		    cmd_status=\${PIPESTATUS[0]}
		    if [ \$cmd_status -ne 0 ]; then
		        exit \$cmd_status
		    fi
		    grep "time_avg =" dwa_fisheye_log.txt | awk '{print \$10}' | cut -d ',' -f1 >> tmp_n_time_dwa_fisheye.txt
		    rm -f dwa_fisheye_log.txt
		done

		md5sum out_fisheye_PANORAMA_360.yuv > md5_dwa_fisheye.txt
		rm -f out_fisheye_PANORAMA_360.yuv
		cat tmp_n_time_dwa_fisheye.txt | awk '{sum+=\$1; mean=sum / $try_n_times} END {printf "%d\n", mean}' > dwa_fisheye_$MODE$TAIL_MARK
		rm -f tmp_n_time_dwa_fisheye.txt
		EOF
    fi
}
