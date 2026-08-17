/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bstests.h"
#include "bs_bt_utils.h"

extern void central(void);
extern void peripheral(void);
extern void central_reboot_subscribe_bond(void);
extern void central_reboot_resubscribe(void);
extern void peripheral_reboot_bond(void);
extern void peripheral_reboot_indicate(void);

static const struct bst_test_instance test_to_add[] = {
	{
		.test_id = "central",
		.test_main_f = central,
	},
	{
		.test_id = "peripheral",
		.test_main_f = peripheral,
	},
	{
		.test_id = "central_reboot_subscribe_bond",
		.test_descr = "Subscribe to Service Changed, then bond and disconnect. The bond "
			      "and the Service Changed configuration must survive a reboot.",
		.test_main_f = central_reboot_subscribe_bond,
	},
	{
		.test_id = "central_reboot_resubscribe",
		.test_descr = "After a reboot, re-arm the subscription from bond data, reconnect "
			      "and expect a Service Changed indication for a database change made "
			      "while disconnected.",
		.test_main_f = central_reboot_resubscribe,
	},
	{
		.test_id = "peripheral_reboot_bond",
		.test_descr = "Accept a connection from a client that subscribes to Service "
			      "Changed before bonding, persisting the bond and the Service "
			      "Changed configuration.",
		.test_main_f = peripheral_reboot_bond,
	},
	{
		.test_id = "peripheral_reboot_indicate",
		.test_descr = "After a reboot, change the database while the bonded client is "
			      "disconnected, then advertise so it can reconnect and receive the "
			      "Service Changed indication.",
		.test_main_f = peripheral_reboot_indicate,
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
