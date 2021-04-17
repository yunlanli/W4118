#ifndef __PAGEWALK_H__
#define __PAGEWALK_H__

#include <linux/kernel.h>

struct pfn_node {
	struct rb_node node;
	unsigned long pfn;
};

struct va_info {
	pte_t last_pte;
};

struct expose_count_args {
	int total;
	int zero;
};

extern int mmap_walk(struct mm_struct *srcmm,
		struct expose_count_args *args, struct va_info *lst);

#endif
