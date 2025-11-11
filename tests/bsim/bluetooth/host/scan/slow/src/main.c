/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include "bs_tracing.h"
#include "bstests.h"
#include "babblekit/testcase.h"
#include "testlib/log_utils.h"

extern void entrypoint_dut(void);
extern void entrypoint_peer(void);
extern enum bst_result_t bst_result;

unsigned long runtime_log_level = LOG_LEVEL_INF;

static void test_args(int argc, char *argv[])
{
	size_t argn = 0;
	const char *arg = argv[argn];

	if (strcmp(arg, "log_level") == 0) {

		runtime_log_level = strtoul(argv[++argn], NULL, 10);

		if (runtime_log_level >= LOG_LEVEL_NONE && runtime_log_level <= LOG_LEVEL_DBG) {
			TEST_PRINT("Runtime log level configuration: %d", runtime_log_level);
		} else {
			TEST_FAIL("Invalid arguments to set log level: %d", runtime_log_level);
		}
	} else {
		TEST_PRINT("Default runtime log level configuration: INFO");
	}
}

static void test_end_cb(void)
{
	if (bst_result != Passed) {
		TEST_PRINT("Test failed.");
	}
}

static const struct bst_test_instance entrypoints[] = {
	{
		.test_id = "dut",
		.test_delete_f = test_end_cb,
		.test_main_f = entrypoint_dut,
		.test_args_f = test_args,
	},
	{
		.test_id = "peer",
		.test_delete_f = test_end_cb,
		.test_main_f = entrypoint_peer,
		.test_args_f = test_args,
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
