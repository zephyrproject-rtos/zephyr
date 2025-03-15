/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <errno.h>

#include <zephyr/sys/__assert.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>

#include "babblekit/testcase.h"
#include "babblekit/sync.h"
#include "testlib/addr.h"

#include "common/bt_str.h"

#define EXPECTED_NUM_ROTATIONS 5

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tester, 4);

struct test_data_t {
	bool		id_addr_ok;
	bt_addr_le_t	old_addr;
	int64_t		old_time;
	int		rpa_rotations;
	bool            addr_set;
} static test_data;

extern bt_addr_le_t dut_addr;

static void cb_expect_rpa(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			  struct net_buf_simple *ad)
{
	int64_t diff_ms, rpa_timeout_ms;

	if (bt_addr_le_eq(addr, &dut_addr)) {
		TEST_FAIL("DUT used identity addr instead of RPA");
	}

	/* Only save the address + time if this is the first scan */
	if (bt_addr_le_eq(&test_data.old_addr, BT_ADDR_LE_ANY)) {
		bt_addr_le_copy(&test_data.old_addr, addr);
		test_data.old_time = k_uptime_get();
		return;
	}

	/* Compare old and new address */
	if (bt_addr_le_eq(&test_data.old_addr, addr)) {
		return;
	}
	test_data.addr_set = true;
	LOG_DBG("Old addr: %s, New addr: %s", bt_testlib_addr_to_str(&test_data.old_addr),
		bt_testlib_addr_to_str(addr));

	test_data.rpa_rotations++;

	/* Ensure the RPA rotation occurs within +-10% of CONFIG_BT_RPA_TIMEOUT */
	diff_ms = k_uptime_get() - test_data.old_time;
	rpa_timeout_ms = CONFIG_BT_RPA_TIMEOUT * MSEC_PER_SEC;

	if (abs(diff_ms - rpa_timeout_ms) > (rpa_timeout_ms / 10)) {
		TEST_FAIL("RPA rotation did not occur within +-10%% of CONFIG_BT_RPA_TIMEOUT");
	}

	bt_addr_le_copy(&test_data.old_addr, addr);
	test_data.old_time = k_uptime_get();

	if (test_data.rpa_rotations > EXPECTED_NUM_ROTATIONS) {
		TEST_PASS("PASS");
	}
}

static void cb_expect_id(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	LOG_DBG("Expecting addr: %s, Got addr: %s", bt_testlib_addr_to_str(&test_data.old_addr),
		bt_testlib_addr_to_str(addr));

	if (addr->type != BT_ADDR_LE_RANDOM) {
		TEST_FAIL("Expected public address (0x%x) got 0x%x", BT_ADDR_LE_RANDOM, addr->type);
	}

	if (!bt_addr_le_eq(&dut_addr, addr)) {
		TEST_FAIL("DUT not using identity address");
	}
}

static void scan_cb(const bt_addr_le_t *addr, int8_t rssi, uint8_t type, struct net_buf_simple *ad)
{
	/* The DUT advertises with the identity address first, to test
	 * that option, but also to allow the DUT time to start its
	 * scanner. The scanner must be ready to capture the one of
	 * first RPA advertisements to accurately judge the RPA
	 * timeout, which is measured from the first RPA advertisement.
	 */
	if (!test_data.id_addr_ok) {
		cb_expect_id(addr, rssi, type, ad);

		/* Tell DUT to switch to RPA */
		bk_sync_send();
		test_data.id_addr_ok = true;
	} else {
		cb_expect_rpa(addr, rssi, type, ad);
	}
}

void tester_procedure(void)
{
	int err;

	/* open a backchannel to the peer */
	TEST_ASSERT(bk_sync_init() == 0);

	err = bt_enable(NULL);
	if (err) {
		TEST_FAIL("Failed to enable bluetooth (err %d)", err);
	}

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE_CONTINUOUS, scan_cb);

	if (err) {
		TEST_FAIL("Failed to start scanning");
	}
}
