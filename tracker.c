#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>

#define PSTRACE_BUF_SIZE 500

struct pstrace {
	char comm[16];		/* The name of the process */
	pid_t pid;		/* The pid of the process */
	long state;		/* The state of the process */
};

int main() {
	long counter = 1;
	pid_t pid;
	int wstatus, count;

	pid = fork();

	if (pid > 0)
		syscall(436, pid);

	if (pid < 0) {
		fprintf(stderr, "error: %s\n", strerror(errno));
	} else if (pid == 0) {
		/* child process */
		execl("./lifecycle", "lifecycle", (char *) NULL);
		fprintf(stderr, "error: %s\n", strerror(errno));
	} else {
		struct pstrace *buf = malloc(sizeof(struct pstrace) * PSTRACE_BUF_SIZE);
		if (waitpid(pid, &wstatus, 0))
			fprintf(stderr, "error: %s\n", strerror(errno));

		count = syscall(438, buf, pid, &counter);

		for (int i = 0; i < count; i++)
			printf("comm: %s, pid: %d, state: %ld",
					(buf+i)->comm, (buf+i)->pid, (buf+i)->state);
		free(buf);
	}

	return 0;
}
