#!/bin/sh
SSH_EXISTS=`ps uxww | grep 140:5656 | grep -v grep`
if [ -z "${SSH_EXISTS}" ]; then
	echo "ssh tunnel error!"
	IPADDRESS=`ip -4 addr list eth0 | grep inet | awk -F " " '{print $2}' | awk -F "/" '{print $1}'`
	echo ${IPADDRESS}
	date
	expect -c "  
		set timeout 10
		spawn ssh -D ${IPADDRESS}:5656 -qgTfnN yzhu1@ala-lpd-susbld2.wrs.com  
		expect \"password:\"
		send \"password!\r\"
		expect eof
	"
fi
