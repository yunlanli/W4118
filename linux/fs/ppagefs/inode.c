#include <linux/fs.h>
#include <uapi/linux/magic.h>
#include <linux/cgroup-defs.h>
#include <linux/init.h>

static int ppage_fill_super(struct super_block *sb, void *data, int silent)
{
	int err = 0;
	static const struct tree_descr ppage_files[] = = {{""}};

	/*
	 * @ppage_files: contains information about all directories under
	 * the root of the filesystem
	 */
	err  =  simple_fill_super(sb, PPAGEFS_MAGIC, ppage_files);
	if (err)
				goto fail;
fail:
	return err;
}

static struct dentry *ppage_mount(struct file_system_type *,
		int flags, const char * dev_name,
		void *data)
{
	return mount_single(fs_type, flags, data, ppage_fill_super);
}

static struct file_system_type ppage_fs_type = {
	.owner =	THIS_MODULE,
	.name =		"ppagefs",
	.mount = 	ppage_mount,
	.kill_sb =	kill_litter_super,
};

static int __init ppagefs_init(void)
{
	return register_filesystem(&ppage_fs_type);;
}
fs_initcall(ppagefs_init);

MODULE_ALIAS_FS("ppagefs");
