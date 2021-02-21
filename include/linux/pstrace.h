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

/* Add a record of the state change into the ring buffer. */
void pstrace_add(struct task_struct *p);
#endif
