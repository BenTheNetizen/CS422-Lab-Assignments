#include <lib/x86.h>
#include <lib/thread.h>
#include <lib/spinlock.h>
#include <lib/debug.h>
#include <dev/lapic.h>
#include <pcpu/PCPUIntro/export.h>

#include "import.h"

#define MS_PER_LAPIC_INTR 1000/LAPIC_TIMER_INTR_FREQ

spinlock_t thread_lock[NUM_CPUS];
unsigned int elapsed[NUM_CPUS];

void thread_init(unsigned int mbi_addr)
{
    for (unsigned int i=0;i<NUM_CPUS;i++){
        elapsed[i] = 0;
        spinlock_init(&thread_lock[i]);
    }
    tqueue_init(mbi_addr);
    set_curid(0);
    tcb_set_state(0, TSTATE_RUN);
}

/**
 * Allocates a new child thread context, sets the state of the new child thread
 * to ready, and pushes pit to the ready queue.
 * It returns the child thread id.
 */
unsigned int thread_spawn(void *entry, unsigned int id, unsigned int quota)
{
    spinlock_acquire(&thread_lock[get_pcpu_idx()]);
    unsigned int pid = kctx_new(entry, id, quota);
    if (pid != NUM_IDS) {
        tcb_set_cpu(pid, get_pcpu_idx());
        tcb_set_state(pid, TSTATE_READY);
        tqueue_enqueue(NUM_IDS + get_pcpu_idx(), pid);
    }
    spinlock_release(&thread_lock[get_pcpu_idx()]);

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

    unsigned int new_cur_pid;
    unsigned int old_cur_pid = get_curid();

    int status = spinlock_try_acquire(&thread_lock[cpu_idx]);

    if (status == 1) return;

    tcb_set_state(old_cur_pid, TSTATE_READY);
    tqueue_enqueue(NUM_IDS + get_pcpu_idx(), old_cur_pid);

    new_cur_pid = tqueue_dequeue(NUM_IDS + get_pcpu_idx());
    tcb_set_state(new_cur_pid, TSTATE_RUN);
    set_curid(new_cur_pid);

    if (old_cur_pid != new_cur_pid) {
        spinlock_release(&thread_lock[cpu_idx]);
        // dprintf("[thread_yield]: switch from %d to %d\n", old_cur_pid, new_cur_pid);
        kctx_switch(old_cur_pid, new_cur_pid);
    }
    else{
        spinlock_release(&thread_lock[cpu_idx]);
    }
}

void thread_wait_helper() {
    unsigned int cpu_idx = get_pcpu_idx();

    unsigned int new_cur_pid;
    unsigned int old_cur_pid = get_curid();

    int status = spinlock_try_acquire(&thread_lock[cpu_idx]);

    if (status == 1) return;

    tcb_set_state(old_cur_pid, TSTATE_SLEEP);
    // DO NOT WANT TO QUEUE UP THIS THREAD
    //tqueue_enqueue(NUM_IDS + get_pcpu_idx(), old_cur_pid);

    new_cur_pid = tqueue_dequeue(NUM_IDS + get_pcpu_idx());
    tcb_set_state(new_cur_pid, TSTATE_RUN);
    set_curid(new_cur_pid);

    if (old_cur_pid != new_cur_pid) {
        spinlock_release(&thread_lock[cpu_idx]);
        // dprintf("[thread_yield]: switch from %d to %d", old_cur_pid, new_cur_pid);
        kctx_switch(old_cur_pid, new_cur_pid);
    }
    else{
        spinlock_release(&thread_lock[cpu_idx]);
    }
    // // turn off old thread and turn on thread off the ready queue
    // //unsigned int cpu_idx = get_pcpu_idx();
    // unsigned int cpu_idx = get_pcpu_idx();
    // cpu_idx = cpu_idx == 1 ? 2 : 1; // swap to other cpu
    // dprintf("[THREAD_WAIT_HELPER] DEBUG: cpu_idx: %ld\n", cpu_idx);
    // // CPU 1
    // unsigned int new_cur_pid;
    // unsigned int old_cur_pid = get_curid();
    // int status = spinlock_try_acquire(&thread_lock[cpu_idx]);

    // if (status == 1) {
    //     dprintf("[THREAD_WAIT_HELPER]: spinlock is already acquired\n");
    //     return;
    // }

    // tcb_set_state(old_cur_pid, TSTATE_SLEEP);
    // dprintf("[THREAD_WAIT_HELPER]: set old_cur_pid %d to sleep\n", old_cur_pid);
    // new_cur_pid = tqueue_dequeue(NUM_IDS + cpu_idx);

    // // THERE SHOULD ALWAYS AT LEAST BE AN IDLE QUEUE IN THE THREAD QUEUE
    // if (new_cur_pid == NUM_IDS) {
    //     dprintf("[THREAD_WAIT_HELPER]: no thread to run\n");
    //     spinlock_release(&thread_lock[cpu_idx]);
    //     return;
    // }

    // tcb_set_state(new_cur_pid, TSTATE_RUN);
    // set_curid(new_cur_pid);

    // if (old_cur_pid != new_cur_pid) {
    //     spinlock_release(&thread_lock[cpu_idx]);
    //     dprintf("[THREAD_WAIT_HELPER]: switch from %d to %d\n", old_cur_pid, new_cur_pid);
    //     kctx_switch(old_cur_pid, new_cur_pid); // THIS LINE TRIGGERS A THREAD TO SLEEP
    //     // NOTE THAT IF THERE ARE NO "THREADS" ON THE READY LIST, THEN THE IDLE THREAD WILL RUN
    // }
    // else{
    //     spinlock_release(&thread_lock[cpu_idx]);
    // }
}

void thread_signal_helper(unsigned int pid) {
    // pid is the thread to add from sleep queue to ready queue
    unsigned int cpu_idx = get_pcpu_idx();
    cpu_idx = cpu_idx == 1 ? 2 : 1; // swap to other cpu
    unsigned int new_cur_pid;

    int status = spinlock_try_acquire(&thread_lock[cpu_idx]);
    if (status == 1) {
        // dprintf("thread_signal_helper: spinlock is already acquired\n");
        return;
    }
    tcb_set_state(pid, TSTATE_READY);
    // dprintf("[THREAD_SIGNAL_HELPER]: append pid %d to CPU's %d ready\n", pid, cpu_idx);
    // enqueue the waiting thread to the ready list
    tqueue_enqueue(NUM_IDS + cpu_idx, pid);
    spinlock_release(&thread_lock[cpu_idx]);
}


void sched_update(){
    int cpu_idx = get_pcpu_idx();
    elapsed[cpu_idx] += MS_PER_LAPIC_INTR;
    if (elapsed[cpu_idx] >= SCHED_SLICE){
        elapsed[cpu_idx] = 0;
        thread_yield();
    }
}
