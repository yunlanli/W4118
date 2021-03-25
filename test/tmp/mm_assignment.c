#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

int main()
{
	int ret, addr = 5;
	unsigned long computed = (unsigned long) &addr;

	printf("before %d\n", addr);
	printf("addr=%ld | hex=%p \n", computed, &addr);

	ret = syscall(438, computed);
	if (ret) {
		fprintf(stderr, "err: %s\n", strerror(errno));
		exit(1);	
	}

	printf("after %d\n", addr);

	return 0;
}
