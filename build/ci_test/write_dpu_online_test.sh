# /bin/bash
# README:
#       wtite test statement into temp test script
function writeDpuOnline(){
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
			./bmcv/test_dpu_online_thread 512 284 4 ./stream/sofa_left_img_512x284.bin ./stream/sofa_right_img_512x284.bin ./stream/fgs_512x284_res.bin 0 0 1 1 2>&1 | tee dpu_online_log.txt
		    cmd_status=\${PIPESTATUS[0]}
		    if [ \$cmd_status -ne 0 ]; then
		        exit \$cmd_status
		    fi
			grep "time_avg =" dpu_online_log.txt | awk '{print \$12}' | cut -d ',' -f1 >> tmp_n_time_dpu.txt
			rm -f dpu_online_log.txt
		done

		cat tmp_n_time_dpu.txt | awk '{sum+=\$1} END {printf "%d\n", sum / $try_n_times}' > dpu_online_$MODE$TAIL_MARK
		rm -f tmp_n_time_dpu.txt
		EOF
    fi
}
