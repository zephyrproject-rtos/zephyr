/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bstests.h"

struct bst_test_list *test_scan_req_tgta_install(struct bst_test_list *tests);
struct bst_test_list *test_anonymous_install(struct bst_test_list *tests);

bst_test_install_t test_installers[] = {
	test_scan_req_tgta_install,
	test_anonymous_install,
	NULL,
};

int main(void)
{
	bst_main();

	return 0;
}
