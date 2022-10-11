#include <lib/gcc.h>
#include <lib/x86.h>
#include <lib/debug.h>

#include "import.h"


// unsigned int *PDirPool[NUM_IDS][1024] gcc_aligned(PAGESIZE);

void copy_pdir(unsigned int from_proc, unsigned int to_proc)
{
   for (int i = 0; i < NUM_IDS; i++) {
        set_pdir_entry(to_proc, )
   }
}


void copy_ptbl(unsigned int from_proc, unsigned int to_proc)
{
    unsigned int i, j;
    unsigned int *pdir = PDirPool[to_proc];
    unsigned int *from_pdir = PDirPool[from_proc];
    for (i = 0; i < 1024; i++) {
        if (from_pdir[i] & PTE_P) {
            unsigned int *ptbl = (unsigned int *) ADDR_MASK(from_pdir[i]);
            unsigned int *new_ptbl = (unsigned int *) alloc_page();
            for (j = 0; j < 1024; j++) {
                new_ptbl[j] = ptbl[j];
            }
            pdir[i] = (unsigned int) new_ptbl | PT_PERM_PTU;
        }
    }
}