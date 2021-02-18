#include <linux/pstrace.h>
#include <linux/syscalls.h>
#include <linux/list.h>
#include <asm-generic/atomic-instrumented.h>
#include <linux/types.h>

int flag = 0; /* 0 stands for enabling tracing */
atomic_t cb_node_num; /* number of actually filled nodes */
int g_count = 0; /* number of records added to ring buffer */
int used_cb_buf = 0;

struct traced_pid_node traced_list[PSTRACE_BUF_SIZE];
struct cbnode circular_buffer[PSTRACE_BUF_SIZE];

LIST_HEAD(traced_pid_list_head);

/* ptr to next write position in traced */
struct traced_pid_node *pid_next_write = traced_list;

/* pointer to the next available/empty cbnode to write to */
LIST_HEAD(cbuf_list_head); /* struct list_head cbuf_list_head */

/* pointer to last record struct cbnode's head field */
struct list_head *last_write = &cbuf_list_head; 

/* rec_list_lock is used to access: circular_buffer & last_write_ptr */ 
DEFINE_SPINLOCK(rec_list_lock);

/* pid_list_lock is used to access: traced_list */
DEFINE_SPINLOCK(pid_list_lock);

static inline void clear_traced_pids(void)
{
	struct traced_pid_node *p, *next;
	list_for_each_entry_safe(p, next, &traced_pid_list_head, head){
		p->valid = 0;
		list_del(&(p->head));
	}
}

static inline void find_pid_next_write(void)
{
	/* 1. search for empty positions in the array
	 * 2. if not found, over-write the first pid whose list field is
	 * *head->next
	 */
	int i;

	for (i = 0; i < PSTRACE_BUF_SIZE; i++) {
		if (!traced_list[i].valid) {
			pid_next_write = &traced_list[i];
			return;
		}
	}
	/* case 2: the first pid */
	pid_next_write = list_entry(traced_pid_list_head.next, 
			struct traced_pid_node, head);
	/* 
	 * to ensure the next write will be updated correctly, we need to move
	 * pid_next_write to the tail of the list
	 *
	 * on next write, it will be added to tail again, but that's fine.
	 */
	list_del(&pid_next_write->head);
	list_add_tail(&pid_next_write->head, &traced_pid_list_head);
}

/* removes all the entries in circular buffer that matches @pid */
static inline void remove_cb_by_pid(pid_t pid){
	struct cbnode *p, *next;

	list_for_each_entry_safe(p, next, &traced_pid_list_head, head) {
		if (p->p_info.pid == pid) {
			list_del(&p->head);
		}
	}
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
	struct traced_pid_node *p;
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
		clear_traced_pids();
		/* removes all cbnodes in the circular buffer */
		list_for_each_entry_safe(p1, next1, &cbuf_list_head, head){
			list_del(&(p1->head));
		}
		cb_node_num.counter = 0;

		/* resets last_write */
		if(used_cb_buf == PSTRACE_BUF_SIZE){
			last_write = cbuf_list_head.prev;
		}else{
			last_write = &cbuf_list_head;
		}
		return 0;
	}
	/* If user passed in an actual pid
	 * iterate through ring buffer and do:
	 * 1. check if that data.pid == pid. If yes:
	 *     - rewire
	 *     - update cb_node_num
	 *    Otherwise, do nothing.
	 * 2. if we are in the disabled pid mode, add the pid;
	 *    Otherwise, remove it
	 * 3. remove the records. No need to rewire
	 */
	list_for_each_entry_safe(p1, next1, &cbuf_list_head, head) {
		if (p1->p_info.pid == pid) {
			/* rewire it to the end, AND update last_write */
			list_del(&(p1->head));
			list_add_tail(&(p1->head), &cbuf_list_head);
			atomic_dec(&cb_node_num);
		}
	}
	
	if (flag) {
		/* in disabled mode, add the pid to list */
		find_pid_next_write();
		pid_next_write->pid = pid;
		pid_next_write->valid = 1;	
	}else{
		/* in enabled mode, delete it */
		list_for_each_entry(p, &traced_pid_list_head, head){
			if (p->pid == pid) {
				list_del(&p->head);
				p->valid = 0;
				break;
			}
		}
	}

	/* removes the records matching the pid */
	remove_cb_by_pid(pid);
	return 0;
}
