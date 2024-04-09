# /bin/bash
# README:
#       wtite test statement into temp test script

function writeBmcvTranscodeTest(){
    local nfs_path=$1
    local MODE=$2
    local TAIL_MARK=$3
    local try_n_times=$4
    if [ $MODE = pcie ];then
        :
    elif [ $MODE = soc ];then
		cat <<-EOF > $nfs_path/tmp_daily.sh
		rm -f ff_bmcv_transcode*.mp4
		rm -f tmp_n_times.txt
		rm -f fbt.txt
		rm -f fbt_fps_log.txt

		for((j=1;j<=$try_n_times;j++))
		do
		    ./samples/ff_bmcv_transcode/test_ff_bmcv_transcode soc ./stream/bilibiligd.264 ff_bmcv_transcode.mp4 I420 h264_bm 416 240 25 3000 1 0 0 2>&1 | tee fbt.txt
		    cmd_status=\${PIPESTATUS[0]}
		    if [ \$cmd_status -ne 0 ]; then
		        exit \$cmd_status
		    fi
		    cat fbt.txt | grep fps > fbt_fps_log.txt
		    tr '\r' '\n' < fbt_fps_log.txt | grep "enc_fps" | awk '{print \$6}' >> tmp_n_times.txt
		done
		awk '{if(\$1 > 100){sum+=\$1; n++}} END{if(n>0){mean=sum/n; printf "%d\n",mean} else {print "No numbers greater than 100 found"}}' tmp_n_times.txt > ff_bmcv_transcode_$MODE$TAIL_MARK
		rm -f tmp_n_times.txt
		rm -f fbt.txt
		rm -f fbt_fps_log.txt
		EOF
    fi
}
