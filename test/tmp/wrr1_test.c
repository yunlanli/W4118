#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>

struct sched_param {
	int sched_priority;
};

int main()
{
	pid_t pid;
	int ret, policy = 7;
	struct sched_param info;

	while(1) {
		printf("\n---\n"
				"pid: ");
		scanf("%d", &pid);
		printf("priority: ");
		scanf("%d", &info.sched_priority);

		ret = syscall(144, pid, policy, &info);

		fprintf(stderr, "sched_setscheduler returned %d\n", ret);
		if (ret) {
			fprintf(stderr, "error: %s\n", strerror(errno));
			exit(1);
		}
	}

	return 0;
}
