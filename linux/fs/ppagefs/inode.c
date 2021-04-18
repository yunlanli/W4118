#include "internal.h"
#include "pagewalk.h"

/* filesystem specific structures */
static struct file_system_type ppage_fs_type = {
	.name =		"ppagefs",
	.init_fs_context = ppagefs_init_fs_context,
	.kill_sb =	kill_litter_super,
};

static const struct fs_context_operations ppagefs_context_ops = {
	.free		= ppage_free_fc,
	.get_tree	= ppage_get_tree,
};

/* ppagefs root directory operations */
static const struct inode_operations ppagefs_root_inode_operations = {
	.lookup		= ppage_root_lookup,
	.permission	= generic_permission,
};

const struct file_operations ppage_root_dir_operations = {
	.open		= ppage_dcache_dir_open,
	.release	= ppage_dcache_dir_close,
	.llseek		= ppage_dcache_dir_lseek,
	.read		= ppage_generic_read_dir,
	.iterate_shared	= ppage_dcache_readdir,
	.fsync		= noop_fsync,
};

/* ppagefs PID.PROCESSNAME directory operations */
static const struct inode_operations ppagefs_piddir_inode_operations = {
	.lookup		= ppage_piddir_lookup,
	.permission	= generic_permission,
};

const struct file_operations ppagefs_piddir_operations = {
	.open		= ppage_piddir_open,
	.release	= ppage_dcache_dir_close,
	.llseek		= ppage_dcache_dir_lseek,
	.read		= ppage_generic_read_dir,
	.iterate_shared	= ppage_dcache_readdir,
	.fsync		= noop_fsync,
};

const struct dentry_operations ppagefs_piddir_d_ops = {
	.d_delete	= ppage_piddir_delete,
};

/* ppagefs file {zero | total} operations */
const struct inode_operations ppagefs_file_inode_operations = {
	.setattr	= simple_setattr,
	.getattr	= simple_getattr,
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

static const struct dentry_operations ppagefs_file_d_ops = {
	.d_delete	= ppage_file_delete,
};

/* generic operations */
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
			inode->i_op	= &ppagefs_piddir_inode_operations;
			inode->i_fop	= &ppagefs_piddir_operations;
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

/* operations for PID.PROCESSNAME */
struct inode
*ppagefs_pid_dir_inode(pid_t pid, const char *comm, struct dentry *parent)
{
	struct inode *inode;
	struct p_info *info;
retry:
	/* obtains the inode with ino=pid */
	inode = iget_locked(parent->d_sb, pid);
	/*
	 * if old inode, set retained and return old dentry
	 * if new inode, clean up the previous dentry+inode
	 * and set relevant field
	 */
	info = inode->i_private;
	if (!(inode->i_state & I_NEW) && info) {
		if (strncmp(info->comm, comm, TASK_COMM_LEN) == 0) {
			info->retain = 1;
			return inode;
		}

		/* found but different process name */
		info->dentry->d_flags |= DCACHE_OP_DELETE;
		info->dentry->d_op = &ppagefs_piddir_d_ops;
		info->dentry->d_lockref.count = 1;
		info->retain = 0;
		dput(info->dentry);
		goto retry;
	}
	/* new inode, sets relevant field */
	info = kmalloc(sizeof(struct p_info), GFP_KERNEL);

	inode->i_atime = inode->i_mtime = inode->i_ctime = current_time(inode);
	inode->i_private = info;
	inode->i_fop = &ppagefs_piddir_operations;
	inode->i_op = &ppagefs_piddir_inode_operations;
	inode->i_mode	= S_IFDIR | 0755;

	/* set private data */
	info->retain = 1;
	info->pid = pid;
	strncpy(info->comm, comm, TASK_COMM_LEN);
	info->dentry = NULL;
	INIT_LIST_HEAD(&info->head);

	unlock_new_inode(inode);
	return inode;
}

struct dentry *ppagefs_create_dir(const char *name,
		struct inode *inode, struct dentry *parent)
{
	struct dentry *dentry;
	struct p_info *info;

	/* attempts to create a folder */
	dentry = d_alloc_name(parent, name);
	if (!dentry)
		return NULL;

	if (!inode)
		return NULL;

	d_add(dentry, inode);
	info = inode->i_private;
	info->dentry = dentry;

	return dentry;
}

struct dentry *ppagefs_pid_dir(struct task_struct *p, struct dentry *parent)
{
	char buf[30];
	pid_t pid;
	struct dentry *dir;
	struct p_info *info;
	struct inode *inode;

	pid = task_pid_vnr(p);
	inode = ppagefs_pid_dir_inode(pid, p->comm, parent);
	info = inode->i_private;

	/* if has dentry, then we are done */
	if (info->dentry)
		return info->dentry;

	/* create new dentry and link the inode */
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

static inline void reset_p_info(struct dentry *parent)
{
	struct list_head *p;

	spin_lock(&parent->d_lock);
	p = &parent->d_subdirs;
	while ((p = p->next) != &parent->d_subdirs) {
		struct dentry *d = list_entry(p, struct dentry, d_child);
		struct inode *i = d_inode(d);
		struct p_info *info = i->i_private;

		/* reset */
		info->retain = 0;
	}
	spin_unlock(&parent->d_lock);
}

int ppage_piddir_delete(const struct dentry *dentry)
{
	struct inode *inode = d_inode(dentry);
	struct p_info *info = (struct p_info *) inode->i_private;

	if (info->retain)
		return 0;

	kfree(inode->i_private);
	return 1;
}

int ppage_dcache_dir_open(struct inode *inode, struct file *file)
{
	int err = 0;
	struct dentry *dentry, *pid_dir;
	struct task_struct *p;
	struct list_head *cursor, *pos, *n;
	struct p_info *info;
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
	cursor = &dentry->d_subdirs;
	while ((cursor = cursor->next) != &dentry->d_subdirs) {
		struct dentry *d = list_entry(cursor, struct dentry, d_child);
		struct inode *i = d_inode(d);
		struct p_info *info = i->i_private;

		/*
		 * add dentry to delete to d_list, and dput later
		 * to avoid racing condition
		 */
		if (!info->retain)
			list_add_tail(&info->head, &d_list);
	}
	spin_unlock(&dentry->d_lock);

	list_for_each_safe(pos, n, &d_list) {
		info = list_entry(pos, struct p_info, head);
		dput(info->dentry);
	}

	return err;
}

struct dentry
*ppage_lookup(struct inode *dir, struct dentry *dentry, unsigned int flags)
{
	return simple_lookup(dir, dentry, flags);
}

static struct dentry *ppage_piddir_lookup(struct inode *dir,
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

int ppage_piddir_open(struct inode *inode, struct file *file)
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

static inline struct task_struct *process_exists(struct p_info *info)
{
	struct task_struct *p;
	pid_t pid = info->pid;
	const char *comm = info->comm;

	if (pid == -1)
		return NULL;

	rcu_read_lock();
	p = find_task_by_vpid(pid);
	rcu_read_unlock();

	if (!p)
		return NULL;
	if (strncmp(p->comm, comm, TASK_COMM_LEN) != 0)
		return NULL;

	return p;
}

ssize_t
ppage_file_read_iter(struct kiocb *iocb, struct iov_iter *iter)
{
	char data[70];
	struct dentry *dentry = file_dentry(iocb->ki_filp);
	struct p_info *p_info = dentry->d_inode->i_private;
	struct task_struct *p;
	size_t ret = 0, size;
	struct expose_count_args args = {0, 0};
	struct va_info lst;
	loff_t fsize, fpos = iocb->ki_pos;

	lst.root = &RB_ROOT;
	p = process_exists(p_info);

	if (!p)
		return 0;

	get_task_struct(p);
	mmap_walk(p->mm, &args, &lst);
	put_task_struct(p);
	free_pfn_rb_tree(lst.root);

	if (is_zero_file(dentry))
		snprintf(data, sizeof(data), "%ld\n", args.zero);
	else
		snprintf(data, sizeof(data), "%ld\n", args.total);

	fsize = strlen(data) + 1;
	size = strlen(data) + 1;

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

/*
 * fills in the @info by parsing dir_name
 * - if something went wrong, @info->pid = -1;
 */
static void parse_pid_dir_name(const char *dir_name, struct p_info *info)
{
	char buf[20];
	char comm[TASK_COMM_LEN];
	pid_t pid;
	struct task_struct *task;
	int match;

	strncpy(buf, dir_name, sizeof(buf));
	buf[sizeof(buf)-1] = '\0';

	// parses the pid and message
	match = sscanf(dir_name, "%d.%s", &pid, comm);
	if (match != 2)
		goto fill_err;

	task = find_task_by_vpid(pid);
	if (!task) {
		info->pid = -1;
		return;
	}
	info->pid = pid;
	strncpy(info->comm, task->comm, TASK_COMM_LEN);

	return;

fill_err:
	info->pid = -1;
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

/*
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

/* operations for ppagefs root directory */
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

/* ppagefs initialization functions */
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
	sb->s_root->d_inode->i_fop = &ppage_root_dir_operations;
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
