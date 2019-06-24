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

struct idc_global idc;

int update_shits() {
  //loop over all shits and find the maxfd
  int i;
  for(i=0;i < idc.shitlen;i++) {
    idc.fdmax = idc.fds[i].fd>idc.fdmax ? idc.fds[i].fd : idc.fdmax;
  }
  return 0;
}

int add_fd(int fd,void (*line_handler)(struct shit *,char *)) {
  int i;
  for(i=0;idc.fds[i].fd != -1 && i <= idc.shitlen;i++);//we're going to use the index of the first -1 or last spot.
  fprintf(stderr,"adding fd: %d to i: %d\n",fd,i);
  if(idc.fds[i].fd != -1) {
    fprintf(stderr,"I fucked up somehow.\n");
  }
  idc.fds[i].fd=fd;
  //idc.fds[i+1].fd=-1;//not always true! we could be inserting to spot 0 while spot 1 is still valid.
  idc.fds[i].backlog=malloc(CHUNK);
  memset(idc.fds[i].backlog,0,CHUNK);
  memset(idc.fds[i].buffer,0,CHUNK);
  idc.fds[i].blsize=CHUNK;
  idc.fds[i].bllen=0;//CHUNK;
  idc.fds[i].read_lines_for_us=1;//default to reading lines for the fd
  idc.fds[i].line_handler=line_handler;
  idc.shitlen = i >= idc.shitlen ? i+1 : idc.shitlen+1 ;
  idc.fds[idc.shitlen].fd=-1;//this should always work to pad the end with a -1 even though the end should stop sooner.
  update_shits();
  return i;//return the index we used so we can add more stuff to the struct
}

int rm_fd(int fd) {
  int i;
  for(i=0;idc.fds[i].fd != fd && i <= idc.shitlen;i++);//loop until found or at end
  if(idc.fds[i].fd != i) return -1;//fd wasn't found
  idc.fds[i].fd=-1;//good enough probably. maybe free()? probably free()
  free(idc.fds[i].backlog);
  idc.fds[i].backlog=0;
//  free(idc.fds[i].buffer);
  idc.fds[i].buffer[0]=0;
  idc.fds[i].blsize=0;
  idc.fds[i].bllen=0;
  idc.fds[i].line_handler=0;
  if(i == idc.shitlen-1) idc.shitlen--;
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
  int at_least_one;
//  FILE *tmpfp;
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
    at_least_one=0;
    for(i=0;i <= idc.shitlen;i++) {
      if(idc.fds[i].fd != -1) {
        at_least_one++;
        fprintf(stderr,"fd:%d ",idc.fds[i].fd);
        FD_SET(idc.fds[i].fd,&master);
      }
    }
    fprintf(stderr,"\n");
    if(!at_least_one) return 0;//we have nothing else to possibly do.
    //  recalc_shit=0;
    //}
    readfs=master;
//  timeout.tv_sec=0; //going to leave these three lines if I change my mind about acting like snow-white
//  timeout.tv_usec=1000;
//  if((j=select(fdmax+1,&readfs,0,0,&timeout)) == -1 ) {
    fprintf(stderr,"about to select\n");
    if((j=select(idc.fdmax+1,&readfs,0,0,NULL)) == -1 ) {//we want to select-sleep as long as possible.
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
    for(i=0;i < idc.shitlen && j>0;i++) {
      if(idc.fds[i].fd == -1) continue;//skip -1s
      if(!FD_ISSET(idc.fds[i].fd,&readfs)) continue;//did not find one. hurry back to the for loop
      j--;//we found one. trying to get j==0 so we can get out of here early.
      if(idc.fds[i].read_lines_for_us == 0) {
        idc.fds[i].line_handler(&idc.fds[i],"");//the line pointer is null. which also is EOF? we'll know what it means.
        continue;//we don't need to read the line.
      }
      fprintf(stderr,"attempting to read from fd: %d\n",idc.fds[i].fd);
      if((n=read(idc.fds[i].fd,idc.fds[i].buffer,CHUNK)) < 0) {
        snprintf(tmp,sizeof(tmp)-1,"fd %d: read perror:",idc.fds[i].fd);//hopefully this doesn't error and throw off error messages.
        perror(tmp);
        return 2;
      }
      fprintf(stderr,"read %d bytes from fd: %d\n",n,idc.fds[i].fd);
      if(n == 0) {
        fprintf(stderr,"reached EOF on fd: %d\n",idc.fds[i].fd);
        if(idc.fds[i].keep_open) {
          //tmpfp=fdopen(idc.fds[i].fd,"r");
          //lseek(idc.fds[i].fd,SEEK_SET,0);
          //clearerr(tmpfp);
          //fuck if I know...
        } else {
         //we need some way to keep it open on EOF.
          if(idc.fds[i].line_handler) idc.fds[i].line_handler(&idc.fds[i],0);//dunno
          idc.fds[i].fd=-1;//kek
          continue;
         //return 3;
        }
      }
      if(idc.fds[i].bllen+n > idc.fds[i].blsize) {//this is probably off...
        t=malloc(idc.fds[i].blsize+n);
        if(!t) exit(253);
        memcpy(t,idc.fds[i].backlog,idc.fds[i].blsize);
        idc.fds[i].blsize+=n;
        free(idc.fds[i].backlog);
        idc.fds[i].backlog=t;
      }
      memcpy(idc.fds[i].backlog+idc.fds[i].bllen,idc.fds[i].buffer,n);
      idc.fds[i].bllen+=n;
      while((t=memchr(idc.fds[i].backlog,'\n',idc.fds[i].bllen))) {//no. backlogs aren't nulled.
        line=idc.fds[i].backlog;
        if(*(t-1) == '\r') {
          t--;
          hack=2;
        } else {
          hack=1;
        }
        *t=0;//we need a way to undo this if the line wasn't eaten.
//        fprintf(stderr,"libidc: line for %d: %s\n",idc.fds[i].fd,line);
        if(idc.fds[i].line_handler) {
          fprintf(stderr,"libidc: about to line_handler for %d\n",idc.fds[i].fd);
          idc.fds[i].line_handler(&idc.fds[i],line);
          fprintf(stderr,"libidc: back from line_handler for %d\n",idc.fds[i].fd);
          idc.fds[i].bllen-=((t+hack)-idc.fds[i].backlog);
          if(idc.fds[i].bllen <= 0) idc.fds[i].bllen=0;
          else memmove(idc.fds[i].backlog,(t+hack),idc.fds[i].bllen);
          //} else {//if the line handler didn't eat the line we should restore it back to original.
          // if(hack == 2) {*t='\r'; t++;}
          // *t='\n';
          //}
        }
      }//end of looping over each line in backlog
    }//end of the loop over every select()d fd
  }//end of infinite loop
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
