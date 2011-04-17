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
#define DBLINESIZE 13 //this should be the real size of the db string without a newline or null termination character
#define BAUDRATE B9600
#define _POSIX_SOURCE 1
#define FALSE 0
#define TRUE 1
#define VERSION "0.1"
//#define TESTING 1

struct simple_rfid_access {
  char *cardnum;
  char hashval;
	char collisioncnt;
  struct simple_rfid_access *next;
} db[TABLESIZE];

char *ans;
char *device;
int fd, logp;
char *version = VERSION;

void signal_handler_IO(int status);

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
  int h;
  char ans = '0';
 
  memset(logbuf, '\0', sizeof(logbuf));
  memset(buf, '\0', sizeof(buf));
  
  if(read(fd, buf, DBLINESIZE) < DBLINESIZE) {
    ans = '3';
    goto error_condition;
  }
	buf[strlen(buf)-1] = '\0';
	h=hash(buf);
  if(h > TABLESIZE) {
		ans = '4';
		goto error_condition;
  }
	//actually look up the answer now
	struct simple_rfid_access rf = db[h];

	char t[256];
	sprintf(t, "I got sent this: %s", buf);
	logdaemonevent(t);
	
	while(1) {
		if(strcmp(rf.cardnum,buf) == 0) { ans = db[h].hashval; break; }
		else if(rf.next != NULL) {
			rf = *(rf.next);
		}
		else {
			ans = '5';
			break;
		}
	}
error_condition:
	sprintf(logbuf, "IO HANDLER -- received: %s +++ answered with: %c", buf, ans);
	logdaemonevent(logbuf);
	write(fd, &ans, 1);
}

void
clear() {
  struct simple_rfid_access rfiddb;
  int i;
	rfiddb.cardnum = "";
	rfiddb.hashval = 0;
	rfiddb.next = NULL;
  for(i=0; i<TABLESIZE; i++) {
   db[i] = rfiddb;
  }
}

int
load(int argc, char **argv) {
  
  
  #ifdef TESTING
  /* for testing only */
  int i,h, collisioncnt;
  for(i=1; i<5000; i++) {	
		char *n = calloc(5, sizeof(char));

		sprintf(n, "%i", i);
		
		h = hash(n);
		
		if(strcmp(n, db[h].cardnum) == 0) {
			//collision!
			struct simple_rfid_access *rfiddb = calloc(1, sizeof(struct simple_rfid_access));			
      db[h].next = rfiddb;
			rfiddb->cardnum = n;
			rfiddb->collisioncnt++;
			rfiddb->hashval = '1'; //just hardcoded to 'yes'
    }
    else {
      db[h].cardnum = n;
			db[h].hashval = '1';
			db[h].collisioncnt = 0;
    }
	}
  /* end test */
  
  
  #else
  FILE *fp;
	char *str;
  char buf[DBLINESIZE+1];
	int h, collisioncnt;
	char ans;
	char logentry[256];
	
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
		
		//this memory never gets freed and shouldn't
		str = calloc(DBLINESIZE, sizeof(char));
		
		strncpy(str, buf, DBLINESIZE-1);
		ans = buf[strlen(buf)-1];
		h=hash(str);
    
		if(strcmp(db[h].cardnum, str) == 0) {
			sprintf(logentry, "\nhash collision for hash: %i from card number: %s creating linked list member.\n", h, str);
			logdaemonevent(logentry);
			
			//this memory never gets freed and shouldn't
			struct simple_rfid_access *rfiddb = calloc(1, sizeof(struct simple_rfid_access));
			
			rfiddb->cardnum = str;
			rfiddb->hashval = ans;
			rfiddb->collisioncnt++;
			db[h].next = rfiddb;
		}
		else {
			db[h].cardnum = str; //need to allocate memory here!
			db[h].hashval = ans;
			db[h].collisioncnt = 0;
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
dumpdatabase() {
	logdaemonevent("starting dump of db\n");
	int i;
	for(i=0; i<TABLESIZE; i++) {
		if(strcmp(db[i].cardnum,"") != 0) {
			char s[100];
			sprintf(s, "key: %i --- value: ( cardnum - %s, answer - %c, number of collisions - %i )", i, db[i].cardnum, db[i].hashval, db[i].collisioncnt);
			logdaemonevent(s);
		}
	}
	logdaemonevent("ending dump of db\n\n");
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
  
	dumpdatabase();
	
  pid = fork();
   
	if(pid < 0) {	logdaemonevent("failed to fork daemon event handler process."); exit(EXIT_FAILURE); }
  if(pid > 0) {	logdaemonevent("forked daemon event handler process."); exit(EXIT_SUCCESS); }
  
  umask(0);
   
  sid = setsid();
  if (sid < 0) { logdaemonevent("sid of daemon event handler less than 0."); exit(EXIT_FAILURE);}
  if((chdir("/")) < 0) { logdaemonevent("daemon event handler: could not chdir to /."); exit(EXIT_FAILURE);}
   
  close(STDIN_FILENO); close(STDOUT_FILENO); close(STDERR_FILENO);
   
  /* open the device to be non-blocking (read will return immediately) */
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