#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "expose_pgtbl.h"

int main()
{
	int ret;
	int *addr;

	addr = (int *) malloc(sizeof(int));
	if (!addr)
		printf("[ error ] malloc failed\n");

	*addr = 5;
	unsigned long computed = (unsigned long) addr;

	printf("before %d\n", *addr);
	printf("addr=%lx | hex=%p \n", computed, addr);

	* (int *) computed = 10;
	printf("after %d\n", *addr);

	ret = syscall(438, computed);
	fprintf(stderr, "after syscall %d\n", *addr);
	if (ret) {
		fprintf(stderr, "err: %s\n", strerror(errno));
		exit(1);	
	}
	
#if 0
	struct pagetable_layout_info pgtbl_info;
	ret = syscall(436, &pgtbl_info);
	printf("Syscall returns %d\n", ret);

	printf("after %d\n", addr);
#endif

	return 0;
}
