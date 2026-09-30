/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bstests.h"

extern struct bst_test_list *test_main_buf_allocate_install(struct bst_test_list *tests);

bst_test_install_t test_installers[] = {
	test_main_buf_allocate_install,
	NULL
};

int main(void)
{
	bst_main();
	return 0;
}
