#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define SCHED_NORMAL		0
#define SCHED_FIFO		1
#define SCHED_RR		2
#define SCHED_BATCH		3
#define SCHED_IDLE		5
#define SCHED_DEADLINE		6
#define SCHED_WRR		7

#define __SCHED_GETSCHEDULER 	145

static inline void get_sched_policy(pid_t pid, int verbose)
{
	char *policy_name;
	int policy = syscall(__SCHED_GETSCHEDULER, pid);
	if (policy < 0 && verbose){
		fprintf(stderr, "[ error ] %s\n", strerror(errno));
		return;
	}

	if (policy == SCHED_NORMAL)
		policy_name = "SCHED_NORMAL";
	else if (policy == SCHED_FIFO)
		policy_name = "SCHED_FIFO";
	else if (policy == SCHED_RR)
		policy_name = "SCHED_RR";
	if (policy == SCHED_BATCH)
		policy_name = "SCHED_BATCH";
	else if (policy == SCHED_IDLE)
		policy_name = "SCHED_IDLE";
	else if (policy == SCHED_DEADLINE)
		policy_name = "SCHED_DEADLINE";
	if (policy == SCHED_WRR)
		policy_name = "SCHED_WRR";

	fprintf(stderr, "[ info ] pid: %d	"
			"policy: %s\n", pid, policy_name);

}

int main(int argc, char **argv)
{

#ifdef all
	int max = atoi(argv[1]);

	for (int i = 0; i < max; i++)
		get_sched_policy(i, 0);
#endif

#ifdef interactive
	pid_t pid;

	while(1) {
		fprintf(stderr, "pid: ");
		scanf("%d", &pid);

		get_sched_policy(pid, 1);
	}
#endif

	return 0;
}
