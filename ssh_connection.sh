#!/bin/sh
SSH_EXISTS=`ps uxww | grep 140:5656 | grep -v grep`
if [ -z "${SSH_EXISTS}" ]; then
	echo "ssh tunnel error!"
	date
	expect -c "  
		set timeout 10
		spawn ssh -D 128.224.162.140:5656 -qgTfnN yzhu1@ala-lpd-susbld2.wrs.com  
		expect \"password:\"
		send \"password!\r\"
		expect eof
	"
fi
