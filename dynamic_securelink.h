#define LOCALSERVER "locallisten"
#define PORT    5656
#define MAXMSG  512

enum {
	CREATE_LOCAL_LISTEN = 0,
	CHECK_LOCAL_LISTEN,
	CREATE_SSH_CONNECTION,
	CHECK_SSH_CONNECTION,
	CHECK_SSH_ACCESS,
	DISCONNECT_SSH,
};

#ifdef DEBUG_MODE
#define INFO_OUTPUT(args ...) do{fprintf(stderr,args);}while(0);
#else
#define INFO_OUTPUT(args ...) do{syslog(6,args);}while(0);
#endif

/* 5 seconds */
#define CHECK_INTERVAL 5

/* ssh connection script path */
#define SSH_CONNECTION_PATH "/root/ssh_connection.sh"

/* fast port reuse in 1 second*/
#define CMD_PORT_REUSE "echo 1 > /proc/sys/net/ipv4/tcp_tw_recycle"

