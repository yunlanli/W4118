#ifndef __EXPOSE_PGTBL_H__
#define __EXPOSE_PGTBL_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>

#define __NR_get_pgtbl_layout		436
#define __NR_expose_pgtbl_args		437
#define __WRITE_TO_VA			438

typedef unsigned int	u32;
typedef u32		uint32_t;

struct pagetable_layout_info {
	uint32_t pgdir_shift;
	uint32_t p4d_shift;
	uint32_t pud_shift;
	uint32_t pmd_shift;
	uint32_t page_shift;
};

struct expose_pgtbl_args {
	unsigned long fake_pgd;
	unsigned long fake_p4ds;
	unsigned long fake_puds;
	unsigned long fake_pmds;
	unsigned long page_table_addr;
	unsigned long begin_vaddr;
	unsigned long end_vaddr;
};

/* Kernel util Macros & Functions */
#define get_index(address, shift) (((address) >> shift) & 511)
#define PAGE_SIZE 4096
#define PAGE_MASK (~(PAGE_SIZE-1))

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

/* Userspace util functions */
static inline void print_error(char *func)
{
	fprintf(stderr, "[ Error ] %s: %s\n", func, strerror(errno));
}

int get_pagetable_layout(struct pagetable_layout_info *pgtbl_info)
{
	return syscall(__NR_get_pgtbl_layout, pgtbl_info);
}

int expose_page_table(pid_t pid, struct expose_pgtbl_args *args)
{
	return syscall(__NR_expose_pgtbl_args, pid, args);
}

static inline void print_pgtbl_info(struct pagetable_layout_info *pgtbl_info)
{
	fprintf(stderr,
		"\n"
		"PGD_SHIFT	P4D_SHIFT	PUD_SHIFT	PMD_SHIFT	PAGE_SHIFT\n"
		"---------	---------	---------	---------	----------\n"
		"   %u		   %u		   %u		   %u		    %u	  \n"
		"\n",
		pgtbl_info->pgdir_shift,
		pgtbl_info->p4d_shift,
		pgtbl_info->pud_shift,
		pgtbl_info->pmd_shift,
		pgtbl_info->page_shift);
}

static inline int get_kernel_pglevel(struct pagetable_layout_info *pgtbl_info)
{
	return pgtbl_info->pgdir_shift == pgtbl_info->p4d_shift ? 4 : 5;
}
#endif
