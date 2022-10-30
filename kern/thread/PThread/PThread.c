#include <lib/x86.h>
#include <lib/thread.h>
#include <lib/spinlock.h>
#include <lib/debug.h>
#include <dev/lapic.h>
#include <pcpu/PCPUIntro/export.h>

#include "import.h"

spinlock_t tqueue_lock[NUM_CPUS];
void thread_init(unsigned int mbi_addr)
{
    for (int i = 0; i < NUM_CPUS; i++) {
        spinlock_init(&tqueue_lock[i]);
    }
    tqueue_init(mbi_addr);
    set_curid(0);
    tcb_set_state(0, TSTATE_RUN);
}

/**
 * Allocates a new child thread context, sets the state of the new child thread
 * to ready, and pushes it to the ready queue.
 * It returns the child thread id.
 */
unsigned int thread_spawn(void *entry, unsigned int id, unsigned int quota)
{
    unsigned int pid; 
    spinlock_acquire(&tqueue_lock[get_pcpu_idx()]);
    pid = kctx_new(entry, id, quota);
    if (pid != NUM_IDS) {
        tcb_set_cpu(pid, get_pcpu_idx());
        tcb_set_state(pid, TSTATE_READY);
        tqueue_enqueue(NUM_IDS + get_pcpu_idx(), pid);
    }
    spinlock_release(&tqueue_lock[get_pcpu_idx()]);
    return pid;
}

/**
 * Yield to the next thread in the ready queue.
 * You should set the currently running thread state as ready,
 * and push it back to the ready queue.
 * Then set the state of the popped thread as running, set the
 * current thread id, and switch to the new kernel context.
 * Hint: If you are the only thread that is ready to run,
 * do you need to switch to yourself?
 */
void thread_yield(void)
{
    unsigned int cpu_idx = get_pcpu_idx();
    int spinlock_status = spinlock_try_acquire(&tqueue_lock[cpu_idx]);
    // spinlock_try_acquire returns 1 if acquire unsuccessful (the lock is held by another CPU)
    if (spinlock_status == 1) return;
    unsigned int new_cur_pid;
    unsigned int old_cur_pid = get_curid();

    tcb_set_state(old_cur_pid, TSTATE_READY);
    tqueue_enqueue(NUM_IDS + get_pcpu_idx(), old_cur_pid);

    new_cur_pid = tqueue_dequeue(NUM_IDS + get_pcpu_idx());
    tcb_set_state(new_cur_pid, TSTATE_RUN);
    set_curid(new_cur_pid);

    if (old_cur_pid != new_cur_pid) {
        spinlock_release(&tqueue_lock[cpu_idx]);
        kctx_switch(old_cur_pid, new_cur_pid);
    } else {
        spinlock_release(&tqueue_lock[cpu_idx]);
    }
}
