#include <linux/prinfo.h>
#include <linux/syscalls.h>
#include <linux/list.h>
#include <linux/sched.h>
#include <linux/string.h>

static struct task_struct *get_root(int root_pid)
{
	if (root_pid == 0)
		return &init_task;

	return find_task_by_vpid(root_pid);
}

int fill_in_prinfo(struct prinfo *dest, 
		struct task_struct *source, 
		pid_t parent_pid,
		int level)
{
	struct prinfo tmp;
	tmp.parent_pid = parent_pid;
	tmp.pid = source->pid;
	tmp.state = source->state;
	tmp.uid = 9;
	strncpy(tmp.comm, source->comm, sizeof(tmp.comm));
	tmp.level = level;

	if (copy_to_user(dest, &tmp, sizeof(tmp)))
		return -EFAULT;
	return 0;
}


SYSCALL_DEFINE3(ptree, struct prinfo *, buf, int *, nr, int, root_pid)
{
	struct task_struct *root;
	
	if (nr <= 0)
		return 0;
	
	root = get_root(root_pid);

	return fill_in_prinfo(buf, root, 11, 11);
}
