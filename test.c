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
	int sys_ret, wstatus;
	long counter;
	struct pstrace buf[500];
	pid_t pid = fork();

	if (pid < 0) {
		fprintf(stderr, "error: %s\n", strerror(errno));
	} else if (pid == 0) {
		/* child process */
		sleep(3);
		/* TASK_RUNNING -> TASK_INTERRUPTIBLE */
		printf("enter a number: \n");
		scanf("%d", &sys_ret);
		/* TASK_INTERRUPTIBLE -> TASK_RUNNING */
	} else {
		/* enable tracing child process pid */
		sys_ret = syscall(436, pid);
		if (sys_ret)
			printf("pstrace_enable returned: %d\n", sys_ret);

		sys_ret = waitpid(pid, &wstatus, 0);
		print_error(sys_ret);

		/* child pid has exited, disable tracing */
		sys_ret = syscall(437, pid);
		if (sys_ret)
			printf("pstrace_disable returned: %d\n", sys_ret);

		/* get all state change records for child process */
		counter = 0;
		sys_ret = syscall(438, pid, buf, &counter);
		if (sys_ret)
			printf("pstrace_get returned: %d\n", sys_ret);

		/* clear all state chagne records of pid */
		sys_ret = syscall(439, pid);
		if (sys_ret)
			printf("pstrace_clear returned: %d\n", sys_ret);

		struct pstrace *rec = buf;
		for (int i = 0; i<500; i++)
			if (rec->pid == pid)
				printf("%s, %d, %ld", rec->comm, rec->pid, rec->state);
	}

	return 0;
}
