#include <proc.h>
#include <stdio.h>
#include <syscall.h>
#include <x86.h>

const int STATE_COMMAND = 0;
const int STATE_FLAG = 1;
const int STATE_PARAM = 2;
const int STATE_PARSE = 4;
const int STATE_INIT = 5;

void parse(char* buff, char* command, char* flag, char* param1, char* param2) {
  int STATE = STATE_INIT;
  char *init_command = command;
  char *init_flag = flag;
  char *init_param1 = param1;
  char *init_param2 = param2;
  char *curr_command = command;
  char *curr_flag = flag;
  char *curr_param1 = param1;
  char *curr_param2 = param2;

  int has_param1 = 0;
  int has_param2 = 0;
  int has_flag = 0;

  while (*buff != '\0') {
    switch (STATE) {
      case STATE_INIT:
        if (*buff != ' ') {
          *curr_command = *buff;
          curr_command++;
          STATE = STATE_COMMAND;
        }
        break;
      case STATE_PARSE:
        if (*buff == '-' && !has_flag) {
          *curr_flag= *buff;
          curr_flag++;
          STATE = STATE_FLAG;
        } else if (*buff != ' ') {
          if (!has_param1) {
            *curr_param1= *buff;
            curr_param1++;
          } else {
            *curr_param2 = *buff;
            curr_param2++;
          }
          STATE = STATE_PARAM;
        }
        break;
      case STATE_COMMAND:
        if (*buff == ' ') {
          // *command = '\0';
          STATE = STATE_PARSE;
        } else {
          *curr_command= *buff;
          curr_command++;
        }
        break;
      case STATE_FLAG:
        if (*buff == ' ') {
          // *flag = '\0';
          has_flag = 1;
          STATE = STATE_PARSE;
        } else {
          *curr_flag = *buff;
          curr_flag++;
        }
        break;
      case STATE_PARAM:
        if (*buff == ' ') {
          if (!has_param1) {
            // *param1 = '\0';
            has_param1 = 1;
          } else if (!has_param2) {
            // *param2 = '\0';
            has_param2 = 1;
          }
          STATE = STATE_PARSE;
        } else {
          if (!has_param1) {
            *curr_param1 = *buff;
            curr_param1++;
          } else if (!has_param2) {
            *curr_param2 = *buff;
            curr_param2++;
          }
        }
        break;
    }
    buff++;
  }

  // reset pointers
  strncpy(command, init_command, strlen(init_command));
  strncpy(flag, init_flag, strlen(init_flag));
  strncpy(param1, init_param1, strlen(init_param1));
  strncpy(param2, init_param2, strlen(init_param2));

  command[strlen(init_command)] = '\0';
  flag[strlen(init_flag)] = '\0';
  param1[strlen(init_param1)] = '\0';
  param2[strlen(init_param2)] = '\0';
}

int main(int argc, char **argv)
{
  char s[10] = "prompt>";
  char buff[128];
  char command[128];
  char flag[128];
  char param1[128];
  char param2[128];
  char shell_buf[10000];
  
  chdir("~");

  while(1) {
    memset(buff, 0, 128);
    memset(command, 0, 128);
    memset(flag, 0, 128);
    memset(param1, 0, 128);
    memset(param2, 0, 128);
    readline(buff, s);

    printf("readline: %s\n", (char*)buff);

    parse(buff, command, flag, param1, param2);

    printf("parsed: command: %s, flag: %s, param1: %s, param2: %s\n", command, flag, param1, param2);

    if (strncmp(command, "mkdir", strlen("mkdir")) == 0){
      if (mkdir(param1) != 0) printf("mkdir failed\n");
    }
    else if (strncmp(command, "cd", strlen("cd")) == 0){
      if (chdir(param1) != 0 ) printf("cd failed\n");
    }
    else if (strncmp(command, "pwd", strlen("pwd")) == 0){
      pwd(shell_buf);
      printf("%s\n", shell_buf);
      memset(shell_buf, 0, 10000);
    }
    else if (strncmp(command, "ls", strlen("ls")) == 0){
      ls(shell_buf, param1);
      printf("%s\n", shell_buf);
      memset(shell_buf, 0, 10000);
    }
    else if (strncmp(command, "cat", strlen("cat")) == 0){
      int fd = sys_open(param1, O_RDONLY);
      if (fd==-1) {
        printf("file does not exist\n");
        continue;
      }
      int read_size = sys_read(fd, shell_buf, sizeof(shell_buf) - 1);
      shell_buf[read_size] = '\0';
      printf("%s\n", shell_buf);
      memset(shell_buf, 0, 10000);
    }
    else if (strncmp(command, "touch", strlen("touch")) == 0){
      if (touch(param1) != 0) printf("touch failed\n");
    }
    else if (strncmp(command, "write", strlen("write")) == 0){
      int fd = open(param2, O_WRONLY);
      if (fd==-1) fd = open(param2, O_CREATE | O_WRONLY);
      int write_size = sys_write(fd, param1, strlen(param1));
      if (write_size != strlen(param1)) printf("write failed\n");
      close(fd);
    }
    else if (strncmp(command, "append", strlen("append")) == 0){
      int fd = open(param2, O_RDWR);
      if (fd==-1) {
        printf("file does not exist\n");
        continue;
      }
      int read_size = sys_read(fd, shell_buf, sizeof(shell_buf) - 1);
      int idx = read_size;
      for (int i=0;i<strlen(param1);i++){
        if (idx<sizeof(shell_buf)-1) shell_buf[idx++] = param1[i];
      }
      shell_buf[idx++] = '\0';
      sys_write(fd, param1, sizeof(shell_buf));
      close(fd);
      memset(shell_buf, 0, 10000);
    }
  }
}
