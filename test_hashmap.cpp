#include <iostream>
<<<<<<< HEAD
#include <fstream>
#include <ext/hash_map>
=======
#include <unordered_map>
>>>>>>> bcbbc24a8e808e0066cfc1eed55ff4464417d695
#include <cstring>
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

using namespace std;
using namespace __gnu_cxx;

namespace __gnu_cxx {
  template<>
  struct hash<std::string>
  {
    hash<char*> h;
    size_t operator()(const std::string &s) const
    {
      return h(s.c_str());
    };
  };
}

#define BAUDRATE B9600
#define _POSIX_SOURCE 1
#define FALSE 0
#define TRUE 1
#define VERSION "0.2"

<<<<<<< HEAD
std::string device;
int fd;
ofstream logp;
=======
char *ans;
std::string device;
int fd, logp;
>>>>>>> bcbbc24a8e808e0066cfc1eed55ff4464417d695
string version = VERSION;
struct termios oldtio;
hash_map<string, string> db;

void signal_handler_IO(int);
<<<<<<< HEAD
void logdaemonevent(char*);
=======
void logdaemonevent(char*); 
>>>>>>> bcbbc24a8e808e0066cfc1eed55ff4464417d695

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
<<<<<<< HEAD
=======
    
>>>>>>> bcbbc24a8e808e0066cfc1eed55ff4464417d695
error_condition:
  if(strcmp(str, "3400C2DF0B22") == 0){ans='1';}//this is the admin key in case the database is destroyed or unavailable
    sprintf(logbuf, "IO HANDLER -- received: %s +++ answered with: %c", str, ans);
    logdaemonevent(logbuf);
    sprintf(ansstr, "%c@%c", ans, station_id);
    write(fd, ansstr, 3);
  }

int
load(int argc, char **argv) {
<<<<<<< HEAD
  ifstream fp;
  std::string key;
  std::string value;
  std::string line;
=======
  FILE *fp;
  char *str;
  char buf[DBLINESIZE+1];
  int h, collisioncnt;
  char ans;
  char logentry[256];
>>>>>>> bcbbc24a8e808e0066cfc1eed55ff4464417d695
	
  fp.open("./db_real.txt");
  device = "/dev/ttyS0";
  
  if (fp.is_open())
  {
    while (fp.good())
    {
      getline(fp, line);
      if(line.length()<13) continue;
      key = line.substr(0,line.length()-2);
      value = line.substr(line.length()-2, line.length()-1);
      db[key] = value;
      logdaemonevent("[cardnum] - " << key << " - [answer] --" answer); 
    }
    fp.close();
  }
  else logdaemonevent("Unable to open database file"); 

<<<<<<< HEAD
=======
  while(fgets(buf, sizeof(buf), fp)) {
    if(*buf == '\n') continue;
    //this memory never gets freed and shouldn't
    str = (char*)calloc(DBLINESIZE, sizeof(char));
    strncpy(str, buf, strlen(buf)-1);
    ans = buf[strlen(buf)-1];
  } 
  return fclose(fp);
>>>>>>> bcbbc24a8e808e0066cfc1eed55ff4464417d695
}

int
opendaemonlog() {
  logp.open("/var/log/rfid_daemon.log");
  if(!logp.is_open()) {
    cout << "failed to open log file: /var/log/rfid_daemon.log" << endl;
    exit(EXIT_FAILURE);
  }
  return 0;
}

int 
closedaemonlog() {
  logp.close();
  return 0;
}

void
logdaemonevent(std::string log) {
  time_t t = time(NULL);
  std::string logtime = (std::string)ctime(&t);
  logp << logtime << " :::  " <<  log << endl;
}

int
dumpdatabase() {
  logdaemonevent((char *)"starting dump of db\n");
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

int main(int argc, char **argv){
  struct termios newtio;
  struct sigaction saio;
  pid_t pid, sid;
  sigset_t st;
  
  (void) signal(SIGINT, cleanup);

  char logentry[256];
	
  opendaemonlog(); //this could make the app fail if there is no log file, watch stdout
	
  load(argc, argv);
  
  dumpdatabase();
	
  pid = fork();
   
  if(pid < 0) {	logdaemonevent((char *)"failed to fork daemon event handler process."); exit(EXIT_FAILURE); }
  if(pid > 0) {	logdaemonevent((char *)"forked daemon event handler process."); exit(EXIT_SUCCESS); }
  
  umask(0);
   
  sid = setsid();
  if (sid < 0) { logdaemonevent((char *)"sid of daemon event handler less than 0."); exit(EXIT_FAILURE);}
  if((chdir("/")) < 0) { logdaemonevent((char *)"daemon event handler: could not chdir to /."); exit(EXIT_FAILURE);}
   
  close(STDIN_FILENO); close(STDOUT_FILENO); close(STDERR_FILENO);
   
  /* open the device to be non-blocking (read will return immediately) */
  fd = open(device.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
  if(fd<0) { logdaemonevent((char *)"failed to open serial device."); perror(device.c_str()); exit(EXIT_FAILURE); }
  
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

