/* Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <string.h>

#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/atomic_builtin.h>
#include <babblekit/testcase.h>
#include <babblekit/sync.h>
#include <babblekit/flags.h>

LOG_MODULE_REGISTER(bt_bsim_scan_start_stop, LOG_LEVEL_DBG);

#define WAIT_TIME_S 60
#define WAIT_TIME   (WAIT_TIME_S * 1e6)

static atomic_t flag_adv_report_received;
static atomic_t flag_periodic_sync_established;
static bt_addr_le_t adv_addr;

static void test_tick(bs_time_t HW_device_time)
{
	if (bst_result != Passed) {
		TEST_FAIL("Test failed (not passed after %d seconds)\n", WAIT_TIME_S);
	}
}

static void test_init(void)
{
	bst_ticker_set_next_tick_absolute(WAIT_TIME);
}

static void bt_sync_established_cb(struct bt_le_per_adv_sync *sync,
				   struct bt_le_per_adv_sync_synced_info *info)
{
	LOG_DBG("Periodic sync established");
	atomic_set(&flag_periodic_sync_established, true);
}

static struct bt_le_per_adv_sync_cb sync_callbacks = {
	.synced = bt_sync_established_cb,
};

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	char addr_str[BT_ADDR_LE_STR_LEN];

	memcpy(&adv_addr, addr, sizeof(adv_addr));

	bt_addr_le_to_str(&adv_addr, addr_str, sizeof(addr_str));
	LOG_DBG("Device found: %s (RSSI %d), type %u, AD data len %u",
	       addr_str, rssi, type, ad->len);
	atomic_set(&flag_adv_report_received, true);
}

void run_dut(void)
{
	/* Test purpose:
	 *
	 * Verifies that the scanner can establish a sync to a device when
	 * it is explicitly enabled and disabled. Disabling the scanner
	 * explicitly should not stop the implicitly started scanner.
	 * Verify that the explicit scanner can be started when the
	 * implicit scanner is already running.
	 *
	 * Two devices:
	 * - `dut`: tries to establish the sync
	 * - `peer`: runs an advertiser / periodic advertiser
	 *
	 * Procedure:
	 * - [dut] start establishing a sync (no peer)
	 * - [peer] starts advertising
	 * - [dut] starts explicit scanning
	 * - [dut] wait for the peer to be found
	 * - [dut] stops explicit scanning
	 * - [dut] stop the periodic sync
	 * - [dut] start establishing a sync to the peer
	 * - [dut] start and stop explicit scanning
	 * - [peer] start periodic advertiser
	 * - [dut] wait until a sync is established
	 *
	 * [verdict]
	 * - dut is able to sync to the peer.
	 */

	LOG_DBG("start");
	bk_sync_init();
	int err;

	LOG_DBG("Starting DUT");

	err = bt_enable(NULL);
	TEST_ASSERT(!err, "Bluetooth init failed (err %d)\n", err);

	LOG_DBG("Bluetooth initialised");

	/* Try to establish a sync to a periodic advertiser.
	 * This will start the scanner.
	 */
	memset(&adv_addr, 0x00, sizeof(adv_addr));
	struct bt_le_per_adv_sync_param per_sync_param = {
		.addr = adv_addr,
		.options = 0x0,
		.sid = 0x0,
		.skip = 0x0,
		.timeout = BT_GAP_PER_ADV_MAX_TIMEOUT,
	};
	struct bt_le_per_adv_sync *p_per_sync;

	bt_le_per_adv_sync_cb_register(&sync_callbacks);

	err = bt_le_per_adv_sync_create(&per_sync_param, &p_per_sync);
	TEST_ASSERT(!err, "Periodic sync setup failed (err %d)\n", err);
	LOG_DBG("Periodic sync started");

	/* Start scanner. Check that we can start the scanner while it is already
	 * running due to the periodic sync.
	 */
	struct bt_le_scan_param scan_params = {
		.type = BT_LE_SCAN_TYPE_ACTIVE,
		.options = 0x0,
		.interval = 123,
		.window = 12,
		.interval_coded = 222,
		.window_coded = 32,
	};

	err = bt_le_scan_start(&scan_params, device_found);
	TEST_ASSERT(!err, "Scanner setup failed (err %d)\n", err);
	LOG_DBG("Explicit scanner started");

	LOG_DBG("Wait for an advertising report");
	WAIT_FOR_FLAG(flag_adv_report_received);

	/* Stop the scanner. That should not affect the periodic advertising sync. */
	err = bt_le_scan_stop();
	TEST_ASSERT(!err, "Scanner stop failed (err %d)\n", err);
	LOG_DBG("Explicit scanner stopped");

	/* We should be able to stop the periodic advertising sync. */
	err = bt_le_per_adv_sync_delete(p_per_sync);
	TEST_ASSERT(!err, "Periodic sync stop failed (err %d)\n", err);
	LOG_DBG("Periodic sync stopped");

	/* Start the periodic advertising sync. This time, provide the address of the advertiser
	 * which it should connect to.
	 */
	per_sync_param = (struct bt_le_per_adv_sync_param) {
		.addr = adv_addr,
		.options = 0x0,
		.sid = 0x0,
		.skip = 0x0,
		.timeout = BT_GAP_PER_ADV_MAX_TIMEOUT
	};
	err = bt_le_per_adv_sync_create(&per_sync_param, &p_per_sync);
	TEST_ASSERT(!err, "Periodic sync setup failed (err %d)\n", err);
	LOG_DBG("Periodic sync started");

	/* Start the explicit scanner */
	err = bt_le_scan_start(&scan_params, device_found);
	TEST_ASSERT(!err, "Scanner setup failed (err %d)\n", err);
	LOG_DBG("Explicit scanner started");

	/* Stop the explicit scanner. This should not stop scanner, since we still try to establish
	 * a sync.
	 */
	err = bt_le_scan_stop();
	TEST_ASSERT(!err, "Scanner stop failed (err %d)\n", err);
	LOG_DBG("Explicit scanner stopped");

	/* Signal to the tester to start the periodic adv. */
	bk_sync_send();

	/* Validate that we can still establish a sync */
	LOG_DBG("Wait for sync to periodic adv");
	WAIT_FOR_FLAG(flag_periodic_sync_established);

	/* Signal to the tester to end the test. */
	bk_sync_send();

	TEST_PASS("Test passed (DUT)\n");
}

void run_tester(void)
{
	LOG_DBG("start");
	bk_sync_init();

	int err;

	LOG_DBG("Starting DUT");

	err = bt_enable(NULL);
	TEST_ASSERT(!err, "Bluetooth init failed (err %d)\n", err);

	LOG_DBG("Bluetooth initialised");

	struct bt_le_ext_adv *per_adv;

	struct bt_le_adv_param adv_param = BT_LE_ADV_PARAM_INIT(BT_LE_ADV_OPT_EXT_ADV,
								BT_GAP_ADV_FAST_INT_MIN_2,
								BT_GAP_ADV_FAST_INT_MAX_2,
								NULL);

	err = bt_le_ext_adv_create(&adv_param, NULL, &per_adv);
	TEST_ASSERT(!err, "Failed to create advertising set: %d", err);
	LOG_DBG("Created extended advertising set.");

	err = bt_le_ext_adv_start(per_adv, BT_LE_EXT_ADV_START_DEFAULT);
	TEST_ASSERT(!err, "Failed to start extended advertising: %d", err);
	LOG_DBG("Started extended advertising.");

	/* Wait for the DUT before starting the periodic advertising */
	bk_sync_wait();
	err = bt_le_per_adv_set_param(per_adv, BT_LE_PER_ADV_DEFAULT);
	TEST_ASSERT(!err, "Failed to set periodic advertising parameters: %d", err);
	LOG_DBG("Periodic advertising parameters set.");

	err = bt_le_per_adv_start(per_adv);
	TEST_ASSERT(!err, "Failed to start periodic advertising: %d", err);
	LOG_DBG("Periodic advertising started.");

	/* Wait for the signal from the DUT before finishing the test */
	bk_sync_wait();

	bt_le_per_adv_stop(per_adv);

	TEST_PASS("Test passed (Tester)\n");
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "scanner",
		.test_descr = "SCANNER",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = run_dut,
	},
	{
		.test_id = "periodic_adv",
		.test_descr = "PER_ADV",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = run_tester,
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_scan_start_stop_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}

bst_test_install_t test_installers[] = {test_scan_start_stop_install, NULL};

int main(void)
{
	bst_main();
	return 0;
}
