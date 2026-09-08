/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include "bstests.h"

extern void entrypoint_dut(void);
extern void entrypoint_peer(void);

static const struct bst_test_instance entrypoints[] = {
	{
		.test_id = "dut",
		.test_main_f = entrypoint_dut,
	},
	{
		.test_id = "peer",
		.test_main_f = entrypoint_peer,
	},
	BSTEST_END_MARKER,
};

static struct bst_test_list *install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, entrypoints);
};

bst_test_install_t test_installers[] = {install, NULL};

int main(void)
{
	bst_main();

	return 0;
}
