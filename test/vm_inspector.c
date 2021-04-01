#include "expose_pgtbl.h"

/* Assumes start and end page aligned */
void compute_pgtbl_num(unsigned long start, unsigned long end, int level, struct pagetable_num_info *info, struct pagetable_layout_info *layout)
{
# if 0
	int p4d_entries = layout->pgdir_shift == layout->p4d_shift ? 0 : 1 << (layout->pgdir_shift - layout->p4d_shift);
	int pud_entries = 1 << (layout->p4d_shift - layout->pud_shift);
	int pmd_entries = 1 << (layout->pud_shift - layout->pmd_shift);
	int page_shift_entries = 1 << (layout->pmd_shift - layout->page_shift);
	int page_entries = 1 << (layout->page_shift);
# endif
	int num_pages = (end - start) >> (layout->page_shift);
	int num_pte = num_pages >> (layout->pmd_shift - layout->page_shift);
	int num_pmd = num_pte >> (layout->pud_shift - layout->pmd_shift);
	int num_pud = num_pmd >> (layout->p4d_shift - layout->pud_shift);
	int num_p4d = num_pud >> (layout->pgdir_shift - layout->p4d_shift);

	info->p4d_num = level==5? num_p4d == 0? 1 : num_p4d : 0;
	info->pud_num = num_pud == 0? 1 : num_pud;
	info->pmd_num = num_pmd == 0? 1 : num_pmd;
	info->pte_num = num_pte == 0? 1 : num_pte;
}

int main(int argc, char **argv)
{
	struct expose_pgtbl_args args;
	struct pagetable_layout_info pgtbl_info;
	struct pagetable_num_info num_info;
	unsigned long pte_entries, pte_entry, fake_p4d_entry, fake_pmd_entry, fake_pud_entry;
	unsigned long virt;
	unsigned long va_begin, va_end;
	pid_t pid;
	int level;
	int verbose, ret, total_page;
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

	compute_pgtbl_num(va_begin, va_end, level, &num_info, &pgtbl_info);
	total_page = 1 + num_info.p4d_num + num_info.pmd_num + num_info.pud_num + num_info.pte_num;

	printf("total = %d\n", total_page);

	temp_addr = mmap(NULL, total_page * PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
	
	if (temp_addr == MAP_FAILED)
		return -1;

	args.fake_pgd = (unsigned long) temp_addr;
	args.fake_p4ds = args.fake_pgd;
	if (level == 5)
		args.fake_p4ds += PAGE_SIZE;

	args.fake_puds = args.fake_p4ds + PAGE_SIZE * (num_info.p4d_num == 0? 1:num_info.p4d_num);
	args.fake_pmds = args.fake_puds + PAGE_SIZE * num_info.pud_num;
	args.page_table_addr = args.fake_pmds + PAGE_SIZE * num_info.pmd_num;

	fprintf(stderr,
		"fake_pgd: %lx\n"
		"fake_p4d: %lx\n"
		"fake_pud: %lx\n"
		"fake_pmd: %lx\n"
		"page_table_addr: %lx\n",
		args.fake_pgd,
		args.fake_p4ds, 
		args.fake_puds, 
		args.fake_pmds, 
		args.page_table_addr);

	/* syscalls 437 */
	if ((ret = expose_page_table(pid, &args)) != 0)
		return ret;
	printf("[ info ] expose_page_table\n");

	/* dump PTEs */
	printf("[ info ] dumping ptes...\n");
	for (virt = va_begin; virt < va_end; virt += PAGE_SIZE)
	{
		unsigned long p4d_base_addr = 
			(unsigned long)(args.fake_pgd) + 8*get_index(virt, pgtbl_info.pgdir_shift);
		if ((fake_p4d_entry = *(unsigned long *)p4d_base_addr) == 0)
			continue;
		
		printf("[ info ] p4d_entry=%lx\n", fake_p4d_entry);
		
		if (level == 5) {
			unsigned long pud_base_addr = 
				(unsigned long)(fake_p4d_entry) + 8*get_index(virt, pgtbl_info.p4d_shift);
			if ((fake_pud_entry = *(unsigned long *)pud_base_addr) == 0)
				continue;
		}
		else
			fake_pud_entry = fake_p4d_entry;
		
		printf("[ info ] pud_entry_addr=%lx\n", 
				(fake_pud_entry) + 8* get_index(virt, pgtbl_info.pud_shift));
		
		unsigned long pmd_base_addr = 
			(unsigned long)(fake_pud_entry) + 8*get_index(virt, pgtbl_info.pud_shift);
			
		if ((fake_pmd_entry = *(unsigned long *)pmd_base_addr) == 0)
			continue;

		printf("[ info ] pmd_table_addr=%lx\n", fake_pmd_entry);
		
		unsigned long pte_base_addr = 
			(unsigned long)(fake_pmd_entry) + 8*get_index(virt, pgtbl_info.pmd_shift);
		
		if ((pte_entries = *(unsigned long *)pte_base_addr) == 0)
			continue;
		
		unsigned long pte_entry_addr = 
			(unsigned long)(pte_entries) + 8*get_index(virt, pgtbl_info.page_shift);

		printf("[ info ] pte_entries=%lx\n", pte_entries);
		
		if ((pte_entry = *(unsigned long *)pte_entry_addr) == 0)
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

