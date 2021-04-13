#include <linux/fs.h>
#include <uapi/linux/magic.h>
#include <linux/cgroup-defs.h>
#include <linux/init.h>
#include <linux/fs_context.h>
#include <uapi/linux/stat.h>
#include <linux/string.h>
#include <linux/ppage_fs.h>
#include <linux/uio.h>

struct dentry *ppagefs_create_dir(const char *name, struct dentry *parent);
struct inode *ppagefs_get_inode(struct super_block *, umode_t);

ssize_t
ppage_file_read_iter(struct kiocb *iocb, struct iov_iter *iter);

int ppagefs_simple_unlink(struct inode *dir, struct dentry *dentry)
{
	printk(KERN_DEBUG "[ DEBUG ] --%s--\n", __func__);
	return  simple_unlink(dir, dentry);
}

int ppage_delete(const struct dentry * dentry)
{
	//dentry->d_flags &= ~DCACHE_SHRINK_LIST;
	printk(KERN_DEBUG "[ DEBUG ] --%s-- ref_count=%d with flag=%x\n", __func__, 
			dentry->d_lockref.count,
			dentry->d_flags);
	return 1;
}

static const struct dentry_operations ppagefs_d_op = {
	.d_delete 		= ppage_delete,
};

int ppage_dcache_dir_open(struct inode *inode, struct file *file)
{
	int err = 0;
	struct dentry *dentry = file->f_path.dentry, *ret_dentry;
	struct dentry *d;
	struct list_head *anchor = &dentry->d_subdirs;
	char buf[20];
	pid_t pid;
	
	printk(KERN_DEBUG "[ DEBUG ] --%s--\n", __func__);

	pid = task_pid_vnr(current);
	snprintf(buf, sizeof(buf), "nieh_test%d", pid);

	if (!(ret_dentry = ppagefs_create_dir(buf, dentry))) {
		printk(KERN_DEBUG "[ DEBUG ] --%s-- create nieh_test/ failed.\n", __func__);
		return -ENOMEM;
	}

	ret_dentry->d_flags |= DCACHE_OP_DELETE;
	ret_dentry->d_lockref.count--;
	ret_dentry->d_op = &ppagefs_d_op;

	//dput(ret_dentry);
#if 0
	
	d = list_entry(anchor->next, struct dentry, d_child);
	printk(KERN_DEBUG "[ DEBUG ] --%s-- writing anchor_next@(%s)\n", __func__, d->d_name.name);
	d = list_entry(anchor->prev, struct dentry, d_child);
	printk(KERN_DEBUG "[ DEBUG ] --%s-- writing anchor_prev@(%s)\n", __func__, d->d_name.name);
	
	printk(KERN_DEBUG "[ DEBUG ] --%s-- writing anchor->next\n", __func__);
	//list_add_tail(&ret_dentry->d_child, anchor->next);
	//anchor->next = &ret_dentry->d_child;
	printk(KERN_DEBUG "[ DEBUG ] --%s-- ref_count=%d with flag=%x\n", __func__, 
			ret_dentry->d_lockref.count,
			ret_dentry->d_flags);

	err = dcache_dir_open(inode, file);
	if (err)
		return err;
	dentry = file->private_data;
	printk(KERN_DEBUG "[ SUCCESS ] --%s-- cursor node @file->private_data allocated.\n", __func__);
	
	fake_inode = ppagefs_get_inode(dentry->d_parent->d_sb, S_IFDIR);
	if (!fake_inode)
		return -ENOMEM;
	printk(KERN_DEBUG "[ SUCCESS ] --%s-- cursor inode allocated\n", __func__);
	fake_inode->i_private = "ppagefs_cursor";

	/* increment directory counts */
	inc_nlink(fake_inode);
	inc_nlink(d_inode(dentry->d_parent));
	d_add(dentry, fake_inode);
	printk(KERN_DEBUG "[ SUCCESS ] --%s-- linked cursor inode and dentry\n", __func__);
	

	if (!ppagefs_create_dir("nieh", dentry)) {
		printk(KERN_DEBUG "[ DEBUG ] --%s-- create nieh/ failed.\n", __func__);
		return -ENOMEM;
	}

	printk(KERN_DEBUG "[ DEBUG ] --%s-- create nieh/ succeeded.\n", __func__);
#endif	
	return dcache_dir_open(inode, file);
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

const struct file_operations ppage_dir_operations = {
	.open		= ppage_dcache_dir_open,
	.release	= ppage_dcache_dir_close,
	.llseek		= ppage_dcache_dir_lseek,
	.read		= ppage_generic_read_dir,
	.iterate_shared	= ppage_dcache_readdir,
	.fsync		= noop_fsync,
};

static struct dentry *ppagefs_root_lookup(struct inode * dir, 
		struct dentry * dentry, unsigned int flags)
{
	printk(KERN_DEBUG "[ DEBUG ] --%s-- with dentry=%s\n", 
			__func__, dentry->d_name.name);
	return NULL;
	//return simple_lookup(dir, dentry, flags);
}

static const struct inode_operations ppagefs_dir_inode_operations = {
	.lookup		= simple_lookup,
	.link		= simple_link,
	.unlink		= ppagefs_simple_unlink,
	.rmdir		= simple_rmdir,
	.rename		= simple_rename,
};

static const struct inode_operations ppagefs_root_inode_operations = {
	.lookup		= ppagefs_root_lookup,
};

const struct inode_operations ppagefs_file_inode_operations = {
	.setattr	= simple_setattr,
	.getattr	= simple_getattr,
};

const struct file_operations ppagefs_file_operations = {
	.read_iter	= ppage_file_read_iter,
//	.write_iter	= generic_file_write_iter,
	.mmap		= generic_file_mmap,
	.fsync		= noop_fsync,
	.splice_read	= generic_file_splice_read,
	.splice_write	= iter_file_splice_write,
	.llseek		= generic_file_llseek,
};

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

struct inode *ppagefs_get_inode(struct super_block *sb, umode_t mode)
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
			default:
				inode->i_op	= &ppagefs_file_inode_operations;
				inode->i_fop	= &ppagefs_file_operations;
				break;
		}
	}

	return inode;
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
	
	inode = ppagefs_get_inode(parent->d_sb, S_IFDIR);
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
#if 0
	if (ppagefs_create_dir("nieh", sb->s_root) == NULL) {
		return -ENOMEM;
	}
#endif
fail:
	return err;
}

static int ppagefs_get_tree(struct fs_context *fc)
{
	return get_tree_nodev(fc, ppagefs_fill_super);
}

static void ppagefs_free_fc(struct fs_context *fc)
{
}

static const struct fs_context_operations ppagefs_context_ops = {
	.free		= ppagefs_free_fc,
	.get_tree	= ppagefs_get_tree,
};

int ppagefs_init_fs_context(struct fs_context *fc)
{
	fc->ops = &ppagefs_context_ops;
	return 0;
}

static struct file_system_type ppage_fs_type = {
	.name =		"ppagefs",
	.init_fs_context = ppagefs_init_fs_context,
	.kill_sb =	kill_litter_super,
};

static int __init ppagefs_init(void)
{
	return register_filesystem(&ppage_fs_type);;
}
fs_initcall(ppagefs_init);
