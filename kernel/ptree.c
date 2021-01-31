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

static struct task_struct *get_root(int root_pid)
{
	if (root_pid == 0)
		return &init_task;

	return find_task_by_vpid(root_pid);
}

struct task_list {
	struct task_struct *task;
	struct list_head *list_node;
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

SYSCALL_DEFINE3(ptree, typeof(struct prinfo *), buf, int *, nr, int, root_pid)
{
	/* initialize linked-list for traversing the task list */


	/* 
	 * initialize variables that will be used for storing task_struct info
	 */
	int count = 0, fill_res;

	/* populate prinfo of root_pid */
	struct task_struct *task = get_root(root_pid),
			   *parent = task->real_parent;
	struct prinfo tmp;

	if ((fill_res = fill_prinfo(task, parent, buf, &tmp)))
		return fill_res;
	count++;
	buf++;

	/* traverse the process tree */
	read_lock(&tasklist_lock);

	read_unlock(&tasklist_lock);

	return 69;
}
