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
#define DBLINESIZE 9
#define BAUDRATE B9600

char db[TABLESIZE][DBLINESIZE];

int initdb() {
	int i;
	for(i=0; i<TABLESIZE; i++) {
		strncpy(db[i], "", DBLINESIZE);	
	}
}

int readindb() {
	FILE *fp;
	char b[DBLINESIZE];
	int i = 0;
	fp = fopen("/var/db/db.txt", "r");	
  if(fp == NULL) {
    printf("\n\n\nfailed to open database.\n\n\n");
    return -1;
  }  
	while(fgets(b, sizeof(b), fp)) {
		if(*b == '\n') continue;
		strncpy(db[i], b, DBLINESIZE);
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
	char received[DBLINESIZE];
	
	(void) signal(SIGINT, cleanup);
	
	initdb();
	readindb();
	
  res = openpty(&am, &as, n, &t, &w);
	if(res<0){
  	exit(1);
	}
	
	grantpt(am);
	unlockpt(am);
	
	pid = fork();

  if(pid < 0) exit(EXIT_FAILURE); //means we are in the parent's thread (still) and the fork failed

	if(pid == 0) {
		char *args[] = {"./daemon", n, "/var/db/db.txt", NULL};
		execv("./daemon", args); //this replaces the child process' pid, so the child process of this app/script is the daemon
	}
	else {
		while(1){
			for(i=0; i<TABLESIZE; i++){
				if(db[i][0] != '\0'){
					printf("sending:	%s\n", db[i]);
					write(am, db[i], strlen(db[i]));
					write(am, "\n", 1);
					read(am, received, DBLINESIZE);
					printf("receiving:	%s\n", received);
					usleep(1000000);
				}
			}
		}
	} 
}
