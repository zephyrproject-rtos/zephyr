/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "babblekit/sync.h"
#include "babblekit/testcase.h"

#include "common.h"

LOG_MODULE_REGISTER(peer, LOG_LEVEL_INF);

void entrypoint_peer(void)
{
	/* Test purpose:
	 *
	 * Verifies that a scanner using the extended scanner filter policy is reported
	 * directed advertisements whose target address is a resolvable private address the
	 * Controller cannot resolve, and that a scanner using the basic filter policy is not.
	 *
	 * Two devices:
	 * - `peer`: advertises directed at an unresolvable target address
	 * - `dut`: scans, first without and then with the extended scanner filter policy
	 *
	 * Procedure:
	 * - [peer] start low duty cycle directed connectable advertising, with a target
	 *   address that is a resolvable private address no Controller can resolve
	 * - [peer] tell the DUT that the advertiser is on air
	 * - [peer] keep advertising until the simulation ends
	 *
	 * [verdict]
	 * - the advertiser starts successfully
	 */
	bt_addr_le_t target = TEST_TARGET_ADDR;
	struct bt_le_adv_param param = *BT_LE_ADV_CONN_DIR_LOW_DUTY(&target);
	int err;

	TEST_START("peer");

	err = bk_sync_init();
	TEST_ASSERT(err == 0, "Failed to initialize sync (err %d)", err);

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Can't enable Bluetooth (err %d)", err);

	err = bt_le_adv_start(&param, NULL, 0, NULL, 0);
	TEST_ASSERT(err == 0, "Failed to start directed advertising (err %d)", err);

	LOG_INF("Directed advertising started");

	/* Let the DUT know that it can start scanning. */
	bk_sync_send();

	TEST_PASS("peer");

	/* Keep advertising for the rest of the simulation. Returning here would stop the
	 * advertiser before the DUT is done scanning.
	 */
	k_sleep(K_FOREVER);
}
