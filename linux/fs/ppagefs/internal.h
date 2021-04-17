#ifndef __INTERNAL_H__
#define __INTERNAL_H__

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

/* extra information for inode of PID.PROCESSNAME */
struct p_info {
	pid_t			pid;
	char			comm[TASK_COMM_LEN];
	int			retain;
	struct dentry		*dentry;
	struct list_head	head;
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

static struct dentry *ppagefs_create_dir(const char *name,
		struct inode *inode, struct dentry *parent);

static struct inode *ppage_get_inode(struct super_block *sb, umode_t flags);

static int ppage_piddir_open(struct inode *inode, struct file *file);

static struct dentry *ppage_piddir_lookup(struct inode *dir,
		struct dentry *dentry, unsigned int flags);

static struct dentry *ppage_create_file(struct dentry *parent,
		const struct tree_descr *files, int count);

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

#endif
