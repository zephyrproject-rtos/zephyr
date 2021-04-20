/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bstests.h"

extern struct bst_test_list *test_vcs_install(struct bst_test_list *tests);
extern struct bst_test_list *test_vcs_client_install(struct bst_test_list *tests);

bst_test_install_t test_installers[] = {
	test_vcs_install,
	test_vcs_client_install,
	NULL
};

void main(void)
{
	bst_main();
}
