/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>

#include <zephyr/kernel.h>

#include "bs_cmd_line.h"
#include "bs_tracing.h"
#include "bstests.h"
#include "babblekit/testcase.h"
#include "testlib/log_utils.h"

extern void entrypoint_dut(void);
extern void entrypoint_peer(void);
extern enum bst_result_t bst_result;

unsigned long runtime_log_level = LOG_LEVEL_INF;
bool use_chain_adv;

static void test_args(int argc, char *argv[])
{
	bs_args_struct_t args_struct[] = {
		{
			.dest = &runtime_log_level,
			.type = 'u',
			.name = "{0..4}",
			.option = "log_level",
			.descript = "Runtime log level (0=NONE .. 4=DBG)",
		},
		{
			.dest = &use_chain_adv,
			.type = 'b',
			.name = "{0, 1}",
			.option = "chain",
			.descript = "Use chained (~1000 B) advertising data",
		},
		ARG_TABLE_ENDMARKER,
	};

	bs_args_parse_all_cmd_line(argc, argv, args_struct);
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
