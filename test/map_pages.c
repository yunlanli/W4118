#include <stdio.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define ANSI_COLOR_RED          "\x1b[31m"
#define ANSI_COLOR_GREEN        "\x1b[32m"
#define ANSI_COLOR_PURPLE       "\x1b[35m"

#define NUM_PAGES		5
#define PAGE_SIZE		4096
#define PAGE_SHIFT		1024

#define MAP_FLAGS		MAP_FIXED | MAP_ANONYMOUS | MAP_PRIVATE
#define START_ADDR		0x556e088ff000

void do_inspect(pid_t pid)
{
	char *argv[3];
	int i = 0;

	for (i = 0; i < 3; i++) {
		argv[i] = malloc(16 * sizeof(char));
		if (argv[i] == NULL) {
			printf(ANSI_COLOR_RED "[ ERROR ] malloc() error\n");
			return;
		}
	}

	strcpy(argv[0], "./inspect_pages");
	sprintf(argv[1], "%d", pid);
	argv[2] = NULL;

	printf(ANSI_COLOR_GREEN "[ INFO ] %s %s", argv[0], argv[1]);
	printf("\n");

	i = fork();
	if (i < 0) {
		printf(ANSI_COLOR_RED "[ ERROR ] fork() failed. exit test.\n");
		return;
	} else if (i == 0) {
		execv(argv[0], argv);
		printf(ANSI_COLOR_RED "[ ERROR ] exec error\n");
		exit(1);
	} else {
		wait(NULL);
	}
}

static inline void zero_page(int *start)
{
	for (int i = 0; i < NUM_PAGES; i++)
		*(start + PAGE_SHIFT * i) = 0;
}

static inline void non_zero_page(int *start)
{
	for (int i = 0; i < NUM_PAGES; i++)
		*(start + PAGE_SHIFT * i) = 1;
}

int main()
{
	int *addr;
	pid_t pid = getpid();

	do_inspect(pid);

	/* map zero pages */
	fprintf(stderr, ANSI_COLOR_PURPLE
			"Press enter to map %d zero pages...\n",
			NUM_PAGES);
	getchar();
	fflush(stdin);

	addr = mmap((void *) START_ADDR, PAGE_SIZE * 5, PROT_READ | PROT_WRITE,
			MAP_FLAGS, -1, 0);
	if (addr == MAP_FAILED) {
		fprintf(stderr, ANSI_COLOR_RED "[ ERROR ] mmap failed.\n");
		goto err;
	}

	do_inspect(pid);


	/* transform zero pages to non-zero pages */
	fprintf(stderr, ANSI_COLOR_PURPLE
			"Press enter to transform %d zero pages "
			"to non-zero pages...\n", NUM_PAGES);
	getchar();
	fflush(stdin);

	zero_page(addr);
	do_inspect(pid);


	/* scrub non-zero pages to zero pages */
	fprintf(stderr, ANSI_COLOR_PURPLE
			"Press enter to scrub %d non-zero pages "
			"to zero pages...\n", NUM_PAGES);
	getchar();
	fflush(stdin);

	non_zero_page(addr);
	do_inspect(pid);


	/* unmap pages */
	fprintf(stderr, ANSI_COLOR_PURPLE
			"Press enter to unmap %d zero pages...\n",
			NUM_PAGES);
	getchar();
	fflush(stdin);

	if (munmap(addr, PAGE_SIZE * 5)) {
		fprintf(stderr, ANSI_COLOR_RED
				"[ ERROR ] %s\n",
				strerror(errno));
		goto err;
	}

	do_inspect(pid);


	/* exit */
	fprintf(stderr, ANSI_COLOR_PURPLE "Press enter to exit...\n");
	getchar();
	fflush(stdin);

	do_inspect(pid);

	return 0;

err:
	return -1;
}
