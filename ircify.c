#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "line.h"

extern struct global libline;

int pin[2];
int pout[2];

void line_handler(struct shit *me,char *line) {
  char tmp[1024];
  if(!strncmp(line,"PING ",5)) {
    line[1]='O';
    printf("%s\r\n",line);
  }
  if(*line == ':') {
    if(strchr(line,' ')) {
      if(!strncmp(strchr(line,' ')," PRIVMSG ",9)) {
        if(strchr(line+1,':')) {
          snprintf(tmp,sizeof(tmp),"%s\n",strchr(line+1,':')+1);
          write(pin[1],tmp,strlen(tmp));
        }
      }
    }
  }
  fprintf(stderr,"READ SHIT: %s\n",line);
}

char channel[256];

void line_handler2(struct shit *me,char *line) {
  printf("PRIVMSG %s :%s\r\n",channel,line);
  fflush(stdout);
}

int main(int argc,char *argv[]) {
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
