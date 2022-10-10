#include "lib/x86.h"

#include "import.h"

/**
 * Initializes all the thread queues with tqueue_init_at_id.
 */
void tqueue_init(unsigned int mbi_addr)
{
    // TODO: define your local variables here.

    tcb_init(mbi_addr);

    for (unsigned int i=0;i<=NUM_IDS;i++){
        tqueue_init_at_id(i);
    }
}

/**
 * Insert the TCB #pid into the tail of the thread queue #chid.
 * Recall that the doubly linked list is index based.
 * So you only need to insert the index.
 * Hint: there are multiple cases in this function.
 */
void tqueue_enqueue(unsigned int chid, unsigned int pid)
{
    unsigned int tail = tqueue_get_tail(chid);
    if (tail==NUM_IDS){
        tqueue_set_head(chid, pid);
    }
    else{
        tcb_set_next(tail, pid);
    }
    tcb_set_prev(pid, tail);
    tqueue_set_tail(chid, pid);
}

/**
 * Reverse action of tqueue_enqueue, i.e. pops a TCB from the head of the specified queue.
 * It returns the popped thread's id, or NUM_IDS if the queue is empty.
 * Hint: there are multiple cases in this function.
 */
unsigned int tqueue_dequeue(unsigned int chid)
{
    unsigned int head = tqueue_get_head(chid);
    if (head==NUM_IDS){ return NUM_IDS; }
    unsigned int next = tcb_get_next(head);
    if (next==NUM_IDS){
        tqueue_set_tail(chid, NUM_IDS);
    }
    tcb_set_prev(head, NUM_IDS);
    tcb_set_next(head, NUM_IDS);
    tqueue_set_head(chid, next);
    return head;
}

/**
 * Removes the TCB #pid from the queue #chid.
 * Hint: there are many cases in this function.
 */
void tqueue_remove(unsigned int chid, unsigned int pid)
{
    unsigned int prev = tcb_get_prev(pid);
    unsigned int next = tcb_get_next(pid);
    unsigned int head = tqueue_get_head(chid);
    unsigned int tail = tqueue_get_tail(chid);

    if (pid==head){
        tqueue_set_head(chid, next);
    }
    if (pid==tail){
        tqueue_set_tail(chid, prev);
    }
    if (prev!=NUM_IDS){
        tcb_set_next(prev, next);
    }
    if (next!=NUM_IDS){
        tcb_set_prev(next, prev);
    }
    tcb_set_next(pid, NUM_IDS);
    tcb_set_prev(pid, NUM_IDS);
}
