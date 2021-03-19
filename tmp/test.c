#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define SCHED_WRR		7
#define MAX_CPUS		8

#define __SCHED_SETSCHEDULER 	144
#define __SCHED_GETSCHEDULER	145
#define __GET_WRR_INFO		436
#define __SET_WRR_WEIGHT	437

struct wrr_info {
	int num_cpus;
	int nr_running[MAX_CPUS];
	int total_weight[MAX_CPUS];
};

struct sched_param {
	int sched_priority;
};

static inline void infinite_loop(void)
{
	while(1) ;
//	for (int i = 0; i < 100; i++) ;
}

static inline void check_sched_policy(void)
{
	int policy = syscall(__SCHED_GETSCHEDULER, getpid());
	if (policy < 0){
		fprintf(stderr, "[ error ] pid %d: %s\n", getpid(), strerror(errno));
		exit(-1);
	} else if (policy != SCHED_WRR) {
		fprintf(stderr, "[ error ] pid %d current policy: %d, setscheduler unsuccessful\n", getpid(), policy);
		exit(-1);
	}

}

static inline void set_sched_policy(int policy)
{
	pid_t mypid;
	struct sched_param param;

	/* initialize parameters for sched_setscheduler call */
	mypid = getpid();
	param.sched_priority = 0;

	printf("Setting process %d to use wrr_sched_class...\n", mypid);
	if (syscall(__SCHED_SETSCHEDULER, mypid, policy, &param)) {
		fprintf(stderr, "error: %s\n", strerror(errno));
		exit(-1);
	}
}

static inline void fork_wrapper(void)
{
	pid_t child = fork();
	if (child < 0) {
		fprintf(stderr, "[ error ] %s\n", strerror(errno));
		exit(-1);
	}
}

static inline void get_wrr_info(void)
{
	int i, ret;
	struct wrr_info *buf;
	buf = malloc(sizeof(struct wrr_info));

	ret = syscall(__GET_WRR_INFO, buf);
	if (ret < 0) {
		fprintf(stderr, "[ failure ] error: %s returned %d\n", strerror(errno), ret);
		exit(1);
	}
	fprintf(stderr, "[ success ] num_cpus: %d\n", ret);

	for (i = 0; i < ret; i++)
		fprintf(stderr, "[ info ] cpu_%d	"
				"nr_running: %d		"
				"total_weight %d\n",
				i, buf->nr_running[i], buf->total_weight[i]);
}

int main()
{
	pid_t parent = getpid();
	fprintf(stderr, "pid: %d\n", parent);

	set_sched_policy(SCHED_WRR);
	check_sched_policy();
	
//	for (int i = 0; i < 4; i++)
//		fork_wrapper();

	if (getpid() == parent) {
		fprintf(stderr, "pid: %d\n", getpid());
		get_wrr_info();
	}

	infinite_loop();

	return 0;
}
