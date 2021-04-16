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

struct dentry *ppagefs_create_dir(const char *name, struct dentry *parent);
struct inode *ppagefs_get_inode(struct super_block *, umode_t);
const struct dentry_operations ppage_dentry_operations;

ssize_t
ppage_file_read_iter(struct kiocb *iocb, struct iov_iter *iter);

int ppage_dcache_dir_open(struct inode *inode, struct file *file)
{
	int err = 0, ret;
	struct dentry *dentry, *subdir;
	char buf[10];
	pid_t pid;
	
	printk(KERN_DEBUG "[ DEBUG ] --%s--\n", __func__);
	
	err = dcache_dir_open(inode, file);
	if (err)
		return err;
	printk(KERN_DEBUG "[ SUCCESS ] --%s-- cursor node @file->private_data allocated.\n", __func__);
	
	dentry = file->f_path.dentry;

	pid = task_pid_vnr(current);
	printk(KERN_DEBUG "[ INFO ] --%s-- current->comm: %s\n", __func__, current->comm);
	
	snprintf(buf, sizeof(buf), "%d", pid);
	subdir = ppagefs_create_dir(buf, dentry);
	if (!subdir) {
		printk(KERN_DEBUG "[ DEBUG ] --%s-- create %s/ failed.\n", __func__, buf);
		return -ENOMEM;
	}
	subdir->d_flags |= DCACHE_OP_DELETE;
	subdir->d_op = &ppage_dentry_operations;
	subdir->d_lockref.count = 0;

	printk(KERN_DEBUG "[ DEBUG ] --%s-- create %s/ succeeded.\n", __func__, buf);
	
	return 0;
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

int ppage_delete(const struct dentry *dentry)
{
	printk(KERN_DEBUG "[ INFO ] --%s-- d_lockref.count: %d\n", __func__, dentry->d_lockref.count);
	return 1;
}

struct dentry *ppage_lookup(struct inode *dir, struct dentry *dentry, unsigned int flags)
{
	printk(KERN_DEBUG "[ DEBUG ] --%s-- @dentry: %s\n",
			__func__, dentry->d_name.name);

	return simple_lookup(dir, dentry, flags);
}

const struct file_operations ppage_dir_operations = {
	.open		= ppage_dcache_dir_open,
	.release	= ppage_dcache_dir_close,
	.llseek		= ppage_dcache_dir_lseek,
	.read		= ppage_generic_read_dir,
	.iterate_shared	= ppage_dcache_readdir,
	.fsync		= noop_fsync,
};

static struct dentry *ppage_root_lookup(struct inode * dir, 
		struct dentry * dentry, unsigned int flags)
{
	struct dentry *ret_dentry;

	/*
	 * temporary, skips re-creation of "nieh_test" folders
	 */
	if (strncmp(dentry->d_name.name, "nieh_test",
				strlen("nieh_test")) == 0)
		return NULL;

	ret_dentry = ppagefs_create_dir(dentry->d_name.name,
			dentry->d_sb->s_root);
	if (!ret_dentry) {
		printk(KERN_DEBUG "[ DEBUG ] --%s-- create %s/ failed.\n",
				__func__, dentry->d_name.name);
		return NULL;
	}
	printk(KERN_DEBUG "[ SUCCESS ] --%s-- created %s/.\n",
			__func__, ret_dentry->d_name.name);

	ret_dentry->d_flags |= DCACHE_OP_DELETE;
	ret_dentry->d_lockref.count = 0;
	ret_dentry->d_op = &ppage_dentry_operations;

	return ret_dentry;
}

static const struct inode_operations ppagefs_dir_inode_operations = {
	.lookup		= ppage_lookup,
	.link		= simple_link,
	.unlink		= simple_unlink,
	.rmdir		= simple_rmdir,
	.rename		= simple_rename,
};

static const struct inode_operations ppagefs_root_inode_operations = {
	.lookup		= ppage_root_lookup,
};

const struct inode_operations ppagefs_file_inode_operations = {
	.setattr	= simple_setattr,
	.getattr	= simple_getattr,
};

const struct file_operations ppagefs_file_operations = {
	.read_iter	= ppage_file_read_iter,
	.mmap		= generic_file_mmap,
	.fsync		= noop_fsync,
	.splice_read	= generic_file_splice_read,
	.splice_write	= iter_file_splice_write,
	.llseek		= generic_file_llseek,
};

const struct dentry_operations ppage_dentry_operations = {
	.d_delete	= ppage_delete,
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

	/* for debugging purposes */
	inode->i_private = name;

	/* increment directory counts */
	inc_nlink(inode);
	inc_nlink(d_inode(dentry->d_parent));
	d_add(dentry, inode);

	return dentry;

#if 0
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

	if (ppagefs_create_dir("kent", sb->s_root) == NULL) {
		return -ENOMEM;
	}
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
