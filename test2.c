#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

struct pstrace {
	char comm[16];		/* The name of the process */
	pid_t pid;		/* The pid of the process */
	long state;		/* The state of the process */
};

static inline void print_error(int ret)
{
	if (ret)
		fprintf(stderr, "error: %s\n", strerror(errno));
}


int main()
{
	pid_t pid;
	int sys_ret, flag;

	while (1) {
		printf("enable? ");
		scanf("%d", &flag);

		printf("pid: ");
		scanf("%d", &pid);

		if (flag) {
			/* enable tracing child process pid */
			sys_ret = syscall(436, pid);
			if (sys_ret)
				printf("pstrace_enable returned: %d\n", sys_ret);
		} else {
			/* child pid has exited, disable tracing */
			sys_ret = syscall(437, pid);
			if (sys_ret)
				printf("pstrace_disable returned: %d\n", sys_ret);
		}
	}

	return 0;
}
