#ifndef _LINUX_PSTRACE_H
#define _LINUX_PSTRACE_H

#include <linux/types.h>
#include <linux/sched.h>

#define PSTRACE_BUF_SIZE 500	/* The maximum size of the ring buffer */

/* The data structure used to save the traced process. */
struct pstrace {
	char comm[16];		/* The name of the process */
	pid_t pid;		/* The pid of the process */
	long state;		/* The state of the process */
};

/* Add a record of the state change into the ring buffer. */
void pstrace_add(struct task_struct *p);
#endif
