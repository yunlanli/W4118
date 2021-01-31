#include <linux/syscalls.h>
#include <linux/prinfo.h>
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

static struct task_struct *get_root(int root_pid)
{
	if (root_pid == 0)
		return &init_task;

	return find_task_by_vpid(root_pid);
}

SYSCALL_DEFINE3(ptree, struct prinfo *, buf, int *, nr, int, root_pid)
{
	struct task_struct *root = get_root(root_pid),
			   *parent = root->real_parent;

	struct prinfo tmp;
	/* tmp.pid = task_tgid_vnr(root); */

	tmp.parent_pid = parent == NULL ? 0 : parent->pid;
	tmp.pid = root->pid;
	tmp.state = root->state;
	/* tmp.uid = from_kuid_munged(root->real_cred->user_ns, root->real_cred->uid); */
	strcpy(tmp.comm, root->comm);
	tmp.level = 0;

	/* copy pid to the buf */
	if (copy_to_user(buf, &tmp, sizeof(struct prinfo)))
		return -EFAULT;

	return 69;
}
