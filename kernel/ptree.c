#include "./prinfo.h"
#include <linux/syscalls.h>

SYSCALL_DEFINE3(ptree, struct prinfo *, buf, int *, nr, int, root_pid)
{
	return 69;
}
