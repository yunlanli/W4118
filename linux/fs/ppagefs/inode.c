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


struct dentry *ppagefs_create_dir(const char *name, struct dentry *parent);

struct inode *ppage_get_inode(struct super_block *, umode_t);

int ppage_fake_dir_open(struct inode *inode, struct file *file);

static struct dentry *ppage_fake_lookup(struct inode * dir, 
		struct dentry * dentry, unsigned int flags);

struct dentry *ppage_create_file(struct dentry *parent, const struct tree_descr *files);

struct dentry *ppage_lookup(struct inode *dir, struct dentry *dentry, unsigned int flags);

int ppage_simple_unlink(struct inode *dir, struct dentry *dentry);

static struct dentry *ppage_root_lookup(struct inode * dir, 
		struct dentry * dentry, unsigned int flags);

ssize_t ppage_file_read_iter(struct kiocb *iocb, struct iov_iter *iter);

static void ppage_free_fc(struct fs_context *fc);

static int ppage_get_tree(struct fs_context *fc);

int ppage_dcache_dir_open(struct inode *inode, struct file *file);

int ppage_dcache_dir_close(struct inode *inode, struct file *file);

loff_t ppage_dcache_dir_lseek(struct file *file, loff_t offset, int whence);

ssize_t ppage_generic_read_dir(struct file *filp, char __user *buf, size_t siz, loff_t *ppos);

int ppage_dcache_readdir(struct file *file, struct dir_context *ctx);

int ppage_delete(const struct dentry * dentry);

int ppagefs_init_fs_context(struct fs_context *fc);

const struct dentry_operations ppage_dentry_operations = {
	.d_delete	= ppage_delete,
};

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

static const struct dentry_operations ppagefs_d_op = {
	.d_delete 		= ppage_delete,
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

struct expose_count_args{
	int total;
	int zero;
};

void pfn_rb_insert(struct rb_root *root, struct pfn_node *new, struct expose_count_args *args)
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
	// int ret;
    	struct pfn_node *new;
   	// struct page *page;
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
	// int err;
	// unsigned long next;
	// unsigned long pfn;
	// struct page *page;

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
	do
	{
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

static inline int mmap_walk(struct mm_struct *srcmm, struct expose_count_args *args,
	struct va_info *lst)
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
	printk(KERN_DEBUG "[ DEBUG ] --%s--\n", __func__);
	return  simple_unlink(dir, dentry);

}

int ppage_delete(const struct dentry * dentry)
{
	printk(KERN_DEBUG "[ DEBUG ] --%s-- ref_count=%d with flag=%x\n", __func__, 
			dentry->d_lockref.count,
			dentry->d_flags);
	return 1;
}

struct dentry *ppagefs_pid_dir(struct task_struct *p, struct dentry *parent)
{
	char buf[30];
	pid_t pid;
	struct dentry *dir;

	pid = task_pid_vnr(p);
	printk(KERN_DEBUG "[ INFO ] --%s-- current->comm: %s\n",
			__func__, p->comm);
	
	/* construct directory name and escape '/' */
	snprintf(buf, sizeof(buf), "%d.%s", pid, p->comm);
	strreplace(buf, '/', '-');

	dir = ppagefs_create_dir(buf, parent);
	if (!dir) {
		printk(KERN_DEBUG "[ DEBUG ] --%s-- create %s/ failed.\n",
				__func__, buf);
		return NULL;
	}
	/*
	dir->d_flags |= DCACHE_OP_DELETE;
	dir->d_op = &ppage_dentry_operations;
	dir->d_lockref.count = 0;
	*/
	// changes file operation such as open
	dir->d_inode->i_fop = &ppagefs_fake_dir_operations;
	dir->d_inode->i_op = &ppagefs_fake_dir_inode_operations;

	printk(KERN_DEBUG "[ DEBUG ] --%s-- create %s/ succeeded.\n",
			__func__, buf);

	return dir;
}
int ppage_dcache_dir_open(struct inode *inode, struct file *file)
{
	int err = 0;
	struct dentry *dentry, *pid_dir;
	struct task_struct *p;
	
	printk(KERN_DEBUG "[ DEBUG ] --%s--\n", __func__);

	err = dcache_dir_open(inode, file);
	if (err)
		return err;
	printk(KERN_DEBUG "[ SUCCESS ] --%s-- cursor node"
			  "@file->private_data allocated.\n", __func__);
	
	dentry = file->f_path.dentry;

	/*
	 * Create a PID.PROCESSNAME directory for each
	 * currently running process
	 */
	rcu_read_lock();

	for_each_process(p) {
		get_task_struct(p);
		pid_dir = ppagefs_pid_dir(p, dentry);
		put_task_struct(p);

		if (!pid_dir) {
			printk(KERN_DEBUG "[ DEBUG ] --%s-- "
					  "create %d/ failed.\n",
					__func__, task_pid_vnr(p));
			return -ENOMEM;
		}
	}

	rcu_read_unlock();
	
	return err;
}

int ppage_dcache_dir_close(struct inode *inode, struct file *file)
{
	printk(KERN_DEBUG "[ DEBUG ] --%s--\n", __func__);
	return  dcache_dir_close(inode, file);
}

loff_t ppage_dcache_dir_lseek(struct file *file, loff_t offset, int whence)
{
	printk(KERN_DEBUG "[ DEBUG ] --%s--\n", __func__);
	return dcache_dir_lseek(file, offset, whence);
}

ssize_t ppage_generic_read_dir(struct file *filp, char __user *buf, size_t siz, loff_t *ppos)
{
	printk(KERN_DEBUG "[ DEBUG ] --%s--\n", __func__);
	return  generic_read_dir(filp, buf, siz, ppos);
}

int ppage_dcache_readdir(struct file *file, struct dir_context *ctx)
{
	printk(KERN_DEBUG "[ DEBUG ] --%s--\n", __func__);

	return dcache_readdir(file, ctx);
}

struct dentry *ppage_lookup(struct inode *dir, struct dentry *dentry, unsigned int flags)
{
	printk(KERN_DEBUG "[ DEBUG ] --%s-- @dentry: %s\n",
			__func__, dentry->d_name.name);

	return simple_lookup(dir, dentry, flags);
}

static struct dentry *ppage_root_lookup(struct inode * dir, 
		struct dentry * dentry, unsigned int flags)
{
	struct dentry *pid_dir;
	struct task_struct *p;
	const char *dirname = dentry->d_name.name;
	long pid = -1;
	char comm[TASK_COMM_LEN] = "", p_comm[TASK_COMM_LEN];

	sscanf(dirname, "%ld.%s", &pid, comm);
	if (pid == -1 || !(*comm))
		goto dir_not_found;

	rcu_read_lock();
	p = find_task_by_vpid((pid_t) pid);
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
	printk(KERN_DEBUG "[ FAILURE ] --%s-- No such directory\n", __func__);
	return NULL;
}

static struct dentry *ppage_fake_lookup(struct inode * dir, 
		struct dentry * dentry, unsigned int flags)
{
	struct dentry *ret_dentry = NULL;
	const char *dirname = dentry->d_name.name;
	static const struct tree_descr ppage_files[] = {
		{"total", &ppagefs_file_operations, 0644},
		{"zero", &ppagefs_file_operations, 0644},
	};
	const char *zero = ppage_files[1].name;
	const char *total = ppage_files[0].name;
	const size_t len_d = strlen(dirname);
	const size_t len_z = strlen(zero);
	const size_t len_t = strlen(total);
	
	printk(KERN_DEBUG "[ DEBUG ] --%s-- looking up %s\n", __func__, dirname);
	
	if (len_d == len_t && strncmp(dirname, total, len_t) == 0) {
		if (!(ret_dentry = ppage_create_file(dentry->d_parent, &ppage_files[0]))) {
			printk(KERN_DEBUG "[ DEBUG ] --%s-- create files failed.\n", __func__);
			return NULL;
		}
		
		printk(KERN_DEBUG "[ DEBUG ] --%s-- created total file\n", __func__);
		
	}


	if (len_d == len_z && strncmp(dirname, zero, len_d) == 0) {
		if (!(ret_dentry = ppage_create_file(dentry->d_parent, &ppage_files[1]))) {
			printk(KERN_DEBUG "[ DEBUG ] --%s-- create files failed.\n", __func__);
			return NULL;
		}

		printk(KERN_DEBUG "[ DEBUG ] --%s-- created zero file\n", __func__);
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
	
	printk(KERN_DEBUG "[ DEBUG ] --%s--\n", __func__);
	
	err = dcache_dir_open(inode, file);
	if (err)
		return err;
	
	if (!(ret_dentry = ppage_create_file(dentry, &ppage_files[0]))) {
		printk(KERN_DEBUG "[ DEBUG ] --%s-- create files failed.\n", __func__);
		return -ENOMEM;
	}
	
	if (!(ret_dentry = ppage_create_file(dentry, &ppage_files[1]))) {
		printk(KERN_DEBUG "[ DEBUG ] --%s-- create files failed.\n", __func__);
		return -ENOMEM;
	}
	
	return err;
}

ssize_t
ppage_file_read_iter(struct kiocb *iocb, struct iov_iter *iter)
{
	char *data = "50";
	size_t ret = 0, size = strlen(data) + 1;
	loff_t fsize = strlen(data) + 1, fpos = iocb->ki_pos;

	if (fpos >= fsize)
		return ret;

	/* we need to copy back some data */
	if (fpos + iter->count > size)
		size -= fpos;
	else
		size = iter->count;

	printk(KERN_DEBUG "[ DEBUG ] --%s-- fpos: %lld\n", __func__, fpos);
	
	printk(KERN_DEBUG "[ DEBUG ] --%s-- copying %s to iter\n", __func__, data);
	ret = _copy_to_iter(data + fpos, size, iter);
	iocb->ki_pos += ret;
	printk(KERN_DEBUG "[ DEBUG ] --%s-- updated kiocb=%lld\n", __func__, iocb->ki_pos);

	return ret;
}

struct inode *ppage_get_inode(struct super_block *sb, umode_t mode)
{
	struct inode *inode = new_inode(sb);

	if (!inode) {
		printk(KERN_DEBUG "[ SUCCESS ] --%s-- created inode.\n", __func__);
	}
	printk(KERN_DEBUG "[ SUCCESS ] --%s-- created inode.\n", __func__);

	if (inode) {
		inode->i_ino = get_next_ino();
		inode->i_atime = inode->i_mtime = inode->i_ctime = current_time(inode);

		switch (mode & S_IFMT) {
			case S_IFDIR:
				inode->i_mode	= S_IFDIR | 0755;
				inode->i_op	= &ppagefs_dir_inode_operations;
				inode->i_fop	= &simple_dir_operations;
				break;
			case S_IFREG:
				inode->i_mode	= S_IFREG | 0644;
				inode->i_op	= &ppagefs_file_inode_operations;
				inode->i_fop 	= &ppagefs_file_operations;
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
 * This function will create files that will be deleted on the fly
 */
struct dentry *ppage_create_file(struct dentry *parent, const struct tree_descr *file)
{
	struct dentry *dentry; 
	struct inode *inode;
	
	printk(KERN_DEBUG "[ DEBUG ] --%s--\n", __func__);

	printk(KERN_DEBUG "[ DEBUG ] --%s-- parent_is=%s, file is=%s\n", 
			__func__, parent->d_name.name, file->name);
	
	/* attempts to create a folder */
	dentry = d_alloc_name(parent, file->name);
	if (!dentry)
		return NULL; // TODO: check error return values
	
	
	inode = ppage_get_inode(parent->d_sb, S_IFREG);
	if (!inode)
		return NULL;

	printk(KERN_DEBUG "[ DEBUG ] --%s-- linking.\n", __func__);

	d_add(dentry, inode);

	printk(KERN_DEBUG "[ DEBUG ] --%s-- done linking.\n", __func__);

	/*
	dentry->d_flags |= DCACHE_OP_DELETE;
	dentry->d_lockref.count = 0;
	dentry->d_op = &ppagefs_d_op;
	*/
	
	return dentry;
}

struct dentry *ppagefs_create_dir(const char *name, struct dentry *parent)
{
	struct dentry *dentry; 
	struct inode *inode;
	
	/* attempts to create a folder */
	dentry = d_alloc_name(parent, name);
	if (!dentry)
		return NULL; // TODO: check error return values
	printk(KERN_DEBUG "[ SUCCESS ] --%s-- created dentry.\n", __func__);
	
	inode = ppage_get_inode(parent->d_sb, S_IFDIR);
	if (!inode)
		return NULL;

	d_add(dentry, inode);
#if 0
	/* for debugging purposes */
	inode->i_private = name;

	/* increment directory counts */
	inc_nlink(inode);
	inc_nlink(d_inode(dentry->d_parent));
	d_add(dentry, inode);

	return dentry;

	/*
	 * cursor dentry is not backed by d_inode, in which case we skip the
	 * following. Otherwise, a null dereference would incur
	 */
	if (parent->d_inode) {
		/* increment directory counts */
		inc_nlink(inode);
		inc_nlink(d_inode(dentry->d_parent));
		d_add(dentry, inode);
	}
#endif
	return dentry;
}

static int ppagefs_fill_super(struct super_block *sb, struct fs_context *fc)
{
	int err = 0;
	static const struct tree_descr ppage_files[] = {
		{NULL},
		{NULL},
		{"jason", &ppagefs_file_operations, S_IRUSR | S_IWUSR},
		{"yunlan", &ppagefs_file_operations, S_IRUSR | 01000},
		{"dashi", &ppagefs_file_operations, 0755},
		{""}
	};

	/*
	 * @ppage_files: contains information about all directories under
	 * the root of the filesystem
	 */
	err  =  simple_fill_super(sb, PPAGEFS_MAGIC, ppage_files);
	if (err)
		goto fail;


	printk(KERN_DEBUG "[ DEBUG ] --%s-- inode_ino=%ld \n", __func__, 
			sb->s_root->d_inode->i_ino);

	sb->s_root->d_inode->i_op = &ppagefs_root_inode_operations;
	sb->s_root->d_inode->i_fop = &ppage_dir_operations;
	if (ppagefs_create_dir("nieh", sb->s_root) == NULL) {
		return -ENOMEM;
	}
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
	return register_filesystem(&ppage_fs_type);;
}
fs_initcall(ppagefs_init);
