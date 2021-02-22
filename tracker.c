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

int main(int argc, char **argv) {
	if (argc != 2)
		fprintf(stderr, "<usage> ./tracker <filepath>\n");
	long counter = -1;
	pid_t pid;
	int wstatus, count;
	char *filepath = argv[1];

#if 0
	printf("filepath: ");
	fscanf(stdin, "%s", filepath);
#endif

	pid = fork();

	if (pid > 0) {
		syscall(436, pid);
		printf("tracking child process %d\n", pid);
	}

	if (pid < 0) {
		fprintf(stderr, "[fork] error: %s\n", strerror(errno));
	} else if (pid == 0) {
		/* child process */
		execl(filepath, filepath, (char *) NULL);
		fprintf(stderr, "[execl] error: %s\n", strerror(errno));
	} else {
		struct pstrace *buf = malloc(sizeof(struct pstrace) * PSTRACE_BUF_SIZE);
		if (waitpid(pid, &wstatus, 0) != pid)
			fprintf(stderr, "[waitpid] error: %s\n", strerror(errno));

		count = syscall(438, pid, buf, &counter);

		for (int i = 0; i < count; i++)
			printf("[rec %d] comm: %s, pid: %d, state: %ld\n",
					i, (buf+i)->comm, (buf+i)->pid, (buf+i)->state);
		free(buf);
	}

	return 0;
}
