#ifndef __PAGEWALK_H__
#define __PAGEWALK_H__

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/hugetlb_inline.h>

struct pfn_node {
	struct rb_node node;
	unsigned long pfn;
};

struct list_rb_node {
	struct list_head head;
	struct rb_node *rb;
};

struct va_info {
	struct rb_root *root;
	unsigned long new_pfn;
};

struct expose_count_args {
	unsigned long total;
	unsigned long zero;
};

extern void free_pfn_rb_tree(struct rb_root *root);

extern int mmap_walk(struct mm_struct *srcmm,
		struct expose_count_args *args, struct va_info *lst);

#endif
