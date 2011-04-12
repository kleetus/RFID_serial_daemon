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
#define _POSIX_SOURCE 1
#define FALSE 0
#define TRUE 1
#define VERSION "0.1"
//#define TESTING 1

struct simple_rfid_access {
  char *cardnum;
  char hashval;
  struct simple_rfid_access *next;
};

void signal_handler_IO(int status);
struct simple_rfid_access db[TABLESIZE];
char buf[DBLINESIZE];
char *ans;
char *device;
int fd, logp;
char *version = VERSION;

unsigned int
hash(char *ch) {
  unsigned int hashvalue = 0;
  if(!*ch) return 0;
  do {
     hashvalue += *ch++;
     hashvalue *= 13;
  } while (*ch);
 return(hashvalue % TABLESIZE);
}

void
signal_handler_IO(int status) {
	char buf[256];
	char logbuf[256];
	int c, h;
	memset(logbuf, '\0', sizeof(logbuf));
	memset(buf, '\0', sizeof(buf));
  c = read(fd, buf, DBLINESIZE);
	buf[strlen(buf)-1] = '\0';
	sprintf(logbuf, "IO HANDLER -- received: %s", buf);
  logdaemonevent(logbuf);
  if(h=hash(buf) > TABLESIZE) {
    write(fd, "ERROR", 5);
    return;
  } 
  write(fd, buf, strlen(buf));
}

void
clear() {
  struct simple_rfid_access rfiddb;
  int i;
	rfiddb.cardnum = "";
	rfiddb.hashval = 0;
  for(i=0; i<TABLESIZE; i++) {
   db[i] = rfiddb;
  }
}

int
load(int argc, char **argv) {
  
  
  #ifdef TESTING
  /* for testing only */
  int i,h;
  for(i=1; i<5000; i++) {
    struct simple_rfid_access rfiddb;
		h = 0;
		char n[5];
		sprintf(n, "%i", i);
		rfiddb.cardnum = n;
		h = hash(rfiddb.cardnum);
    rfiddb.hashval = '1'; //just hardcoded to 'yes'
    if(db[h].cardnum != "") {
      db[h].next = &rfiddb;
    }
    else {
      db[h] = rfiddb;
    }
	}
  /* end test */
  
  
  #else
  FILE *fp;
	char str[DBLINESIZE];
  char buf[DBLINESIZE];
	int h;
  if(argc>2) {
		fp = fopen(argv[2], "r");
		device = argv[1];
	}
  else {
		fp = fopen("/var/db/db.txt", "r");
		device = "/dev/ttyS0";
	} 
  if(fp == NULL) {
    printf("failed to open database.\n");
		exit(EXIT_FAILURE);
  }
  while(fgets(buf, sizeof(buf), fp)) {
		if(*buf == '\n') continue;
		strncpy(str, buf, DBLINESIZE);
		h=hash(str);
		if(strcmp(db[h].cardnum, buf) == 0) {
			printf("\n\n\nhash collision! creating linked list member.\n\n\n");
		  struct simple_rfid_access rfiddb;
			rfiddb.cardnum = str;
			rfiddb.hashval = 1; //TODO replace this with the actual
			db[h].next = &rfiddb;
		}
		else {
			db[h].cardnum = str;
			db[h].hashval = 1; //TODO replace this with the actual
		}
  }
  return fclose(fp);
  #endif
}

int
opendaemonlog() {
	logp = open("/var/log/rfid_daemon.log", O_RDWR | O_APPEND);
	if(logp < 0) { 
		printf("failed to open log file: /var/log/rfid_daemon.log");
		exit(EXIT_FAILURE);
	}
	return 1;
}

int closedaemonlog() {
	close(logp);
	return 1;
}

int
logdaemonevent(char *event) {
	char log[256];
	log[0] = '\0';
	time_t t = time(NULL);
	char *logtime = ctime(&t);
	logtime[strlen(logtime)-1] = ' ';
	logtime[strlen(logtime)] = '\0';
	strcat(log, logtime);
	strcat(log, event);
	strcat(log, "\n");
	write(logp, log, strlen(log));
	return 1;
}

int
main(int argc, char **argv) {
  struct termios newtio;
  struct sigaction saio;
  pid_t pid, sid;
  sigset_t st;

	char logentry[256];
	
	opendaemonlog(); //this could make the app fail if there is no log file, watch stdout
	
  clear();
  load(argc, argv);

	sprintf(logentry, "Starting rfid serial daemon, version %s", version);
	logdaemonevent(logentry);
   
  pid = fork();
   
	if(pid < 0) {	logdaemonevent("failed to fork process."); exit(EXIT_FAILURE); }
  if(pid > 0) {	logdaemonevent("forked process."); exit(EXIT_SUCCESS); }
  
  umask(0);
   
  sid = setsid();
  if (sid < 0) { logdaemonevent("sid less than 0."); exit(EXIT_FAILURE);}
  if((chdir("/")) < 0) { logdaemonevent("could not chdir to /."); exit(EXIT_FAILURE);}
   
  close(STDIN_FILENO); close(STDOUT_FILENO); close(STDERR_FILENO);
   
  /* open the device to be non-blocking (read will return immediately) */
	logdaemonevent(device);
  fd = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK);
  if(fd<0) { logdaemonevent("failed to open serial device."); perror(device); exit(EXIT_FAILURE); }
  
  /* install the signal handler before making the device asynchronous */
  saio.sa_handler = signal_handler_IO;
  saio.sa_mask = st;
  saio.sa_flags = 0;
  saio.sa_restorer = NULL;
  sigaction(SIGIO,&saio,NULL);
  
  /* allow the process to receive SIGIO */
	char buff[10];
	sprintf(buff, "pid of event handler: %i", getpid());
	logdaemonevent(buff);
	
  fcntl(fd, F_SETOWN, getpid());
  fcntl(fd, F_SETFL, FASYNC);
  
  /* set new port settings for canonical input processing */
  newtio.c_cflag = BAUDRATE | CRTSCTS | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR | ICRNL;
  newtio.c_oflag = 0;
  newtio.c_lflag = ICANON;
  newtio.c_cc[VMIN]=1;
  newtio.c_cc[VTIME]=0;
  tcflush(fd, TCIFLUSH);
  tcsetattr(fd,TCSANOW,&newtio);
  
  while (1) {
    usleep(100000);
  }   

	closedaemonlog();
	
  exit(EXIT_SUCCESS);
}