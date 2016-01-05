#!/bin/sh
IPADDRESS=`hostname -I | tr -d ' '`
SSH_EXISTS=`ps uxww | grep ${IPADDRESS}:5656 | grep -v grep`
if [ -z "${SSH_EXISTS}" ]; then
	echo "ssh tunnel error!"
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
