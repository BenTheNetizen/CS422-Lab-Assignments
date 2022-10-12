#ifndef _KERN_VMM_MPTFORK_H_
#define _KERN_VMM_MPTFORK_H_

#ifdef _KERN_

unsigned int copy_pdir_and_ptbl(unsigned int from_proc, unsigned int to_proc);
unsigned int copy_on_write(unsigned int pid, unsigned int vaddr);

#endif  /* _KERN_ */

#endif  /* !_KERN_VMM_MPTFORK_H_ */
