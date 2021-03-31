#include "expose_pgtbl.h"

#define SIZE	512

int main()
{
	int tmp, *data = malloc(sizeof(int) * SIZE);

	if (data == NULL)
		exit(1);

	for (int i = 0; i < SIZE; i++)
		*(data+i) = i;

	fprintf(stderr, "[ info ] pid: %d, start: %p, end: %p\n",
			getpid(), data, data+SIZE);

	while (1) {
		printf("Type number to continue..\n");
		scanf("%d", &tmp);
	}

	return 0;
}
