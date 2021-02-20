#include <linux/pstrace.h>
#include <linux/syscalls.h>
#include <asm-generic/atomic-instrumented.h>
#include <asm-generic/atomic-long.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/spinlock_types.h>
#include <linux/spinlock.h>
#include <linux/preempt.h>
#include <linux/slab.h>
#include <linux/errno.h>

atomic_t cb_node_num = ATOMIC_INIT(0); /* number of actually filled nodes */
atomic_long_t g_count = ATOMIC_LONG_INIT(0);/* number of records added to ring buffer */
int enabled_all = 0; /* flag to indicate if all processes are enabled tracing */

struct pspid traced[PSTRACE_BUF_SIZE] = { 
        [0 ... PSTRACE_BUF_SIZE - 1] = { .valid = 0 } 
};  /* buffer to store traced process pid */

struct cbnode *cbhead = NULL;
struct cbnode *last_write = NULL;

LIST_HEAD(pid_list_head); /* list head of traced_list */
DEFINE_SPINLOCK(pid_list); /* spinlock for modifying traced */
DEFINE_SPINLOCK(rec_list_lock); /* rec_list_lock is used to access: circular_buffer & last_write_ptr */ 

static inline void remove_cb_all(void)
{
	struct cbnode *curr = cbhead, *temp;
	
	while (curr && cb_node_num.counter) {
		temp = curr;
		curr = curr->next;
		/* freeing */
		spin_unlock(&rec_list_lock);
		printk(KERN_DEBUG "remove_all freeing: %s\n", temp->data.comm);
		kfree(temp);
		spin_lock(&rec_list_lock);
		/* done freeing */
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
			/* freeing */
			spin_unlock(&rec_list_lock);
			printk(KERN_DEBUG "remove_all freeing: %s\n", temp->data.comm);
			kfree(temp);	
			spin_lock(&rec_list_lock);
			/* done freeing */
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
			/* freeing */
			spin_unlock(&rec_list_lock);
			printk(KERN_DEBUG "remove_all freeing: %s\n", curr->data.comm);
			kfree(curr);
			spin_lock(&rec_list_lock);
			/* done freeing */
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
			/* freeing */
			spin_unlock(&rec_list_lock);
			printk(KERN_DEBUG "remove_all freeing: %s\n", temp->data.comm);
			kfree(temp);	
			spin_lock(&rec_list_lock);
			/* done freeing */
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
        spin_unlock(&pid_list);
        if (cb_node_num.counter < 500) {
                ncbnode = kmalloc(sizeof(struct cbnode), GFP_KERNEL);
                strncpy(ncbnode->data.comm, p->comm, sizeof(p->comm));
                ncbnode->data.pid = pid;
                ncbnode->data.state = p->state;
                printk(KERN_DEBUG "comm %s, pid %d, state %ld\n", ncbnode->data.comm, ncbnode->data.pid, ncbnode->data.state);
                atomic_inc(&cb_node_num);
                atomic_long_inc(&g_count);
                spin_lock(&rec_list_lock);

                if (!cbhead) {/* the circular buffer is empty*/  
                        cbhead = ncbnode; /* head always points to the first node*/
                        ncbnode->next = cbhead;
                        last_write = ncbnode; 
                } else {
                        last_write->next = ncbnode;
                        last_write = ncbnode;
                        last_write->next = cbhead;
                }
                spin_unlock(&rec_list_lock);
        } else {
        	/* cb_node_num == 500 */
        	spin_lock(&rec_list_lock);
                last_write = last_write->next;
                atomic_long_inc(&g_count);
                strncpy(last_write->data.comm, p->comm, sizeof(p->comm));
                last_write->data.pid = pid;
                last_write->data.state = p->state;
                cbhead = cbhead->next;
                spin_unlock(&rec_list_lock);	
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


static inline void copy_pstrace(struct pstrace *dest, struct pstrace *src) {
	strncpy(dest->comm, src->comm, sizeof(src->comm));
	dest->pid = src->pid;
	dest->state = src->state;
}

SYSCALL_DEFINE3(pstrace_get, pid_t, pid, struct pstrace __user *, buf, long __user *, counter)
{
	long kcounter;
	int cp_count;
	struct pstrace *kbuf;
	struct cbnode *pos;

	cp_count = 0;
	kbuf = (struct pstrace *) kmalloc(sizeof(struct pstrace) * PSTRACE_BUF_SIZE, GFP_KERNEL);

	/* invalid pid */
	if (pid !=  -1 && pid < 0)
		return -EINVAL;

	if (buf == NULL || counter == NULL)
		return -EINVAL;

	if (copy_from_user(&kcounter, counter, sizeof(long)))
		return -EFAULT;

	if (kcounter <= 0) {
		/* return immediately */
		spin_lock(&rec_list_lock);
		/* g_count will not change while we hold the lock, so can assign
		 * it to kcounter now
		 */
		kcounter = g_count.counter;

		if (cbhead == NULL)
			goto get_unlock;
		
		for (pos = cbhead; pos != cbhead; pos = pos->next) {
			copy_pstrace(kbuf+cp_count, &pos->data);
			cp_count++;
		}

get_unlock:
		spin_unlock(&rec_list_lock);
		goto get_ret2user;
	}

get_ret2user:
	if (copy_to_user(buf, kbuf, sizeof(struct pstrace) * cp_count)) {
		kfree(kbuf);
		return -EFAULT;
	}

	if (copy_to_user(counter, &kcounter, sizeof(long))) {
		kfree(kbuf);
		return -EFAULT;
	}

	kfree(kbuf);

	return cp_count;
}

SYSCALL_DEFINE1(pstrace_clear, pid_t, pid)
{
	if (pid != -1 && pid < 0) {
		return -EINVAL;
	}

	/* removes the records matching the pid */
	spin_lock(&rec_list_lock);
	remove_cb_by_pid(pid);
	spin_unlock(&rec_list_lock);
	return 0;
}
