#ifndef __PAGEWALK_H__
#define __PAGEWALK_H__

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/hugetlb_inline.h>

struct pfn_node {
	struct rb_node node;
	unsigned long pfn;
};

struct va_info {
	struct rb_root *root;
	unsigned long new_pfn;
};

struct expose_count_args {
	unsigned long total;
	unsigned long zero;
};

extern int mmap_walk(struct mm_struct *srcmm,
		struct expose_count_args *args, struct va_info *lst);

#endif
