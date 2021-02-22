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
	int enable_pid;
	int pid_to_trace;
	
	pid_t pid = fork();

	if (pid < 0) {
		fprintf(stderr, "error: %s\n", strerror(errno));
	} else if (pid == 0) {
		/* child process */
		sleep(3);
		/* TASK_RUNNING -> TASK_INTERRUPTIBLE */
		//printf("enter a number: \n");
		//scanf("%d", &sys_ret);
		/* TASK_INTERRUPTIBLE -> TASK_RUNNING */
	} else {
		/* enable tracing child process pid */
		printf("child pid: %d\n\n", pid);
		
		printf("Enter enable pid: ");
		scanf("%d", &enable_pid);
		printf("Enter pid to trace: ");
		scanf("%d", &pid_to_trace);
		printf("Enter counter: ");
		scanf("%ld", &counter);
#if 1
		sys_ret = syscall(436, enable_pid);
		if (sys_ret)
			printf("pstrace_enable returned: %d\n", sys_ret);

		sys_ret = waitpid(pid, &wstatus, 0);
		print_error(sys_ret);
		/* child pid has exited, disable tracing */
		sys_ret = syscall(437, pid);
		if (sys_ret)
			printf("pstrace_disable returned: %d\n", sys_ret);

		/* get all state change records for child process */
		sys_ret = syscall(438, pid_to_trace, buf, &counter);
		printf("pstrace_get returned: %d\n", sys_ret);
		
		struct pstrace *rec = buf;
		for (int i = 0; i<sys_ret; i++){
<<<<<<< HEAD
			if (rec->pid == pid)
				printf("%s, %d, %ld\n", rec->comm, rec->pid, rec->state);
		}
#endif
=======
			printf("%s, %d, %ld\n", rec->comm, rec->pid, rec->state);
			rec++;
		}
#endif
#if 0
>>>>>>> 893f352e9... Attempted to imitate the lifecycle of a pro
		/* clear all state chagne records of pid */
		sys_ret = syscall(439, -1);
		if (sys_ret)
			printf("pstrace_clear returned: %d\n", sys_ret);
<<<<<<< HEAD

=======
#endif
>>>>>>> 893f352e9... Attempted to imitate the lifecycle of a pro
	}

	return 0;
}
