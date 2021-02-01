#include <linux/syscalls.h>
#include <linux/prinfo.h>
#include <linux/types.h>
/* for struct task_struct */
#include <linux/sched.h>
#include <linux/sched/task.h>
/* for retrieving uid from task struct */
#include <linux/cred.h>
#include <linux/sched/user.h>
#include <linux/uaccess.h>
#include <linux/uidgid.h>
/* for strcpy */
#include <linux/string.h>
/* for errno macro */
#include <uapi/asm-generic/errno-base.h>
/* lock */
#include <linux/rwlock.h>
#include <linux/kfifo.h>
#include <linux/list.h>
#include <linux/printk.h>

static struct task_struct *get_root(int root_pid)
{
	if (root_pid == 0)
		return &init_task;

	return find_task_by_vpid(root_pid);
}

struct task_node {
	struct task_struct *task;
	struct list_head list_node;
};

int fill_prinfo(struct task_struct *task,
		struct task_struct *parent,
		struct prinfo *buf,
		struct prinfo *tmp)
{
	tmp->parent_pid = parent == NULL ? 0 : parent->pid;
	tmp->pid = task->pid;
	tmp->state = task->state;
	/* tmp.uid = from_kuid_munged(task->real_cred->user_ns, task->real_cred->uid); */
	strcpy(tmp->comm, task->comm);
	tmp->level = 0;

	/* copy pid to the buf */
	if (copy_to_user(buf, tmp, sizeof(struct prinfo)))
		return -EFAULT;

	return 0;
}

struct task_struct *task_add(
		struct task_struct *task,
		struct list_head *tail, 
		int *seen,
		int *nr,
		int first_child)
{
	if (*seen < *nr) {
		struct task_struct *tmp;

		if (first_child)
			tmp = list_entry(&(task->children), struct task_struct, children);
		else
			tmp = list_entry(&(task->sibling), struct task_struct, sibling);

		LIST_HEAD(tmp_head);

		struct task_node tmp_task;
		tmp_task.task = tmp;
		tmp_task.list_node = tmp_head;

		list_add(&tmp_head, tail);

		/* update tail */
		*tail = *tail->next;
		(*seen)++;

		return tmp;
	}

	return NULL;
}

SYSCALL_DEFINE3(ptree, typeof(struct prinfo *), buf, int *, nr, int, root_pid)
{
	/* 
	 * initialize variables that will be used for storing task_struct info
	 */
	int seen = 0, count = 0, fill_res;
	struct task_struct *task_struct_ptr, *parent;
	struct prinfo tmp;
	struct task_node *tmp_task;

	/* initialize linked-list for traversing the task list */
	struct task_node root;

	LIST_HEAD(head);
	INIT_LIST_HEAD(&head);
	root.list_node = head;
	
	root.task = get_root(root_pid);

	seen++;

	/* keep track of the head and tail of the list */
	struct list_head tail;
	tail = head;

	/* traverse the process tree */
	read_lock(&tasklist_lock);

//	while (count < *nr && !list_empty(&head)) {
//		/* retrieve container */
//		tmp_task = list_entry(&head, struct task_node, list_node);
//
//		/* populate prinfo */
//		task_struct_ptr = tmp_task->task;
//		parent = task_struct_ptr->real_parent;

//		if ((fill_res = fill_prinfo(task_struct_ptr, parent, buf, &tmp)))
//			return fill_res;
//		count++;
//		buf++;
//		printk("%d process done.\n", count);
//
//		break;
//
		/* add tmp_task's children to the list */
//		task_struct_ptr = task_add(task_struct_ptr, &tail, &seen, nr, 1);

		/* add first_child's siblings */
//		while (task_struct_ptr != NULL && !list_empty(&task_struct_ptr->sibling))
//			task_struct_ptr = task_add(task_struct_ptr, &tail, &seen, nr, 0); 

		/* delete and update head */
//		head = *head.next;
//		list_del(head.prev);
//	}

	read_unlock(&tasklist_lock);

	return 69;
}
