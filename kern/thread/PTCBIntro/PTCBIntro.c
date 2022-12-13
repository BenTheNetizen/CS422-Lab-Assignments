#include <lib/x86.h>
#include <lib/debug.h>
#include <lib/string.h>
#include <lib/thread.h>
#include <lib/signal.h>

#include <kern/fs/params.h>
#include <kern/fs/stat.h>
#include <kern/fs/dinode.h>
#include <kern/fs/inode.h>
#include <kern/fs/path.h>
#include <kern/fs/file.h>

#define NPENDING_SIGNALS 32
/**
 * The structure for the thread control block (TCB).
 * We are storing the set of TCBs in doubly linked lists.
 * To this purpose, in addition to the thread state, you also
 * need to save the thread (process) id of the previous and
 * the next TCB.
 * Since the value 0 is reserved for thread id 0, we use NUM_IDS
 * to represent the NULL index.
 */
struct TCB {
    t_state state;
    unsigned int cpuid;
    unsigned int prev;
    unsigned int next;
    void *channel;
    struct file *openfiles[NOFILE];  // Open files
    struct inode *cwd;               // Current working directory
    sigfunc* sigfuncs[NUM_SIGNALS];   // Signal handlers
    unsigned int pending_signals[NPENDING_SIGNALS]; // Pending signals

} in_cache_line;

struct TCB TCBPool[NUM_IDS];

int tcb_pending_signal_pop(unsigned int pid)
{
    for (int i = 0; i < NPENDING_SIGNALS; i++) {
        if (TCBPool[pid].pending_signals[i] != 0) {
            dprintf("tcb_pending_signal_pop: TCBPool[%d].pending_signals[%d]: %d\n", pid, i, TCBPool[pid].pending_signals[i]);
            int result = TCBPool[pid].pending_signals[i];
            TCBPool[pid].pending_signals[i] = 0;
            dprintf("returning %d\n", result);
            return result;
        }
    }
    return 0;
}

void tcb_pending_signal_push(unsigned int pid, unsigned int signum)
{
    for (int i = 0; i < NPENDING_SIGNALS; i++) {
        if (TCBPool[pid].pending_signals[i] == 0) {
            dprintf("tcb_pending_signal_push: pushing signal %d to pid %d\n", signum, pid);
            TCBPool[pid].pending_signals[i] = signum;
            dprintf("tcb_pending_signal_push: TCBPool[%d].pending_signals[%d]: %d\n", pid, i, TCBPool[pid].pending_signals[i]);
            return;
        }
    }
}

void tcb_set_sigfunc(unsigned int pid, unsigned int signum, sigfunc* func)
{
    TCBPool[pid].sigfuncs[signum] = func;
}

sigfunc* tcb_get_sigfunc(unsigned int pid, unsigned int signum)
{
    return TCBPool[pid].sigfuncs[signum];
}

unsigned int tcb_get_state(unsigned int pid)
{
    return TCBPool[pid].state;
}

void tcb_set_state(unsigned int pid, unsigned int state)
{
    TCBPool[pid].state = state;
}

unsigned int tcb_get_cpu(unsigned int pid)
{
    return TCBPool[pid].cpuid;
}

void tcb_set_cpu(unsigned int pid, unsigned int cpu)
{
    TCBPool[pid].cpuid = cpu;
}

unsigned int tcb_get_prev(unsigned int pid)
{
    return TCBPool[pid].prev;
}

void tcb_set_prev(unsigned int pid, unsigned int prev_pid)
{
    TCBPool[pid].prev = prev_pid;
}

unsigned int tcb_get_next(unsigned int pid)
{
    return TCBPool[pid].next;
}

void tcb_set_next(unsigned int pid, unsigned int next_pid)
{
    TCBPool[pid].next = next_pid;
}

void tcb_init_at_id(unsigned int pid)
{
    TCBPool[pid].state = TSTATE_DEAD;
    TCBPool[pid].cpuid = NUM_CPUS;
    TCBPool[pid].prev = NUM_IDS;
    TCBPool[pid].next = NUM_IDS;
    TCBPool[pid].channel = 0;
    memzero(TCBPool[pid].openfiles, sizeof *TCBPool[pid].openfiles);
    memzero(TCBPool[pid].sigfuncs, sizeof *TCBPool[pid].sigfuncs);
    memzero(TCBPool[pid].pending_signals, sizeof *TCBPool[pid].pending_signals);
    TCBPool[pid].cwd = namei("/");
}

void *tcb_get_chan(unsigned int pid)
{
    return TCBPool[pid].channel;
}

void tcb_set_chan(unsigned int pid, void *chan)
{
    TCBPool[pid].channel = chan;
}

struct file **tcb_get_openfiles(unsigned int pid)
{
    return TCBPool[pid].openfiles;
}

void tcb_set_openfiles(unsigned int pid, int fd, struct file *f)
{
    TCBPool[pid].openfiles[fd] = f;
}

struct inode *tcb_get_cwd(unsigned int pid)
{
    return TCBPool[pid].cwd;
}

void tcb_set_cwd(unsigned int pid, struct inode *d)
{
    TCBPool[pid].cwd = d;
}
