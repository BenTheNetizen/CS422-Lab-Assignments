#include <lib/gcc.h>
#include <lib/x86.h>

#include "import.h"

extern char STACK_LOC[NUM_IDS][PAGESIZE] gcc_aligned(PAGESIZE);

/**
 * Checks whether enough resources are available for allocation and
 * allocates memory for the new child thread, then sets the eip, and esp
 * of the thread states. The eip should be set to [entry], and the
 * esp should be set to the corresponding stack TOP in STACK_LOC.
 * Don't forget the stack is going down from high address to low.
 * We do not care about the rest of states when a new thread starts.
 * The function returns the child thread (process) id.
 * In case of an error, return NUM_IDS.
 */
unsigned int kctx_new(void *entry, unsigned int id, unsigned int quota)
{
    // TODO
    unsigned int child_id;
    // allocates memory for the new child thread
    if (!container_can_consume(id, quota)) return NUM_IDS;
    child_id = alloc_mem_quota(id, quota);
    if (child_id == NUM_IDS) return NUM_IDS; // error

    // sets the eip, and esp of the thread states
    kctx_set_eip(child_id, entry);
    kctx_set_esp(child_id, STACK_LOC[child_id] + PAGESIZE); // is this correct?

    return child_id;
}
