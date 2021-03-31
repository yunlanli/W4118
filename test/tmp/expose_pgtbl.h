#ifndef __EXPOSE_PGTBL_H__
#define __EXPOSE_PGTBL_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>

#define __GET_PAGETABLE_LAYOUT		436
#define __EXPOSE_PGTBL_ARGS		437
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

static inline void print_error(char *func)
{
	fprintf(stderr, "[ Error ] %s: %s\n", func, strerror(errno));
}

static inline void get_pgtbl_layout(struct pagetable_layout_info *pgtbl_info)
{
	if (syscall(__GET_PAGETABLE_LAYOUT, pgtbl_info))
		print_error("get_pgtbl_layout");
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
#endif
