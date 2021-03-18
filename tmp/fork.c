#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define MAX_CPUS		8
#define SCHED_WRR		7

#define __SCHED_SETSCHEDULER	144
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

static inline void do_some_work(void)
{
	for (int i = 1; i < 100000000; i++) ;
}

static inline void check_sched_policy(void)
{
	int policy = syscall(__SCHED_GETSCHEDULER, getpid());
	if (policy < 0){
		fprintf(stderr, "error: %s\n", strerror(errno));
		exit(-1);
	} else if (policy != SCHED_WRR) {
		fprintf(stderr, "error: current policy: %d, setscheduler unsuccessful\n", policy);
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

int main(int argc, char **argv)
{
	fprintf(stderr, "Process %d started...\n", getpid());

	get_wrr_info();

	set_sched_policy(SCHED_WRR);
	check_sched_policy();

	for (int i = 1; i < atoi(argv[1]); i++) {
		fork();
//		set_weight(2*i);
	}

	/* test if our sched_class implementation can run the task successfully
	 */
	do_some_work();

#ifdef test_pnt
	pid_t child = fork();
	if (child < 0) {
		fprintf(stderr, "error: %s\n", strerror(errno));
		exit(-1);
	} else if (child == 0) {
		check_sched_policy();
		
		/* child process */
		do_some_work();
	} else {
		fprintf(stderr, "[ info ] parent");
	}
#endif

#ifdef test_ttnb
	fprintf(stderr, "[ info ] entering inifinite loop\n");
	while(1)
		fprintf(stderr, "[ info ] next\n");
#endif

#ifdef test_ttb
	fprintf(stderr, "entering finite blocking and waking loop...\n");
	int k = 100;
	while(--k > 0){
		printf("sleeping\n");
		sleep(2);
		printf("waking up\n");
	}
#endif
	return 0;
}
