#ifndef _LINUX_PSTRACE_H
#define _LINUX_PSTRACE_H

#include <linux/types.h>
#include <linux/sched.h>

#define PSTRACE_BUF_SIZE 500	/* The maximum size of the ring buffer */

struct pstrace {
	char comm[16];		/* The name of the process */
	pid_t pid;		/* The pid of the process */
	long state;		/* The state of the process */
};

/* The data structure used to keep track of process being traced */
struct pspid {
    pid_t pid;
    int valid;
    struct list_head list;
};

struct cbnode {
	struct pstrace data;
	struct cbnode *next;
	long counter;
};

/* used for the private field of wait_queue_entry */
struct psstruct {
	struct task_struct *tsk;
	long counter; /* so pstrace_add nows whether tsk should be woken up */
	pid_t wait_pid; /* so pstrace_clear nows whether tsk should be woken up */
};

/* Add a record of the state change into the ring buffer. */
void pstrace_add(struct task_struct *p);
#endif
