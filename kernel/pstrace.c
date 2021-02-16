#include <linux/pstrace.h>
#include <linux/syscalls.h>

void pstrace_add(struct task_struct *p)
{
	
}

SYSCALL_DEFINE1(pstrace_enable, pid_t, pid)
{
	return 0;
}

SYSCALL_DEFINE1(pstrace_disable, pid_t, pid)
{
	return 0;
}

SYSCALL_DEFINE3(pstrace_get, pid_t, pid, struct pstrace *, buf, long *, counter)
{
	return 0;
}

SYSCALL_DEFINE1(pstrace_clear, pid_t, pid)
{
	return 0;
}
