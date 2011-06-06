#include <iostream>
#include <hash_map>
#include <cstring>
#include <termios.h>
#include <sys/signal.h>
#include <pty.h>
using namespace std;
using namespace __gnu_cxx;

#define TABLESIZE 10000000
#define DBLINESIZE 13 //this should be the real size of the db string without a newline or null termination character
#define BAUDRATE B9600
#define _POSIX_SOURCE 1
#define FALSE 0
#define TRUE 1
#define VERSION "0.1"
//#define TESTING 1

char *ans;
char *device;
int fd, logp;
char *version = VERSION;
struct termios oldtio;

void signal_handler_IO(int status);

void
signal_handler_IO(int status) {
  char buf[50];
  char str[DBLINESIZE];
  char logbuf[256];
  int h,i,j;
  char ans = '0';
  char station_id = '0';
  char ansstr[4];
  
  memset(logbuf, '\0', sizeof(logbuf));
  memset(buf, '\0', sizeof(buf));
  memset(str, '\0', sizeof(str));
  
  read(fd, buf, 50);
  
  tcflush(fd, TCIFLUSH);
  
  j=0;
  for(i=0; j<DBLINESIZE; i++) {
    if(buf[i]==3 || buf[i]==2 || buf[i]==10) { continue; }
    if(i==2) { station_id = buf[i]; continue; }
    str[j++] = buf[i];
  }
  
  str[DBLINESIZE-1] = '\0';
  
  if(strlen(str) < DBLINESIZE-1) {
    ans = '3'; 
    goto error_condition;
  }
 
  if(h >= TABLESIZE) {
    ans = '4';
    goto error_condition;
  }
    
  struct simple_rfid_access rf = db[h];

  while(1) {
    if(strcmp(rf.cardnum,str) == 0) { ans = db[h].hashval; break; }
    else if(rf.next != NULL) {
      rf = *(rf.next);
    }
    else {
      ans = '5';
      break;
    }
  }
error_condition:
  if(strcmp(str, "3400C2DF0B22") == 0){ans='1';}//this is the admin key in case the database is destroyed or unavailable
    sprintf(logbuf, "IO HANDLER -- received: %s +++ answered with: %c", str, ans);
    logdaemonevent(logbuf);
    sprintf(ansstr, "%c@%c", ans, station_id);
    write(fd, ansstr, 3);
  }

int
load(int argc, char **argv) {
  
  #ifdef TESTING
  /* for testing only */
  int i,h, collisioncnt;
  for(i=1; i<5000; i++) {	
    char *n = calloc(5, sizeof(char));
    sprintf(n, "%i", i);
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
    fp = fopen("./db_real.txt", "r");
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
    strncpy(str, buf, strlen(buf)-1);
    ans = buf[strlen(buf)-1];
  
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
  int i,n;
  n=0;
  for(i=0; i<TABLESIZE; i++) {
    if(strcmp(db[i].cardnum,"") != 0) {
      char s[100];
      sprintf(s, "key: %i --- value: ( cardnum - %s, answer - %c, number of collisions - %i )", i, db[i].cardnum, db[i].hashval, db[i].collisioncnt);
      logdaemonevent(s);
      n++;
    }
  }
  char f[256];
  sprintf(f, "ending dump of db\n\nbrought in: %d records.\n\n", n);
  logdaemonevent(f);
  return 1;
}

void
cleanup(int sig) {
  tcsetattr(fd,TCSANOW,&oldtio);
  close(fd);
  closedaemonlog();
  system("killall daemon");
  (void) signal(SIGINT, SIG_DFL);
}

int main(){
 struct termios newtio;
 struct sigaction saio;
 pid_t pid, sid;
 sigset_t st;

 (void) signal(SIGINT, cleanup);

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
  
  tcgetattr(fd,&oldtio); /* save current port settings */
  /* set new port settings for canonical input processing */
  newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
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
	
  exit(EXIT_SUCCESS);
  hash_map<const char*, const char*> rfidcard;
  //cout << "rfidcard hashval is: " << rfidcard["AAAAAAAAAA"] << endl;
  return 0;
}

