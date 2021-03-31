#include <linux/syscalls.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/expose_pgtbl.h>

/*
 * move base addresses of userspace fake tables by 1 Page
 * It will be added back on pgd_walk, pmd_walk, etc first write
 */
static inline void init_base(struct expose_pgtbl_args *args)
{
	args->fake_pgd -= PAGE_SIZE;
	args->fake_p4ds -= PAGE_SIZE;
	args->fake_puds -= PAGE_SIZE;
	args->fake_pmds -= PAGE_SIZE;
	args->page_table_addr -= PAGE_SIZE;
}

static inline int pte_copy(pmd_t *pmd, struct vm_area_struct *vma,
		struct expose_pgtbl_args *args, struct va_info *lst)
{
	unsigned long pfn = pmd_pfn(*pmd);
	unsigned long user_pte_addr = args->page_table_addr;

	struct vm_area_struct *user_vma = find_vma(current->mm, user_pte_addr);
	return remap_pfn_range(user_vma, user_pte_addr, pfn, PAGE_SIZE, vma->vm_page_prot);
}

/*
 * @src_pud: used to get the base address of pmd table containing the first pmd_entry 
 * @addr: the start address of the PMD_TABLE
 * @end: the last address of the PMD_TABLE or vma->end, 
 * 		depending on which comes faster
 * @args: contains the base addresses of page directories & tables in user space
 * @lst: contains most recent page entries copied / written
 */
static inline int pmd_walk(pud_t *src_pud,
		unsigned long addr,
		unsigned long end,
		struct vm_area_struct *vma,
		struct expose_pgtbl_args *args,
		struct va_info *lst)
{
	int err;
	pmd_t *src_pmd = pmd_offset(src_pud, addr);
	unsigned long usr_pmd_addr, next;

	/* 
	 * iterating over each pmd_entry in the
	 * 	@addr's pmd table , for each pmd do:
	 * remap the entire pte table containing @addr to the userspace
	 */
	do{
		next = pmd_addr_end(addr, end);

		/* no pte table to be copied, continue */
		if (pmd_none_or_clear_bad(src_pmd))
			continue;

		/* 
		 * copies 
		 * 	va of pte_tbl_base @args->page_table_addr
		 * into
		 *      fake_pmd_table entry @usr_pmd_addr
		 */
		if (src_pmd != lst->pmd) {
			/* increase user space base_pte to the next one */
			args->page_table_addr += PAGE_SIZE;
			lst->pmd = src_pmd;

			if( (err = pte_copy(src_pmd, vma, args, lst)) )
				return err;

			usr_pmd_addr = args->fake_pmds + pmd_index(addr);
			if(copy_to_user((void *) usr_pmd_addr,
					&args->page_table_addr,
					sizeof(unsigned long)))
				return -EFAULT;
		}
	} while (src_pmd++, addr = next, addr != end);

	return 0;
}

static inline int pud_walk(p4d_t 		*src_p4d,
		unsigned long                	addr,
		unsigned long                	end,
		struct vm_area_struct        	*vma,
		struct expose_pgtbl_args     	*args,
		struct va_info        		*lst)
{
	int err;
	pud_t *src_pud = pud_offset(src_p4d, addr);
	unsigned long usr_pud_addr, next;

	do{
		next = pud_addr_end(addr, end);

		if (pud_none_or_clear_bad(src_pud))
			continue;

		/* if @src_pud == @lst->pud:
		 *   do not update fake_pmd_base, 
		 *   same pmd table because same pud entry
		 *
		 * otherwise, we are looing at a new pmd table
		 *   increase the @args->fake_pmds, 
		 *   update the most recent written @lst->pud
		 *   which represents the a new pmd table in user space
		 */
		if (src_pud != lst->pud){
			args->fake_pmds += PAGE_SIZE;
			lst->pud = src_pud;
		}

		/* 
		 * still need to do the walk if regardless 
		 * if we are in the same pmd table or not
		 * because we could be using a different pmd_entry on the next level
		 */
		if( (err = pmd_walk(src_pud, addr, next, vma, args, lst)) )
			return err;

		/* 2. now we put the fake_pmd_addr to usr_pud_addr */
		usr_pud_addr = args->fake_pmds + pmd_index(addr);
		if(copy_to_user((void *)usr_pud_addr, 
					&args->fake_pmds, 
					sizeof(unsigned long)))
			return -EFAULT;

	}while(src_pud++, addr = next, addr != end);

	return 0;
}

static inline int p4d_walk(pgd_t		*src_pgd,
		unsigned long                	addr,
		unsigned long                	end,
		struct vm_area_struct        	*vma,
		struct expose_pgtbl_args     	*args,
		struct va_info        		*lst)
{
	int err;
	p4d_t *src_p4d = p4d_offset(src_pgd, addr);
	unsigned long usr_p4d_addr, next;

	if (CONFIG_PGTABLE_LEVELS == 4)
		return pud_walk((p4d_t *)src_pgd, addr, end, vma, args, lst);

	do{
		next = p4d_addr_end(addr, end);

		if (p4d_none_or_clear_bad(src_p4d))
			continue;
		
		if(src_p4d != lst->p4d){
			args->fake_puds += PAGE_SIZE;
			lst->p4d = src_p4d;
		}

		if( (err = pud_walk(src_p4d, addr, next, vma, args, lst)) )
			return err;

		usr_p4d_addr = args->fake_p4ds + p4d_index(addr);
		if(copy_to_user((void *)usr_p4d_addr, 
					&args->fake_puds, 
					sizeof(unsigned long)))
			return -EFAULT;

	}while(src_p4d++, addr = next, addr != end);

	return 0;
}

static inline int pgd_walk(unsigned long addr,
		unsigned long end,
		struct vm_area_struct *vma,
		struct expose_pgtbl_args *args,
		struct va_info *lst)
{
	int err;
	unsigned long usr_pgd_addr, next;
	struct mm_struct *src_mm = vma->vm_mm;

	pgd_t *src_pgd = pgd_offset(src_mm, addr);

	do{
		next = pgd_addr_end(addr, end);

		if (pgd_none_or_clear_bad(src_pgd))
			continue;

		if(src_pgd != lst->pgd){
			args->fake_p4ds += PAGE_SIZE;
			lst->pgd = src_pgd;
		}

		if( (err = p4d_walk(src_pgd, addr, next, vma, args, lst)) )
			return err;

		usr_pgd_addr = args->fake_pgd + pgd_index(addr);
		if(copy_to_user((void *)usr_pgd_addr, 
					&args->fake_p4ds, 
					sizeof(unsigned long)))
			return -EFAULT;

	}while(src_pgd++, addr = next, addr != end);

	return 0;
}

SYSCALL_DEFINE1(get_pagetable_layout,
		struct pagetable_layout_info __user *, pgtbl_info)
{
	struct pagetable_layout_info kinfo = {
		.pgdir_shift	= PGDIR_SHIFT,
		.p4d_shift	= P4D_SHIFT,
		.pud_shift	= PUD_SHIFT,
		.pmd_shift	= PMD_SHIFT,
		.page_shift	= PAGE_SHIFT,
	};

	if (copy_to_user(pgtbl_info, &kinfo,
			 sizeof(struct pagetable_layout_info)))
		return -EFAULT;

	return 0;
}

SYSCALL_DEFINE2(expose_page_table, pid_t, pid,
		struct expose_pgtbl_args __user *, args)
{
	struct expose_pgtbl_args kargs;
	struct va_info lst = { 0, 0, 0, 0, 0};
	struct vm_area_struct *vma;
	struct mm_struct *mm;
	struct task_struct *p;
	int err;
	unsigned long addr, end;

	if (copy_from_user(&kargs, args, sizeof(kargs)))
		return -EFAULT;

	init_base(&kargs);

	addr = kargs.begin_vaddr;

	p = pid == -1 ? current : find_task_by_vpid(pid);
	mm = p->mm;

	do {
		vma = find_vma(mm, addr);

		/* no vma is found */
		if (vma == NULL || (vma->vm_start > kargs.end_vaddr))
			return 0;

		/* our start vaddr @addr is the max of @vma->vm_start, @addr */
		addr = vma->vm_start < addr ? addr : vma->vm_start;
		end = vma->vm_end < kargs.end_vaddr ? vma->vm_end : kargs.end_vaddr;

		if ((err = pgd_walk(addr, end, vma, &kargs, &lst)))
			return err;

	} while (addr = vma->vm_end, addr <= kargs.end_vaddr);

	return 0;
}
