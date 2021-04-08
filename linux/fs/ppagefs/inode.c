#include <linux/fs.h>
#include <uapi/linux/magic.h>
#include <linux/cgroup-defs.h>
#include <linux/init.h>
#include <linux/fs_context.h>
#include <uapi/linux/stat.h>

static const struct inode_operations ppagefs_dir_inode_operations = {
	.lookup		= simple_lookup,
	.link		= simple_link,
	.unlink		= simple_unlink,
	.rmdir		= simple_rmdir,
	.rename		= simple_rename,
};

static int ppagefs_fill_super(struct super_block *sb, struct fs_context *fc)
{
	struct inode *inode;
	struct dentry *dentry;
	int err = 0;
	static const struct tree_descr ppage_files[] = {
		{NULL},
		{NULL},
		{"jason", &simple_dir_operations, S_IRUSR | S_IWUSR},
		{"yunlan", &simple_dir_operations, S_IRUSR | 01000},
		{"dashi", &simple_dir_operations, 0755},
		{""}
	};

	/*
	 * @ppage_files: contains information about all directories under
	 * the root of the filesystem
	 */
	err  =  simple_fill_super(sb, PPAGEFS_MAGIC, ppage_files);
	if (err)
		goto fail;

	/* attempts to create a folder */
	dentry = d_alloc_name(sb->s_root, "nieh");
	if (!dentry)
		goto fail;
	inode = new_inode(sb);
	if (!inode) {
		dput(dentry);
		goto fail;
	}
	inode->i_mode = S_IFDIR | 0755;
	inode->i_atime = inode->i_mtime = inode->i_ctime = current_time(inode);
	inode->i_op = &ppagefs_dir_inode_operations;
	inode->i_fop = &simple_dir_operations; /* changed from i_fop */
	inode->i_ino = 5;
	/* increment directory counts */
	inc_nlink(inode);
	inc_nlink(d_inode(dentry->d_parent));
	d_add(dentry, inode);
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
