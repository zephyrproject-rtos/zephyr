/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bstests.h"

void tester_procedure(void);
void tester_procedure_periph_delayed_start_of_conn_adv(void);
void dut_procedure(void);
void dut_procedure_connect_short_rpa_timeout(void);
void dut_procedure_connect_timeout(void);

static const struct bst_test_instance test_to_add[] = {
	{
		.test_id = "central",
		.test_descr = "Central performs active scanning using RPA",
		.test_main_f = dut_procedure,
	},
	{
		.test_id = "central_connect_short_rpa_timeout",
		.test_descr = "Central connects to a peripheral using a short RPA timeout",
		.test_main_f = dut_procedure_connect_short_rpa_timeout,
	},
	{
		.test_id = "central_connect_fails_with_short_rpa_timeout",
		.test_descr = "Central connects to a peripheral using a short RPA timeout"
			      " but expects connection establishment to time out.",
		.test_main_f = dut_procedure_connect_timeout,
	},
	{
		.test_id = "peripheral",
		.test_descr = "Performs scannable advertising, validates that the scanner"
			      " RPA address refreshes",
		.test_main_f = tester_procedure,
	},
	{
		.test_id = "periph_delayed_start_of_conn_adv",
		.test_descr = "Performs connectable advertising. "
			      "The advertiser is stopped for 10 seconds when instructed by the DUT"
			      " to allow it to run the initiator for longer than its RPA timeout.",
		.test_main_f = tester_procedure_periph_delayed_start_of_conn_adv,
	},
	BSTEST_END_MARKER,
};

static struct bst_test_list *install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_to_add);
};

bst_test_install_t test_installers[] = {install, NULL};

int main(void)
{
	bst_main();
	return 0;
}
