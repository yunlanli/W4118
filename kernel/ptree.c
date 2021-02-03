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
#include <linux/kern_levels.h>
#include <linux/slab.h>

static struct task_struct *get_root(int root_pid)
{
	if (root_pid == 0)
		return &init_task;

	return find_task_by_vpid(root_pid);
}

#if 0
struct test_struct {
	int num;
	struct list_head list;
};
#endif

#if 0
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

#endif

SYSCALL_DEFINE3(ptree, typeof(struct prinfo *), buf, int *, nr, int, root_pid)
{
	struct task_struct *task, *task_parent;
	struct prinfo *buf_tmp, *buf_hd;
	struct list_head *list;
	int nr_cp, i, seen, stored;

	if (copy_from_user(&nr_cp, nr, sizeof(nr_cp)))
		return -EFAULT;
	printk(KERN_DEBUG "nr: %d\n", nr_cp);

	seen = 0, stored = 0;
	

	// printk(KERN_DEBUG "Allocating memory on the heap for array...\n");
	buf_tmp = (struct prinfo *) kmalloc(sizeof(struct prinfo) * (nr_cp), GFP_KERNEL);
	// printk(KERN_DEBUG "Memory allocation success!\n");
	
	buf_hd = buf_tmp;

	task = get_root(root_pid);
	printk(KERN_DEBUG "get_root success.\n");
	
	task_parent = task->real_parent;

	buf_hd->pid = task->pid;
	buf_hd->parent_pid = task_parent == NULL ? 0 : task_parent->pid;
	printk(KERN_DEBUG "[root] pid: %d, parent: %d\n", task->pid, buf_hd->parent_pid);
	
	buf_hd++;
	seen++;
	stored++;

	list = task->children.next;
	printk(KERN_DEBUG "[root: %s] ptr: %p, childen: %p\n", task->comm, task, &task->children);
	printk(KERN_DEBUG "[1st child] children field address: %p\n", list);
	

	printk(KERN_DEBUG "start traversal...\n");

	/* traverse the process tree */
	/* read_lock(&tasklist_lock); */

	/* get the task_struct of next element */
	task = list_entry(list, struct task_struct, sibling);
	task_parent = task->real_parent;
	printk(KERN_DEBUG "[1st child: %s] ptr address: %p\n", task->comm, task);
	
	if (task_parent == NULL)
		printk(KERN_DEBUG "Failed to retrieve parent task.\n");
		

	buf_hd->pid = task->pid;
	buf_hd->parent_pid = task_parent == NULL ? 0 : task_parent->pid;

	printk(KERN_DEBUG "[1st child] pid: %d, parent pid: %d\n",
				task->pid, buf_hd->parent_pid);


	buf_hd++;
	seen++;
	stored++;
	list = task->sibling.next;

	/* siblings */
	while (list != NULL && seen < nr_cp) {
		task = list_entry(list, struct task_struct, sibling);
		task_parent = task->real_parent;

		buf_hd->pid = task->pid;
		buf_hd->parent_pid = task_parent == NULL ? 0 : task_parent->pid;

		printk(KERN_DEBUG "[sibling] pid: %d, parent pid: %d\n",
				buf_hd->pid, buf_hd->parent_pid);

		buf_hd++;
		seen++;
		stored++;
		list = list->next;
	}

	/* read_unlock(&tasklist_lock); */

	printk(KERN_DEBUG "Unlocked, traversal done.\n");
	
	/* print each process's pid and pid */
	buf_hd = buf_tmp;
	for (i = 0; i < stored; i++) {
		printk(KERN_DEBUG "pid: %d, parent pid: %d\n",
				buf_hd->pid, buf_hd->parent_pid);
		buf_hd++;
	}

	kfree(buf_tmp);

	return stored;
		
#if 0
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
#endif

#if 0
	struct test_struct *lst, *cur, *tail;
	struct list_head *lst_head;
	int nr_cp, i, sum;

	printk(KERN_DEBUG "Initialized variables.\n");
	/* copies the content ptr points to, should 3rd param be sizeof(int)? */
	sum = 0;
	if (copy_from_user(&nr_cp, nr, sizeof(nr_cp)))
		return -EFAULT;
	printk(KERN_DEBUG "sum: %d, nr: %d\n", sum, nr_cp);
	

	printk(KERN_DEBUG "Allocating memory on the heap for llist...\n");
	/* initialize linked list on the heap */
	lst = (struct test_struct *) kmalloc(sizeof(struct test_struct) * nr_cp, GFP_KERNEL);
	printk(KERN_DEBUG "Memory allocation done\n");

	/* iterate through lst to initialize the linked list */
	INIT_LIST_HEAD(&lst->list);
	cur = lst, tail = lst;
	cur->num = 0;


	printk(KERN_DEBUG "Iterating the list of struct...\n");
	for (i = 1; i < nr_cp; i++) {
		cur++;
		
		cur->num = i;
		printk(KERN_DEBUG "pupulating %d-th test-struct with %d\n", i, cur->num);
		
		
		/* add cur->list to the end of the list */
		list_add(&cur->list, &tail->list);
		tail++;
	}

	printk(KERN_DEBUG "Iteration done\n");

	lst_head = &lst->list;
	for (i = 0; i < nr_cp; i++) {
		cur = list_entry(lst_head, struct test_struct, list);
		printk(KERN_DEBUG "%d-th: %d, ", i, cur->num);
		sum += cur->num;

		lst_head = lst_head->next;
	}

	printk(KERN_DEBUG "sum: %d\n", sum);
	
	kfree(lst);


	return sum;
#endif
}
