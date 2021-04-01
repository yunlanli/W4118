#include "expose_pgtbl.h"

/* Assumes start and end page aligned */
void compute_pgtbl_num(unsigned long start, unsigned long end, int level, struct pagetable_num_info *info, struct pagetable_layout_info *layout)
{
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
	unsigned long pte_t, pteval, fake_p4d, fake_pmd, fake_pud;
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

	level = get_kernel_pglevel(&pgtbl_info);

	compute_pgtbl_num(va_begin, va_end, level, &num_info, &pgtbl_info);
	total_page = 1 + num_info.p4d_num + num_info.pmd_num + num_info.pud_num + num_info.pte_num;

	temp_addr = mmap(NULL, total_page * PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
	
	if (temp_addr == MAP_FAILED)
		return -1;

	args.fake_pgd = (unsigned long) temp_addr;
	args.fake_p4ds = args.fake_pgd;
	if (level == 5)
		args.fake_p4ds += PAGE_SIZE;

	args.fake_puds = args.fake_p4ds + PAGE_SIZE * (num_info.p4d_num ? num_info.p4d_num : 1);
	args.fake_pmds = args.fake_puds + PAGE_SIZE * num_info.pud_num;
	args.page_table_addr = args.fake_pmds + PAGE_SIZE * num_info.pmd_num;

	/* syscalls 437 */
	if ((ret = expose_page_table(pid, &args)) != 0)
		return ret;

	/* dump PTEs */
	for (virt = va_begin & PAGE_MASK; virt < va_end; virt += PAGE_SIZE)
	{
		/* pgd */
		if (!(fake_p4d = pgd_entry(virt, args.fake_pgd, &pgtbl_info)))
			goto verbose;

		/* p4d */
		if (level == 5 &&
				!(fake_pud = p4d_entry(virt, fake_p4d, &pgtbl_info)))
			goto verbose;
		else
			fake_pud = fake_p4d;

		/* pud */
		if (!(fake_pmd = pud_entry(virt, fake_pud, &pgtbl_info)))
			goto verbose;

		/* pmd */
		if (!(pte_t = pmd_entry(virt, fake_pmd, &pgtbl_info)))
			goto verbose;

		if (!(pteval = pte_entry(virt, pte_t, &pgtbl_info)))
			goto verbose;

		printf("%#014lx %#013lx %d %d %d %d\n", 
				virt,
				get_phys_addr(pteval),
				young_bit(pteval),
				dirty_bit(pteval),
				write_bit(pteval),
				user_bit(pteval));
		goto next;

verbose:
		if (verbose)
			printf("0xdead00000000 0x00000000000 0 0 0 0\n");
next:
		continue;
	}

	return 0;
}

