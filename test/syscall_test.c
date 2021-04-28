#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sched.h>

#define MAX_CPUS		8
#define SCHED_WRR		7

#define __SCHED_SETSCHEDULER	144
#define __GET_WRR_INFO		436
#define __SET_WRR_WEIGHT	437

struct wrr_info {
	int num_cpus;
	int nr_running[MAX_CPUS];
	int total_weight[MAX_CPUS];
};

static inline void set_invalid_weight(void)
{
	int ret, weight;

	weight = -1;
	ret = syscall(__SET_WRR_WEIGHT, weight);
	if (errno == EINVAL)
		fprintf(stderr, "[ success ] denied request to set weight < 1\n");
	else
		fprintf(stderr, "[ failure ] set_wrr_weight(-1) returned %d "
				"with error: %s\n",
				ret, strerror(errno));
}

static inline void set_weight_above_ten(void)
{
	int ret, weight;
	weight = 20;
	ret = syscall(__SET_WRR_WEIGHT, weight);
	if (errno == EACCES)
		fprintf(stderr, "[ success ] denied request to set weight > 10"
				" from a non-root user\n");
	else if (ret)
		fprintf(stderr, "[ failure ] set_wrr_weight(20) returned %d "
				"with error: %s\n",
				ret, strerror(errno));
}

static inline void set_weight(int weight)
{
	int ret;

	ret = syscall(__SET_WRR_WEIGHT, weight);
	if (ret && errno == EPERM)
		fprintf(stderr, "[ failure ] can't set wrr_weight on a process "
				"not using SCHED_WRR policy\n");
	else if (ret)
		fprintf(stderr, "[ failure ] %s\n", strerror(errno));

	fprintf(stderr, "[ success ] set pid %d's wrr_weight to %d\n", getpid(), weight);
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

static inline void set_sched_policy(int policy)
{
	pid_t mypid;
	struct sched_param param;

	/* initialize parameters for sched_setscheduler call */
	mypid = getpid();
	param.sched_priority = 0;

	printf("[ info ] Setting process %d to use wrr_sched_class...\n", mypid);
	if (syscall(__SCHED_SETSCHEDULER, mypid, policy, &param)) {
		fprintf(stderr, "[ failure ] error: %s\n", strerror(errno));
		exit(-1);
	}
	printf("[ success ] pid %d set to use policy %d\n", mypid, policy);
}

int main()
{
	int weight;

	set_sched_policy(SCHED_WRR);
	set_invalid_weight();
	set_weight_above_ten();

	while (1) {
		fprintf(stderr, "Enter weight: ");
		scanf("%d", &weight);

		set_weight(weight);

		get_wrr_info();
	}

	return 0;
}
