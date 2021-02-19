#include <linux/pstrace.h>
#include <linux/syscalls.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/spinlock_types.h>
#include <linux/spinlock.h>
#include <linux/preempt.h>
#include <linux/printk.h>

int enabled_all = 0; /* flag to indicate if all processes are enabled tracing */
struct pspid traced[PSTRACE_BUF_SIZE] = { 
	[0 ... PSTRACE_BUF_SIZE - 1] = { .valid = 0 } 
};  /* buffer to store traced process pid */

LIST_HEAD(pid_list_head); /* list head of traced_list */

struct pspid *pid_next_write = traced; /* ptr to next write position in traced */

DEFINE_SPINLOCK(pid_list); /* spinlock for modifying traced */

static inline void set_pid_invalid(struct list_head *node)
{
	struct pspid *container;

	container = list_entry(node, struct pspid, list);
	container->valid = 0;
}

/* 
 * clear linked list of pids
 *
 *	To clear linked list of pid, for each node in the linked
 *	list:
 *	a) set its container's valid field to 0
 *	b) remove it from the list
 */
static inline void clear_pid_list(void)
{
	struct list_head *pos;

	list_for_each(pos, &pid_list_head) {
		set_pid_invalid(pos);
		__list_del(pos->prev, pos->next); /* delete pid from list */
	}
}

static inline struct list_head *list_find(struct list_head *head, pid_t pid)
{
	struct list_head *pos;
	struct pspid *container;

	list_for_each(pos, head) {
		container = list_entry(pos, struct pspid, list);
		if (container->pid == pid)
			return pos;
	}
	
	/* pid not found */
	return NULL;
}

static inline void find_pid_next_write(void)
{
	/* 1. search for empty positions in the array
	 * 2. if 1) not found, over-write the first pid whose list field is
	 * *head->next
	 */
	int i;

	for (i = 0; i < PSTRACE_BUF_SIZE; i++) {
		if (!traced[i].valid) {
			pid_next_write = &traced[i];
			return;
		}
	}

	/* case 2: the first pid */
	pid_next_write = list_entry(pid_list_head.next, struct pspid, list);
	/* 
	 * to ensure the next write will be updated correctly, we need to move
	 * pid_next_write to the tail of the list
	 *
	 * on next write, it will be added to tail again, but that's fine.
	 */
	__list_del(pid_next_write->list.prev, pid_next_write->list.next);
	list_add_tail(&pid_next_write->list, &pid_list_head);
}

void pstrace_add(struct task_struct *p)
{
}

SYSCALL_DEFINE1(pstrace_enable, pid_t, pid)
{
	struct list_head *pos;
	struct pspid *tmp;

	preempt_disable();
	spin_lock(&pid_list);

	if (pid == -1) {
		/* trace all processes
		 * 1. enabled_all = 0, we are currently tracing a list of pids:
		 *    set enabled_all = 1, clear linked list of pids, update
		 *    pid_next_write
		 *
		 * 2. enabled_all = 1, we are currently disabe tracing a list of pids:
		 *    clear linked list of pid, update pid_next_write
		 */
		
		if (!enabled_all)
			enabled_all = 1;

		clear_pid_list();

		/* update pid_next_write to first spot in the array traced */
		pid_next_write = traced;
		goto return_from_enable;
	} else if (pid < 0) {
		/* invalid pid, do nothing */
		goto return_from_enable;
	}

	/* single valid pid to disable tracing
	 * 1. enabled_all = 0, pid list contains pids being traced
	 * add pid to linked list if it's not in it, update next write
	 *
	 * 2. enabled_all = 1, pid list contains pids being disabled tracing
	 * remove pid from linked list if it is in it, set pspid invalid
	 */
	if (!enabled_all) {
		pos = list_find(&pid_list_head, pid);
		if (pos == NULL) {
			/* fill struct and add to pid list */
			pid_next_write->pid = pid;
			pid_next_write->valid = 1;
			list_add_tail(&pid_next_write->list, &pid_list_head);

			/* find next write position */
			find_pid_next_write();
		}
	} else {
		pos = list_find(&pid_list_head, pid);
		if (pos != NULL) {
			set_pid_invalid(pos);
			__list_del(pos->prev, pos->next); /* remove pid from linked list */
		}
	}

return_from_enable:
	/* debug: print all disabled / enabled pids */
	if (enabled_all == 0)
		printk(KERN_DEBUG "pid enabled: ");
	else
		printk(KERN_DEBUG "pid disabled: ");
		
	list_for_each_entry(tmp, &pid_list_head, list) {
		printk(KERN_DEBUG "%d ", tmp->pid);
	}
	printk(KERN_DEBUG "\n");
		
	spin_unlock(&pid_list);
	preempt_enable();
	return 0;
}

SYSCALL_DEFINE1(pstrace_disable, pid_t, pid)
{
	struct list_head *pos;
	struct pspid *tmp;

	preempt_disable();
	spin_lock(&pid_list);

	if (pid == -1) {
		/* stop tracing all processes
		 * 1. enabled_all = 0, we are currently tracing a list of pids:
		 *    clear linked list of pids
		 *
		 * 2. enabled_all = 1, we are currently disable tracing a list of pids:
		 *    set enabled_all = 0, clear linked list of pid. list of
		 *    pids are now used to store pids to trace.
		 */

		if (enabled_all)
			enabled_all = 0;

		clear_pid_list();

		/* update pid_next_write to first spot in the array traced */
		pid_next_write = traced;
		goto return_from_disable;
	} else if (pid < 0) {
		/* invalid pid, do nothing */
		goto return_from_disable;
	}

	/* single valid pid to disable tracing
	 * 1. enabled_all = 0, pid list contains pids being traced
	 * remove pid from linked list if it is in it, set pspid invalid
	 * 
	 * 2. enabled_all = 1, pid list contains pids being disabled tracing
	 * add pid to linked list if it's not in it, update next write
 	 */

	if (enabled_all) {
		pos = list_find(&pid_list_head, pid);
		if (pos == NULL) {
			/* fill struct and add to pid list */
			pid_next_write->pid = pid;
			pid_next_write->valid = 1;
			list_add_tail(&pid_next_write->list, &pid_list_head);

			/* find next write position */
			find_pid_next_write();
		}
	} else {
		pos = list_find(&pid_list_head, pid);
		if (pos != NULL) {
			set_pid_invalid(pos);
			__list_del(pos->prev, pos->next); /* remove pid from linked list */
		}
	}

return_from_disable:
	/* debug: print all disabled / enabled pids */
	if (enabled_all == 0)
		printk(KERN_DEBUG "pid enabled: ");
	else
		printk(KERN_DEBUG "pid disabled: ");
		
	list_for_each_entry(tmp, &pid_list_head, list) {
		printk(KERN_DEBUG "%d ", tmp->pid);
	}
	printk(KERN_DEBUG "\n");
		
	spin_unlock(&pid_list);
	preempt_enable();
	return 0;
}

SYSCALL_DEFINE3(pstrace_get, pid_t, pid, struct pstrace *, buf, long *, counter)
{
	return 0;
}

SYSCALL_DEFINE1(pstrace_clear, pid_t, pid)
{
	return 0;
}
