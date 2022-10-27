/*
 * Copyright (c) 2022 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bstests.h"

extern struct bst_test_list *test_main_disable_install(struct bst_test_list *tests);

bst_test_install_t test_installers[] = {
	test_main_disable_install,
	NULL
};

void main(void)
{
	bst_main();
}
