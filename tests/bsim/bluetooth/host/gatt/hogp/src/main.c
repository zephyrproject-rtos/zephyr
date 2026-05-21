/*
 * Copyright 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bstests.h"

extern struct bst_test_list *test_hogp_device_install(struct bst_test_list *tests);

bst_test_install_t test_installers[] = {
	test_hogp_device_install,
	NULL
};

int main(void)
{
	bst_main();
	return 0;
}
