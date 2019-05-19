#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "idc.h"

extern struct global libline;

int pin[2];
int pout[2];

char channel[256];

void line_handler(struct shit *me,char *line) {
  char tmp[1024];
  if(!strncmp(line,"PING ",5)) {
    printf("PONG :%s\r\n",line+5);
    fflush(stdout);
  }
  if(*line == ':') {
    if(strchr(line,' ')) {
      if(!strncmp(strchr(line,' ')," 376 ",5)) {
        printf("JOIN %s\r\n",channel);
        fflush(stdout);
      }
      if(!strncmp(strchr(line,' ')," PRIVMSG ",9)) {
        if(strchr(line+1,':')) {
          snprintf(tmp,sizeof(tmp),"%s\n",strchr(line+1,':')+1);
          write(pin[1],tmp,strlen(tmp));
        }
      }
    }
  }
  fprintf(stderr,"%s\n",line);
}

void line_handler2(struct shit *me,char *line) {
  printf("PRIVMSG %s :%s\r\n",channel,line);
}

int main(int argc,char *argv[]) {
  setlinebuf(stdout);
  printf("NICK revbot\r\n");
  printf("USER asdf asdf asdf :asdf\r\n");
  fflush(stdout);
  strcpy(channel,argv[1]);
  argv+=2;
  int i=0;
  for(i=0;i<100;i++) {
    libline.fds[i].fd=-1;
  }
  libline.shitlen=0;
  pipe(pin);
  pipe(pout);
  if(!fork()) {
    close(0);
    close(1);
    dup2(pin[0],0);
    dup2(pout[1],1);
    execv(argv[0],argv);
  }
  add_fd(0,line_handler);
  add_fd(pout[0],line_handler2);
  select_on_everything();
}
