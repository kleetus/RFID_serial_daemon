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

#define TABLESIZE 5000
#define DBLINESIZE 16
#define BAUDRATE B9600
#define DEVICE "/dev/ttyS0"
#define _POSIX_SOURCE 1
#define FALSE 0
#define TRUE 1
#define TESTING 1

struct simple_rfid_access {
  char cardnum[16];
  char hashval;
  struct simple_rfid_access *next;
};

void signal_handler_IO(int status);
struct simple_rfid_access db[TABLESIZE];
char buf[DBLINESIZE];
char *ans;
int fd;

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
  int c, h;
  char a = '0';
  c = read(fd, buf, DBLINESIZE);
  if(h=hash(buf) > TABLESIZE) {
    write(fd, &a, 1);
    return;
  } 
  ans = &(db[hash(buf)].hashval);
  write(fd, ans, 1);
  //clear out the buffer for the next time??  
}

void
clear() {
  struct simple_rfid_access rfiddb;
  int i;
  for(i=0; i<TABLESIZE; i++) {
   db[i] = rfiddb;
  }
}

int
load(int argc, char **argv) {
  
  
  
  #ifdef TESTING
  /* for testing only */
  int i;
  for(i=1; i<501; i++) {
    struct simple_rfid_access rfiddb;
    sprintf(rfiddb.cardnum, "%i", i);
    int h;
    h = hash(rfiddb.cardnum);
    rfiddb.hashval = '1'; //just hardcoded to 'yes'
    if(db[h].cardnum) {
      db[h].next = &rfiddb;
    }
    else {
      db[h] = rfiddb;
    }
    printf("hash of %s: %i\n", rfiddb.cardnum, hash(rfiddb.cardnum));
  }    
  /* end test */
  
  
  #else
  FILE *fp;
  char *p;
  char buf[DBLINESIZE];
  if(argc>1) fp = fopen(argv[1], "r");
  else fp = fopen("/var/db/db.txt", "r");
  if(fp == NULL) {
    printf("failed to open database.\n");
    return -1;
  }
  while(fgets(buf, sizeof(buf), fp)) {
    char res = buf[DBLINESIZE - 1];
    buf[DBLINESIZE - 1] = '\0';   
    p = &buf[0];
    db[hash(p)] = res;
  }
  return fclose(fp);
  #endif
}

int
main(int argc, char **argv) {
  // clear();
  // load(argc, argv);
  int am, as, res;
  char n[20];
  const struct termios t;
  const struct winsize w;
  res = openpty(&am, &as, n, &t, &w);
  printf("result of openpty: %i\n\n", res);
  printf("master's file descriptor is: %i\n\n", am);
  printf("slave's file descriptor is: %i\n\n", as);
  return 1;
  
  struct termios newtio;
  struct sigaction saio;
  pid_t pid, sid;
  sigset_t st;
   
  pid = fork();
   
  if(pid < 0) exit(EXIT_FAILURE);
  if(pid > 0) exit(EXIT_SUCCESS);
   
  umask(0);
   
  sid = setsid();
  if (sid < 0) exit(EXIT_FAILURE);
  if((chdir("/")) < 0) exit(EXIT_FAILURE);
   
  close(STDIN_FILENO); close(STDOUT_FILENO); close(STDERR_FILENO);
   
  /* open the device to be non-blocking (read will return immediately) */
  fd = open(DEVICE, O_RDWR | O_NOCTTY | O_NONBLOCK);
  if(fd<0) {perror(DEVICE); exit(-1);}
  
  /* install the signal handler before making the device asynchronous */
  saio.sa_handler = signal_handler_IO;
  saio.sa_mask = st;
  saio.sa_flags = 0;
  saio.sa_restorer = NULL;
  sigaction(SIGIO,&saio,NULL);
  
  /* allow the process to receive SIGIO */
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
    usleep(1000000);
  }
   
  exit(EXIT_SUCCESS);
}
