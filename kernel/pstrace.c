#include <linux/pstrace.h>
#include <linux/syscalls.h>
#include <asm-generic/atomic-instrumented.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/spinlock_types.h>
#include <linux/spinlock.h>
#include <linux/preempt.h>
#include <linux/slab.h>

atomic_t cb_node_num = ATOMIC_INIT(0); /* number of actually filled nodes */
atomic_t g_count = ATOMIC_INIT(0);/* number of records added to ring buffer */
int enabled_all = 0; /* flag to indicate if all processes are enabled tracing */

struct pspid traced[PSTRACE_BUF_SIZE] = { 
        [0 ... PSTRACE_BUF_SIZE - 1] = { .valid = 0 } 
};  /* buffer to store traced process pid */

struct cbnode *cbhead = NULL;
struct cbnode *last_write = NULL;

/* rec_list_lock is used to access: circular_buffer & last_write_ptr */ 
DEFINE_SPINLOCK(rec_list_lock);

LIST_HEAD(pid_list_head); /* list head of traced_list */
DEFINE_SPINLOCK(pid_list); /* spinlock for modifying traced */

static inline void remove_cb_all(void)
{
	struct cbnode *curr = cbhead, *temp;
	while (curr) {
		temp = curr;
		curr = curr->next;
		kfree(temp);
		atomic_dec(&cb_node_num);		
	}
	cbhead = NULL;
	last_write = cbhead;
}

/* pre-process the list, which:
 * - makes sure @cbhead is at the correct position, and does no more.
 */
static inline void remove_and_find_head(pid_t pid)
{
	struct cbnode *curr = cbhead, *prev = last_write, *temp;

	while (curr) {
		if (curr->data.pid == pid) {
			temp = curr;
			curr = curr->next;
			kfree(temp);	
			prev->next = curr;
			cbhead = curr;
			atomic_dec(&cb_node_num);
		}else {
			break;
		}
	}
	/* if it happens that everything is removed */
	if (!curr) {
		cbhead = NULL;
		last_write = cbhead;
		return;
	}
}

/* removes all the entries in circular buffer that matches @pid and frees it */
static inline void remove_cb_by_pid(pid_t pid){
	struct cbnode *curr = cbhead, *temp, *prev, *last_correct;

	/* do nothing when empty list */
	if (!curr) {
		return;
	}

	/* remove all */
	if (pid == -1) {
		remove_cb_all();
		return;	
	}

	/* traverse and remove the matching records, and 
	 * update last_write to the correct position last_correct */
	last_correct = cbhead; /* last correct value in list */
	prev = last_write; /* tail of list */

	/* there is only one entry */
	if (prev == curr) {
		if (curr->data.pid == pid) {
			kfree(curr);
			cbhead = NULL;
			last_write = cbhead;
		}
		return;
	}

	/* we have at least two entries */
	/* first advance head to ensure it is correct */
	remove_and_find_head(pid);
	
	/* job done, there is no entry */
	if (cbhead == NULL) {
		return;
	}

	/* there is at least one entry */
	last_correct = cbhead;
	prev = cbhead;
	curr = cbhead->next;
	/* to prevent loop */
	while (curr != NULL && curr != cbhead) {	
		/* if found, free it */
		if (curr->data.pid == pid) {
			temp = curr;
			curr = curr->next;
			kfree(temp);	
			prev->next = curr;
			atomic_dec(&cb_node_num);
		}else{
			last_correct = curr;
			prev = curr;
			curr = curr->next;
		}
	}
	last_write = last_correct;
}

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

/*
 * finds the next write position in the array named traced 
 * and sets the global pointer pid_next_write to it.
 *
 * This will called before adding a pid to the linked list
 */
static inline struct pspid *find_pid_next_write(void)
{
	/* 1. search for empty positions in the array
	 * 2. if 1) not found, over-write the first pid whose list field is
	 * *head->next
	 */
	struct pspid *next;
	int i;

	for (i = 0; i < PSTRACE_BUF_SIZE; i++) {
		if (!traced[i].valid) {
			next = &traced[i];
			goto return_from_find_pid_next;
		}
	}

	/* case 2: we over-write the oldest pid, which is 1st in the list
	 * we simply remove it from the list and return its reference
	 *
	 * after adding the new pid, it will be added to the end of the list
	 */
	next = list_entry(pid_list_head.next, struct pspid, list);
	__list_del(next->list.prev, next->list.next);

return_from_find_pid_next:
	return next;
}

void pstrace_add(struct task_struct *p)
{
        pid_t pid;
        struct cbnode *ncbnode;
        pid = p->pid;
        if (pid < 0) /* this pid is invalid */
                goto end;

        spin_lock(&pid_list);
        if (!enabled_all) {
                if (list_find(&pid_list_head, pid) == NULL) {
                        spin_unlock(&pid_list);
                        goto end;   
                }
        }
        else {
                if (list_find(&pid_list_head, pid) != NULL) {
                        spin_unlock(&pid_list);
                        goto end;
                }
        }
        if (cb_node_num.counter < 500) {
                ncbnode = kmalloc(sizeof(struct cbnode), GFP_KERNEL);
                strncpy(ncbnode->data.comm, p->comm, sizeof(p->comm));
                ncbnode->data.pid = pid;
                ncbnode->data.state = p->state;
                atomic_inc(&cb_node_num);
                atomic_inc(&g_count);

                if (!cbhead) {/* the circular buffer is empty*/  
                        cbhead = ncbnode; /* head always points to the first node*/
                        ncbnode->next = cbhead;
                        last_write = ncbnode; 
                } else {
                        last_write->next = ncbnode;
                        last_write = ncbnode;
                        last_write->next = cbhead;
                }
                

        } else {
        	/* cb_node_num == 500 */
                last_write = last_write->next;
                atomic_inc(&g_count);
                strncpy(last_write->data.comm, p->comm, sizeof(p->comm));
                last_write->data.pid = pid;
                last_write->data.state = p->state;
                cbhead = cbhead->next;	
        }
        	
end:
	;
}

SYSCALL_DEFINE1(pstrace_enable, pid_t, pid)
{
	struct list_head *pos;
	struct pspid *pid_next_write;

	preempt_disable();
	spin_lock(&pid_list);

	if (pid == -1) {
		/* trace all processes
		 * 1. enabled_all = 0, we are currently tracing a list of pids:
		 *    set enabled_all = 1, clear linked list of pids
		 *
		 * 2. enabled_all = 1, we are currently disabe tracing a list of pids:
		 *    clear linked list of pid
		 */
		
		if (!enabled_all)
			enabled_all = 1;

		clear_pid_list();

		goto return_from_enable;
	} else if (pid < 0) {
		/* invalid pid, do nothing */
		goto return_from_enable;
	}

	/* single valid pid to disable tracing
	 * 1. enabled_all = 0, pid list contains pids being traced
	 * add pid to linked list if it's not in it
	 *
	 * 2. enabled_all = 1, pid list contains pids being disabled tracing
	 * remove pid from linked list if it is in it, set pspid invalid
	 */
	if (!enabled_all) {
		pos = list_find(&pid_list_head, pid);
		if (pos == NULL) {
			pid_next_write = find_pid_next_write();

			/* fill struct and add to pid list */
			pid_next_write->pid = pid;
			pid_next_write->valid = 1;
			list_add_tail(&pid_next_write->list, &pid_list_head);
		}
	} else {
		pos = list_find(&pid_list_head, pid);
		if (pos != NULL) {
			set_pid_invalid(pos);
			__list_del(pos->prev, pos->next); /* remove pid from linked list */
		}
	}

return_from_enable:
	spin_unlock(&pid_list);
	preempt_enable();
	return 0;
}

SYSCALL_DEFINE1(pstrace_disable, pid_t, pid)
{
	struct list_head *pos;
	struct pspid *pid_next_write;

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
			pid_next_write = find_pid_next_write();

			/* fill struct and add to pid list */
			pid_next_write->pid = pid;
			pid_next_write->valid = 1;
			list_add_tail(&pid_next_write->list, &pid_list_head);
		}
	} else {
		pos = list_find(&pid_list_head, pid);
		if (pos != NULL) {
			set_pid_invalid(pos);
			__list_del(pos->prev, pos->next); /* remove pid from linked list */
		}
	}

return_from_disable:
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
	/* removes the records matching the pid */
	spin_lock(&rec_list_lock);
	remove_cb_by_pid(pid);
	spin_unlock(&rec_list_lock);
	return 0;
}
