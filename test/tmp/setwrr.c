#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define SCHED_WRR 7
#define __SCHED_SETSCHEDULER 144
#define __SCHED_GETSCHEDULER 145

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

int main()
{
	pid_t pid;

	while (1) {
		fprintf(stderr, "Set SCHED_WRR class to pid: ");
		scanf("%d", &pid);

		set_sched_policy(SCHED_WRR, pid);
		check_sched_policy(pid);
	}
	
	return 0;
}
