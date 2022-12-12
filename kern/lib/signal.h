#ifndef _KERN_LIB_SIGNAL_H_
#define _KERN_LIB_SIGNAL_H_

#ifdef _KERN_

#define NUM_SIGNALS 32

#define SIGINT 2
#define SIGKILL 9
#define SIGSTOP 19

typedef void sigfunc(int);

#endif /* _KERN_ */

#endif /* !_KERN_LIB_SIGNAL_H_ */