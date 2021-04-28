#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define SCHED_WRR 		7
#define __SCHED_SETSCHEDULER	144
#define __SCHED_GETSCHEDULER	145
#define __SET_WRR_WEIGHT	437

struct sched_param {
	int sched_priority;
};

static inline void check_sched_policy(pid_t pid)
{
	int policy = syscall(__SCHED_GETSCHEDULER, pid);
	if (policy < 0){
		fprintf(stderr, "[ failure ] error: %s\n", strerror(errno));
		exit(-1);
	} else if (policy != SCHED_WRR) {
		fprintf(stderr, "[ failure ] error: current policy: %d "
				", setscheduler unsuccessful\n", policy);
		exit(-1);
	}

	fprintf(stderr, "[ success ] set process %d to use SCHED_WRR class\n", pid);
}

static inline void set_sched_policy(int policy, pid_t pid)
{
	struct sched_param param;

	/* initialize parameters for sched_setscheduler call */
	param.sched_priority = 0;

	printf("[ info ] Setting process %d to use wrr_sched_class...\n", pid);
	if (syscall(__SCHED_SETSCHEDULER, pid, policy, &param)) {
		fprintf(stderr, "[ failure ] error: %s\n", strerror(errno));
	}
}

static inline void set_weight(int weight)
{
	int ret;

	fprintf(stderr, "[ info ] setting to weight %d\n", weight);
	ret = syscall(__SET_WRR_WEIGHT, weight);
	if (ret && errno == EPERM)
		fprintf(stderr, "[ failure ] can't set wrr_weight on a process "
				"not using SCHED_WRR policy\n");
	else if (ret)
		fprintf(stderr, "[ failure ] %s\n", strerror(errno));

	fprintf(stderr, "[ success ] set pid %d's wrr_weight to %d\n", getpid(), weight);
}

static inline void start_process(char *program, int weight)
{
	pid_t pid;

	set_sched_policy(SCHED_WRR, getpid());
	set_weight(weight);
		

	pid = fork();

	if (pid < 0) {
		fprintf(stderr, "[ failure ] error: %s, fork failed", strerror(errno));
	} else if (pid == 0) {
		/* child process */
		fprintf(stderr, "[info] starting %s\n", program);
		execlp(program, program, (char *) NULL);
		fprintf(stderr, "[ failure ] start %s failed.\n", program);
	} else {
		int wstatus;

		wait(&wstatus);
	}
}

int main()
{
	int weight;
	char program[100];

	while (1) {
		fprintf(stderr, "Enter program to start: ");
		scanf("%s", program);
		fprintf(stderr, "Enter weight: ");
		scanf("%d", &weight);

		start_process(program, weight);
	}
	
	return 0;
}
