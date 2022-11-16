#include <proc.h>
#include <stdio.h>
#include <syscall.h>
#include <x86.h>

const int STATE_LS = 0;
const int STATE_PWD = 1;
const int STATE_CD = 2;
const int STATE_CP = 3;
const int STATE_MV = 4;
const int STATE_RM = 5;
const int STATE_MKDIR = 6;
const int STATE_CAT = 7;
const int STATE_TOUCH = 8;
const int STATE_PARSE = 9;


int main(int argc, char **argv)
{
  int STATE = STATE_PARSE;
  printf("shell\n");
  char *s = "prompt>";
  char buff[128];
  readline(buff, s);

  printf("readline: %s\n", buff);

  unsigned int index = 0;
  char curr[128];
  switch(STATE) {
    case STATE_PARSE:
      
  }
}
