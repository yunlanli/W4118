#include <linux/fs.h>
#include <uapi/linux/magic.h>
#include <linux/cgroup-defs.h>
#include <linux/init.h>
#include <linux/fs_context.h>
#include <uapi/linux/stat.h>
#include <linux/string.h>
#include <linux/ppage_fs.h>
#include <linux/uio.h>

int count = 0;
struct dentry *ppagefs_create_dir(const char *name, struct dentry *parent);

ssize_t
ppage_file_read_iter(struct kiocb *iocb, struct iov_iter *iter);

int ppage_dcache_dir_open(struct inode *inode, struct file *file)
{
	printk(KERN_DEBUG "[ DEBUG ] --%s--\n", __func__);
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
	char name_buf[10];
	struct dentry *dentry;
	
	printk(KERN_DEBUG "[ DEBUG ] --%s--\n", __func__);

	dentry = file_dentry(file);
	
	sprintf(name_buf, "%s%d", "nieh", count++);
	
	printk(KERN_DEBUG "[ DEBUG ] --%s-- creating dir %s\n", __func__, name_buf);
	ppagefs_create_dir(name_buf, dentry);

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

static struct dentry *proc_root_lookup(struct inode * dir, 
		struct dentry * dentry, unsigned int flags)
{
	char name_buf[10];
	sprintf(name_buf, "%s%d", "nieh", count++);

	printk(KERN_DEBUG "[ DEBUG ] --%s-- creating dir %s\n", __func__, name_buf);
	return ppagefs_create_dir(name_buf, dentry);
}

static const struct inode_operations ppagefs_dir_inode_operations = {
	.lookup		= simple_lookup,
	.link		= simple_link,
	.unlink		= simple_unlink,
	.rmdir		= simple_rmdir,
	.rename		= simple_rename,
};

static const struct inode_operations ppagefs_root_inode_operations = {
	.lookup		= proc_root_lookup,
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
	size_t bytes = strlen(data), ret;

	printk(KERN_DEBUG "[ DEBUG ] --%s-- copying %s to iter\n", __func__, data);
	ret = _copy_to_iter(data, bytes, iter);
	iocb->ki_pos += ret;
	printk(KERN_DEBUG "[ DEBUG ] --%s-- updated kiocb=%lld\n", __func__, iocb->ki_pos);
	return ret;
}

struct dentry *ppagefs_create_dir(const char *name, struct dentry *parent)
{
	struct dentry *dentry; 
	struct inode *inode;
	
	/* attempts to create a folder */
	dentry = d_alloc_name(parent, name);
	if (!dentry)
		return NULL; // TODO: check error return values
	inode = new_inode(parent->d_sb);
	if (!inode) {
		dput(dentry);
		return NULL;
	}

	inode->i_mode = S_IFDIR | 0755;
	inode->i_atime = inode->i_mtime = inode->i_ctime = current_time(inode);
	inode->i_op = &ppagefs_dir_inode_operations;
	inode->i_fop = &simple_dir_operations; /* TODO: do we still need this */
	inode->i_ino = get_next_ino();
	/* increment directory counts */
	inc_nlink(inode);
	inc_nlink(d_inode(dentry->d_parent));
	d_add(dentry, inode);

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
