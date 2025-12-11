/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bstests.h"

void tester_verify_rpa_procedure(void);
void dut_rpa_expired_procedure(void);

static const struct bst_test_instance test_to_add[] = {
	{
		.test_id = "central_rpa_check",
		.test_main_f = tester_verify_rpa_procedure,
	},
	{
		.test_id = "peripheral_rpa_expired",
		.test_main_f = dut_rpa_expired_procedure,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_main_rpa_expired_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_to_add);
};
