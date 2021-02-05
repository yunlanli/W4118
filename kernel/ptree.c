#include <linux/prinfo.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/kern_levels.h>
#include <linux/rwlock.h>

/* helper functions */
static struct task_struct *get_root(int root_pid)
{
	if (root_pid == 0)
		return &init_task;

	return find_task_by_vpid(root_pid);
}

void fill_in_prinfo(struct prinfo *data, struct task_struct *source, int level)
{
	data->pid = source->pid;
	data->parent_pid = source->real_parent->pid;
	data->state = source->state;
	data->uid = source->cred->uid.val;
	strncpy(data->comm, source->comm, sizeof(data->comm));
	data->level = level;
}

SYSCALL_DEFINE3(ptree, struct prinfo *, buf, int *, nr, int, root_pid)
{	
	/* kernel space variables */
	int nr_loc;
	int level = 0, head = 0, size = 0;
	struct task_struct *root, *child, *tmp;
	struct prinfo *res_list, *curr;
	struct list_head *p;

	/* error detection */
	if (buf == NULL || nr == NULL)
		return -EINVAL;

	if (copy_from_user(&nr_loc, nr, sizeof(int)))
		return -EFAULT;
	
	root = get_root(root_pid);
	if (root == NULL || nr_loc < 1)
		return -EINVAL;
		
	res_list = (struct prinfo*) kmalloc(sizeof(struct prinfo) * nr_loc, GFP_KERNEL);
	read_lock(&tasklist_lock);

	/* first element/initialization */
	fill_in_prinfo(&(res_list[head]), root, level);
	size++;

	/* array traversal */
	while (size < nr_loc && head < size) {
		/* obtains the un-visited element */
		curr = &(res_list[head++]);
		tmp = get_root(curr->pid);
		if (list_empty( &(tmp->children) ))
			continue;
		
		level = curr->level+1;
		/* adds the children into the back of the  res_list */
		list_for_each(p, &tmp->children) {
			if (size < nr_loc) {
				child = list_entry(p, struct task_struct, sibling);
				fill_in_prinfo(&(res_list[size++]), child, level);
			}else{
				break;
			}
		}
	}
	
	read_unlock(&tasklist_lock);
	if (copy_to_user(buf, res_list, sizeof(struct prinfo) * nr_loc) || 
			copy_to_user(nr, &size, sizeof(int)))
			return -EFAULT;

	kfree(res_list);
	return 0;
}
