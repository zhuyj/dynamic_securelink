#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <string.h>
#include <pthread.h>

#define LOCALSERVER "locallisten"
#define PORT    5656
#define MAXMSG  512

/* stage 0: local listen
 * stage 1: check local listen
 * stage 2: create ssh connection
 * stage 3: check ssh connection established
 * stage 4: check other clients connecting to this port
 * stage 5: wait for 5 mintues, then disconnection ssh
 */
static int stage = 0;

#ifdef DEBUG_MODE
#define INFO_OUTPUT(args ...) do{fprintf(stderr,args);}while(0);
#else
#define INFO_OUTPUT(args ...) do{syslog(6,args);}while(0);
#endif
static void begin_listen()
{
	char cmdline[256] = {0}, buf[BUFSIZ] = {0};
	FILE *pfp = NULL;
	if (!getcwd(buf, BUFSIZ)) {
		INFO_OUTPUT("getcwd error!\n");
		exit(0);
	}
	sprintf(cmdline, "%s/%s &", buf, LOCALSERVER);
	INFO_OUTPUT("%s\n", cmdline);
	system(cmdline);
}

/* 5 seconds */
#define CHECK_INTERVAL 5

/* ssh connection script path */
#define SSH_CONNECTION_PATH "/root/ssh_connection.sh"

/* fast port reuse in 1 second*/
#define CMD_PORT_REUSE "echo 1 > /proc/sys/net/ipv4/tcp_tw_recycle"

void check_locallisten()
{
	/* netstat -napt | grep 5656 | grep -v grep
	 */
	char cmdline[256] = {0}, buf[BUFSIZ] = {0};
	FILE *pfp = NULL;

	sprintf(cmdline, "netstat -napt | grep :%d | grep -v grep", PORT);
	pfp = popen(cmdline, "r");
	if (NULL == pfp) {
		INFO_OUTPUT("popen error!\n");
		exit(0);
	}

	if (fgets(buf, BUFSIZ, pfp) == NULL) {
		stage = 2;
	}

	while (fgets(buf, BUFSIZ, pfp) != NULL) {
		INFO_OUTPUT("buf:%s\n", buf);
		memset(buf, 0, BUFSIZ);
	}

	pclose(pfp);
	pfp = NULL;
	INFO_OUTPUT("local listen check!\n");
}

void create_ssh_connection()
{
	/* create ssh connection */
	FILE *pfp = NULL;
	char cmdline[256] = {0}, buf[BUFSIZ] = {0};
	sprintf(cmdline, "sh %s", SSH_CONNECTION_PATH);
	pfp = popen(cmdline, "r");
	if (pfp == NULL) {
		INFO_OUTPUT("popen error!\n");
		exit(0);
	}

	while (fgets(buf, BUFSIZ, pfp) != NULL) {
		INFO_OUTPUT("buf:%s\n", buf);
		memset(buf, 0, BUFSIZ);
	}
	pclose(pfp);
	pfp = NULL;
	INFO_OUTPUT("ssh connection creation!\n");
	stage = 3;
}

void check_ssh_connection()
{
	/* check ssh connection 
	 * netstat -napt | grep :5656 | grep -v grep
	 */
	char cmdline[256] = {0}, buf[BUFSIZ] = {0};
	FILE *pfp = NULL;

	sprintf(cmdline, "netstat -napt | grep :%d | grep -v grep", PORT);
	pfp = popen(cmdline, "r");
	if (NULL == pfp) {
		INFO_OUTPUT("popen error!\n");
		exit(0);
	}

	while (fgets(buf, BUFSIZ, pfp) != NULL) {
		INFO_OUTPUT("buf:%s\n", buf);
		/* check LISTEN */
		if (!strstr(buf, "LISTEN")) {
			INFO_OUTPUT(" No LISTEN!\n");
		} else {
			if (strstr(buf, "ssh")) {
				INFO_OUTPUT("ssh connection created!\n");
				stage = 4;
			}
		}
		memset(buf, 0, BUFSIZ);
	}

	pclose(pfp);
	pfp = NULL;

	INFO_OUTPUT("ssh connection check!\n");
}

void check_client_access_ssh()
{
	char cmdline[256] = {0}, buf[BUFSIZ] = {0};
	FILE *pfp = NULL;

	sprintf(cmdline, "netstat -napt | grep :%d | grep -v grep", PORT);
	pfp = popen(cmdline, "r");
	if (NULL == pfp) {
		INFO_OUTPUT("popen error!\n");
		exit(0);
	}
	while (fgets(buf, BUFSIZ, pfp) != NULL) {
		INFO_OUTPUT("buf:%s\n", buf);
		if (strstr(buf, "ESTABLISHED")) {
			INFO_OUTPUT("some client is using!\n");
		}
		if (strstr(buf, "CLOSE_WAIT")) {
			INFO_OUTPUT("some client does not use it!\n");
		}
		if (strstr(buf, "LISTEN") && !strstr(buf, "ESTABLISHED") && !strstr(buf, "CLOSE_WAIT")) {
			INFO_OUTPUT("listening, no client using it!\n");
			sleep(300);
			stage = 5;
		}
	}
	INFO_OUTPUT("client access ssh connection check!\n");
}

void disconnect_ssh()
{
	char cmdline[256] = {0}, buf[BUFSIZ] = {0};
	FILE *pfp = NULL;

	/* wait for 10 minutes, then disconnect ssh */
	sleep(600);
	sprintf(cmdline, "netstat -napt | grep :%d | grep -v grep", PORT);
	pfp = popen(cmdline, "r");
	if (NULL == pfp) {
		INFO_OUTPUT("popen error!\n");
		exit(0);
	}
	while (fgets(buf, BUFSIZ, pfp) != NULL) {
		INFO_OUTPUT("buf:%s\n", buf);
		if (strstr(buf, "LISTEN") && !strstr(buf, "ESTABLISHED") && !strstr(buf, "CLOSE_WAIT")) {
			FILE *tmp_fp = NULL;
			INFO_OUTPUT("listening, no client using it!\n");
			sprintf(cmdline, "netstat -napt | grep :%d | grep LISTEN | awk -F \"LISTEN\" '{print $2}' | awk -F \"/\" '{print $1}' | tr -d ' '", PORT);
			tmp_fp = popen(cmdline, "r");
			if (NULL != tmp_fp) {
				char tmp[BUFSIZ] = {0};
				if (fgets(tmp, BUFSIZ, tmp_fp) != NULL) {
					INFO_OUTPUT("pid:%s\n", tmp);
					if (strlen(tmp) > 0) {
						char tmp_cmdline[256] = {0};
						sprintf(tmp_cmdline, "kill -9 %s", tmp);
						system(tmp_cmdline);
					}
				}
			}
			sleep(300);
			stage = 5;
		}
	}

	stage = 0;
	INFO_OUTPUT("client access ssh connection check!\n");

}

int main()
{
	/* make the port can be reused in 1 second */
	system(CMD_PORT_REUSE);

	/*main thread*/
	while (1) {
		switch(stage) {
			case 0:
				/* create local listen */ 
				begin_listen();
				/* check local listen */
				stage = 1;
				break;
			case 1:
			{
				/* check local listen 
				 */
				check_locallisten();
				break;
			}
			case 2:
			{
				/* create ssh connection */
				create_ssh_connection();
				break;
			}
			case 3:
			{
				/* check ssh connection 
				 */
				check_ssh_connection();
				break;
			}
			case 4:
			{
				/* check other client connecting to this port */
				check_client_access_ssh();
				break;
			}
			case 5:
			{
				/*disconnect ssh connection*/
				disconnect_ssh();
				break;
			}
			default:
				INFO_OUTPUT("stage is set wrong value!\n");
		}
		sleep(CHECK_INTERVAL);
	}
}
