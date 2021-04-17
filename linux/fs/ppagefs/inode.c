#include <linux/fs.h>
#include <uapi/linux/magic.h>
#include <linux/cgroup-defs.h>
#include <linux/init.h>
#include <linux/fs_context.h>
#include <uapi/linux/stat.h>
#include <linux/string.h>
#include <linux/ppage_fs.h>
#include <linux/uio.h>
#include <linux/kernel.h>
#include <linux/sched/signal.h>


static struct dentry
*ppagefs_create_dir(const char *name, struct inode *inode, struct dentry *parent);

static struct inode *ppage_get_inode(struct super_block *sb, umode_t flags);

static int ppage_fake_dir_open(struct inode *inode, struct file *file);

static struct dentry *ppage_fake_lookup(struct inode *dir,
		struct dentry *dentry, unsigned int flags);

static struct dentry *ppage_create_file(struct dentry *parent,
		const struct tree_descr *files, int count);

static struct dentry *ppage_lookup(struct inode *dir,
		struct dentry *dentry, unsigned int flags);

static int ppage_simple_unlink(struct inode *dir, struct dentry *dentry);

static struct dentry *ppage_root_lookup(struct inode *dir,
		struct dentry *dentry, unsigned int flags);

static ssize_t ppage_file_read_iter(struct kiocb *iocb, struct iov_iter *iter);

static void ppage_free_fc(struct fs_context *fc);

static int ppage_get_tree(struct fs_context *fc);

static int ppage_dcache_dir_open(struct inode *inode, struct file *file);

static int ppage_dcache_dir_close(struct inode *inode, struct file *file);

static loff_t
ppage_dcache_dir_lseek(struct file *file, loff_t offset, int whence);

static ssize_t ppage_generic_read_dir(struct file *filp, char __user *buf,
		size_t siz, loff_t *ppos);

static int ppage_dcache_readdir(struct file *file, struct dir_context *ctx);

static int ppage_piddir_delete(const struct dentry *dentry);

static int ppage_file_delete(const struct dentry *dentry);

static int ppagefs_init_fs_context(struct fs_context *fc);

static const struct inode_operations ppagefs_dir_inode_operations = {
	.lookup		= ppage_lookup,
	.link		= simple_link,
	.unlink		= ppage_simple_unlink,
	.rmdir		= simple_rmdir,
	.rename		= simple_rename,
	.permission	= generic_permission,
};

static const struct inode_operations ppagefs_root_inode_operations = {
	.lookup		= ppage_root_lookup,
	.permission	= generic_permission,
};

const struct file_operations ppagefs_file_operations = {
	.read_iter	= ppage_file_read_iter,
	.mmap		= generic_file_mmap,
	.fsync		= noop_fsync,
	.splice_read	= generic_file_splice_read,
	.splice_write	= iter_file_splice_write,
	.llseek		= generic_file_llseek,
};

static const struct fs_context_operations ppagefs_context_ops = {
	.free		= ppage_free_fc,
	.get_tree	= ppage_get_tree,
};

const struct file_operations ppage_dir_operations = {
	.open		= ppage_dcache_dir_open,
	.release	= ppage_dcache_dir_close,
	.llseek		= ppage_dcache_dir_lseek,
	.read		= ppage_generic_read_dir,
	.iterate_shared	= ppage_dcache_readdir,
	.fsync		= noop_fsync,
};

static const struct inode_operations ppagefs_fake_dir_inode_operations = {
	.lookup		= ppage_fake_lookup,
	.permission	= generic_permission,
};

const struct file_operations ppagefs_fake_dir_operations = {
	.open		= ppage_fake_dir_open,
	.release	= ppage_dcache_dir_close,
	.llseek		= ppage_dcache_dir_lseek,
	.read		= ppage_generic_read_dir,
	.iterate_shared	= ppage_dcache_readdir,
	.fsync		= noop_fsync,
};

const struct inode_operations ppagefs_file_inode_operations = {
	.setattr	= simple_setattr,
	.getattr	= simple_getattr,
	.permission	= generic_permission,
};

const struct dentry_operations ppagefs_piddir_d_ops = {
	.d_delete	= ppage_piddir_delete,
};

static const struct dentry_operations ppagefs_file_d_ops = {
	.d_delete	= ppage_file_delete,
};

static struct file_system_type ppage_fs_type = {
	.name =		"ppagefs",
	.init_fs_context = ppagefs_init_fs_context,
	.kill_sb =	kill_litter_super,
};

struct pfn_node {
	struct rb_node node;
	unsigned long pfn;
};

struct va_info {
	pte_t last_pte;
};

struct expose_count_args {
	int total;
	int zero;
};

/* private information for ppage_create_file */
struct p_info {
	pid_t			pid;
	char			comm[TASK_COMM_LEN];
	int			retain;
	struct dentry 		*dentry;
	struct list_head 	head;
};

static inline int is_zero_file(const struct dentry *dentry)
{
	const char *dirname = dentry->d_name.name;
	const char *zero = "zero";
	const size_t len_d = strlen(dirname);
	const size_t len_z = strlen(zero);

	return (len_d == len_z && strncmp(dirname, zero, len_z) == 0);
}

static inline int is_total_file(const struct dentry *dentry)
{
	const char *dirname = dentry->d_name.name;
	const char *total = "total";
	const size_t len_d = strlen(dirname);
	const size_t len_t = strlen(total);

	return (len_d == len_t && strncmp(dirname, total, len_t) == 0);
}

static inline int is_zero_or_total_file(const struct dentry *dentry)
{
	return is_total_file(dentry) || is_zero_file(dentry);
}

void pfn_rb_insert(struct rb_root *root,
		struct pfn_node *new, struct expose_count_args *args)
{
	struct rb_node **node = &root->rb_node, *parent;
	unsigned long target_pfn = new->pfn;
	struct pfn_node *tmp;

	parent = *node;

	/* Go to the bottom of the tree */
	while (*node) {
		parent = *node;
		tmp = rb_entry(parent, struct pfn_node, node);

		if (tmp->pfn == target_pfn)
			return; /* duplicate */

		if (tmp->pfn > target_pfn)
			node = &parent->rb_left;
		else
			node = &parent->rb_right;
	}

	/* Put the new node there */
	rb_link_node(&new->node, parent, node);
	rb_insert_color(&new->node, root);

	if (is_zero_pfn(target_pfn))
		args->zero++;
	args->total++;
}

static inline int count_page(pte_t *pte, struct expose_count_args *args)
{
	struct pfn_node *new;
	struct rb_root *root;

	new->pfn = pte_pfn(*pte);
	pfn_rb_insert(root, new, args);

	return 0;
}

static inline int pte_walk(pmd_t *src_pmd,
	unsigned long addr,
	unsigned long end,
	struct mm_struct *src_mm,
	struct vm_area_struct *vma,
	struct expose_count_args *args,
	struct va_info *lst)
{
	pte_t *pte = pte_offset_map(src_pmd, addr);

	for (;;) {
		count_page(pte, args);

		addr += PAGE_SIZE;
		if (addr >= end)
			break;
		pte++;
	}

	return 0;
}

static inline int pmd_walk(pud_t *src_pud,
	unsigned long addr,
	unsigned long end,
	struct mm_struct *src_mm,
	struct vm_area_struct *vma,
	struct expose_count_args *args,
	struct va_info *lst)
{

	int err;
	unsigned long next;
	pmd_t *src_pmd = pmd_offset(src_pud, addr);

	do {
		next = pmd_addr_end(addr, end);

		if (pmd_none_or_clear_bad(src_pmd))
			continue;

		err = pte_walk(src_pmd, addr, next, src_mm, vma, args, lst);
		if (err < 0)
			return err;

	} while (src_pmd++, addr = next, addr != end);

	return 0;
}

static inline int pud_walk(p4d_t *src_p4d,
	unsigned long addr,
	unsigned long end,
	struct mm_struct *src_mm,
	struct vm_area_struct *vma,
	struct expose_count_args *args,
	struct va_info *lst)
{
	int err;
	unsigned long next;
	pud_t *src_pud = pud_offset(src_p4d, addr);

	do {
		next = pud_addr_end(addr, end);

		if (pud_none_or_clear_bad(src_pud))
			continue;

		err = pmd_walk(src_pud, addr, next, src_mm, vma, args, lst);

		if (err < 0)
			return err;

	} while (src_pud++, addr = next, addr != end);

	return 0;
}

static inline int p4d_walk(pgd_t *src_pgd,
	unsigned long addr,
	unsigned long end,
	struct mm_struct *src_mm,
	struct vm_area_struct *vma,
	struct expose_count_args *args,
	struct va_info *lst)
{
	int err;
	p4d_t *src_p4d = p4d_offset(src_pgd, addr);
	unsigned long next;

	if (CONFIG_PGTABLE_LEVELS == 4)
		return pud_walk((p4d_t *)src_pgd, addr, end,
			src_mm, vma, args, lst);

	do {
		next = p4d_addr_end(addr, end);

		if (p4d_none_or_clear_bad(src_p4d))
			continue;

		err = pud_walk(src_p4d, addr, next,
			src_mm, vma, args, lst);
		if (err < 0)
			return err;

	} while (src_p4d++, addr = next, addr != end);

	return 0;
}

static inline int pgd_walk(
	struct mm_struct *src_mm,
	struct vm_area_struct *vma,
	struct expose_count_args *args,
	struct va_info *lst)
{
	int err;
	unsigned long addr = vma->vm_start;
	unsigned long end = vma->vm_end;
	unsigned long next;

	pgd_t *src_pgd = pgd_offset(src_mm, addr);

	do {
		next = pgd_addr_end(addr, end);

		if (pgd_none_or_clear_bad(src_pgd))
			continue;

		err = p4d_walk(src_pgd, addr, next,
		       src_mm, vma, args, lst);

		if (err < 0)
			return err;
	} while (src_pgd++, addr = next, addr != end);

	return 0;
}

static inline int mmap_walk(struct mm_struct *srcmm,
		struct expose_count_args *args, struct va_info *lst)
{
	struct vm_area_struct *mpnt;
	int ret;

	for (mpnt = srcmm->mmap; mpnt; mpnt = mpnt->vm_next) {
		ret = pgd_walk(srcmm, mpnt, args, lst);
		if (ret < 0)
			return ret;
	}
	return 0;
}

ssize_t
ppage_file_read_iter(struct kiocb *iocb, struct iov_iter *iter);

int ppage_simple_unlink(struct inode *dir, struct dentry *dentry)
{
	return  simple_unlink(dir, dentry);
}

int ppage_piddir_delete(const struct dentry *dentry)
{
	struct inode *inode = d_inode(dentry);
	struct p_info *info = (struct p_info *) inode->i_private;

	
	/*
	 * if inode should be retained,
	 *
	 * 1. clear the retain field, this will be set again in open
	 * if the process related to this inode is still running
	 *
	 * 2. we return 0 so that it will not be killed by dput
	 */
	if (info->retain)
		return info->retain = 0;

	kfree(inode->i_private);
	return 1;
}

int ppage_file_delete(const struct dentry *dentry)
{
	struct inode *inode;

	if (is_zero_or_total_file(dentry)) {
		inode = dentry->d_inode;
		kfree(inode->i_private);
	}

	return 1;
}

struct inode *ppagefs_pid_dir_inode(pid_t pid, const char *comm, struct dentry *parent)
{
	struct inode *inode;
	struct p_info *info;
retry:
	printk(KERN_DEBUG "[ DEBUG ] [%s]\n", __func__);
	/* obtains the inode with ino=pid */
	inode = iget_locked(parent->d_sb, pid);
	/* 
	 * if old inode, set retained and return old dentry
	 * if new inode, clean up the previous dentry+inode and set relevant field
	 */
	info = inode->i_private;
	if (!(inode->i_state & I_NEW)) {
		if (strncmp(info->comm, comm, TASK_COMM_LEN) == 0) {
			printk(KERN_DEBUG "[ DEBUG ] [%s] old inode %d.%s\n", 
					__func__, pid, info->comm);
			info->retain = 1;
			return inode;
		}

		/* found but different process name */
		printk(KERN_DEBUG "[ DEBUG ] [%s] delete and go retry\n", __func__);
		dput(info->dentry);
		goto retry;
	}
	/* new inode, sets relevant field */
	info = kmalloc(sizeof(struct p_info), GFP_KERNEL);

	inode->i_atime = inode->i_mtime = inode->i_ctime = current_time(inode);
	inode->i_private = info;
	inode->i_fop = &ppagefs_fake_dir_operations;
	inode->i_op = &ppagefs_fake_dir_inode_operations;
	inode->i_mode	= S_IFDIR | 0755;

	/* set private data */
	info->retain = 1;
	info->pid = pid;
	strncpy(info->comm, comm, TASK_COMM_LEN);
	info->dentry = NULL;
	INIT_LIST_HEAD(&info->head);
			
	printk(KERN_DEBUG "[ DEBUG ] [%s] new inode initialized %d\n", 
					__func__, info->pid);
	
	unlock_new_inode(inode);
	return inode;
}

struct dentry *ppagefs_pid_dir(struct task_struct *p, struct dentry *parent)
{
	char buf[30];
	pid_t pid;
	struct dentry *dir;
	struct p_info *info;
	struct inode *inode;
	
	printk(KERN_DEBUG "[ DEBUG ] [%s]\n", __func__);

	pid = task_pid_vnr(p);
	inode = ppagefs_pid_dir_inode(pid, p->comm, parent);
	info = inode->i_private;

	/* if has dentry, then we are done */
	if (info->dentry) {
		return info->dentry;
	}
	
	/* create new dentry and link the inode */
	printk(KERN_DEBUG "[ DEBUG ] [%s] new_dir \n", __func__);
	
	/* construct directory name and escape '/' */
	snprintf(buf, sizeof(buf), "%d.%s", pid, p->comm);
	strreplace(buf, '/', '-');

	dir = ppagefs_create_dir(buf, inode, parent);
	if (!dir)
		goto create_dir_fail;

	dir->d_flags |= DCACHE_OP_DELETE;
	dir->d_op = &ppagefs_piddir_d_ops;
	dir->d_lockref.count = 0;

	return dir;

create_dir_fail:
	return NULL;
}

static void inline reset_p_info(struct dentry *parent)
{
	struct list_head *p;

	spin_lock(&parent->d_lock);
	p = &parent->d_subdirs;
	while((p = p->next) != &parent->d_subdirs) {
		struct dentry *d = list_entry(p, struct dentry, d_child);
		struct inode *i = d_inode(d);
		struct p_info *info = i->i_private;

		/* reset */
		info->retain = 0;
	}
	spin_unlock(&parent->d_lock);
}

int ppage_dcache_dir_open(struct inode *inode, struct file *file)
{
	int err = 0;
	struct dentry *dentry, *pid_dir;
	struct task_struct *p;
	struct list_head *cursor, *pos, *n;
	LIST_HEAD(d_list);

	err = dcache_dir_open(inode, file);
	if (err)
		return err;

	dentry = file->f_path.dentry;

	reset_p_info(dentry);

	/*
	 * Create a PID.PROCESSNAME directory for each
	 * currently running process
	 */
	rcu_read_lock();

	for_each_process(p) {
		get_task_struct(p);
		pid_dir = ppagefs_pid_dir(p, dentry);
		put_task_struct(p);

		if (!pid_dir)
			return -ENOMEM;
	}

	rcu_read_unlock();

	/* delete PID.PROCESSNAME directories where PID is not running */
	spin_lock(&dentry->d_lock);
	cursor = dentry->d_subdirs.next;
	while(cursor != &dentry->d_subdirs) {
		struct dentry *d = list_entry(cursor, struct dentry, d_child);
		struct inode *i = d_inode(d);
		struct p_info *info = i->i_private;

		/*
		 * get the next element before
		 * possible deletion of current dentry
		 */
		cursor = cursor->next;

		/*
		 * add dentry to delete to d_list, and dput later
		 * to avoid racing condition
		 */
		if(!info->retain)
			list_add_tail(&info->head, &d_list);
	}
	spin_unlock(&dentry->d_lock);

	list_for_each_safe(pos, n, &d_list) {
		struct p_info *info = list_entry(pos, struct p_info, head);
		dput(info->dentry);
	}

	return err;
}

int ppage_dcache_dir_close(struct inode *inode, struct file *file)
{
	return  dcache_dir_close(inode, file);
}

loff_t ppage_dcache_dir_lseek(struct file *file, loff_t offset, int whence)
{
	return dcache_dir_lseek(file, offset, whence);
}

ssize_t ppage_generic_read_dir(struct file *filp, char __user *buf,
		size_t siz, loff_t *ppos)
{
	return  generic_read_dir(filp, buf, siz, ppos);
}

int ppage_dcache_readdir(struct file *file, struct dir_context *ctx)
{
	return dcache_readdir(file, ctx);
}

struct dentry
*ppage_lookup(struct inode *dir, struct dentry *dentry, unsigned int flags)
{
	return simple_lookup(dir, dentry, flags);
}

static struct dentry *ppage_root_lookup(struct inode *dir,
		struct dentry *dentry, unsigned int flags)
{
	struct dentry *pid_dir;
	struct task_struct *p;
	const char *dirname = dentry->d_name.name;
	pid_t pid;
	char comm[TASK_COMM_LEN] = "", p_comm[TASK_COMM_LEN];
	int match;

	match = sscanf(dirname, "%d.%s", &pid, comm);
	if (match != 2)
		goto dir_not_found;

	rcu_read_lock();
	p = find_task_by_vpid(pid);
	rcu_read_unlock();

	if (!p)
		goto dir_not_found;

	/* process found, check if process name matches */
	get_task_struct(p);
	strncpy(p_comm, p->comm, sizeof(p_comm));
	strreplace(p_comm, '/', '-');

	if (strncmp(comm, p_comm, strlen(comm) + 1)) {
		put_task_struct(p);
		goto dir_not_found;
	}

	/* process exists and process name matches */
	pid_dir = ppagefs_pid_dir(p,
			dentry->d_sb->s_root);

	put_task_struct(p);

	return pid_dir;

dir_not_found:
	return NULL;
}

static struct dentry *ppage_fake_lookup(struct inode *dir,
		struct dentry *dentry, unsigned int flags)
{
	struct dentry *ret_dentry = NULL;
	static const struct tree_descr ppage_files[] = {
		{"total", &ppagefs_file_operations, 0644},
		{"zero", &ppagefs_file_operations, 0644},
	};

	if (is_total_file(dentry)) {
		ret_dentry = ppage_create_file(dentry->d_parent,
				&ppage_files[0], 1);
		if (!ret_dentry)
			return NULL;
	}


	if (is_zero_file(dentry)) {
		ret_dentry = ppage_create_file(dentry->d_parent,
				&ppage_files[1], 1);
		if (!ret_dentry)
			return NULL;
	}

	return ret_dentry;
}

int ppage_fake_dir_open(struct inode *inode, struct file *file)
{
	int err = 0;
	struct dentry *dentry = file->f_path.dentry, *ret_dentry;
	static const struct tree_descr ppage_files[] = {
		{"total", &ppagefs_file_operations, 0644},
		{"zero", &ppagefs_file_operations, 0644},
	};

	err = dcache_dir_open(inode, file);
	if (err)
		return err;

	ret_dentry = ppage_create_file(dentry, &ppage_files[0], 0);
	if (!ret_dentry)
		return -ENOMEM;

	ret_dentry = ppage_create_file(dentry, &ppage_files[1], 0);
	if (!ret_dentry)
		return -ENOMEM;

	return err;
}

ssize_t
ppage_file_read_iter(struct kiocb *iocb, struct iov_iter *iter)
{
	struct dentry *dentry = file_dentry(iocb->ki_filp);
	char *data = "50\n";
	size_t ret = 0, size = strlen(data) + 1;
	loff_t fsize = strlen(data) + 1, fpos = iocb->ki_pos;

	if (fpos >= fsize)
		return ret;

	/* we need to copy back some data */
	if (fpos + iter->count > size)
		size -= fpos;
	else
		size = iter->count;

	ret = _copy_to_iter(data + fpos, size, iter);
	iocb->ki_pos += ret;

	// configures this dentry to be deleted after read
	if (dentry->d_lockref.count != 0)
		dentry->d_lockref.count = 0;

	return ret;
}

struct inode *ppage_get_inode(struct super_block *sb, umode_t mode)
{
	struct inode *inode = new_inode(sb);

	if (inode) {
		inode->i_ino = get_next_ino();
		inode->i_atime = inode->i_mtime =
			inode->i_ctime = current_time(inode);

		switch (mode & S_IFMT) {
		case S_IFDIR:
			inode->i_mode	= S_IFDIR | 0755;
			inode->i_op	= &ppagefs_dir_inode_operations;
			inode->i_fop	= &simple_dir_operations;
			break;
		case S_IFREG:
			inode->i_mode	= S_IFREG | 0644;
			inode->i_op	= &ppagefs_file_inode_operations;
			inode->i_fop	= &ppagefs_file_operations;
			break;
		default:
			inode->i_op	= &ppagefs_file_inode_operations;
			inode->i_fop	= &ppagefs_file_operations;
			break;
		}
	}

	return inode;
}

/**
 * fills in the @info by parsing dir_name
 * - if something went wrong, @info->pid = -1;
 */
static void parse_pid_dir_name(const char *dir_name, struct p_info *info)
{
	char buf[20];
	char comm[TASK_COMM_LEN];
	pid_t pid;
	int match;

	strncpy(buf, dir_name, sizeof(buf));
	buf[sizeof(buf)-1] = '\0';

	// parses the pid and message
	match = sscanf(dir_name, "%d.%s", &pid, comm);
	if (match != 2)
		goto fill_err;

	info->pid = pid;
	strncpy(info->comm, comm, TASK_COMM_LEN);

	return;

fill_err:
	info->pid = -1;
}

/**
 * This function will create files that will be:
 * - deleted on the fly if @count=0
 * - remains if @count=1
 */
struct dentry *ppage_create_file(struct dentry *parent,
		const struct tree_descr *file, int count)
{
	struct dentry *dentry;
	struct inode *inode;
	struct p_info *info;

	/* attempts to create a folder */
	dentry = d_alloc_name(parent, file->name);
	if (!dentry)
		return NULL;

	inode = ppage_get_inode(parent->d_sb, S_IFREG);
	if (!inode)
		return NULL;

	d_add(dentry, inode);

	dentry->d_flags |= DCACHE_OP_DELETE;
	dentry->d_lockref.count = count;
	dentry->d_op = &ppagefs_file_d_ops;

	info = kmalloc(sizeof(struct p_info), GFP_KERNEL);
	parse_pid_dir_name(parent->d_name.name, info);

	inode->i_private = info;

	return dentry;
}

struct dentry *ppagefs_create_dir(const char *name, struct inode *inode, struct dentry *parent)
{
	struct dentry *dentry;

	/* attempts to create a folder */
	dentry = d_alloc_name(parent, name);
	if (!dentry)
		return NULL;

	if (!inode)
		return NULL;

	d_add(dentry, inode);

	return dentry;
}

static int ppagefs_fill_super(struct super_block *sb, struct fs_context *fc)
{
	int err = 0;
	static const struct tree_descr ppage_files[] = {{""}};

	/*
	 * @ppage_files: contains information about all directories under
	 * the root of the filesystem
	 */
	err  =  simple_fill_super(sb, PPAGEFS_MAGIC, ppage_files);
	if (err)
		goto fail;

	sb->s_root->d_inode->i_op = &ppagefs_root_inode_operations;
	sb->s_root->d_inode->i_fop = &ppage_dir_operations;
fail:
	return err;
}

static int ppage_get_tree(struct fs_context *fc)
{
	return get_tree_nodev(fc, ppagefs_fill_super);
}

static void ppage_free_fc(struct fs_context *fc)
{
}


int ppagefs_init_fs_context(struct fs_context *fc)
{
	fc->ops = &ppagefs_context_ops;
	return 0;
}


static int __init ppagefs_init(void)
{
	return register_filesystem(&ppage_fs_type);
}
fs_initcall(ppagefs_init);
