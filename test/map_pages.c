#include <stdio.h>
#include <sys/mman.h>
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


	/* transform zero pages to non-zero pages */
	fprintf(stderr, ANSI_COLOR_PURPLE
			"Press enter to transform %d zero pages "
			"to non-zero pages...\n", NUM_PAGES);
	getchar();
	fflush(stdin);

	zero_page(addr);


	/* scrub non-zero pages to zero pages */
	fprintf(stderr, ANSI_COLOR_PURPLE
			"Press enter to scrub %d non-zero pages "
			"to zero pages...\n", NUM_PAGES);
	getchar();
	fflush(stdin);

	non_zero_page(addr);


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


	/* exit */
	fprintf(stderr, ANSI_COLOR_PURPLE "Press enter to exit...\n");
	getchar();
	fflush(stdin);

	return 0;

err:
	return -1;
}
