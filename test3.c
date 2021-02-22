#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

struct pstrace {
	char comm[16];		/* The name of the process */
	pid_t pid;		/* The pid of the process */
	long state;		/* The state of the process */
};

static inline void print_error(int ret)
{
	if (ret)
		fprintf(stderr, "error: %s\n", strerror(errno));
}


int main()
{
	syscall(439, -1);
	return 0;
}
