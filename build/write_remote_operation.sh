
# /bin/bash
# README:
#        Write the operations on remote machine to python file

function writeRemoteOperation(){
    local IP_ADDR=$1
    local cmd=$2
cat << EOF > /tmp/tmp_soc.py
import paramiko
ssh = paramiko.SSHClient()
ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
ssh.connect('$IP_ADDR', 22, 'linaro', 'linaro')
stdin, stdout, stderr = ssh.exec_command('$cmd', get_pty=True)
for line in stdout.readlines():
    print(line.strip())
for line in stderr.readlines():
    print(line.strip())

exit_status = stdout.channel.recv_exit_status()  # 获取命令的退出状态
stdin.close()
ssh.close()

if exit_status != 0:
    print(f"Error occurred. Exit status: {exit_status}")
    exit(1)
else:
    exit(0)
EOF

chmod +x /tmp/tmp_soc.py
}
