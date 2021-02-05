#include <unistd.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

struct prinfo {
	pid_t parent_pid;       /* process id of parent */
	pid_t pid;              /* process id */
	long state;             /* current state of process */
	uid_t uid;              /* user id of process owner */
	char comm[16];          /* name of program executed */
	int level;              /* level of this process in the subtree */
};

int isdigits (const char *str)
{
    while (*str) {
        if (isdigit(*str++) == 0) return 0;
    }

    return 1;
}

int main(int argc, char **argv)
{
	int root_pid;
	if (argc == 1)
	{
		root_pid = 0;
	} else if (argc == 2 && isdigits(*(argv+1))) {
		root_pid = atoi(*(argv+1));
	} else {
		return 0;
	}

	int size = 2, nr = 2;
	long ret = 0, i;
	int count = 1;
	while (1)
	{
		struct prinfo buf[size];
		ret = syscall(436, buf, &nr, root_pid);

		if (ret)
			return ret;

		if (nr == size)
		{
			size *= 2;
			nr = size;
			count++;
			continue;
		} else if (nr < size) {
			for (i = 0; i < nr; i++){
				printf("%s,%d,%d,%ld,%d,%d\n", 
					buf[i].comm, 
					buf[i].pid, 
					buf[i].parent_pid, 
					buf[i].state, 
					buf[i].uid, 
					buf[i].level);
			}
			printf("nr %d | count %d\n", nr, count);
			break;
		} else
			return -1;

	} 

	return 0;
}