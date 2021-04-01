#include <sys/mman.h>
#include <sys/types.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define __NR_get_pgtbl_layout 436
#define __NR_expose_pgtbl_args 437

/* copied from the kernel */
#define get_index(address, shift) (((address) >> shift) & 511)


#define PAGE_SIZE 4096

struct expose_pgtbl_args {
	unsigned long fake_pgd;
	unsigned long fake_p4ds;
	unsigned long fake_puds;
	unsigned long fake_pmds;
	unsigned long page_table_addr;
	unsigned long begin_vaddr;
	unsigned long end_vaddr;
};
struct pagetable_layout_info {
	uint32_t pgdir_shift;
	uint32_t p4d_shift;
	uint32_t pud_shift;
	uint32_t pmd_shift;
	uint32_t page_shift;
 };

int get_pagetable_layout(struct pagetable_layout_info *pgtbl_info)
{
	return syscall(__NR_get_pgtbl_layout, pgtbl_info);
}
int expose_page_table(pid_t pid, struct expose_pgtbl_args *args)
{
	return syscall(__NR_expose_pgtbl_args, pid, args);
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
	struct expose_pgtbl_args args;
	struct pagetable_layout_info pgtbl_info;
	unsigned long pte_entries, pte_entry, fake_p4d_entry, fake_pmd_entry, fake_pud_entry;
	unsigned long virt;
	unsigned long va_begin, va_end;
	pid_t pid;
	int verbose, ret;
	int level = 5;
	void *temp_addr;

	verbose = 0;

	if (argc != 4 && argc != 5)
		return -1;
	else if (argc == 4) /* verbose mode TRUE */
	{
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

	/* nonsense */
	if (va_begin >= va_end)
		return -1;

	args.begin_vaddr = va_begin;
	args.end_vaddr = va_end;

	if ((ret = get_pagetable_layout(&pgtbl_info)) != 0)
			return ret;
	printf("[ info ]  get_pagetable_layout\n");
	if (pgtbl_info.pgdir_shift == pgtbl_info.p4d_shift)
		level = 4;

	temp_addr = mmap(NULL, 5*PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
	
	if (temp_addr == MAP_FAILED)
		return -1;

	args.fake_pgd = (unsigned long) temp_addr;
	args.fake_p4ds = args.fake_pgd;
	args.fake_puds = args.fake_p4ds + PAGE_SIZE;
	args.fake_pmds = args.fake_puds + PAGE_SIZE;
	args.page_table_addr = args.fake_pmds + PAGE_SIZE;

	fprintf(stderr,
		"fake_pgd: %lx\n"
		"fake_p4d: %lx\n"
		"fake_pud: %lx\n"
		"fake_pmd: %lx\n"
		"fake_pte: %lx\n",
		args.fake_pgd,
		args.fake_p4ds, 
		args.fake_pmds, 
		args.fake_puds, 
		args.page_table_addr);

	/* syscalls */
	if ((ret = expose_page_table(pid, &args)))
		return ret;

	printf("[ info ] expose_page_table\n");

	/* dump PTEs */
	printf("[ info ] dump ptes\n");
	for (virt = va_begin; virt < va_end; virt += PAGE_SIZE)
	{
		fake_p4d_entry = 
			*((unsigned long *)(args.fake_pgd) + get_index(virt, pgtbl_info.pgdir_shift));
		
		printf("[ info ] pgd_entry_addr=%p\n", 
				(unsigned long *)(args.fake_pgd + get_index(virt, pgtbl_info.pgdir_shift)));
		printf("[ info ] p4d_entry=%lx\n", fake_p4d_entry);

		if (fake_p4d_entry == 0)
			continue;
		
		if (level == 5) {
			fake_pud_entry = 
				*((unsigned long *)(args.fake_p4ds) + get_index(virt, pgtbl_info.p4d_shift));
		
			printf("[ info ] pud_entry=%lx\n", fake_pud_entry);
			if (fake_pud_entry == 0)
				continue;
		}
		else
			fake_pud_entry = fake_p4d_entry;
			
		
		fake_pmd_entry = 
			*((unsigned long *)(args.fake_puds) + get_index(virt, pgtbl_info.pud_shift));
		
		printf("[ info ] pmd_entry=%lx\n", fake_pmd_entry);

		if (fake_pmd_entry  == 0)
			continue;
		
		pte_entries = 
			*((unsigned long *)(args.fake_pmds) + get_index(virt, pgtbl_info.pmd_shift));

		if (pte_entries == 0)
			continue;
		
		printf("[ info ] pte_entries=%lx\n", pte_entries);
		
		pte_entry = 
			*((unsigned long *)(args.page_table_addr) + get_index(virt, pgtbl_info.page_shift));

		if (pte_entry == 0)
		{
			if (verbose)
				printf("0xdead00000000 0x00000000000 0 0 0 0\n");
			continue;
		}
		printf("[ info ] pte_entry=%lx\n", pte_entry);

		printf("%#014lx %#013lx %d %d %d %d\n", 
			virt,
			get_phys_addr(pte_entry),
			young_bit(pte_entry),
			dirty_bit(pte_entry),
			write_bit(pte_entry),
			user_bit(pte_entry));
	}
	

	return 0;
}

