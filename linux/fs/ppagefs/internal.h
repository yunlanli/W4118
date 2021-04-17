#ifndef __INTERNAL_H__
#define __INTERNAL_H__

/* extra information for inode of PID.PROCESSNAME */
struct p_info {
	pid_t			pid;
	char			comm[TASK_COMM_LEN];
	int			retain;
	struct dentry 		*dentry;
	struct list_head 	head;
};

#endif
