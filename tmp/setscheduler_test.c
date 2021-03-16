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

static inline void do_some_work(void)
{
	for (int i = 0; i < 1000; i++) ;
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

int main()
{
	fprintf(stderr, "Process %d started...\n", getpid());

	set_sched_policy(SCHED_WRR);
	check_sched_policy();
	
	/* test if our sched_class implementation can run the task successfully
	 */
	do_some_work();

#ifdef test_pnt
	pid_t child = fork();
	if (child < 0) {
		fprintf(stderr, "error: %s\n", strerror(errno));
		exit(-1);
	} else if (child > 0) {
		check_sched_policy();
		
		/* child process */
		do_some_work();
	}
#endif

#ifdef test_ttnb
	fprintf(stderr, "entering finite blocking and waking loop...\n");
	while(1)
		;
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
