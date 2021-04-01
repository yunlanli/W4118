#include "expose_pgtbl.h"

int main(int argc, char **argv)
{
	struct expose_pgtbl_args args;
	struct pagetable_layout_info pgtbl_info;
	unsigned long pte_entries, pte_entry, fake_p4d_entry, fake_pmd_entry, fake_pud_entry;
	unsigned long virt;
	unsigned long va_begin, va_end;
	pid_t pid;
	int verbose, ret;
	int level;
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

	/* syscalls 436 */
	if ((ret = get_pagetable_layout(&pgtbl_info)))
			return ret;
	printf("[ info ]  get_pagetable_layout\n");

	level = get_kernel_pglevel(&pgtbl_info);

	temp_addr = mmap(NULL, 6*PAGE_SIZE, PROT_READ | PROT_WRITE,
			MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	
	if (temp_addr == MAP_FAILED)
		return -1;

	args.fake_pgd = (unsigned long) temp_addr;
	args.fake_p4ds = args.fake_pgd;
	if (level == 5)
		args.fake_p4ds += PAGE_SIZE;

	args.fake_puds = args.fake_p4ds + PAGE_SIZE;
	args.fake_pmds = args.fake_puds + PAGE_SIZE;
	args.page_table_addr = args.fake_pmds + PAGE_SIZE;

	fprintf(stderr,
		"fake_pgd: %lx\n"
		"fake_p4d: %lx\n"
		"fake_pud: %lx\n"
		"fake_pmd: %lx\n"
		"page_table_addr: %lx\n",
		args.fake_pgd,
		args.fake_p4ds, 
		args.fake_pmds, 
		args.fake_puds, 
		args.page_table_addr);

	/* syscalls 437 */
	if ((ret = expose_page_table(pid, &args)) != 0)
		return ret;
	printf("[ info ] expose_page_table\n");

	/* dump PTEs */
	printf("[ info ] dumping ptes...\n");
	for (virt = va_begin; virt < va_end; virt += PAGE_SIZE)
	{
		if ((fake_p4d_entry =
			*((unsigned long *)(args.fake_pgd) + get_index(virt, pgtbl_info.pgdir_shift))) == 0)
			continue;
		
		printf("[ info ] pgd_entry_addr=%p\n", 
				(unsigned long *)(args.fake_pgd) + get_index(virt, pgtbl_info.pgdir_shift));
		printf("[ info ] p4d_entry=%lx\n", fake_p4d_entry);
		
		if (level == 5) {
			if ((fake_pud_entry =
				*((unsigned long *)(args.fake_p4ds) + get_index(virt, pgtbl_info.p4d_shift))) == 0)
				continue;
		}
		else
			fake_pud_entry = fake_p4d_entry;
		
		printf("[ info ] pud_table_addr=%lx\n", fake_pud_entry);
		
		printf("[ info ] pud_table_index=%lx\n", get_index(virt, pgtbl_info.pud_shift));
		printf("[ info ] pud_entry_addr=%lx\n", 
				(fake_pud_entry) + 8* get_index(virt, pgtbl_info.pud_shift));
			
		if ((fake_pmd_entry = 
			*((unsigned long *)(fake_pud_entry) + get_index(virt, pgtbl_info.pud_shift))) == 0)
			continue;
		printf("[ info ] pmd_table_addr=%lx\n", fake_pmd_entry);
		
		if ((pte_entries = 
			*((unsigned long *)(fake_pmd_entry) + get_index(virt, pgtbl_info.pmd_shift))) == 0)
			continue;
		printf("[ info ] pte_entries=%lx\n", pte_entries);
		
		if ((pte_entry = 
			*((unsigned long *)(pte_entries) + get_index(virt, pgtbl_info.page_shift))) == 0)
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

