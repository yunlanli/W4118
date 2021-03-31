#include <sys/mman.h>
#include <sys/types.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define __NR_expose 437
#define PGD_SIZE 4096

struct expose_pgtbl_args {
	unsigned long fake_pgd;
	unsigned long fake_p4ds;
	unsigned long fake_puds;
	unsigned long fake_pmds;
	unsigned long page_table_addr;
	unsigned long begin_vaddr;
	unsigned long end_vaddr;
};

int expose_page_table(pid_t pid, struct expose_pgtbl_args *args)
{
	return syscall(__NR_expose, pid, args);
}
static inline unsigned long get_phys_addr(unsigned long pte_entry)
{
	return (((1UL << 52) - 1) & pte_entry) >> 12 << 12;
}
static inline int young_bit(unsigned long pte_entry)
{
	return 1UL << 5 & pte_entry ? 1 : 0;
}
static inline int dirty_bit(unsigned long pte_entry)
{
	return 1UL << 6 & pte_entry ? 1 : 0;
}
static inline int write_bit(unsigned long pte_entry)
{
	return 1UL << 1 & pte_entry ? 1 : 0;
}
static inline int user_bit(unsigned long pte_entry)
{
	return 1UL << 2 & pte_entry ? 1 : 0;
}

int main(int argc, char **argv)
{
	// struct expose_pgtbl_args args;
	unsigned long va_begin, va_end;
	pid_t pid;
	int verbose;

	verbose = 0;

	if (argc != 4 && argc != 5)
		return -1;
	else if (argc == 4) /* verbose mode TRUE */
	{
		printf("called");
		pid = atoi(argv[1]);
		sscanf(argv[2], "%lx", &va_begin);
		sscanf(argv[3], "%lx", &va_end);
	}
	else if (argc == 5) /* verbose mode FALSE */
	{
		if (strcmp(argv[1], "-v") != 0)
			return -1;
		verbose = 1;
		pid = atoi(argv[2]);
		sscanf(argv[3], "%lx", &va_begin);
		sscanf(argv[4], "%lx", &va_end);
	}

	/*nonsense*/
	if (va_begin >= va_end)
		return -1;

	printf("argc %d, pid: %d, verbose: %d, va_begin: %#014lx, va_end: %#014lx\n", 
		argc, pid, verbose, va_begin, va_end);

	return 0;
}

