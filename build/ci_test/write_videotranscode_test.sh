# /bin/bash
# README:
#       wtite test statement into temp test script
function writeVideoTranscode(){
    local nfs_path=$1
    local MODE=$2
    local TAIL_MARK=$3
    if [ $MODE = pcie ];then
        :
    elif [ $MODE = soc ];then
		cat <<-EOF > $nfs_path/tmp_daily.sh
		rm -f fbt.txt
		rm -f fbt_fps_log.txt
		rm -f tmp_n_times.txt
		rm -f dump*.jpg
		rm -f md5_videotranscode.txt
		./bin/videotranscode ./stream/bilibiligd.264 H264enc 3000 videotranscode.h264 1 1 bitrate=1000 2>&1 | tee fbt.txt
		cmd_status=\${PIPESTATUS[0]}
		if [ \$cmd_status -ne 0 ]; then
		    exit \$cmd_status
		fi
		cat fbt.txt | grep fps > fbt_fps_log.txt
		awk '/fps/{print \$4}' fbt_fps_log.txt > tmp_n_times.txt
		awk '{sum+=\$1; count++} END{if (count > 0) printf "%d\n", sum/count; else print "No fps data found"}' tmp_n_times.txt > videotranscode_$MODE$TAIL_MARK
		md5sum videotranscode.h264 | awk '{print \$1}' > md5_videotranscode.txt
		rm -f fbt.txt
		rm -f fbt_fps_log.txt
		rm -f tmp_n_times.txt
		EOF
    fi
}
