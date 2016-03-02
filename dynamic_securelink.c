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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "dynamic_securelink.h"

/* stage CREATE_LOCAL_LISTEN: local listen
 * stage CHECK_LOCAL_LISTEN: check local listen
 * stage CREATE_SSH_CONNECTION: create ssh connection
 * stage CHECK_SSH_CONNECTION: check ssh connection established
 * stage CHECK_SSH_ACCESS: check other clients connecting to this port
 * stage DISCONNECT_SSH: wait for 5 mintues, then disconnection ssh
 */
#define BUF_LEN		256
static int stage = 0;
static char locallisten_cmd[BUF_LEN] = {0};

static void begin_listen()
{
	char buf[BUFSIZ] = {0};
	if (strlen(locallisten_cmd) == 0) {
		if (!getcwd(buf, BUFSIZ)) {
			INFO_OUTPUT("getcwd error!\n");
			exit(0);
		}
		sprintf(locallisten_cmd, "setsid %s/%s &", buf, LOCALSERVER);
	}
	INFO_OUTPUT("%s\n", locallisten_cmd);
	system(locallisten_cmd);
}

static void proc_tcp_check_locallisten()
{
	/* grep "1618 00000000:0000 0A" /proc/net/tcp 
	 */
	char cmdline[BUF_LEN] = {0}, buf[BUFSIZ] = {0};
	FILE *pfp = NULL;

	sprintf(cmdline, "grep \"%x 00000000:0000 0A\" /proc/net/tcp", PORT);
	pfp = popen(cmdline, "r");
	if (NULL == pfp) {
		INFO_OUTPUT("popen error!\n");
		exit(0);
	}

	if (fgets(buf, BUFSIZ, pfp) == NULL) {
		stage = CREATE_SSH_CONNECTION;
	}

	while (fgets(buf, BUFSIZ, pfp) != NULL) {
		INFO_OUTPUT("buf:%s\n", buf);
		memset(buf, 0, BUFSIZ);
	}

	pclose(pfp);
	pfp = NULL;
	INFO_OUTPUT("local listen check!\n");
}

static void check_locallisten()
{
	/* netstat -napt | grep 5656 | grep -v grep
	 */
	char cmdline[BUF_LEN] = {0}, buf[BUFSIZ] = {0};
	FILE *pfp = NULL;

	sprintf(cmdline, "netstat -napt | grep :%d | grep -v grep", PORT);
	pfp = popen(cmdline, "r");
	if (NULL == pfp) {
		INFO_OUTPUT("popen error!\n");
		exit(0);
	}

	if (fgets(buf, BUFSIZ, pfp) == NULL) {
		stage = CREATE_SSH_CONNECTION;
	}

	while (fgets(buf, BUFSIZ, pfp) != NULL) {
		INFO_OUTPUT("buf:%s\n", buf);
		memset(buf, 0, BUFSIZ);
	}

	pclose(pfp);
	pfp = NULL;
	INFO_OUTPUT("local listen check!\n");
}

static void create_ssh_connection()
{
	/* create ssh connection */
	FILE *pfp = NULL;
	char cmdline[BUF_LEN] = {0}, buf[BUFSIZ] = {0};
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
	stage = CHECK_SSH_CONNECTION;
}

/* TCP state:
	"Unused", 	0
	"Established", 	1
	"Syn Sent", 	2
	"Syn Recv", 	3
	"Fin Wait 1", 	4
	"Fin Wait 2", 	5
	"Time Wait",  	6
	"Close", 	7
        "Close Wait", 	8
	"Last ACK", 	9
	"Listen", 	10
	"Closing" 	11
*/
/* check 10 times */
#define CHECK_COUNT			 12
static int check_ssh_connection_count = 0;

static void proc_tcp_check_ssh_connection()
{
	/* check ssh connection 
	 * grep ":1618 " /proc/net/tcp
	 */
	char cmdline[BUF_LEN] = {0}, buf[BUFSIZ] = {0};
	FILE *pfp = NULL;

	sprintf(cmdline, "grep \"%x 00000000:0000 0A\" /proc/net/tcp", PORT);
	pfp = popen(cmdline, "r");
	if (NULL == pfp) {
		INFO_OUTPUT("popen error!\n");
		exit(0);
	}

	if (fgets(buf, BUFSIZ, pfp) == NULL) {
		stage = CREATE_SSH_CONNECTION;
		INFO_OUTPUT("NO LISTEN ESATBLISHED!\n");
	}

	while (fgets(buf, BUFSIZ, pfp) != NULL) {
		INFO_OUTPUT("buf:%s\n", buf);
		INFO_OUTPUT("ssh connection created!\n");
		stage = CHECK_SSH_ACCESS;
		memset(buf, 0, BUFSIZ);
	}

	pclose(pfp);
	pfp = NULL;

	INFO_OUTPUT("ssh connection check!\n");
}

static void check_ssh_connection()
{
	/* check ssh connection 
	 * netstat -napt | grep :5656 | grep -v grep
	 */
	char cmdline[BUF_LEN] = {0}, buf[BUFSIZ] = {0};
	FILE *pfp = NULL;

	sprintf(cmdline, "netstat -napt | grep :%d | grep -v grep", PORT);
	pfp = popen(cmdline, "r");
	if (NULL == pfp) {
		INFO_OUTPUT("popen error!\n");
		exit(0);
	}

	if (fgets(buf, BUFSIZ, pfp) == NULL) {
		stage = CREATE_SSH_CONNECTION;
		INFO_OUTPUT("NO LISTEN ESATBLISHED!\n");
	}

	while (fgets(buf, BUFSIZ, pfp) != NULL) {
		INFO_OUTPUT("buf:%s\n", buf);
		/* check LISTEN */
		if (!strstr(buf, "LISTEN") && !strstr(buf, "ESTABLISHED")) {
			INFO_OUTPUT("NO LISTEN ESATBLISHED!\n");
			check_ssh_connection_count ++;
			if (check_ssh_connection_count >= CHECK_COUNT) {
				stage = CREATE_SSH_CONNECTION;
			}
		} else {
			if (strstr(buf, "ssh")) {
				INFO_OUTPUT("ssh connection created!\n");
				stage = CHECK_SSH_ACCESS;
			}
		}
		memset(buf, 0, BUFSIZ);
	}

	pclose(pfp);
	pfp = NULL;

	INFO_OUTPUT("ssh connection check!\n");
}

static void check_client_access_ssh()
{
	char cmdline[BUF_LEN] = {0}, buf[BUFSIZ] = {0};
	FILE *pfp = NULL;
	int listen_exist = 0;

	sprintf(cmdline, "netstat -napt | grep :%d | grep -v grep", PORT);
	pfp = popen(cmdline, "r");
	if (NULL == pfp) {
		INFO_OUTPUT("popen error!\n");
		exit(0);
	}
	while (fgets(buf, BUFSIZ, pfp) != NULL) {
		INFO_OUTPUT("buf:%s\n", buf);

		/* check ssh connection exist or not */
		if ((!strstr(buf, "LISTEN")) && !listen_exist) {
			/* ssh connection does not exist */
			stage = CREATE_SSH_CONNECTION;
		} else {
			listen_exist = 1;
		}

		if (strstr(buf, "ESTABLISHED")) {
			INFO_OUTPUT("some client is using!\n");
		}
		if (strstr(buf, "CLOSE_WAIT")) {
			INFO_OUTPUT("some client does not use it!\n");
		}
		if (strstr(buf, "LISTEN") && !strstr(buf, "ESTABLISHED") && !strstr(buf, "CLOSE_WAIT")) {
			INFO_OUTPUT("check, listening, no client using it!\n");
			sleep(300);
			stage = DISCONNECT_SSH;
		}
	}
	pclose(pfp);
	pfp = NULL;
	INFO_OUTPUT("client access ssh connection check!\n");
}

static void disconnect_ssh()
{
	char cmdline[BUF_LEN] = {0}, buf[BUFSIZ] = {0};
	FILE *pfp = NULL;

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
						char tmp_cmdline[BUF_LEN] = {0};
						sprintf(tmp_cmdline, "kill -9 %s", tmp);
						system(tmp_cmdline);
					}
				}
			}
			pclose(tmp_fp);
			tmp_fp = NULL;
		} else {
			stage = CHECK_SSH_ACCESS;
		}
	}
	pclose(pfp);
	pfp = NULL;
	stage = CREATE_LOCAL_LISTEN;
	INFO_OUTPUT("disconnect ssh!\n");
}

void cleanup_ssh()
{
	FILE *tmp_fp = NULL;
	char cmdline[BUF_LEN] = {0};

	sprintf(cmdline, "netstat -napt | grep :%d | grep LISTEN | awk -F \"LISTEN\" '{print $2}' | awk -F \"/\" '{print $1}' | tr -d ' '", PORT);
	tmp_fp = popen(cmdline, "r");
	if (NULL != tmp_fp) {
		char tmp[BUFSIZ] = {0};
		if (fgets(tmp, BUFSIZ, tmp_fp) != NULL) {
			INFO_OUTPUT("clean ssh pid:%s\n", tmp);
			if (strlen(tmp) > 0) {
				char tmp_cmdline[BUF_LEN] = {0};
				sprintf(tmp_cmdline, "kill -9 %s", tmp);
				system(tmp_cmdline);
			}   
		}  
	} 
	pclose(tmp_fp);
	tmp_fp = NULL;
}

void cleanup_locallisten()
{
	int retval = access("/var/run/locallisten.pid", F_OK);
	FILE *fp = NULL;
	char tmp[16] = {0}, cmdline[BUF_LEN] = {0};

	if (retval != -1) {
		fp = fopen("/var/run/locallisten.pid", "r");
		if (NULL != fp) {
			fscanf(fp, "%s", tmp);
		}
		fclose(fp);
		if (strlen(tmp) > 0) {
			sprintf(cmdline, "kill -9 %s\n", tmp);
			system(cmdline);
			system("killall -9 locallisten");
		}
	}
}

int main()
{
	/* make the port can be reused in 1 second */
	system(CMD_PORT_REUSE);

	/* disconnect ssh connection after main function exit */
	atexit(cleanup_ssh);

	/* make locallisten exit after main function exit */
	atexit(cleanup_locallisten);

	/*main thread*/
	while (1) {
		switch(stage) {

			case CREATE_LOCAL_LISTEN:
				/* create local listen */ 
				begin_listen();
				/* check local listen */
				stage = CHECK_LOCAL_LISTEN;
				break;

			case CHECK_LOCAL_LISTEN:
				/* check local listen 
				 */
				//check_locallisten();
				proc_tcp_check_locallisten();
				break;

			case CREATE_SSH_CONNECTION:
				/* create ssh connection */
				create_ssh_connection();
				break;

			case CHECK_SSH_CONNECTION:
				/* check ssh connection 
				 */
				//check_ssh_connection();
				proc_tcp_check_ssh_connection();
				break;

			case CHECK_SSH_ACCESS:
				/* check other client connecting to this port */
				check_client_access_ssh();
				break;

			case DISCONNECT_SSH:
				/* wait for 10 minutes, then disconnect ssh */
				sleep(3600);
				/*disconnect ssh connection*/
				disconnect_ssh();
				break;

			default:
				INFO_OUTPUT("stage is set wrong value!\n");
		}

		sleep(CHECK_INTERVAL);
	}
}
