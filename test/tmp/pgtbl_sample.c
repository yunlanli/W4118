#include "expose_pgtbl.h"

void stop(void)
{
	char input[10];

	while (1) {
		fprintf(stderr, "Enter y to continue: \n");
		scanf("%s", input);

		if (strstr(input, "y"))
			break;
	}
}

void *malloc_stuff(int size, void *start, int offset){
	int *addr;
	unsigned long VM_FLAGS = MAP_SHARED | MAP_ANONYMOUS;

	if (start)
		VM_FLAGS |= MAP_FIXED;

	addr = mmap(start, 5 * sizeof(int), PROT_WRITE,
			VM_FLAGS, -1, offset);

	if (addr == MAP_FAILED) {
		fprintf(stderr, "[ error ] mmap: %s\n", strerror(errno));
		exit(1);
	}

	fprintf(stderr, "[ info ] %d %p %p\n",
			getpid(), addr, addr + sizeof(int) * size);

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

	return addr + sizeof(int) * size;
}

int main(int argc, char **argv)
{
	if (argc != 2) {
		fprintf(stderr, "<usage> ./pgtbl_sample <num_page>\n");
		exit(1);
	}

	int size = atoi(argv[1]);
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

	int away = PAGE_SIZE * PAGE_SIZE;
	unsigned long aligned_addr = (unsigned long) (tmp_addr + away) & PAGE_MASK;

	tmp_addr = malloc_stuff(size, (void *) aligned_addr, 0);

	stop();

	fprintf(stderr, "[ info ] writing to %p\n", tmp_addr);

	*(int *) tmp_addr = 10;

	stop();

	return 0;
}
