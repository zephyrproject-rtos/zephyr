/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include "bs_tracing.h"
#include "bstests.h"
#include "babblekit/testcase.h"

#include "common.h"

extern enum bst_result_t bst_result;

static void test_end_cb(void)
{
	if (bst_result != Passed) {
		TEST_PRINT("Test has not passed.");
	}
}

static const struct bst_test_instance entrypoints[] = {
	{
		.test_id = "server",
		.test_delete_f = test_end_cb,
		.test_main_f = server_procedure,
	},
	{
		.test_id = "holder",
		.test_delete_f = test_end_cb,
		.test_main_f = holder_procedure,
	},
	{
		.test_id = "writer",
		.test_delete_f = test_end_cb,
		.test_main_f = writer_procedure,
	},
	BSTEST_END_MARKER,
};

static struct bst_test_list *install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, entrypoints);
}

bst_test_install_t test_installers[] = { install, NULL };

int main(void)
{
	bst_main();

	return 0;
}
