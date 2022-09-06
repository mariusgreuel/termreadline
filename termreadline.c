#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

#ifdef _MSC_VER
#include "msvc/unistd.h"
#else
#include <unistd.h>
#endif

#ifdef WIN32
#include <conio.h>
#endif

// Standalone readline test

typedef struct programmer_t { int hi; } PROGRAMMER;
typedef struct avrpart { int ho; } AVRPART;

static int cmd_quit(PROGRAMMER *pgm, struct avrpart *p, int argc, char * argv[]) {
  printf("quit: bye\n");
  fflush(stdout);
  rl_set_prompt(NULL);
  
  return 1;
}

static int process_line(char *cmdstr, PROGRAMMER *pgm, struct avrpart *p) {
  if(strcmp(cmdstr, "quit") == 0)
    return cmd_quit(pgm, p, 0, NULL);

  printf("working hard on %s\n", cmdstr);
  fflush(stdout);

  return 0;
}

static void cmd_keep_board_alive(PROGRAMMER *pgm) {
  // send GET_SYNC command if pgm is bootloader
  printf(".");
  fflush(stdout);
}

int terminal_mode(PROGRAMMER *pgm, struct avrpart *p);

int main() {
  PROGRAMMER mypgm = {1}; 
  AVRPART mypart = {2}; 

  int rc = terminal_mode(&mypgm, &mypart);
}

// readline() part: needs to be made portable!  ********************

static PROGRAMMER *term_pgm;
static struct avrpart *term_p;

static int term_running;

// Any character in standard input available?
static int readytoread() {
#ifdef WIN32
    return _kbhit();
#else
  struct timeval tv = { 0L, 0L };
  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(0, &fds);

  return select(1, &fds, NULL, NULL, &tv) > 0;
#endif
}

// Callback processes commands whenever readline() has finished
void term_gotline(char* cmdstr){
  if(cmdstr) {
    if(*cmdstr) {
      add_history(cmdstr);
      // only quit/abort returns a value > 0
      if(process_line(cmdstr, term_pgm, term_p) > 0)
        term_running = 0;
    }
    free(cmdstr);
  } else {
    // call quit at end of file or terminal ^D
    fflush(stderr); fflush(stdout);
    printf("\n");
    cmd_quit(term_pgm, term_p, 0, NULL);
    term_running = 0;
  }
}

int terminal_mode(PROGRAMMER *pgm, struct avrpart *p) {
  term_pgm = pgm;               // for callback routine
  term_p = p;

  rl_callback_handler_install("avrdude> ", (rl_vcpfunc_t*) &term_gotline);

  term_running = 1;
  for(int n=1; term_running; n++) {
    if(n%360 == 0)              // every 1800 ms send GET_SYNC to board
      cmd_keep_board_alive(pgm);
    usleep(5000);
    if(readytoread() && term_running)
      rl_callback_read_char();
  }

  rl_callback_handler_remove();

  return 0;
}
