#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>

#define MAX_CPUS 8

struct wrr_info {
	int num_cpus;
	int nr_running[MAX_CPUS];
	int total_weight[MAX_CPUS];
};

int main()
{
	struct wrr_info buf;
	int ret, weight;

	ret = syscall(436, &buf);
	fprintf(stderr, "get_wrr_info returned %d\n", ret);
	if (ret) {
		fprintf(stderr, "error: %s\n", strerror(errno));
		exit(1);
	}


	weight = 10;
	ret = syscall(437, weight);
	fprintf(stderr, "set_wrr_weight returned %d\n", ret);
	if (ret) {
		fprintf(stderr, "error: %s\n", strerror(errno));
		exit(1);
	}

	return 0;
}
