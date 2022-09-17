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
#include <windows.h>
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
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);

    while (1)
    {
        INPUT_RECORD input[1] = { 0 };
        DWORD dwNumberOfEventsRead = 0;
        BOOL success = PeekConsoleInputA(hStdin, input, ARRAYSIZE(input), &dwNumberOfEventsRead);
        if (!success)
        {
            DWORD dwError = GetLastError();
            if (dwError == ERROR_INVALID_HANDLE)
            {
                // STD_INPUT_HANDLE was redirected to a pipe or file
                return 1;
            }
            else
            {
                printf("PeekConsoleInputA failed: %u\n", dwError);
                return -1;
            }
        }
        else if (dwNumberOfEventsRead > 0)
        {
            // Filter out all the events that readline does not handle...

            if ((input[0].EventType & KEY_EVENT) != 0 && input[0].Event.KeyEvent.bKeyDown)
            {
                //printf("Got event %d\n", input[0].EventType);
                return 1; // Got some.
            }
            else
            {
                //printf("Skipping event %d\n", input[0].EventType);

                // Some event not handled by readline, drain it.
                success = ReadConsoleInputA(hStdin, input, ARRAYSIZE(input), &dwNumberOfEventsRead);
                if (!success)
                {
                    DWORD dwError = GetLastError();
                    printf("ReadConsoleInputA failed: %u\n", dwError);
                    return -1;
                }
            }
        }
        else
        {
            return 0; // Nothing in the input buffer.
        }
    }
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
