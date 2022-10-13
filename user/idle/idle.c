#include <proc.h>
#include <stdio.h>
#include <syscall.h>
#include <x86.h>

int main(int argc, char **argv)
{
    printf("idle\n");

    pid_t ping_pid, pong_pid, ding_pid, fork_test_pid;

    if ((fork_test_pid = spawn(4, 1000)) != -1)
        printf("idle.c: fork_test in process %d.\n", fork_test_pid);
    else
        printf("idle.c: Failed to launch fork_test.\n");
        
    return 0;
}
