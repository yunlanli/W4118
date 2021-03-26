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
	return 0;
}

SYSCALL_DEFINE1(write_to_va, unsigned long, addr)
{
	int val = 100;

	printk(KERN_DEBUG "address: %lu\n", addr);
	if (copy_to_user((void *) addr, &val, sizeof(int)))
		return -EFAULT;
//	* (int *) addr = 99;
	
	return 0;
}
