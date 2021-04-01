#include "expose_pgtbl.h"
#define PAGE_MASK (~511)

void *malloc_stuff(int size, void *start, int offset){
	int *addr;

	addr = mmap(start, 5 * sizeof(int), PROT_WRITE,
			MAP_SHARED | MAP_ANONYMOUS, -1, offset);

	if (addr == MAP_FAILED) {
		fprintf(stderr, "[ error ] mmap: %s\n", strerror(errno));
		exit(1);
	}

	*addr = 10;

# if 0
	int *data = malloc(sizeof(int) * size * PAGE_SIZE);
	temp_addr = mmap(NULL, 6 * PAGE_SIZE, PROT_WRITE,
			MAP_SHARED | MAP_ANONYMOUS, -1, 0);

	if (temp_addr == MAP_FAILED) {
		fprintf(stderr, "[ error ] mmap: %s\n", strerror(errno));
		return -1;
	}


	if (data == NULL)
		exit(1);
	for (int i = 0; i < size * PAGE_SIZE; i++)
		*(data+i) = i;
# endif

	fprintf(stderr, "[ info ] pid: %d, start: %p, end: %p\n",
			getpid(), addr, addr+ sizeof(int) * size);
	return addr+sizeof(int) * size;
}

int main(int argc, char **argv)
{
	if (argc != 2) {
		fprintf(stderr, "<usage> ./pgtbl_sample <num_page>\n");
		exit(1);
	}

	int size = atoi(argv[1]), tmp;
	void *tmp_addr;
# if 0
	int tmp, *data = malloc(sizeof(int) * size * PAGE_SIZE);

	if (data == NULL)
		exit(1);

	for (int i = 0; i < size * PAGE_SIZE; i++)
		*(data+i) = i;

	fprintf(stderr, "[ info ] pid: %d, start: %p, end: %p\n",
			getpid(), data, data+ PAGE_SIZE * size);
# endif

	tmp_addr = malloc_stuff(size, NULL, 0);

	int pte_away = 4096;
	int pte_table_away = pte_away * PAGE_SIZE;
	unsigned long masked_address = (unsigned long)(tmp_addr+2*pte_table_away) & PAGE_MASK;

	tmp_addr = malloc_stuff(size, (void *)masked_address, 0);

		
	while (1) {
		printf("Type number to continue..\n");
		scanf("%d", &tmp);
	}

	return 0;
}
