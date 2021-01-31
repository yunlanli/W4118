#include <linux/syscalls.h>
#include <linux/prinfo.h>

SYSCALL_DEFINE3(ptree, struct prinfo *, buf, int *, nr, int, root_pid)
{
	return 69;
}
