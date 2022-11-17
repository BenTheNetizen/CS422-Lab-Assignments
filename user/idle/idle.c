#include <proc.h>
#include <stdio.h>
#include <syscall.h>
#include <x86.h>

// #define TEST

int main(int argc, char **argv)
{
    printf("idle\n");

    pid_t fstest_pid;

#ifdef TEST
    if ((fstest_pid = spawn(4, 1000)) != -1)
        printf("fstest in process %d.\n", fstest_pid);
    else
        printf("Failed to launch fstest.\n");
#else
    if ((fstest_pid = spawn(5, 1000)) != -1)
        printf("fstest in process %d.\n", fstest_pid);
    else
        printf("Failed to launch fstest.\n");
#endif

    return 0;
}
