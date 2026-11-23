/* Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/testing.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/atomic_builtin.h>

#include "testlib/log_utils.h"
#include "babblekit/flags.h"
#include "babblekit/testcase.h"

LOG_MODULE_REGISTER(dut, LOG_LEVEL_DBG);

extern unsigned long runtime_log_level;
extern bool use_chain_adv;

static atomic_t reassembly_timeout_count;
static atomic_t reassembly_complete_count;
static atomic_t reassembly_complete_after_timeout_count;

void bt_testing_trace_ext_adv_reassembly_timeout(void)
{
	atomic_inc(&reassembly_timeout_count);
}

void bt_testing_trace_ext_adv_reassembly_complete(void)
{
	atomic_inc(&reassembly_complete_count);

	if (atomic_get(&reassembly_timeout_count) > 0) {
		atomic_inc(&reassembly_complete_after_timeout_count);
	}
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	k_msleep(500); /* simulate a slow memcpy (or user processing the scan data) */

	LOG_DBG("Device found: %s (RSSI %d), type %u, AD data len %u", bt_addr_le_str(addr),
		rssi, type, ad->len);
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
	 * A scanner with privacy enabled sleeps in every advertising report
	 * callback, simulating slow processing on real hardware. This
	 * exhausts the event buffer pools.
	 *
	 * A short RPA timeout is used to prompt the host into periodically
	 * sending a bunch of commands to the controller. Those commands should
	 * not fail.
	 *
	 * There are two variants (selected via the "chain" argstest flag):
	 *   base  - peer sends default extended advertising data.
	 *   chain - peer sends ~1000 B across ~6 AUX_CHAIN_IND PDUs so
	 *           fragmented HCI ext adv reports (PARTIAL -> COMPLETE) are
	 *           produced. When buffers are exhausted the chain-aware
	 *           discard logic must drop entire chains and the host
	 *           reassembly timer must recover the slot.
	 *
	 * Note: The base test variant only fails by the stack crashing.
	 *
	 * The base variant is a regression test for
	 * https://github.com/zephyrproject-rtos/zephyr/issues/78223
	 *
	 * Two devices:
	 * - `dut`: active-scans with privacy ON
	 * - `peer`: extended advertiser
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

	if (use_chain_adv) {
		atomic_val_t timeout_cnt = atomic_get(&reassembly_timeout_count);
		atomic_val_t complete_after_timeout_cnt =
			atomic_get(&reassembly_complete_after_timeout_count);
		atomic_val_t complete_cnt = atomic_get(&reassembly_complete_count);

		LOG_INF("Reassembly stats: timeout=%ld complete=%ld complete_after_timeout=%ld",
			timeout_cnt, complete_cnt, complete_after_timeout_cnt);

		TEST_ASSERT(timeout_cnt > 0,
			    "Expected at least one ext adv reassembly timeout");
		TEST_ASSERT(complete_after_timeout_cnt > 0,
			    "Expected fragmented reports to recover after timeout");
	}

	TEST_PASS_AND_EXIT("DUT passed");
}
