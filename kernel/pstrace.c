#include <linux/pstrace.h>
#include <linux/syscalls.h>
#include <linux/list.h>
#include <asm-generic/atomic-instrumented.h>
#include <linux/types.h>

atomic_t cb_node_num; /* number of actually filled nodes */
atomic_t g_count; /* number of records added to ring buffer */

struct cbnode *cbhead = NULL;
struct cbnode *last_write = NULL;

/* rec_list_lock is used to access: circular_buffer & last_write_ptr */ 
DEFINE_SPINLOCK(rec_list_lock);

/* pid_list_lock is used to access: traced_list */
DEFINE_SPINLOCK(pid_list_lock);

static inline void remove_cb_all(void)
{
	struct cbnode *curr = cbhead, *temp;
	while (curr) {
		temp = curr;
		curr = curr->next;
		kvfree(temp);
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
			kvfree(temp);	
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
			kvfree(curr);
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
			kvfree(temp);	
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

void pstrace_add(struct task_struct *p)
{
	
}

SYSCALL_DEFINE1(pstrace_enable, pid_t, pid)
{
	return 0;
}

SYSCALL_DEFINE1(pstrace_disable, pid_t, pid)
{
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
