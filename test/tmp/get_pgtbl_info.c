#include "expose_pgtbl.h"

int main()
{
	struct pagetable_layout_info pgtbl_info;

	get_pgtbl_layout(&pgtbl_info);
	print_pgtbl_info(&pgtbl_info);

	return 0;
}
