# /bin/bash
# README:
#       wtite test statement into temp test script
function writeTpuAddWeight(){
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
		    ./bmcv/test_cv_add_weight 1 1 1 1080 1920 10 0.5 0.5 10 ./stream/1920x1080_rgb.bin ./stream/1920x1080_rgb.bin out_add_weight.bin 2>&1 | tee cv_add_weight_log.txt
		    cmd_status=\${PIPESTATUS[0]}
		    if [ \$cmd_status -ne 0 ]; then
		        exit \$cmd_status
		    fi
		    awk '/add_weighted TPU using time/ {split(\$6, a, "("); print a[1]}' cv_add_weight_log.txt >> tmp_n_time.txt
		    rm -f cv_add_weight_log.txt
		done
		md5sum out_add_weight.bin > md5_tpu_add_weight.txt
		rm -f out_add_weight.bin
		cat tmp_n_time.txt | awk '{sum+=\$1; mean=sum/$try_n_times} END {printf "%d\n", mean}' > tpu_add_weight_$MODE$TAIL_MARK
		rm -f tmp_n_time.txt
		EOF
    fi
}
