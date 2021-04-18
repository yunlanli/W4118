#include <linux/mm.h>

#include "pagewalk.h"

void free_pfn_rb_tree(struct rb_root *root)
{
	struct rb_node *curr;
	struct list_head stack;
	struct list_rb_node *tmp;
	int done = 0;

	INIT_LIST_HEAD(&stack);

	if (!root->rb_node)
		return;

	/* clone a root for freeing */
	curr = kmalloc(sizeof(struct rb_node), GFP_KERNEL);
	*curr = *(root->rb_node);

	while (!done) {
		if (curr) {
			tmp = kmalloc(sizeof(struct list_rb_node), GFP_KERNEL);
			INIT_LIST_HEAD(&tmp->head);
			tmp->rb = curr;
			list_add_tail(&tmp->head, &stack);
			curr = curr->rb_left;
		} else {
			if (!list_empty(&stack)) {
				tmp = list_last_entry(&stack,
						struct list_rb_node, head);
				curr = tmp->rb->rb_right;
				list_del(&tmp->head); //pop
				kfree(tmp->rb); // removes left most
				kfree(tmp);
			} else {
				done = 1;
			}
		}
	}
}

static void pfn_rb_insert(struct expose_count_args *args, struct va_info *lst)
{
	struct rb_node **node = &lst->root->rb_node, *parent;
	unsigned long new_pfn = lst->new_pfn;
	struct pfn_node *tmp;

	parent = *node;

	/* Go to the bottom of the tree */
	while (*node) {
		parent = *node;
		tmp = rb_entry(parent, struct pfn_node, node);

		if (tmp->pfn == new_pfn)
			return; /* duplicate */

		if (tmp->pfn > new_pfn)
			node = &parent->rb_left;
		else
			node = &parent->rb_right;
	}

	/* Put the new node there */
	tmp = kmalloc(sizeof(struct pfn_node), GFP_KERNEL);
	tmp->pfn = new_pfn;
	rb_link_node(&tmp->node, parent, node);
	rb_insert_color(&tmp->node, lst->root);

	if (is_zero_pfn(new_pfn))
		args->zero++;
	args->total++;
}

static int count_page(pte_t *pte, struct expose_count_args *args,
		struct va_info *lst)
{
	if (pte_present(*pte)) {
		lst->new_pfn = pte_pfn(*pte);
		pfn_rb_insert(args, lst);
	}

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
		if (!pte_none(*pte))
			count_page(pte, args, lst);


		addr += PAGE_SIZE;
		if (addr == end)
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
	unsigned long kvaddr;
	int ret;

	kvaddr = pgd_page_vaddr(*(srcmm->pgd + KERNEL_PGD_BOUNDARY));

	if (!srcmm)
		return 0;

	for (mpnt = srcmm->mmap; mpnt; mpnt = mpnt->vm_next) {
		if (unlikely(is_vm_hugetlb_page(mpnt)))
			continue;

		if (mpnt->vm_start >= kvaddr)
			break;

		ret = pgd_walk(srcmm, mpnt, args, lst);
		if (ret < 0)
			return ret;
	}
	return 0;
}
