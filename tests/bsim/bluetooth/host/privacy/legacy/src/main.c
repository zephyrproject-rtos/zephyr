/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bstests.h"

void tester_procedure(void);
void dut_procedure(void);
struct bst_test_list *test_scan_identity_adv_install(struct bst_test_list *tests);
struct bst_test_list *test_auto_identity_adv_install(struct bst_test_list *tests);

static const struct bst_test_instance test_to_add[] = {
	{
		.test_id = "central",
		.test_main_f = tester_procedure,
	},
	{
		.test_id = "peripheral",
		.test_main_f = dut_procedure,
	},
	BSTEST_END_MARKER,
};

static struct bst_test_list *install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_to_add);
};

bst_test_install_t test_installers[] = {install, test_scan_identity_adv_install,
					test_auto_identity_adv_install, NULL};

int main(void)
{
	bst_main();
	return 0;
}
