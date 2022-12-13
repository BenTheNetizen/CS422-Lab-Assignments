#include <proc.h>
#include <stdio.h>
#include <syscall.h>
#include <signal.h>

void sig_handler(int signo) {
    printf("IT WORKS!!!\n");
    if (signo == SIGKILL) {
        printf("ping: received SIGKILL.\n");
    }
}

int main(int argc, char **argv)
{
    if (signal(SIGKILL, sig_handler) == -1) {
        printf("ping: failed to register signal handler.\n");
    } else {
        printf("ping: registered signal handler.\n");
    }

    kill(4, SIGKILL); // send SIGKILL to pong
    while(1) {
    }
    return 0;
}
