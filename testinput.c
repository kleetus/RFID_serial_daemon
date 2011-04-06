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
		printf("%i. %s\n", i, b);
	}
	fclose(fp);
	return 1;
}

/*
This is a test script that randomly sends input to the main rfid serial daemon to see how it 
reacts under somewhat realistic loads -- very small loads more than likely
*/
int
main() {
	int am, as, res;
  char n[100];
  const struct termios t;
  const struct winsize w;
	char random_array[100];
	readindb();
  res = openpty(&am, &as, n, &t, &w);
	if(!res){
		return -1;
	}
}