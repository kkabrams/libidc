//what data type to use for storing all of the shit?
//hashtable with hash == fd?

//should I use FILE *, or fd, or write functions for both?

#define _GNU_SOURCE //I want memmem out of string.h
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

//#define CHUNK 4096
#include "idc.h"

#define TSIZE 65536

/* might not put this function in here... or maybe I will, but not use it.
char *read_line_hack(FILE *fp,int len) {
 short in;
 char *t;
 errno=0;
 switch(in=fgetc(fp)) {
  case '\n':
   t=malloc(len+1);
   t[len]=0;
   return t;
  case -1:
   if(errno == EAGAIN) return 0;
   if(feof(fp)) {
    fprintf(stderr,"# reached EOF. exiting.\n");
    exit(0);
   }
   fprintf(stderr,"# some other error happened while reading. %d %d\n",EAGAIN,errno);
   perror("hackvr");
   exit(1);
  default: //we need to check the last strlen(delim) bytes for delim to indicate end of line-read.
   if((t=read_line_hack(fp,len+1))) t[len]=in;
   break;
 }
 return t;
}
*/


/* in line.h
struct shit {
  //I need to choose which of these two.
  //FILE *fp;
  int fd;
  char *backlog;
  int blsize;
  int bllen;
  char buffer[CHUNK];//THIS IS *NOT* NULL TERMINATED.

  char *delim;
  //other stuffs?
//  union {
  void (*line_handler)(struct shit *me,char *line);//function pointer to the handler. ???
//    (void *line_handler_fd)(int fd,char *)
//  };//???
  void *extra_info;//extra info that I don't care to name atm
};
*/

struct global libline;

int update_shits() {
  //loop over all shits and find the maxfd
  int i;
  for(i=0;i < libline.shitlen;i++) {
    libline.fdmax = libline.fds[i].fd>libline.fdmax ? libline.fds[i].fd : libline.fdmax;
  }
  return 0;
}

int add_fd(int fd,void (*line_handler)(struct shit *,char *)) {
  int i;
  for(i=0;libline.fds[i].fd != -1 && i <= libline.shitlen;i++);//we're going to use the index of the first -1 or last spot.
  fprintf(stderr,"adding fd: %d to i: %d\n",fd,i);
  if(libline.fds[i].fd != -1) {
    fprintf(stderr,"I fucked up somehow.\n");
  }
  libline.fds[i].fd=fd;
  //libline.fds[i+1].fd=-1;//not always true! we could be inserting to spot 0 while spot 1 is still valid.
  libline.fds[i].backlog=malloc(CHUNK);
  memset(libline.fds[i].backlog,0,CHUNK);
  memset(libline.fds[i].buffer,0,CHUNK);
  libline.fds[i].blsize=CHUNK;
  libline.fds[i].bllen=0;//CHUNK;
  libline.fds[i].read_lines_for_us=1;//default to reading lines for the fd
  libline.fds[i].line_handler=line_handler;
  libline.shitlen = i >= libline.shitlen ? i+1 : libline.shitlen+1 ;
  libline.fds[libline.shitlen].fd=-1;//this should always work to pad the end with a -1 even though the end should stop sooner.
  update_shits();
  return i;//return the index we used so we can add more stuff to the struct
}

int rm_fd(int fd) {
  int i;
  for(i=0;libline.fds[i].fd != fd && i <= libline.shitlen;i++);//loop until found or at end
  if(libline.fds[i].fd != i) return -1;//fd wasn't found
  libline.fds[i].fd=-1;//good enough probably. maybe free()? probably free()
  free(libline.fds[i].backlog);
  libline.fds[i].backlog=0;
//  free(libline.fds[i].buffer);
  libline.fds[i].buffer[0]=0;
  libline.fds[i].blsize=0;
  libline.fds[i].bllen=0;
  libline.fds[i].line_handler=0;
  if(i == libline.shitlen-1) libline.shitlen--;
  return fd;
}

//functions to add:
/*
main_looper()
add_fd_and_handler()?
remove_fd_and_handler()?
*/


#define SILLYLIMIT 1024

char *memstr(char *s,char *find,size_t l) {
 return memmem(s,l,find,strlen(find));
}

int select_on_everything() {
  int hack;
  FILE *tmpfp;
  fd_set master;
  fd_set readfs;
//  struct timeval timeout;
//  int fdmax=0,n,i;
  int n,i,j;
  char tmp[256];
  char *t,*line=0;
  FD_ZERO(&master);
  FD_ZERO(&readfs);
  for(;;) { //at the start of each loop we'll need to recalculate some stuff if there was a change.
    fprintf(stderr,"in mainloop\n");
    //if(recalc_shit) { //this is set by anything changing the table of descriptors
    fprintf(stderr,"building master: ");
    FD_ZERO(&master);
    for(i=0;i <= libline.shitlen;i++) {
      if(libline.fds[i].fd != -1) {
        fprintf(stderr,"fd:%d ",libline.fds[i].fd);
        FD_SET(libline.fds[i].fd,&master);
      }
    }
    fprintf(stderr,"\n");
    //  recalc_shit=0;
    //}
    readfs=master;
//  timeout.tv_sec=0; //going to leave these three lines if I change my mind about acting like snow-white
//  timeout.tv_usec=1000;
//  if((j=select(fdmax+1,&readfs,0,0,&timeout)) == -1 ) {
    if((j=select(libline.fdmax+1,&readfs,0,0,NULL)) == -1 ) {//we want to select-sleep as long as possible.
      //any reason to wake up should be a file descriptor in the list. (works for X11 events, dunno about others)
      //on error filedescriptors aren't changed
      //the value of timeout is undefined
      //so says the linux man page for select.
      if(errno == EINTR) {
        perror("asdf");
        continue;//just try again
      }
      if(errno != 0) return perror("select"),1;
      else perror("wtf? select");
      //continue;
    }
    fprintf(stderr,"after select(). ret: %d\n",j);
//  for(i=0;fds[i] != -1;i++) if(extra_handler) extra_handler(fds[i]);
    if(j == 0) continue;//don't bother to loop over them.
    for(i=0;i < libline.shitlen && j>0;i++) {
      if(libline.fds[i].fd == -1) continue;//skip -1s
      if(!FD_ISSET(libline.fds[i].fd,&readfs)) continue;//did not find one. hurry back to the for loop
      j--;//we found one. trying to get j==0 so we can get out of here early.
      if(libline.fds[i].read_lines_for_us == 0) {
        libline.fds[i].line_handler(&libline.fds[i],0);//the line pointer is null.
        continue;//we don't need to read the line.
      }
      if((n=read(libline.fds[i].fd,libline.fds[i].buffer,CHUNK)) < 0) {
        snprintf(tmp,sizeof(tmp)-1,"fd %d: read perror:",libline.fds[i].fd);//hopefully this doesn't error and throw off error messages.
        perror(tmp);
        return 2;
      }
      if(n == 0) {
        fprintf(stderr,"reached EOF on fd: %d\n",libline.fds[i].fd);
        if(libline.fds[i].keep_open) {
          tmpfp=fdopen(libline.fds[i].fd,"r");
          clearerr(tmpfp);
          //fuck if I know...
        } else {
         //we need some way to keep it open on EOF.
          libline.fds[i].fd=-1;//kek
         //return 3;
        }
      }
      if(libline.fds[i].bllen+n > libline.fds[i].blsize) {//this is probably off...
        t=malloc(libline.fds[i].blsize+n);
        if(!t) exit(253);
        memcpy(t,libline.fds[i].backlog,libline.fds[i].blsize);
        libline.fds[i].blsize+=n;
        free(libline.fds[i].backlog);
        libline.fds[i].backlog=t;
      }
      memcpy(libline.fds[i].backlog+libline.fds[i].bllen,libline.fds[i].buffer,n);
      libline.fds[i].bllen+=n;
      while((t=memchr(libline.fds[i].backlog,'\n',libline.fds[i].bllen))) {//no. backlogs aren't nulled.
        line=libline.fds[i].backlog;
        if(*(t-1) == '\r') {
          t--;
          hack=2;
        } else {
          hack=1;
        }
        *t=0;//we need a way to undo this if the line wasn't eaten.
        fprintf(stderr,"libidc: line for %d: %s\n",libline.fds[i].fd,line);
        if(libline.fds[i].line_handler) {
          libline.fds[i].line_handler(&libline.fds[i],line);
          libline.fds[i].bllen-=((t+hack)-libline.fds[i].backlog);
          if(libline.fds[i].bllen <= 0) libline.fds[i].bllen=0;
          else memmove(libline.fds[i].backlog,(t+hack),libline.fds[i].bllen);
          //} else {//if the line handler didn't eat the line we should restore it back to original.
          // if(hack == 2) {*t='\r'; t++;}
          // *t='\n';
          //}
        }
      }
    }
  }
  return 0;
}

//this function mangles the input.
//gotta free the returned pointer but not each pointer in the array.

/*
char **line_cutter(int fd,char *line,struct user *user) {
 int i;
 char **a=malloc(sizeof(char *) * 256);//heh.
 if(!a) exit(54);
 memset(a,0,sizeof(char *) * 256);
 if(!user) return 0;
 user->nick=0;
 user->user=0;
 user->host=0;
 if(!line) return 0;
 if(strchr(line,'\r')) *strchr(line,'\r')=0;
 if(strchr(line,'\n')) *strchr(line,'\n')=0;
 if(line[0]==':') {
  if((user->nick=strchr(line,':'))) {
   *(user->nick)=0;
   (user->nick)++;
  }
 }
 if(user->nick) {

  if((a[0]=strchr((user->nick),' '))) {
   *a[0]=0;
   a[0]++;
   for(i=0;(a[i+1]=strchr(a[i],' '));i++) {
    *a[i+1]=0;
    a[i+1]++;
    if(*a[i+1] == ':') {//we're done.
     *a[i+1]=0;
     a[i+1]++;
     break;
    }
   }
  }

  if(((user->user)=strchr((user->nick),'!'))) {
   *(user->user)=0;
   (user->user)++;
   if(((user->host)=strchr((user->user),'@'))) {
    *(user->host)=0;
    (user->host)++;
   }
  } else {
   user->host=user->nick;
  }
 }
 return a;
}
*/
