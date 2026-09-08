/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>

#include "babblekit/flags.h"
#include "babblekit/sync.h"
#include "babblekit/testcase.h"

#include "common.h"

LOG_MODULE_REGISTER(dut, LOG_LEVEL_INF);

/* How long to scan before concluding that nothing is reported. The peer advertises every 100 to
 * 150 ms, so this covers many advertising events.
 */
#define SCAN_NOTHING_REPORTED_TIMEOUT K_SECONDS(2)

DEFINE_FLAG_STATIC(flag_directed_report);

static void scan_recv_cb(const struct bt_le_scan_recv_info *info, struct net_buf_simple *buf)
{
	bt_addr_le_t expected_target = TEST_TARGET_ADDR;

	if ((info->adv_props & BT_GAP_ADV_PROP_DIRECTED) == 0U) {
		return;
	}

	/* A directed advertisement whose target address the Controller resolved is reported
	 * without one, so direct_addr is NULL. Neither that nor a resolved target address is
	 * the advertisement this test is about.
	 */
	if (info->direct_addr == NULL || info->direct_addr->type != BT_ADDR_LE_UNRESOLVED) {
		return;
	}

	TEST_ASSERT(bt_addr_eq(&info->direct_addr->a, &expected_target.a),
		    "Reported target address does not match the advertised one");
	TEST_ASSERT(info->adv_type == BT_GAP_ADV_TYPE_ADV_DIRECT_IND,
		    "Unexpected advertising type %u", info->adv_type);
	TEST_ASSERT((info->adv_props & BT_GAP_ADV_PROP_CONNECTABLE) != 0U,
		    "Directed advertisement not reported as connectable");

	SET_FLAG(flag_directed_report);
}

static struct bt_le_scan_cb scan_callbacks = {
	.recv = scan_recv_cb,
};

static void scan_start(bool ext_filter_policy)
{
	struct bt_le_scan_param param = {
		.type = BT_LE_SCAN_TYPE_PASSIVE,
		.options = BT_LE_SCAN_OPT_NONE,
		.interval = BT_GAP_SCAN_FAST_INTERVAL,
		.window = BT_GAP_SCAN_FAST_WINDOW,
	};
	int err;

	if (ext_filter_policy) {
		param.options |= BT_LE_SCAN_OPT_EXT_FILTER_POLICY;
	}

	err = bt_le_scan_start(&param, NULL);
	TEST_ASSERT(err == 0, "Failed to start scanning (err %d)", err);
}

static void scan_stop(void)
{
	int err = bt_le_scan_stop();

	TEST_ASSERT(err == 0, "Failed to stop scanning (err %d)", err);
}

void entrypoint_dut(void)
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
	 * - [dut] wait until the peer is advertising
	 * - [dut] scan without BT_LE_SCAN_OPT_EXT_FILTER_POLICY and check that the directed
	 *   advertisement is not reported
	 * - [dut] scan with BT_LE_SCAN_OPT_EXT_FILTER_POLICY and wait for the directed
	 *   advertisement to be reported
	 *
	 * [verdict]
	 * - nothing is reported while scanning with the basic filter policy
	 * - the directed advertisement is reported with the target address marked as
	 *   unresolved while scanning with the extended scanner filter policy
	 *
	 * The second phase reporting the advertisement is what proves the peer was on air
	 * during the first.
	 */
	int err;

	TEST_START("dut");

	err = bk_sync_init();
	TEST_ASSERT(err == 0, "Failed to initialize sync (err %d)", err);

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Can't enable Bluetooth (err %d)", err);

	err = bt_le_scan_cb_register(&scan_callbacks);
	TEST_ASSERT(err == 0, "Failed to register scan callbacks (err %d)", err);

	/* Wait until the peer is advertising. */
	bk_sync_wait();

	LOG_INF("Scan with the basic filter policy");
	scan_start(false);
	k_sleep(SCAN_NOTHING_REPORTED_TIMEOUT);
	scan_stop();

	TEST_ASSERT(!IS_FLAG_SET(flag_directed_report),
		    "Directed advertisement reported without the extended filter policy");

	LOG_INF("Scan with the extended scanner filter policy");
	scan_start(true);
	WAIT_FOR_FLAG(flag_directed_report);
	scan_stop();

	TEST_PASS("dut");
}
