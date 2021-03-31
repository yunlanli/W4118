#include "expose_pgtbl.h"

int main(int argc, char **argv)
{
	if (argc != 2) {
		fprintf(stderr, "<usage> ./pgtbl_sample <num_page>\n");
		exit(1);
	}

	int size = atoi(argv[1]);
	int tmp, *data = malloc(sizeof(int) * size * PAGE_SIZE);

	if (data == NULL)
		exit(1);

	for (int i = 0; i < size * PAGE_SIZE; i++)
		*(data+i) = i;

	fprintf(stderr, "[ info ] pid: %d, start: %p, end: %p\n",
			getpid(), data, data+ PAGE_SIZE * size);

	while (1) {
		printf("Type number to continue..\n");
		scanf("%d", &tmp);
	}

	return 0;
}
