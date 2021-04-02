#include "expose_pgtbl.h"

void stop(void)
{
	char input[10];

	while (1) {
		fprintf(stderr, "Enter y to continue: ");
		scanf("%s", input);

		if (strstr(input, "y"))
			break;
	}
}

void *malloc_stuff(int size, void *start, int offset)
{
	int *addr;
	unsigned long VM_FLAGS = MAP_SHARED | MAP_ANONYMOUS;

	if (start)
		VM_FLAGS |= MAP_FIXED;

	addr = mmap(start, size, PROT_READ | PROT_WRITE,
			VM_FLAGS, -1, offset);

	fprintf(stderr, "[ info ] mmap size: %x\n", size);

	if (addr == MAP_FAILED) {
		fprintf(stderr, "[ error ] mmap: %s\n", strerror(errno));
		exit(1);
	}

	fprintf(stderr, "[ info ] %d %p %p\n",
			getpid(), addr, addr + sizeof(int) * size);

	return !start ? addr + size / sizeof(int) : addr;
}

int main(int argc, char **argv)
{
	if (argc != 2) {
		fprintf(stderr, "<usage> ./pgtbl_sample <num_page>\n");
		exit(1);
	}

	int size = atoi(argv[1]);
	int *tmp_addr;

	tmp_addr = malloc_stuff(size * PAGE_SIZE, NULL, 0);

	int away = PAGE_SIZE * PAGE_SIZE;
	unsigned long aligned_addr =
		(unsigned long) (tmp_addr + away) & PAGE_MASK;

	/* allocate heap memory, but not using it */
	tmp_addr = malloc_stuff(size * PAGE_SIZE, (void *) aligned_addr, 0);

	stop();

	/* write fault */
	fprintf(stderr, "[ info ] %p writing(w/ fault)\n", tmp_addr);
	*(int *) tmp_addr = 10;

	stop();

	/* read fault */
	int one_pg_offset = PAGE_SIZE / sizeof(int);
	fprintf(stderr, "[ info ] %p reading (w/ fault), value: %d\n",
			tmp_addr + PAGE_SIZE,
			*(int *) (tmp_addr + one_pg_offset));
	
	stop();

	/* followed by a write */
	fprintf(stderr, "[ info ] %p read fault followed by a write\n",
			tmp_addr + one_pg_offset);
	*(int *) (tmp_addr + one_pg_offset) = 20;

	stop();

	/* write without fault */
	fprintf(stderr, "[ info ] %p writing without fault\n",
			tmp_addr + one_pg_offset);
	*(int *) (tmp_addr + one_pg_offset) = 20;

	stop();

	/* copy_on_write */
	pid_t pid = fork();
	if (pid < 0) {
		print_error("fork");
		exit(1);
	} else if (pid > 0) {
		/* parent process, tmp_addr is now shared */
		fprintf(stderr, "[ info ] %p: forked, now shared.\n",
				tmp_addr);

		stop();

		/* copy_on_write */
		*(int *) tmp_addr = 100;
		fprintf(stderr, "[ info ] %p:copy_on_write 100\n",
				tmp_addr);
	}


	return 0;
}
