/*
 * Verify the behaviour of the le_param_update_rejected connection callback.
 *
 * Scenario "peripheral" / "central":
 *   The peripheral (DUT) registers the callback and requests an
 *   application-initiated parameter update. The central rejects every parameter
 *   request via le_param_req, which results in the rejection being reported to
 *   the DUT either through the HCI Connection Parameter Request procedure or
 *   through the L2CAP CPUP fallback. The DUT verifies that the callback fires.
 *
 * Scenario "peripheral_auto" / "central":
 *   The peripheral does NOT request an update itself; instead the host
 *   auto-initiates one (CONFIG_BT_GAP_AUTO_UPDATE_CONN_PARAMS) which the central
 *   rejects. The DUT verifies that the rejection callback is NOT invoked, since
 *   it is only meant for application-initiated updates.
 *
 * Copyright (c) 2026 Sharon Lin
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "babblekit/testcase.h"

void test_peripheral_main(void);
void test_peripheral_auto_main(void);
void test_central_main(void);

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "peripheral",
		.test_descr = "DUT peripheral requesting a rejected parameter update",
		.test_main_f = test_peripheral_main,
	},
	{
		.test_id = "peripheral_auto",
		.test_descr = "DUT peripheral whose host-initiated update is rejected",
		.test_main_f = test_peripheral_auto_main,
	},
	{
		.test_id = "central",
		.test_descr = "Central rejecting the parameter update",
		.test_main_f = test_central_main,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_conn_param_update_reject_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}

bst_test_install_t test_installers[] = {test_conn_param_update_reject_install, NULL};

int main(void)
{
	bst_main();
	return 0;
}
