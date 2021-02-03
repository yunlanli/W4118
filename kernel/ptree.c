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


static inline void fill_prinfo(struct task_struct *task, struct prinfo *tmp)
{
	tmp->pid = task->pid;
	tmp->parent_pid = task->real_parent == NULL ? 0 : task->real_parent->pid;
	tmp->state = task->state;
	/* tmp.uid = from_kuid_munged(task->real_cred->user_ns, task->real_cred->uid); */
	strncpy(tmp->comm, task->comm, sizeof(tmp->comm));
}

SYSCALL_DEFINE3(ptree, typeof(struct prinfo *), buf, int *, nr, int, root_pid)
{
	struct task_struct *task;
	struct prinfo *buf_tmp;
	struct list_head *list, *head;
	int max, level, seen, visited;

	if (copy_from_user(&max, nr, sizeof(max)))
		return -EFAULT;

	
	seen = 0, visited = 0, level = 0;
	buf_tmp = (struct prinfo *) kmalloc(sizeof(struct prinfo) * (max), GFP_KERNEL);

	buf_tmp->pid = root_pid;
	buf_tmp->level = 0;
	seen++;

	printk(KERN_DEBUG "start traversal...\n");

	/* traverse the process tree */
	read_lock(&tasklist_lock);

	/* prinfo for each process gets populated in 2 steps:
	 * 	1. pid, level are filled when it's first seen
	 * 	2. the rest gets populated when it gets visited again
	 */
	while (seen < max && visited < seen) {
		/* process next element on queue: retrieve children, fill prinfo */
		task = get_root(buf_tmp[visited].pid);
		head = &task->children;
		fill_prinfo(task, &buf_tmp[visited++]);
		level = buf_tmp->level;

		printk(KERN_DEBUG "get_root while holding lock works\n");

		/* children */
		list_for_each(list, head) {
			if (seen < max) {
				task = list_entry(list, struct task_struct, sibling);
				buf_tmp[seen].pid = task->pid;
				buf_tmp[seen++].level = level+1;
			} else 
				break;
		}
	}

	read_unlock(&tasklist_lock);

	printk(KERN_DEBUG "Unlocked, traversal done.\n");
	
	if (copy_to_user(buf, buf_tmp, sizeof(struct prinfo) * visited)
			|| copy_to_user(nr, &visited, sizeof(int)))
			return -EFAULT;

	kfree(buf_tmp);

	return visited;
}
