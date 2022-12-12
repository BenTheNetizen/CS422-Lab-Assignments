#ifndef _USER_SIGNAL_H
#define _USER_SIGNAL_H

#include <syscall.h> 

#define SIGINT 2
#define SIGKILL 9

typedef void sigfunc(int);

// siginfo_t {
//     int si_signo;
//     int si_errno;
//     int si_code;
//     int si_pid;
//     int si_uid;
//     int si_status;
//     void *si_addr;
//     int si_value;
//     int si_int;
//     void *si_ptr;
//     int si_overrun;
//     int si_timerid;
//     void *si_addr_lsb;
// }

struct sigaction {
    void (*sa_handler)(int);
    // sigset_t sa_mask;
    int sa_flags;
    void (*sa_restorer)(void);
};

#define signal(int, sigfunc) sys_signal((int), (sigfunc))
#define kill(pid, sig)       sys_kill((pid), (sig))


#endif /* !_USER_SIGNAL_H */