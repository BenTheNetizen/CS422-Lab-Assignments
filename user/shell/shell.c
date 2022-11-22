#include <proc.h>
#include <stdio.h>
#include <syscall.h>
#include <x86.h>

#define BUFFER_SIZE 1024

const int STATE_COMMAND = 0;
const int STATE_FLAG = 1;
const int STATE_PARAM = 2;
const int STATE_PARSE = 4;
const int STATE_INIT = 5;
const int STATE_QUOTE = 6;

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
        } else if (*buff == '"') {
          STATE = STATE_QUOTE;
          break;
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
      case STATE_QUOTE:
        if (*buff == '"') {
          has_param1 = 1;
          STATE = STATE_PARSE;
        } else {
          if (!has_param1) {
            *curr_param1 = *buff;
            curr_param1++;
          }
        }
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

int is_dir(char *filename);
int does_file_exist(char *filename);

void get_filename(char* path, char*filename) {
  int n = strlen(path);
  if (n == 0) {
    return;
  }
  int i = n-1;
  while (i >= 0 && path[i] != '/') {
    i--;
  }
  strncpy(filename, path+i+1, n-i-1);
}

void get_parent_path(char* path, char* parent_path) {
  int n = strlen(path);
  if (n == 0) {
    return;
  }
  int i = n-1;
  while (i >= 0 && path[i] != '/') {
    i--;
  }
  strncpy(parent_path, path, i);
}

void copy_file(char* dst, char*src) {
  // if source is a directory, need to make destination a directory
  if (is_dir(src)) {
    mkdir(dst);
    return;
  }  
  int fd = open(src, O_RDONLY);
  char buf[BUFFER_SIZE];
  read(fd, buf, BUFFER_SIZE);
  close(fd);
  fd = open(dst, O_CREATE|O_RDWR);
  write(fd, buf, strlen(buf));
  close(fd);
}

void remove_file(char *filename) {
  if (sys_unlink(filename) < 0) {
    printf("rm: cannot remove '%s': No such file or directory\n", filename);
  }
}
int does_file_exist(char *filename) {
  int fd = open(filename, O_RDONLY);
  if (fd < 0) {
    return 0;
  }
  close(fd);
  return 1;
}

// returns 1 if the file is a directory else 0
int is_dir(char *filename) {
  int fd;
  int isDir;
  if (!does_file_exist(filename)) {
    printf("is_dir: file does not exist!\n");
    return 0;
  }
  fd = open(filename, O_RDONLY);
  isDir = sys_is_dir(fd);
  close(fd);
  if (isDir == 0) {
    // printf("is_dir: not a directory\n");
    return 0;
  } else {
    return 1;
  }
}

int shell_cp_helper(char *dst, char *src, int isRecursive) {
  char path[BUFFER_SIZE];
  char filename[BUFFER_SIZE];
  // char dst_buf[BUFFER_SIZE];
  // char src_buf[BUFFER_SIZE];
  char *p;
  if (!does_file_exist(src)) {
    printf("shell_cp_helper: file does not exist!\n");
    return 0;
  }

  if (isRecursive == 0) {
    if (is_dir(src)) {
      printf("shell_cp_helper: cannot copy directory without -r flag!\n");
      return 0;
    }

    // src is a file
    if (does_file_exist(dst) && is_dir(dst)) {
      // dst is a directory
      get_filename(src, filename);
      strcpy(path, dst);
      p = path + strlen(path);
      *p++ = '/';
      strcpy(p, filename);
      shell_cp_helper(path, src, isRecursive);
    } else {
      // dst is a file or DNE
      copy_file(dst, src);
    }
  } else {
    // recursive flag on
    if (is_dir(src)) {
      if (does_file_exist(dst)) {
        if (is_dir(dst)) {
          // dst is a directory
          get_filename(src, filename);
          strcpy(path, dst);
          p = path + strlen(path);
          *p++ = '/';
          strcpy(p, filename);
          shell_cp_helper(path, src, isRecursive);
        } else {
          // dst is a file
          printf("shell_cp_helper: cannot copy directory to file!\n");
          return 0;
        }
      } else {
        // dst DNE
        copy_file(dst, src);
        char ls_buff[BUFFER_SIZE];
        // char leaf_buff[BUFFER_SIZE];
        int num_dir_elements = ls(ls_buff, src);
        // int ls_buff_idx = 0;
        // int leaf_buff_idx = 0;

        // printf("ls_buff: %s\n", ls_buff);
        // printf("curr path: %s\n", path);
        // printf("num_dir_elements in src %s: %d\n", src, num_dir_elements);

        /*
          loop over character in ls_buff
          append ls_buff[ls_buff_idx] to leaf_buff until see \t
          then run shell_cp_helper on get_parent_path(src) + leaf_buff
        */
        // while (ls_buff[ls_buff_idx] != '\0') {
        //   if (ls_buff[ls_buff_idx] == '\t') {
        //     leaf_buff[leaf_buff_idx] = '\0';

        //     char parent_path[BUFFER_SIZE];
        //     get_parent_path(src, parent_path);
        //     printf("parent_path: %s, leaf_buff: %s\n", parent_path, leaf_buff);
        //     // set path equal to concatenation of parent_path and leaf_buff
        //     for (int i = 0; i < strlen(parent_path); i++) {
        //       path[i] = parent_path[i];
        //     }

        //     path[strlen(parent_path)] = '/';

        //     for (int i = 0; i < strlen(leaf_buff); i++) {
        //       path[strlen(parent_path)+1+i] = leaf_buff[i];
        //     }
        //     path[strlen(parent_path)+1+strlen(leaf_buff)] = '\0';
        //     // we now need to run the shell_cp_helper
        //     shell_cp_helper(dst, path, isRecursive);
        //   } else {
        //     leaf_buff[leaf_buff_idx] = ls_buff[ls_buff_idx];
        //     leaf_buff_idx++;
        //   }
        //   ls_buff_idx++;
        // }
      }
    } else {
      // src is a file 
      shell_cp_helper(dst, src, 0);
    }
  }

  return 0;
}

void shell_rm_helper(char *path, int isRecursive) {
  remove_file(path);
  // if (isRecursive) {
  //   // remove file is simple
  //   if (!is_dir(path)) {
  //     remove_file(path);
  //     return;
  //   }

  //   // path is a non empty directory

  // }
}

void shell_mv(char *dst, char *src) {
  shell_cp_helper(dst, src, 1);
  sys_unlink(src);
  // just copy src into dest, then remove src
  // shell_cp_helper(dst, src, 0);
  // shell_rm_helper(src, 0);
}
void run_cmd(char *buff) {
  char command[BUFFER_SIZE];
  char flag[BUFFER_SIZE];
  char param1[BUFFER_SIZE];
  char param2[BUFFER_SIZE];
  char shell_buf[10000];

  memset(command, 0, BUFFER_SIZE);
  memset(flag, 0, BUFFER_SIZE);
  memset(param1, 0, BUFFER_SIZE);
  memset(param2, 0, BUFFER_SIZE);

  // printf("readline: %s\n", (char*)buff);

  parse(buff, command, flag, param1, param2);

  // printf("parsed: command: %s, flag: %s, param1: %s, param2: %s\n", command, flag, param1, param2);

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
    if (ls(shell_buf, param1) == -1) {
      printf("cannot access '%s': no such file or directory\n", param1);
    } else {
      printf("%s\n", shell_buf);
    }
    memset(shell_buf, 0, 10000);
  }
  else if (strncmp(command, "cat", strlen("cat")) == 0){
    int fd = sys_open(param1, O_RDONLY);
    if (fd==-1) {
      printf("file does not exist\n");
      return;
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
      return;
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
  else if (strncmp(command, "mv", strlen("mv")) == 0) {
    char *src = param1;
    char *dest = param2;
    shell_mv(dest, src);
  }
  else if (strncmp(command, "isDir", strlen("isDir")) == 0) {
    if (is_dir(param1)) {
      printf("isDir: %s is a directory\n", param1);
    } else {
      printf("isDir: %s is not a directory\n", param1);
    }
  } else if (strncmp(command, "cp", strlen("cp")) == 0) {
    char *src = param1;
    char *dest = param2;
    if (strncmp(flag, "-r", strlen("-r")) == 0) {
      shell_cp_helper(dest, src, 1);
    } else {
      shell_cp_helper(dest, src, 0);
    }
  } else if (strncmp(command, "get_filename", strlen("get_filename")) == 0) {
    char filename[100];

    get_filename(param1, filename);
    printf("get_filename: %s\n", filename);
  } else if (strncmp(command, "get_parent", strlen("get_parent")) == 0) {
    char parent[100];

    get_parent_path(param1, parent);
    printf("get_parent: %s\n", parent);
    memset(parent, 0, 100);
  } else if (strncmp(command, "rm", strlen("rm")) == 0) {
    if (strncmp(flag, "-r", strlen("-r")) == 0) {
      shell_rm_helper(param1, 1);
    } else {
      shell_rm_helper(param1, 0);
    }
  }
}

void shell_test() {
  printf("--------------------TESTING SHELL--------------------\n");
  char* cmds[6] = {
    "mkdir test", "touch testfile.txt", "write hello testfile.txt", "cat testfile.txt", "append world testfile.txt", "cat testfile.txt"
  };

  for (int i = 0; i < 6; i++) {
    run_cmd(cmds[i]);
  }

  printf("--------------------TESTING SHELL FINISHED--------------------\n");
}
int main(int argc, char **argv)
{
  char s[10] = "prompt>";
  char buff[BUFFER_SIZE];
  
  /* MODES 
    0 = normal
    1 = test
  */
  int shell_mode = 0;
  
  chdir(".");

  if (shell_mode == 1) {
    shell_test();
  } else if (shell_mode == 0) {
    while(1) {
      memset(buff, 0, BUFFER_SIZE);
      readline(buff, s);
      run_cmd(buff);
    }
  }
}
