#ifndef _PSTRACE_H_
#define _PSTRACE_H_

#include <linux/types.h>
#include <linux/sched.h>

#define PSTRACE_BUF_SIZE 500	/* The maximum size of the ring buffer */

struct pstrace {
	char comm[16];		/* The name of the process */
	pid_t pid;		/* The pid of the process */
	long state;		/* The state of the process */
};

struct cbnode { /* circular buffer node, singly linked list */
	struct pstrace data;
	struct cbnode *next;
};

/* Add a record of the state change into the ring buffer. */
void pstrace_add(struct task_struct *p);

#endif

