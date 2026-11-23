/*
 * Copyright (c) 2026 Koppel Electronic
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bstests.h"

extern struct bst_test_list *test_local_gap_svc_central_install(struct bst_test_list *tests);

bst_test_install_t test_installers[] = {
	test_local_gap_svc_central_install,
	NULL
};

int main(void)
{
	bst_main();
	return 0;
}
