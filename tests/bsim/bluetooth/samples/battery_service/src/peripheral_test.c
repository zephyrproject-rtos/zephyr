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
#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/services/bas.h>
#include <zephyr/bluetooth/services/bas_bls.h>

#include <zephyr/bluetooth/services/hrs.h>

#include <zephyr/sys/byteorder.h>

static struct bt_conn *default_conn;

/*
 * Battery Service test:
 *   We expect a central to connect to us.
 */

#define WAIT_TIME 15 /*seconds*/

extern enum bst_result_t bst_result;

#define FAIL(...)                                                                                  \
	do {                                                                                       \
		bst_result = Failed;                                                               \
		bs_trace_error_time_line(__VA_ARGS__);                                             \
	} while (0)

#define PASS(...)                                                                                  \
	do {                                                                                       \
		bst_result = Passed;                                                               \
		bs_trace_info_time(1, __VA_ARGS__);                                                \
	} while (0)

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
		FAIL("test_bas_peripheral failed (not passed after %i seconds)\n", WAIT_TIME);
	}
}

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_BAS_VAL)),
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		FAIL("Connection failed (err 0x%02x)\n", err);
	} else {
		default_conn = bt_conn_ref(conn);

		printk("Peripheral Connected\n");
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Peripheral %s (reason 0x%02x)\n", __func__, reason);

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

	printk("Peripheral Bluetooth initialized\n");

	err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		FAIL("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");
}

static void update_bas_char(void)
{
	printk("[PERIPHERAL] setting battery level\n");
	bt_bas_set_battery_level(90);
	printk("[PERIPHERAL] setting battery present\n");
	bt_bas_bls_set_battery_present(BT_BAS_BLS_BATTERY_PRESENT);
	printk("[PERIPHERAL] setting battery charge level\n");
	bt_bas_bls_set_battery_charge_level(BT_BAS_BLS_CHARGE_LEVEL_GOOD);
	printk("[PERIPHERAL] setting battery service required true\n");
	bt_bas_bls_set_service_required(BT_BAS_BLS_SERVICE_REQUIRED_TRUE);
}

static void test_bas_peripheral_main(void)
{
	static int notify_count;
	int err;

	bt_conn_cb_register(&conn_callbacks);

	err = bt_enable(NULL);
	if (err) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	bt_ready();

	/* Implement notification. At the moment there is no suitable way
	 * of starting delayed work so we do it here
	 */
	while (1) {

		k_sleep(K_SECONDS(1));

		/* Battery level simulation */
		update_bas_char();

		if (notify_count++ == 10) { /* We consider it passed */
			PASS("Peripheral Testcase passed\n");
		}
	}
}

static const struct bst_test_instance test_bas_peripheral[] = {
	{.test_id = "peripheral",
	 .test_descr = "Battery Service test. It expects that a central device can be found "
		       "The test will pass if ind/ntf can be sent without crash. ",
	 .test_pre_init_f = test_bas_peripheral_init,
	 .test_tick_f = test_bas_peripheral_tick,
	 .test_main_f = test_bas_peripheral_main},
	BSTEST_END_MARKER};

struct bst_test_list *test_bas_peripheral_install(struct bst_test_list *tests)
{
	tests = bst_add_tests(tests, test_bas_peripheral);
	return tests;
}
