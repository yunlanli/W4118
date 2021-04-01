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

struct pagetable_num_info {
	uint32_t p4d_num;
	uint32_t pud_num;
	uint32_t pmd_num;
	uint32_t pte_num;
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
		"   %u		   %u		   %u		   %u		    %u\n"
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

static inline
unsigned long pgd_index(unsigned long addr,
		struct pagetable_layout_info *pgtbl_info)
{
	return get_index(addr, pgtbl_info->pgdir_shift) * sizeof(unsigned long);
}

static inline
unsigned long *pgd_offset(unsigned long addr, unsigned long pgd,
		struct pagetable_layout_info	*pgtbl_info)
{
	return  (unsigned long *) (pgd + pgd_index(addr, pgtbl_info));
}


static inline
unsigned long pgd_entry(unsigned long addr, unsigned long pgd,
		struct pagetable_layout_info	*pgtbl_info)
{
	return *pgd_offset(addr, pgd, pgtbl_info);
}

static inline
unsigned long p4d_index(unsigned long addr,
		struct pagetable_layout_info *pgtbl_info)
{
	return get_index(addr, pgtbl_info->p4d_shift) * sizeof(unsigned long);
}

static inline
unsigned long *p4d_offset(unsigned long addr, unsigned long p4d,
		struct pagetable_layout_info	*pgtbl_info)
{
	return  (unsigned long *) (p4d + p4d_index(addr, pgtbl_info));
}

static inline
unsigned long p4d_entry(unsigned long addr, unsigned long p4d,
		struct pagetable_layout_info	*pgtbl_info)
{
	return *p4d_offset(addr, p4d, pgtbl_info);
}

static inline
unsigned long pud_index(unsigned long addr,
		struct pagetable_layout_info *pgtbl_info)
{
	return get_index(addr, pgtbl_info->pud_shift) * sizeof(unsigned long);
}

static inline
unsigned long *pud_offset(unsigned long addr, unsigned long pud,
		struct pagetable_layout_info	*pgtbl_info)
{
	return  (unsigned long *) (pud + pud_index(addr, pgtbl_info));
}

static inline
unsigned long pud_entry(unsigned long addr, unsigned long pud,
		struct pagetable_layout_info	*pgtbl_info)
{
	return *pud_offset(addr, pud, pgtbl_info);
}

static inline
unsigned long pmd_index(unsigned long addr,
		struct pagetable_layout_info *pgtbl_info)
{
	return get_index(addr, pgtbl_info->pmd_shift) * sizeof(unsigned long);
}

static inline
unsigned long *pmd_offset(unsigned long addr, unsigned long pmd,
		struct pagetable_layout_info	*pgtbl_info)
{
	return  (unsigned long *) (pmd + pmd_index(addr, pgtbl_info));
}

static inline
unsigned long pmd_entry(unsigned long addr, unsigned long pmd,
		struct pagetable_layout_info	*pgtbl_info)
{
	return *pmd_offset(addr, pmd, pgtbl_info);
}

static inline
unsigned long pte_index(unsigned long addr,
		struct pagetable_layout_info *pgtbl_info)
{
	return get_index(addr, pgtbl_info->page_shift) * sizeof(unsigned long);
}

static inline
unsigned long *pte_offset(unsigned long addr, unsigned long pte,
		struct pagetable_layout_info	*pgtbl_info)
{
	return  (unsigned long *) (pte + pte_index(addr, pgtbl_info));
}

static inline
unsigned long pte_entry(unsigned long addr, unsigned long pte,
		struct pagetable_layout_info	*pgtbl_info)
{
	return *pte_offset(addr, pte, pgtbl_info);
}
#endif
