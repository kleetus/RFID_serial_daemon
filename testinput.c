#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <pty.h>
#include <time.h>  


#define TABLESIZE 5000
#define DBLINESIZE 9 //this should be the real size of the db string without a newline or null termination character
#define BAUDRATE B9600

char db[TABLESIZE][DBLINESIZE];
char answers[TABLESIZE];

int initdb() {
	int i;
	for(i=0; i<TABLESIZE; i++) {
		strncpy(db[i], "", DBLINESIZE);	
	}
}

int readindb() {
	FILE *fp;
	char b[DBLINESIZE+1];
	int i = 0;
	fp = fopen("/var/db/db.txt", "r");	
  if(fp == NULL) {
    printf("\n\n\nfailed to open database.\n\n\n");
    return -1;
  }  
	while(fgets(b, sizeof(b), fp)) {
		if(*b == '\n') continue;
		strncpy(db[i], b, DBLINESIZE-1);
		answers[i] = b[strlen(b)-1];
		b[strlen(b)-1] = '\0';
		i++;
	}
	fclose(fp);
	return 1;
}

void cleanup(int sig) {
	if(sig == 2) {
		system("killall daemon");
	}
	(void) signal(SIGINT, SIG_DFL);
}


/*
This is a test script that randomly sends input to the main rfid serial daemon to see how it 
reacts under somewhat realistic loads -- very small loads more than likely

This script will open a master/slave pty pair and attach itself to the master end of the communications line. 
It will expect the application under test (the daemon) to attach itself to the slave
Example: the master/slave pair created, the exposed device in linux will be the slave device of /dev/pts/0. This is the device name that
the daemon app will be called with. So this app can write and read from its master file descriptor and the daemon will write and read from the slave device
(/dev/pts/0).
*/
int
main() {
	int am, as, res, i;
  char n[100];
  char line[256];
  const struct termios t;
	const struct winsize w;
	pid_t pid;
	char received[1];
	char ans;
	char cmp;
	
	(void) signal(SIGINT, cleanup);
	
	initdb();
	readindb();
	
  res = openpty(&am, &as, n, &t, &w);
	if(res<0){
		printf("couldn't open the pty, so quitting.\n\n");
  	exit(1);
	}
	
	grantpt(am);
	unlockpt(am);
	
	pid = fork();

	if(pid < 0) { printf("could not fork a process, so quitting.\n\n"); exit(EXIT_FAILURE); }//means we are in the parent's thread (still) and the fork failed

	if(pid == 0) {
		char *args[] = {"./daemon", n, "/var/db/db.txt", NULL};
		execv("./daemon", args); //this replaces the child process' pid, so the child process of this app/script is the daemon
	}
	else {
		printf("sleeping for 1 second to ensure daemon is ready to start taking requests...\n\n");
		sleep(1);
		while(1){
			for(i=0; i<TABLESIZE; i++){
				if(db[i][0] != '\0'){
					printf("sending:	%s\n", db[i]);
					printf("should be receiving back: \t%c\n", answers[i]);
					write(am, db[i], strlen(db[i]));
					write(am, "\n", 1); //this means "do it"
					read(am, received, 1);
					printf("receiving:	\t\t%s\n", received);
					//sprintf(cmp, "%c", ans);
					// if(strcmp(received, cmp) != 0) {
					// 	printf("should of received:	%s\n", cmp);
					// }
					usleep(500000);
				}
			}
		}
	} 
}
