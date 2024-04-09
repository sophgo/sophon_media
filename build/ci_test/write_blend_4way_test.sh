# /bin/bash
# README:
#       wtite test statement into temp test script
function writeBlend4way(){
    local nfs_path=$1
    local MODE=$2
    local TAIL_MARK=$3
    if [ $MODE = pcie ];then
        :
    elif [ $MODE = soc ];then
		cat <<-EOF > $nfs_path/tmp_daily.sh
		for((i=1; i<=$try_n_times; i++))
		do
		    ./bmcv/test_4way_blending -N 2 -a ./stream/1920x1080.yuv -b ./stream/1920x1080.yuv -e 1920 -f 1080 -g 0 -h 2way-3840x1080.yuv420p -i 3840 -j 1080 -k 0 -l 1920 -m 1919 -z ./stream/c01_result_420p_c2_3840x1080_none_ovlp.yuv 2>&1 | tee blend_4way_log.txt
		    cmd_status=\${PIPESTATUS[0]}
		    if [ \$cmd_status -ne 0 ]; then
		        exit \$cmd_status
		    fi
		    grep "time_avg =" blend_4way_log.txt | awk '{print \$6}' | cut -d ',' -f1 >> tmp_n_time_blend.txt
		    rm -f blend_4way_log.txt
		done

		md5sum 2way-3840x1080.yuv420p > md5_blend_4way.txt
		rm -f 2way-3840x1080.yuv420p
		cat tmp_n_time_blend.txt | awk '{sum+=\$1} END {printf "%d\n", sum / $try_n_times}' > blend_4way_$MODE$TAIL_MARK
		rm -f tmp_n_time_blend.txt
		EOF
    fi
}
