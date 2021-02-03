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
#include <linux/list.h>
#include <linux/printk.h>
#include <linux/kern_levels.h>
#include <linux/slab.h>

static struct task_struct *get_root(int root_pid)
{
	if (root_pid == 0)
		return &init_task;

	return find_task_by_vpid(root_pid);
}


static inline void fill_prinfo(struct task_struct *task, struct prinfo *tmp, int level)
{
	tmp->pid = task->pid;
	tmp->parent_pid = task->real_parent == NULL ? 0 : task->real_parent->pid;
	tmp->state = task->state;
	/* tmp.uid = from_kuid_munged(task->real_cred->user_ns, task->real_cred->uid); */
	strncpy(tmp->comm, task->comm, sizeof(tmp->comm));
	tmp->level = level;

}

SYSCALL_DEFINE3(ptree, typeof(struct prinfo *), buf, int *, nr, int, root_pid)
{
	struct task_struct *task;
	struct prinfo *buf_tmp, *buf_hd;
	struct list_head *list, *head;
	int nr_cp, level, seen, stored;

	if (copy_from_user(&nr_cp, nr, sizeof(nr_cp)))
		return -EFAULT;

	
	seen = 0, stored = 0, level = 0;
	buf_tmp = (struct prinfo *) kmalloc(sizeof(struct prinfo) * (nr_cp), GFP_KERNEL);
	buf_hd = buf_tmp;

	task = get_root(root_pid);
	head = &task->children;
	
	fill_prinfo(task, buf_hd++, level);
	
	seen++;
	stored++;

	printk(KERN_DEBUG "start traversal...\n");

	/* traverse the process tree */
	read_lock(&tasklist_lock);

	/* children */
	list_for_each(list, head) {
		task = list_entry(list, struct task_struct, sibling);
		fill_prinfo(task, buf_hd++, level);

		seen++;
		stored++;
	}
	read_unlock(&tasklist_lock);

	printk(KERN_DEBUG "Unlocked, traversal done.\n");
	
	if (copy_to_user(buf, buf_tmp, sizeof(struct prinfo) * stored)
			|| copy_to_user(nr, &stored, sizeof(int)))
			return -EFAULT;

	kfree(buf_tmp);

	return stored;
}
