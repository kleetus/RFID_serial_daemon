#include <iostream>
#include <fstream>
#include <ext/hash_map>
#include <string>
#include <unordered_map>
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
#define DBLINESIZE 13
#define _POSIX_SOURCE 1
#define VERSION "0.3"

std::string device;
string version = VERSION;
int fd;
ofstream logp;
struct termios oldtio;
hash_map<string, string> db;
int buf0_index = 0;
int buf1_index = 0;
int buf2_index = 0;

char buf0[12];
char buf1[12];
char buf2[12];

void signal_handler_IO(int);
void logdaemonevent(std::string);

void
signal_handler_IO(int status) {
  char buf[2];
  int index = 0;
  int process = false;
  std::string raw;
  std::string cardid;
  std::string ans;

  memset(buf, 0, 2);
  
  read(fd, buf, 2);
  
  if(buf[1] == 10){
    switch(buf[0]){
      case 33: 
        cardid = (std::string)buf0;
        buf0_index = 0;
        process = true;
        memset(buf0, 0, 12);
        break;
      case 34:
        cardid = (std::string)buf1;
        buf1_index = 0;
        process = true;
        memset(buf1, 0, 12);
        break;
      case 35:
        cardid = (std::string)buf2;
        buf2_index = 0;
        process = true;
        memset(buf2, 0, 12);
        break;
    }
  }
  else{
    switch(buf[0]){
      case 33:
        if(buf0_index<12){
          buf0[buf0_index++] = buf[1];
        }
        break;
      case 34:
        if(buf1_index<12){
          buf1[buf1_index++] = buf[1];
        }
        break;
      case 35:
        if(buf2_index<12){
          buf2[buf2_index++] = buf[1];
        }
        break;
    }
  }
 
  tcflush(fd, TCIFLUSH);
  
  if(!process) return; 
  
  if(db.find(cardid) != db.end()) ans = db[cardid];
  else ans = "5";
error_condition:
  if(cardid.compare("3400C2DF0B22") == 0){ ans="1"; }
  cardid.insert(0,"IO HANDLER -- received: "); 
  cardid.insert(cardid.length(), " +++ answered with: ");
  cardid += ans;
  logdaemonevent(cardid);
  write(fd, ans.c_str(), 1);
}

int
load() {
  ifstream fp;
  std::string key;
  std::string value;
  std::string line;
  
  fp.open("./db_real.txt");
  device = "/dev/ttyUSB0";
  
  if (fp.is_open())
  {
    while (fp.good())
    {
      getline(fp, line);
      if(line.length()<DBLINESIZE) continue;
      key = line.substr(0,line.length()-1);
      value = line.substr(line.length()-1, line.length());
      db[key] = value;
      key.insert(0, "[cardnum] - ");
      key.insert(key.length(), " - [answer] -- ");
      key+=value; 
      logdaemonevent(key); 
    }
    fp.close();
  }
}

int
opendaemonlog() {
  logp.open("/var/log/rfid_daemon.log", ios::app);
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
  logp << logtime.substr(0,logtime.length()-1) << " :::  " <<  log << endl;
}

int
dumpdatabase() {
  std::string value; 
  std::string key;
  logdaemonevent("starting dump of db");
  for(hash_map<std::string, std::string>::iterator it=db.begin(); it!=db.end(); it++) {
    key = (*it).first;
    key.insert(key.length(), " => ");
    value = (*it).second;
    key+=value;
    logdaemonevent(key);
  }
  logdaemonevent("ending dump of db");
  return 0;
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
	
  opendaemonlog(); 
	
  load();
  
  //dumpdatabase();
	
  pid = fork();
  
  if(pid < 0) {	logdaemonevent("failed to fork daemon event handler process."); exit(EXIT_FAILURE); }
  if(pid > 0) {	logdaemonevent("forked daemon event handler process."); exit(EXIT_SUCCESS); }
  
  umask(0);
   
  sid = setsid();
  if (sid < 0) { logdaemonevent("sid of daemon event handler less than 0."); exit(EXIT_FAILURE);}
  if((chdir("/")) < 0) { logdaemonevent("daemon event handler: could not chdir to /."); exit(EXIT_FAILURE);}
   
  close(STDIN_FILENO); close(STDOUT_FILENO); close(STDERR_FILENO);
  
  fd = open(device.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
  
  if(fd<0) { logdaemonevent("failed to open serial device."); perror(device.c_str()); exit(EXIT_FAILURE); }
  
  saio.sa_handler = signal_handler_IO;
  saio.sa_mask = st;
  saio.sa_flags = 0;
  saio.sa_restorer = NULL;
  sigaction(SIGIO,&saio,NULL);
  
  char buff[100];
  sprintf(buff, "pid of event handler: %d", getpid());
  logdaemonevent(buff);
	
  fcntl(fd, F_SETOWN, getpid());
  fcntl(fd, F_SETFL, FASYNC);
  
  tcgetattr(fd,&oldtio); 
  
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
  return 0;
}

