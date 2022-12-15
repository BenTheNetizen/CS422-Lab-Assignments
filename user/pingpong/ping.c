#include <proc.h>
#include <stdio.h>
#include <syscall.h>
#include <signal.h>

extern void hswitch(uintptr_t handler);

void handler_wrapper(uintptr_t handler){
    hswitch(handler);
}

void sig_handler(int signo) {
    printf("INSIDE HANDLER\n");
    if (signo == SIGKILL) {
        printf("ping: received SIGKILL.\n");
    }
}

int main(int argc, char **argv)
{
    register_wrapper(handler_wrapper);
    if (signal(SIGKILL, sig_handler) == -1) {
        printf("ping: failed to register signal handler.\n");
    } else {
        printf("ping: registered signal handler for signal %d.\n", SIGKILL);
    }

    if (signal(SIGINT, sig_handler) == -1) {
        printf("ping: failed to register signal handler.\n");
    } else {
        printf("ping: registered signal handler for signal %d.\n", SIGINT);
    }

    // kill(4, SIGKILL); // send SIGKILL to pong
    while(1) {
    }
    return 0;
}
