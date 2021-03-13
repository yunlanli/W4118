#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>

#define MAX_CPUS		8

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
	if (ret == -EINVAL)
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
	if (ret == -EPERM)
		fprintf(stderr, "[ success ] denied request to set weight > 10"
				" from a non-root user\n");
	else
		fprintf(stderr, "[ failure ] set_wrr_weight(20) returned %d "
				"with error: %s\n",
				ret, strerror(errno));
}

static inline void set_weight(int weight)
{
	int ret;

	ret = syscall(__SET_WRR_WEIGHT, weight);
	if (ret)
		fprintf(stderr, "[ failure ] %s\n", strerror(errno));
}

static inline void get_wrr_info(void)
{
	int i, ret;
	struct wrr_info buf;

	ret = syscall(__GET_WRR_INFO, &buf);
	if (ret) {
		fprintf(stderr, "error: %s\n", strerror(errno));
		exit(1);
	}
	fprintf(stderr, "[ success ] num_cpus: %d\n", ret);

	for (i = 0; i < ret; i++)
		fprintf(stderr, "[ info ] cpu_%d	"
				"nr_running: %d		"
				"total_weight %d\n",
				i, buf.nr_running[i], buf.total_weight[i]);
}

int main()
{
	set_invalid_weight();
	set_weight_above_ten();
	set_weight(5);

	get_wrr_info();

	return 0;
}
