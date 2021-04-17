#include <linux/mm.h>

#include "pagewalk.h"

static inline void pfn_rb_insert(struct rb_root *root,
		struct pfn_node *new, struct expose_count_args *args)
{
	struct rb_node **node = &root->rb_node, *parent;
	unsigned long target_pfn = new->pfn;
	struct pfn_node *tmp;

	parent = *node;

	/* Go to the bottom of the tree */
	while (*node) {
		parent = *node;
		tmp = rb_entry(parent, struct pfn_node, node);

		if (tmp->pfn == target_pfn)
			return; /* duplicate */

		if (tmp->pfn > target_pfn)
			node = &parent->rb_left;
		else
			node = &parent->rb_right;
	}

	/* Put the new node there */
	rb_link_node(&new->node, parent, node);
	rb_insert_color(&new->node, root);

	if (is_zero_pfn(target_pfn))
		args->zero++;
	args->total++;
}

static inline int count_page(pte_t *pte, struct expose_count_args *args)
{
	struct pfn_node *new;
	struct rb_root *root;

	new->pfn = pte_pfn(*pte);
	pfn_rb_insert(root, new, args);

	return 0;
}

static inline int pte_walk(pmd_t *src_pmd,
	unsigned long addr,
	unsigned long end,
	struct mm_struct *src_mm,
	struct vm_area_struct *vma,
	struct expose_count_args *args,
	struct va_info *lst)
{
	pte_t *pte = pte_offset_map(src_pmd, addr);

	for (;;) {
		count_page(pte, args);

		addr += PAGE_SIZE;
		if (addr >= end)
			break;
		pte++;
	}

	return 0;
}

static inline int pmd_walk(pud_t *src_pud,
	unsigned long addr,
	unsigned long end,
	struct mm_struct *src_mm,
	struct vm_area_struct *vma,
	struct expose_count_args *args,
	struct va_info *lst)
{

	int err;
	unsigned long next;
	pmd_t *src_pmd = pmd_offset(src_pud, addr);

	do {
		next = pmd_addr_end(addr, end);

		if (pmd_none_or_clear_bad(src_pmd))
			continue;

		err = pte_walk(src_pmd, addr, next, src_mm, vma, args, lst);
		if (err < 0)
			return err;

	} while (src_pmd++, addr = next, addr != end);

	return 0;
}

static inline int pud_walk(p4d_t *src_p4d,
	unsigned long addr,
	unsigned long end,
	struct mm_struct *src_mm,
	struct vm_area_struct *vma,
	struct expose_count_args *args,
	struct va_info *lst)
{
	int err;
	unsigned long next;
	pud_t *src_pud = pud_offset(src_p4d, addr);

	do {
		next = pud_addr_end(addr, end);

		if (pud_none_or_clear_bad(src_pud))
			continue;

		err = pmd_walk(src_pud, addr, next, src_mm, vma, args, lst);

		if (err < 0)
			return err;

	} while (src_pud++, addr = next, addr != end);

	return 0;
}

static inline int p4d_walk(pgd_t *src_pgd,
	unsigned long addr,
	unsigned long end,
	struct mm_struct *src_mm,
	struct vm_area_struct *vma,
	struct expose_count_args *args,
	struct va_info *lst)
{
	int err;
	p4d_t *src_p4d = p4d_offset(src_pgd, addr);
	unsigned long next;

	if (CONFIG_PGTABLE_LEVELS == 4)
		return pud_walk((p4d_t *)src_pgd, addr, end,
			src_mm, vma, args, lst);

	do {
		next = p4d_addr_end(addr, end);

		if (p4d_none_or_clear_bad(src_p4d))
			continue;

		err = pud_walk(src_p4d, addr, next,
			src_mm, vma, args, lst);
		if (err < 0)
			return err;

	} while (src_p4d++, addr = next, addr != end);

	return 0;
}

static inline int pgd_walk(
	struct mm_struct *src_mm,
	struct vm_area_struct *vma,
	struct expose_count_args *args,
	struct va_info *lst)
{
	int err;
	unsigned long addr = vma->vm_start;
	unsigned long end = vma->vm_end;
	unsigned long next;

	pgd_t *src_pgd = pgd_offset(src_mm, addr);

	do {
		next = pgd_addr_end(addr, end);

		if (pgd_none_or_clear_bad(src_pgd))
			continue;

		err = p4d_walk(src_pgd, addr, next,
		       src_mm, vma, args, lst);

		if (err < 0)
			return err;
	} while (src_pgd++, addr = next, addr != end);

	return 0;
}

int mmap_walk(struct mm_struct *srcmm,
		struct expose_count_args *args, struct va_info *lst)
{
	struct vm_area_struct *mpnt;
	int ret;

	for (mpnt = srcmm->mmap; mpnt; mpnt = mpnt->vm_next) {
		ret = pgd_walk(srcmm, mpnt, args, lst);
		if (ret < 0)
			return ret;
	}
	return 0;
}
