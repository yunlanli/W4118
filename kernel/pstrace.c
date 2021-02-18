#include <linux/pstrace.h>
#include <linux/syscalls.h>
#include <linux/list.h>

int flag = 0; /* 0 stands for enabling tracing */
int cb_node_num = 0; /* number of actually filled nodes */
int g_count = 0; /* number of records added to ring buffer */
int used_cb_buf = 0;

struct traced_pid_node traced_list[PSTRACE_BUF_SIZE];
struct cbnode circular_buffer[PSTRACE_BUF_SIZE];

LIST_HEAD(traced_pid_list_head);

/* pointer to the next available/empty cbnode to write to */
LIST_HEAD(cbuf_list_head); /* struct list_head cbuf_list_head */

/* pointer to last record struct cbnode's head field */
struct list_head *last_write = &cbuf_list_head; 

/* rec_list_lock is used to access: circular_buffer & last_write_ptr */ 
DEFINE_SPINLOCK(rec_list_lock);

/* pid_list_lock is used to access: traced_list */
DEFINE_SPINLOCK(pid_list_lock);

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
	struct traced_pid_node *p, *next;
	struct cbnode *p1, *next1;
	/* If user passed in -1:
	 * 1. clear all pid in the traced_list
	 * 2. clear all cbnodes in the circular buffer
	 * 3. reset the last_write depending on used_cb_buf
	 *    - if used_cb_buf == 500, make it
	 *       so that last_write->next == cbuf_list_head
	 *    - otherwise (used_cb_buf < 500) make it
	 *      so that last_write == cbuf_list_head
	 */
	if(pid == -1){
		/* removes all pid in the traced list */
		list_for_each_entry_safe(p, next, &traced_pid_list_head, head){
			list_del(&(p->head));
		}
		/* removes all cbnodes in the circular buffer */
		list_for_each_entry_safe(p1, next1, &cbuf_list_head, head){
			list_del(&(p1->head));
		}
		cb_node_num = 0;

		/* resets last_write */
		if(used_cb_buf == PSTRACE_BUF_SIZE){
			last_write = cbuf_list_head.prev;
		}else{
			last_write = &cbuf_list_head;
		}
		return 0;
	}
#if 0
	/* If user passed in an actual pid
	 * iterate through ring buffer and do:
	 * 1. check if that data.pid == pid. If yes:
	 *     - rewire
	 *     - update cb_node_num
	 *    Otherwise, do nothing.
	 */
	list_for_each_entry_safe(pos, &cbuf_list_head) {
		container = list_entry(pos, struct cbnode, head);
		if (container->pid == pid) {
			/* rewire it to the end, AND update last_write */
			list_remove(this_element);
			list_add_tail(this_element);
			atomic_decr(cb_node_num);
		}
	}
	list_for_each(pos, &traced_list) {
		container = list_entry(pos, struct traced_pid_node, head);
		if (container->pid == pid) {
			list_remove(this_element);
			return;
		}
	}
#endif
	return 0;
}
