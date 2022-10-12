#include <lib/gcc.h>
#include <lib/x86.h>
#include <lib/debug.h>

#include "import.h"

#define PTE_ADDR(x) ((x >> 12) & 0x3ff)

#define PAGESIZE      4096
#define PDIRSIZE      (PAGESIZE * 1024)
#define VM_USERLO     0x40000000
#define VM_USERHI     0xF0000000
#define VM_USERLO_PDE (VM_USERLO / PDIRSIZE)
#define VM_USERHI_PDE (VM_USERHI / PDIRSIZE)

#define PT_PERM_FORK (PTE_P | PTE_U | PTE_COW)

void copy_page_tables(unsigned int parent_index, unsigned int child_index){
    for (pde_index = VM_USERLO_PDE; pde_index < VM_USERHI_PDE; pde_index++) {
        unsigned int vaddr = pde_index*PDIRSIZE;
        alloc_ptbl(child_index, vaddr);

        for (pte_index = 0; pte_index < 1024; pte_index++) {
            unsigned int pte = get_ptbl_entry(parent_index, pde_index, pte_index);
            unsigned int pte_addr = PTE_ADDR(pte);
            set_ptbl_entry(parent_index, pde_index, pte_index, pte_addr, PT_PERM_FORK);
            set_ptbl_entry(child_index, pde_index, pte_index, pte_addr, PT_PERM_FORK);
        }
    }
}

void copy_on_write(unsigned int pid, unsigned int vaddr){
    unsigned int from_pte = get_ptbl_entry_by_va(pid, vaddr);
    unsigned int from_addr = PTE_ADDR(from_pte);
    alloc_page(pid, vaddr, PTE_P | PTE_U | PTE_W);
    unsigned int to_pte = get_pdir_entry_by_va(pid, vaddr);
    unsigned int to_addr = PTE_ADDR(to_pte);

    memcpy(to_addr, from_addr, PAGESIZE);
}