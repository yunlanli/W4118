#include <linux/prinfo.h>
#include <linux/syscalls.h>
#include <linux/list.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/cred.h>
#include <linux/slab.h>

struct task_node {
	struct prinfo info; 
	struct list_head list;
};

static struct task_struct *get_root(int root_pid)
{
	if (root_pid == 0)
		return &init_task;

	return find_task_by_vpid(root_pid);
}

/* change this */
int fill_in_prinfo(struct prinfo *dest, 
		struct prinfo *source)
{
	struct prinfo tmp;
	kuid_t uid_tmp;

	// TODO: see if there are getters
	tmp.parent_pid = parent_pid;
	tmp.pid = source->pid;
	tmp.state = source->state;
	uid_tmp = task_uid(source);
	tmp.uid = uid_tmp.val;
	strncpy(tmp.comm, source->comm, sizeof(tmp.comm));
	tmp.level = level;

	if (copy_to_user(dest, &tmp, sizeof(tmp)))
		return -EFAULT;
	return 0;
}


SYSCALL_DEFINE3(ptree, struct prinfo *, buf, int *, nr, int, root_pid)
{
	struct task_struct *root;
	struct task_node *tsk_list;
	struct task_node tmp;
	int task_num;
	int nr_tmp;

	/* 1. copy nr */
	if (nr < 1)
		return -EINVAL;
	
	root = get_root(root_pid);
	if (root == NULL)
		return -EINVAL;
	
	tsk_list = kmalloc(sizeof(struct task_node) * (*nr), GFP_KERNEL);

	/* locked */
	/* initializing the queue */
	initialize_linked_list()

	/* BFS */
	/*  TODO: check empty queue */
	for (; task_num < nr && list_empty(); task_num++) {
		// pop first element
		// add all children's siblings and configure the level/parent
	}
	/* unlock */
	
	/* copy struct information in a loop */

	return fill_in_prinfo(buf, root, 11, 11);
}
