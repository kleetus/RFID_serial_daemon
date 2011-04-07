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

char *db[TABLESIZE];
int exitcondition = 0;

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
		db[i] = b;
		i++;
	}
	fclose(fp);
	return 1;
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
	FILE *fpipe;
	int am, as, res;
  char n[100];
  char line[256];
  const struct termios t;
  const struct winsize w;
	pid_t pid;
	
	readindb();

  res = openpty(&am, &as, n, &t, &w);
	if(res<-1){
  	exit(1);
	}
	
	//TODO: add checking for a running daemon and stop it, then restart it under known testing conditions
	//for now consider that the user has shutdown the daemon prior to starting this script



	/* get the process id */
	// if ((pid = getpid()) < 0) {
	//   perror("unable to get pid\n\n");
	// } 
	// else {
	//   printf("The process id is %d\n\n", pid);
	// }
	// 
	// /* get the parent process id */
	// if ((ppid = getppid()) < 0) {
	//   perror("unable to get the ppid\n\n");
	// } 
	// else {printf("The parent process id is %d\n\n", ppid);}
	// 
	// /* get the group process id */
	// if ((gid = getgid()) < 0) {
	//   perror("unable to get the group id\n\n");
	// } 
	// else {printf("The group id is %d\n\n", gid);
	// }
	// return 0;
	// pid = getpid();
	pid_t thispid = fork();


	if(thispid != 0) {
		//we're in the forked process
		char *args[] = {'\0'};
		execv("./daemon", args);
		exitcondition = 1; //will never get here
	}
	else {
		while(exitcondition == 0) {
			usleep(1000000);
		}
	}
 
 // if(!(fpipe = (FILE*)popen(command,"r"))){ 
  // 	  perror("Problems with pipe");
  // 	  exit(1);
  //  }
  //  while ( fgets( line, sizeof line, fpipe)){
  //    printf("%s", line);
  //  }
  // 	pclose(fpipe);
}