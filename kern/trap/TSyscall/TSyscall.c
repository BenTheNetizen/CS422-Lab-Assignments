#include <lib/debug.h>
#include <lib/pmap.h>
#include <lib/types.h>
#include <lib/x86.h>
#include <lib/trap.h>
#include <lib/syscall.h>
#include <dev/intr.h>
#include <pcpu/PCPUIntro/export.h>
#include <lib/spinlock.h>
#include <thread/PTQueueIntro/export.h>

#include "import.h"

#define MAX_BUF_SIZE 4

static char sys_buf[NUM_IDS][PAGESIZE];

typedef struct {
    unsigned int queue[4];
    unsigned int next_in;
    unsigned int next_out;
    unsigned int count;
    spinlock_t lk;
} cv_t;

typedef struct {
    cv_t producer;
    cv_t consumer;
    unsigned int items[MAX_BUF_SIZE];
    spinlock_t lk;
    unsigned int next_in;
    unsigned int next_out;
    unsigned int count;
} bbq_t;

typedef struct bounded_buffer_t {
    unsigned int val[MAX_BUF_SIZE];
    unsigned int count;
    unsigned int next_in;
    unsigned int next_out;
} bounded_buffer_t;

typedef struct sleep_queue_t {
    unsigned int val[4];
    unsigned int count;
    unsigned int next_in;
    unsigned int next_out;
} sleep_queue_t;

static bbq_t bbq;
static unsigned int currThread[NUM_CPUS];

static bounded_buffer_t bounded_buffer;

static sleep_queue_t sleep_queue;

static spinlock_t buf_lock;
static spinlock_t sleep_queue_lock; 

void cv_init(cv_t *cv) {
    cv->count = 0;
    cv->next_in = 0;
    cv->next_out = 0;
    spinlock_init(&(cv->lk));
}

void bbq_init() {
    spinlock_init(&(bbq.lk));
    bbq.count = 0;
    bbq.next_in = 0;
    bbq.next_out = 0;

    cv_init(&(bbq.producer));
    cv_init(&(bbq.consumer));

    // init the currThread array
    for (unsigned int i = 0; i < NUM_CPUS; i++) {
        currThread[i] = -1;
    }
    return;
}


bool isEmptyCV(cv_t *cv) { return cv->count == 0; }
bool isEmptyBBQ(bbq_t *bbq) { return bbq->count == 0; }
bool isFullCV(cv_t *cv) { return cv->count == MAX_BUF_SIZE; }
bool isFullBBQ(bbq_t *bbq) { return bbq->count == MAX_BUF_SIZE; }

void cv_insert(unsigned int item, cv_t *cv) {
    spinlock_acquire(&(cv->lk));
    if (isFullCV(cv)) {
        spinlock_release(&(cv->lk));
        return;
    }
    cv->queue[cv->next_in] = item;
    cv->count++;
    cv->next_in = (cv->next_in + 1) % NUM_IDS;

    spinlock_release(&(cv->lk));
    return;
}

unsigned int cv_remove(cv_t *cv) {
    spinlock_acquire(&(cv->lk));
    dprintf("[sleep_queue_remove] acquired sleep_queue_lock\n");
    if (isEmptyCV(cv)) {
        spinlock_release(&(cv->lk));
        return -1;
    } 
        
    unsigned int ret = cv->queue[cv->next_out];
    cv->count--;
    cv->next_out = (cv->next_out + 1) % NUM_IDS;
    spinlock_release(&(cv->lk));
    return ret;
}

void thread_wait(cv_t *cv){
    spinlock_release(&(bbq.lk));
    
    // add current thread to sleep queue
    unsigned int pid = get_curid();
    cv_insert(pid, cv); // atomic instruction

    // take thread off execution and turns on a thread in the ready list of other CPU
    //thread_wait_helper();
    dprintf("[THREAD_WAIT] yield to thread on ready list\n");
    thread_wait_helper();

    // acquire spinlock again after being woken up by thread_wait from other CPU
    spinlock_acquire(&(bbq.lk));
}

void thread_signal(cv_t *cv) {
    dprintf("[THREAD_SIGNAL] \n");
    // remove thread from sleep queue and add to the ready list
    unsigned int new_cur_pid = cv_remove(cv);
    dprintf("[THREAD_SIGNAL] removed pid %d from producer sleep queue\n", new_cur_pid);
    if (new_cur_pid == -1) {
        dprintf("[THREAD_SIGNAL] SLEEP QUEUE is empty\n");
        return;
    }
    // take thread of sleep list and put on ready list of other CPU
    thread_signal_helper(new_cur_pid);
}

unsigned int buff_remove(){
    spinlock_acquire(&(bbq.lk));
    while (isEmptyBBQ(&bbq)){
        // dprintf("[BUFF_REMOVE] buffer is empty, waiting\n");
        thread_wait(&(bbq.consumer));
    }
    unsigned int ret = bbq.items[bbq.next_out];
    bbq.count--;
    bbq.next_out = (bbq.next_out + 1) % MAX_BUF_SIZE;
    // for (int i = 0; i < 4; i++) {
    //     dprintf("[BUFF_REMOVE] sleep queue at index %d is %ld\n", i, bbq.consumer.queue[i]);
    // }
    thread_signal(&(bbq.producer));
    spinlock_release(&(bbq.lk));
    dprintf("[BUFF_REMOVE] removed %ld\n", ret);
    return ret;
}


void buff_insert(unsigned int item){
    spinlock_acquire(&(bbq.lk));
    while (isFullBBQ(&bbq)){
        dprintf("[BUFF_INSERT] buffer is full, waiting\n");
        thread_wait(&(bbq.producer));
    }
    // for (int i = 0; i < 4; i++) {
    //     dprintf("[BUFF_INSERT] sleep queue at index %d is %ld\n", i, bbq.producer.queue[i]);
    // }
    dprintf("[BUFF_INSERT] buffer is not full, inserting %ld\n", item);
    bbq.items[bbq.next_in] = item;
    bbq.count++;
    bbq.next_in = (bbq.next_in + 1) % MAX_BUF_SIZE;

    thread_signal(&(bbq.consumer));
    spinlock_release(&(bbq.lk));
    return;
}





/**
 * Copies a string from user into buffer and prints it to the screen.
 * This is called by the user level "printf" library as a system call.
 */
void sys_puts(tf_t *tf)
{
    unsigned int cur_pid;
    unsigned int str_uva, str_len;
    unsigned int remain, cur_pos, nbytes;

    cur_pid = get_curid();
    str_uva = syscall_get_arg2(tf);
    str_len = syscall_get_arg3(tf);

    if (!(VM_USERLO <= str_uva && str_uva + str_len <= VM_USERHI)) {
        syscall_set_errno(tf, E_INVAL_ADDR);
        return;
    }

    remain = str_len;
    cur_pos = str_uva;

    while (remain) {
        if (remain < PAGESIZE - 1)
            nbytes = remain;
        else
            nbytes = PAGESIZE - 1;

        if (pt_copyin(cur_pid, cur_pos, sys_buf[cur_pid], nbytes) != nbytes) {
            syscall_set_errno(tf, E_MEM);
            return;
        }

        sys_buf[cur_pid][nbytes] = '\0';
        KERN_INFO("From cpu %d: %s", get_pcpu_idx(), sys_buf[cur_pid]);

        remain -= nbytes;
        cur_pos += nbytes;
    }

    syscall_set_errno(tf, E_SUCC);
}

extern uint8_t _binary___obj_user_pingpong_ping_start[];
extern uint8_t _binary___obj_user_pingpong_pong_start[];
extern uint8_t _binary___obj_user_pingpong_ding_start[];

/**
 * Spawns a new child process.
 * The user level library function sys_spawn (defined in user/include/syscall.h)
 * takes two arguments [elf_id] and [quota], and returns the new child process id
 * or NUM_IDS (as failure), with appropriate error number.
 * Currently, we have three user processes defined in user/pingpong/ directory,
 * ping, pong, and ding.
 * The linker ELF addresses for those compiled binaries are defined above.
 * Since we do not yet have a file system implemented in mCertiKOS,
 * we statically load the ELF binaries into the memory based on the
 * first parameter [elf_id].
 * For example, ping, pong, and ding correspond to the elf_ids
 * 1, 2, 3, and 4, respectively.
 * If the parameter [elf_id] is none of these, then it should return
 * NUM_IDS with the error number E_INVAL_PID. The same error case apply
 * when the proc_create fails.
 * Otherwise, you should mark it as successful, and return the new child process id.
 */
void sys_spawn(tf_t *tf)
{
    unsigned int new_pid;
    unsigned int elf_id, quota;
    void *elf_addr;

    elf_id = syscall_get_arg2(tf);
    quota = syscall_get_arg3(tf);

    unsigned int pid = get_curid();
    elf_id = syscall_get_arg2(tf);
    quota = syscall_get_arg3(tf);

    // check if the quota is valid
    pid = get_curid();
    if (!container_can_consume(pid, quota)) {
        syscall_set_errno(tf, E_EXCEEDS_QUOTA);
        syscall_set_retval1(tf, NUM_IDS);
        return;
    }

    // check if max num children reached
    if (container_get_nchildren(pid) >= MAX_CHILDREN) {
        syscall_set_errno(tf, E_MAX_NUM_CHILDEN_REACHED);
        syscall_set_retval1(tf, NUM_IDS);
        return;
    }

    // check if the child id would be invalid
    new_pid = pid * MAX_CHILDREN + container_get_nchildren(pid) + 1;
    if (new_pid >= NUM_IDS) {
        syscall_set_errno(tf, E_INVAL_CHILD_ID);
        syscall_set_retval1(tf, NUM_IDS);
        return;
    }

    switch (elf_id) {
    case 1:
        elf_addr = _binary___obj_user_pingpong_ping_start;
        break;
    case 2:
        elf_addr = _binary___obj_user_pingpong_pong_start;
        break;
    case 3:
        elf_addr = _binary___obj_user_pingpong_ding_start;
        break;
    default:
        syscall_set_errno(tf, E_INVAL_PID);
        syscall_set_retval1(tf, NUM_IDS);
        return;
    }

    new_pid = proc_create(elf_addr, quota);

    if (new_pid == NUM_IDS) {
        syscall_set_errno(tf, E_INVAL_PID);
        syscall_set_retval1(tf, NUM_IDS);
    } else {
        syscall_set_errno(tf, E_SUCC);
        syscall_set_retval1(tf, new_pid);
    }
}

/**
 * Yields to another thread/process.
 * The user level library function sys_yield (defined in user/include/syscall.h)
 * does not take any argument and does not have any return values.
 * Do not forget to set the error number as E_SUCC.
 */
void sys_yield(tf_t *tf)
{
    thread_yield();
    syscall_set_errno(tf, E_SUCC);
}

void sys_produce(tf_t *tf)
{
    unsigned int i;
    for (i = 0; i < 5; i++) {
        intr_local_disable();
        KERN_DEBUG("CPU %d: Process %d: Produced %d\n", get_pcpu_idx(), get_curid(), i);
        buff_insert(i);
        intr_local_enable();
    }
    syscall_set_errno(tf, E_SUCC);
}

void sys_consume(tf_t *tf)
{
    unsigned int i;
    for (i = 0; i < 5; i++) {
        intr_local_disable();
        KERN_DEBUG("CPU %d: Process %d: Consumed %d\n", get_pcpu_idx(), get_curid(), i);
        buff_remove(i);
        intr_local_enable();
    }
    syscall_set_errno(tf, E_SUCC);
}
