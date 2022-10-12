#ifndef _KERN_PROC_PPROC_H_
#define _KERN_PROC_PPROC_H_

#ifdef _KERN_

unsigned int get_curid(void);
void set_pdir_base(unsigned int index);
unsigned int thread_spawn(void *entry, unsigned int id,
                          unsigned int quota);

// container functions
void container_init(unsigned int mbi_addr);
unsigned int container_get_parent(unsigned int id);
unsigned int container_get_nchildren(unsigned int id);
unsigned int container_get_quota(unsigned int id);
unsigned int container_get_usage(unsigned int id);
unsigned int container_can_consume(unsigned int id, unsigned int n);
unsigned int container_split(unsigned int id, unsigned int quota);
unsigned int container_alloc(unsigned int id);
void container_free(unsigned int id, unsigned int page_index);

// from MPTFork
unsigned int copy_pdir_and_ptbl(unsigned int from_proc, unsigned int to_proc);

#endif  /* _KERN_ */

#endif  /* !_KERN_PROC_PPROC_H_ */
