# /bin/bash
# README:
#       wtite test statement into temp test script
function writeVpssConvert(){
    local nfs_path=$1
    local MODE=$2
    local TAIL_MARK=$3
    local try_n_times=$4
    if [ $MODE = pcie ];then
        :
    elif [ $MODE = soc ];then
		cat <<-EOF > $nfs_path/tmp_daily.sh
		for((i=1; i<=$try_n_times; i++))
		do
		    ./bmcv/test_vpss_convert_thread 1920 1080 10 ./stream/1920x1080_rgb.bin 0 0 1920 1080 600 800 10 out_vpss_convert.bin 1 0 1 1 2>&1 | tee vpss_convert_log.txt
		    cmd_status=\${PIPESTATUS[0]}
		    if [ \$cmd_status -ne 0 ]; then
		        exit \$cmd_status
		    fi
		    grep "time_avg =" vpss_convert_log.txt | awk '{print \$10}' | cut -d ',' -f1 >> tmp_n_time_vpss.txt
		    rm -f vpss_convert_log.txt
		done
		md5sum out_vpss_convert.bin > md5_vpss_convert.txt
		rm -f out_vpss_convert.bin
		cat tmp_n_time_vpss.txt | awk '{sum+=\$1; mean=sum/$try_n_times} END {printf "%d\n", mean}' > vpss_convert_$MODE$TAIL_MARK
		rm -f tmp_n_time_vpss.txt
		EOF
    fi
}
