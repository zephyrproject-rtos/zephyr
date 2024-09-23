/* Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/atomic_builtin.h>

#include "testlib/log_utils.h"
#include "babblekit/flags.h"
#include "babblekit/testcase.h"

LOG_MODULE_REGISTER(dut, LOG_LEVEL_DBG);

extern unsigned long runtime_log_level;

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	char addr_str[BT_ADDR_LE_STR_LEN];

	k_msleep(500); /* simulate a slow memcpy (or user processing the scan data) */

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
	LOG_DBG("Device found: %s (RSSI %d), type %u, AD data len %u", addr_str, rssi, type,
		ad->len);
}

#define BT_LE_SCAN_ACTIVE_CONTINUOUS_WITH_DUPLICATES                                               \
	BT_LE_SCAN_PARAM(BT_LE_SCAN_TYPE_ACTIVE, 0, BT_GAP_SCAN_FAST_INTERVAL_MIN,                 \
			 BT_GAP_SCAN_FAST_WINDOW)

void entrypoint_dut(void)
{
	/* Test purpose:
	 *
	 * Verifies that the host can handle running out of HCI RX buffers.
	 *
	 * To test this, we use a scanner with privacy enabled and sleep a bit
	 * when we get every advertising report. This sleep period simulates a
	 * slow "memcpy" operation on actual hardware. In effect, this uses up
	 * the event buffer pools.
	 *
	 * A short RPA timeout is used to prompt the host into periodically
	 * sending a bunch of commands to the controller. Those commands should
	 * not fail.
	 *
	 * Note: This test only fails by the stack crashing.
	 *
	 * It is a regression test for
	 * https://github.com/zephyrproject-rtos/zephyr/issues/78223
	 *
	 * Two devices:
	 * - `dut`: active-scans with privacy ON
	 * - `peer`: bog-standard advertiser
	 *
	 * [verdict]
	 * - dut is able to run for a long enough time without triggering asserts
	 */
	int err;

	TEST_START("DUT");

	/* Set the log level given by the `log_level` CLI argument */
	bt_testlib_log_level_set("dut", runtime_log_level);

	err = bt_enable(NULL);
	TEST_ASSERT(!err, "Bluetooth init failed (err %d)\n", err);

	LOG_DBG("Bluetooth initialised");

	err = bt_le_scan_start(BT_LE_SCAN_ACTIVE_CONTINUOUS_WITH_DUPLICATES, device_found);
	TEST_ASSERT(!err, "Scanner setup failed (err %d)\n", err);
	LOG_DBG("Explicit scanner started");

	/* 40 seconds ought to be enough for anyone */
	k_sleep(K_SECONDS(40));

	TEST_PASS_AND_EXIT("DUT passed");
}
