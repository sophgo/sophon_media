# /bin/bash
# README:
#       wtite test statement into temp test script
function writeIveStcandicorner(){
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
		    ./bmcv/test_ive_stcandicorner_thread 640 480 25 ./stream/sky_640x480.yuv ./stream/1686/sample_tile_Shitomasi_sky_640x480.yuv 0 1 1 0 2>&1 | tee ive_stcandicorner_log.txt
		    cmd_status=\${PIPESTATUS[0]}
		    if [ \$cmd_status -ne 0 ]; then
		        exit \$cmd_status
		    fi
		    grep "time_avg =" ive_stcandicorner_log.txt | awk '{print \$11}' | cut -d ',' -f1 >> tmp_n_time_ive.txt
		    rm -f ive_stcandicorner_log.txt
		done

		cat tmp_n_time_ive.txt | awk '{sum+=\$1} END {printf "%d\n", sum / $try_n_times}' > ive_stcandicorner_$MODE$TAIL_MARK
		rm -f tmp_n_time_ive.txt
		EOF
    fi
}
