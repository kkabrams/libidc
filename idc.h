#include <sys/select.h>

char *read_line_hack(FILE *fp,int len);
int update_shits();
char *memstr(char *s,char *find,size_t l);
int select_on_everything();

#define CHUNK 4096

struct shit {
  int fd;
  char *backlog;
  int blsize;
  int bllen;
  char buffer[CHUNK];//THIS IS *NOT* NULL TERMINATED.

  char *delim;

  char read_lines_for_us;
  char keep_open;
  //other stuffs?
//  union {
  void (*line_handler)(struct shit *me,char *line);//function pointer to the handler. ???
//    (void *line_handler_fd)(int fd,char *)
//  };//???
  void *extra_info;//extra info that I don't care to name atm
};

struct idc_global {
  int fdmax;
  int shitlen;
  struct shit fds[FD_SETSIZE];
};

int add_fd(int fd,void (*line_handler)(struct shit *,char *));
