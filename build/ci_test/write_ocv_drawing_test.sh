# /bin/bash
# README:
#       wtite test statement into temp test script
function writeOcvDrawing(){
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
		./bin/ocv-drawing ./stream/station.avi 1000 0 1 0 out.jpg line=100_100_200_200_255_0_0_1 2>&1 | tee fbt.txt
		cmd_status=\${PIPESTATUS[0]}
		if [ \$cmd_status -ne 0 ]; then
		    exit \$cmd_status
		fi
		md5sum dump_0_out.jpg > md5_ocv_drawing.txt
		rm -f dump_0_out.jpg
		cat fbt.txt | grep fps > fbt_fps_log.txt
		awk '/fps/{print \$4}' fbt_fps_log.txt > tmp_n_times.txt
		awk '{sum+=\$1; count++} END{if (count > 0) printf "%d\n", sum/count; else print "No fps data found"}' tmp_n_times.txt > ocv_drawing_$MODE$TAIL_MARK
		rm -f fbt.txt
		rm -f fbt_fps_log.txt
		rm -f tmp_n_times.txt
		EOF
    fi
}
