/*
 * Copyright (c) 2024 Demant A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>

#include "bs_types.h"
#include "bs_tracing.h"
#include "time_machine.h"
#include "bstests.h"

#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/services/bas.h>
#include <zephyr/sys/byteorder.h>

#include "testlib/conn.h"
#include "testlib/scan.h"
#include "testlib/log_utils.h"

#include "babblekit/flags.h"
#include "babblekit/sync.h"
#include "babblekit/testcase.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(bt_bsim_bas, CONFIG_BT_BAS_LOG_LEVEL);

static struct bt_conn *default_conn;

static struct k_work_delayable update_bas_char_work;

/*
 * Battery Service test:
 *   We expect a central to connect to us.
 */

#define WAIT_TIME 10 /*seconds*/

extern enum bst_result_t bst_result;

static void test_bas_peripheral_init(void)
{
	bst_ticker_set_next_tick_absolute(WAIT_TIME * 1e6);
	bst_result = In_progress;
}

static void test_bas_peripheral_tick(bs_time_t HW_device_time)
{
	/*
	 * If in WAIT_TIME seconds the testcase did not already pass
	 * (and finish) we consider it failed
	 */
	if (bst_result != Passed) {
		TEST_FAIL("test_bas_peripheral failed (not passed after %i seconds)\n", WAIT_TIME);
	}
}

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_BAS_VAL)),
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		TEST_FAIL("Connection failed (err 0x%02x)\n", err);
	} else {
		default_conn = bt_conn_ref(conn);

		LOG_DBG("Peripheral Connected\n");
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_DBG("Peripheral %s (reason 0x%02x)\n", __func__, reason);

	if (default_conn) {
		bt_conn_unref(default_conn);
		default_conn = NULL;
	}
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

static void bt_ready(void)
{
	int err;

	LOG_DBG("Peripheral Bluetooth initialized\n");

	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		TEST_FAIL("Advertising failed to start (err %d)\n", err);
		return;
	}

	LOG_DBG("Advertising successfully started");
}

static void update_bas_char(void)
{
	LOG_DBG("[PERIPHERAL] setting battery level");
	bt_bas_set_battery_level(90);
	LOG_DBG("[PERIPHERAL] setting battery present");
	bt_bas_bls_set_battery_present(BT_BAS_BLS_BATTERY_PRESENT);
	LOG_DBG("[PERIPHERAL] setting battery charge level");
	bt_bas_bls_set_battery_charge_level(BT_BAS_BLS_CHARGE_LEVEL_CRITICAL);
	LOG_DBG("[PERIPHERAL] setting battery service required true");
	bt_bas_bls_set_service_required(BT_BAS_BLS_SERVICE_REQUIRED_TRUE);
	LOG_DBG("[PERIPHERAL] setting battery service charge type ");
	bt_bas_bls_set_battery_charge_type(BT_BAS_BLS_CHARGE_TYPE_FLOAT);
}

/* Work handler function */
void update_bas_char_work_handler(struct k_work *work)
{
	update_bas_char();
	k_work_reschedule(&update_bas_char_work, K_SECONDS(1));
}

static void test_bas_peripheral_main(void)
{
	int err;

	bt_conn_cb_register(&conn_callbacks);

	/* Mark test as in progress. */
	TEST_START("peripheral");

	/* Initialize device sync library */
	bk_sync_init();

	/* Initialize Bluetooth */
	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Can't enable Bluetooth (err %d)", err);

	LOG_DBG("Bluetooth initialized");

	bt_ready();

	/* Initialize the update bas char work handler */
	k_work_init_delayable(&update_bas_char_work, update_bas_char_work_handler);

	/* Schedule the update bas char work for delayed execution */
	k_work_schedule(&update_bas_char_work, K_SECONDS(1));

	/* Main thread waits for the sync signal from other device */
	bk_sync_wait();

	/*
	 * Once BLS Additional status service required flag is set to false,
	 * BCS Immediate service flag is also set to false. BCS char is
	 * read from central.
	 */
	bt_bas_bls_set_service_required(BT_BAS_BLS_SERVICE_REQUIRED_FALSE);
	bk_sync_wait();

	bst_result = Passed;
	TEST_PASS_AND_EXIT("Peripheral Test Passed");
}

static const struct bst_test_instance test_bas_peripheral[] = {
	{
		.test_id = "peripheral",
		.test_descr = "Battery Service test. It expects that a central device can be found "
			      "The test will pass if ind/ntf can be sent without crash. ",
		.test_pre_init_f = test_bas_peripheral_init,
		.test_tick_f = test_bas_peripheral_tick,
		.test_main_f = test_bas_peripheral_main,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_bas_peripheral_install(struct bst_test_list *tests)
{
	tests = bst_add_tests(tests, test_bas_peripheral);
	return tests;
}
