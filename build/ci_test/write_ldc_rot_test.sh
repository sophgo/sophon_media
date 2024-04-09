# /bin/bash
# README:
#       wtite test statement into temp test script
function writeLdcRot(){
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
			./bmcv/test_ldc_rot_thread ./stream/1920x1088_nv21.bin out_1920x1088_rot0.yuv 1920 1088 0 4 4 1 1 2>&1 | tee ldc_rot_log.txt
		    cmd_status=\${PIPESTATUS[0]}
		    if [ \$cmd_status -ne 0 ]; then
		        exit \$cmd_status
		    fi
			grep "time_avg =" ldc_rot_log.txt | awk '{print \$11}' | cut -d ',' -f1 >> tmp_n_time_ldc.txt
			rm -f ldc_rot_log.txt
		done

		md5sum out_1920x1088_rot0.yuv > md5_ldc_rot.txt
		rm -f out_1920x1088_rot0.yuv
		cat tmp_n_time_ldc.txt | awk '{sum+=\$1} END {printf "%d\n", sum / $try_n_times}' > ldc_rot_$MODE$TAIL_MARK
		rm -f tmp_n_time_ldc.txt
		EOF
    fi
}
