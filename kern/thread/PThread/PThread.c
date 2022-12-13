#include <lib/x86.h>
#include <lib/thread.h>
#include <lib/spinlock.h>
#include <lib/debug.h>
#include <lib/trap.h>
#include <dev/lapic.h>
#include <pcpu/PCPUIntro/export.h>
#include <kern/thread/PTCBIntro/export.h>

#include "import.h"

static spinlock_t sched_lk;
extern tf_t uctx_pool[NUM_IDS];
unsigned int sched_ticks[NUM_CPUS];

void thread_init(unsigned int mbi_addr)
{
    unsigned int i;
    for (i = 0; i < NUM_CPUS; i++) {
        sched_ticks[i] = 0;
    }

    spinlock_init(&sched_lk);
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

    spinlock_acquire(&sched_lk);

    pid = kctx_new(entry, id, quota);
    if (pid != NUM_IDS) {
        tcb_set_state(pid, TSTATE_READY);
        tqueue_enqueue(NUM_IDS, pid);
    }

    spinlock_release(&sched_lk);

    return pid;
}

void thread_set_signal_ctx(unsigned int pid) {
    return;
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
    unsigned int old_cur_pid;
    unsigned int new_cur_pid;

    spinlock_acquire(&sched_lk);

    old_cur_pid = get_curid();
    tcb_set_state(old_cur_pid, TSTATE_READY);
    tqueue_enqueue(NUM_IDS, old_cur_pid);

    new_cur_pid = tqueue_dequeue(NUM_IDS);
    tcb_set_state(new_cur_pid, TSTATE_RUN);
    set_curid(new_cur_pid);

    // dprintf("thread_yield: new_pid = %d, old_pid = %d\n", new_cur_pid, old_cur_pid);
    if (old_cur_pid != new_cur_pid) {
        // dprintf("IN IF BLOCK\n");
        spinlock_release(&sched_lk);
        int signal = tcb_pending_signal_pop(new_cur_pid);
        if (signal != 0) {
            sigfunc* handler = tcb_get_sigfunc(new_cur_pid, signal);
            dprintf("signal %d is handled by %p\n", signal, handler);
            if (handler) {
                dprintf("signal %d is handled by %p\n", signal, handler);
                // set user context eip to handler
                uintptr_t old_eip = uctx_pool[new_cur_pid].eip;
                dprintf("old eip is %p\n", old_eip);
                uctx_pool[new_cur_pid].eip = (uintptr_t)handler;
                // push old eip to user context esp
                // uctx_pool[new_cur_pid].esp -= 4;
                
                // c calling conventions
                // uintptr_t old_esp = uctx_pool[new_cur_pid].esp;
                // uctx_pool[new_cur_pid].esp -= 4;
                // *(uintptr_t*)uctx_pool[new_cur_pid].esp = old_eip;
                // // first parameter
                // uctx_pool[new_cur_pid].esp -= 4;
                // *(uintptr_t*)uctx_pool[new_cur_pid].esp = signal;

                // // push ebp on stack
                // uctx_pool[new_cur_pid].esp -= 4;
                // *(uintptr_t*)uctx_pool[new_cur_pid].esp = uctx_pool[new_cur_pid].regs.ebp;

                // // copy esp into ebp
                // uctx_pool[new_cur_pid].regs.ebp = old_esp;
                
            }
        }

        kctx_switch(old_cur_pid, new_cur_pid);
    }
    else {
        dprintf("IN ELSE BLOCK\n");
        spinlock_release(&sched_lk);
        int signal = tcb_pending_signal_pop(new_cur_pid);
        if (signal != 0) {
            sigfunc* handler = tcb_get_sigfunc(new_cur_pid, signal);
            dprintf("signal %d is handled by %p\n", signal, handler);
            if (handler) {
                dprintf("signal %d is handled by %p\n", signal, handler);
                // set user context eip to handler
                uintptr_t old_eip = uctx_pool[new_cur_pid].eip;
                dprintf("old eip is %p\n", old_eip);
                uctx_pool[new_cur_pid].eip = (uintptr_t)handler;
                // push old eip to user context esp
                // uctx_pool[new_cur_pid].esp -= 4;
                
                // c calling conventions
                uintptr_t old_esp = uctx_pool[new_cur_pid].esp;
                uctx_pool[new_cur_pid].esp -= 4;
                *(uintptr_t*)uctx_pool[new_cur_pid].esp = old_eip;
                // first parameter
                uctx_pool[new_cur_pid].esp -= 4;
                *(uintptr_t*)uctx_pool[new_cur_pid].esp = signal;

                // push ebp on stack
                uctx_pool[new_cur_pid].esp -= 4;
                *(uintptr_t*)uctx_pool[new_cur_pid].esp = uctx_pool[new_cur_pid].regs.ebp;

                // copy esp into ebp
                uctx_pool[new_cur_pid].regs.ebp = old_esp;

            }
        }

        handler_switch(new_cur_pid);
    }
}

void sched_update(void)
{
    spinlock_acquire(&sched_lk);
    sched_ticks[get_pcpu_idx()] += 1000 / LAPIC_TIMER_INTR_FREQ;
    
    if (sched_ticks[get_pcpu_idx()] >= SCHED_SLICE) {
        sched_ticks[get_pcpu_idx()] = 0;
        spinlock_release(&sched_lk);
        thread_yield();
    }
    else {
        spinlock_release(&sched_lk);
    }
}

/**
 * Atomically release lock and sleep on chan.
 * Reacquires lock when awakened.
 */
void thread_sleep(void *chan, spinlock_t *lk)
{
    // TODO: your local variables here.
    unsigned int old_cur_pid;
    unsigned int new_cur_pid;

    if (lk == 0)
        KERN_PANIC("sleep without lock");

    // Must acquire sched_lk in order to change the current thread's state and
    // then switch. Once we hold sched_lk, we can be guaranteed that we won't
    // miss any wakeup (wakeup runs with sched_lk locked), so it's okay to
    // release lock.
    spinlock_acquire(&sched_lk);
    spinlock_release(lk);

    // Go to sleep.
    old_cur_pid = get_curid();
    new_cur_pid = tqueue_dequeue(NUM_IDS);
    KERN_ASSERT(new_cur_pid != NUM_IDS);
    tcb_set_chan(old_cur_pid, chan);
    tcb_set_state(old_cur_pid, TSTATE_SLEEP);
    tcb_set_state(new_cur_pid, TSTATE_RUN);
    set_curid(new_cur_pid);

    // Context switch.
    spinlock_release(&sched_lk);
    kctx_switch(old_cur_pid, new_cur_pid);
    spinlock_acquire(&sched_lk);

    // Tidy up.
    tcb_set_chan(old_cur_pid, NULL);

    // Reacquire original lock.
    spinlock_acquire(lk);
    spinlock_release(&sched_lk);
}

/**
 * Wake up all processes sleeping on chan.
 */
void thread_wakeup(void *chan)
{
    unsigned int pid;
    spinlock_acquire(&sched_lk);

    for (pid = 0; pid < NUM_IDS; pid++) {
        if (chan == tcb_get_chan(pid)) {
            tcb_set_state(pid, TSTATE_READY);
            tqueue_enqueue(NUM_IDS, pid);
        }
    }

    spinlock_release(&sched_lk);
}
