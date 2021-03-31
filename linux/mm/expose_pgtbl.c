#include <linux/syscalls.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/expose_pgtbl.h>

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
	struct va_info lst;
	struct vm_area_struct *vma;
	struct mm_struct *mm;
	struct task_struct *p;
	int err;
	unsigned long addr, end;

	if (copy_from_user(&kargs, args, sizeof(kargs)))
		return -EFAULT;

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

	} while (addr = vma->vm_end, addr <= kargs.end_vaddr);

	return 0;
}


static inline int pte_copy(pmd_t *pmd, struct vm_area_struct *vma,
		struct expose_pgtbl_args *arg, struct va_info *lst)
{
	return 0;
}

/*
 * @src_pud: used to get the base address of pmd table containing the first pmd_entry
 * @addr: the start address 
 * @vma: vm_area_struct of @addr
 * @args: contains the base addresses of page directories & tables in user space
 * @lst: contains most recent page entries copied / written
 */
static inline int pmd_walk(pud_t *src_pud,
		unsigned long addr,
		struct vm_area_struct *vma,
		struct expose_pgtbl_args *args,
		struct va_info *lst)
{
	int err;
	unsigned long end = vma->vm_end;
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

			err = pte_copy(src_pmd, vma, args, lst);

			usr_pmd_addr = args->fake_pmds + pmd_index(addr);
			copy_to_user((void *) usr_pmd_addr,
					&args->page_table_addr,
					sizeof(unsigned long));              
		}
	} while (src_pmd++, addr = next, addr != end);

	return err;
}
