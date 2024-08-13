/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include "bs_tracing.h"
#include "bstests.h"
#include "babblekit/testcase.h"

extern void entrypoint_broadcaster(void);
extern enum bst_result_t bst_result;


static void test_end_cb(void)
{
	if (bst_result != Passed) {
		TEST_PRINT("Test has not passed.");
	}
}

static const struct bst_test_instance entrypoints[] = {
	{
		.test_id = "broadcaster",
		.test_delete_f = test_end_cb,
		.test_main_f = entrypoint_broadcaster,
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
