/*
 * Copyright (c) 2015-2016 Intel Corporation
 * Copyright (c) 2017-2019 Oticon A/S
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
#include <zephyr/bluetooth/services/hrs.h>

#include <zephyr/sys/byteorder.h>

static struct bt_conn *default_conn;

/*
 * Basic connection test:
 *   We expect a central to connect to us.
 *
 *   The thread code is mostly a copy of the peripheral_hr sample device
 */

#define WAIT_TIME 5 /*seconds*/
#define WAIT_TIME_REPEAT 22 /*seconds*/
extern enum bst_result_t bst_result;
static uint8_t repeat_connect;

#define FAIL(...)					\
	do {						\
		bst_result = Failed;			\
		bs_trace_error_time_line(__VA_ARGS__);	\
	} while (0)

#define PASS(...)					\
	do {						\
		bst_result = Passed;			\
		bs_trace_info_time(1, __VA_ARGS__);	\
	} while (0)

static void test_con2_init(void)
{
	bst_ticker_set_next_tick_absolute(WAIT_TIME*1e6);
	bst_result = In_progress;
}

static void test_con2_repeat_init(void)
{
	bst_ticker_set_next_tick_absolute(WAIT_TIME_REPEAT*1e6);
	bst_result = In_progress;
}

static void test_con2_tick(bs_time_t HW_device_time)
{
	/*
	 * If in WAIT_TIME seconds the testcase did not already pass
	 * (and finish) we consider it failed
	 */
	if (bst_result != Passed) {
		FAIL("test_connect2 failed (not passed after %i seconds)\n",
		     WAIT_TIME);
	}
}

static void test_con2_repeat_tick(bs_time_t HW_device_time)
{
	/*
	 * If in WAIT_TIME seconds the testcase did not already pass
	 * (and finish) we consider it failed
	 */
	if (bst_result != Passed) {
		FAIL("test_connect2 failed (not passed after %i seconds)\n",
		     WAIT_TIME_REPEAT);
	}
}

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
		      BT_UUID_16_ENCODE(BT_UUID_HRS_VAL),
		      BT_UUID_16_ENCODE(BT_UUID_BAS_VAL),
		      BT_UUID_16_ENCODE(BT_UUID_CTS_VAL)),
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		FAIL("Connection failed (err 0x%02x)\n", err);
	} else {
		default_conn = bt_conn_ref(conn);

		printk("Peripheral Connected\n");
		repeat_connect++;

		if (repeat_connect >= 20) { /* We consider it passed */
			PASS("Peripheral Repeat20 Testcase passed\n");
		}
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Peripheral disconnected (reason 0x%02x)\n", reason);

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

static void bas_notify(void)
{
	uint8_t battery_level = bt_bas_get_battery_level();

	battery_level--;

	if (!battery_level) {
		battery_level = 100U;
	}

	bt_bas_set_battery_level(battery_level);
}

static void hrs_notify(void)
{
	static uint8_t heartrate = 90U;

	/* Heartrate measurements simulation */
	heartrate++;
	if (heartrate == 160U) {
		heartrate = 90U;
	}

	bt_hrs_notify(heartrate);
}

static void test_con2_main(void)
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

		/* Heartrate measurements simulation */
		hrs_notify();

		/* Battery level simulation */
		bas_notify();

		if (notify_count++ == 1) { /* We consider it passed */
			PASS("Peripheral Testcase passed\n");
		}
	}
}

static void test_con2_repeat_main(void)
{
	int err;

	bt_conn_cb_register(&conn_callbacks);

	err = bt_enable(NULL);
	if (err) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	bt_ready();

	while (1) {
		k_sleep(K_SECONDS(1));
	}
}

static const struct bst_test_instance test_connect[] = {
	{
		.test_id = "peripheral",
		.test_descr = "Basic connection test. It expects that a "
			      "central device can be found. The test will "
			      "pass if notifications can be sent without "
			      "crash.",
		.test_post_init_f = test_con2_init,
		.test_tick_f = test_con2_tick,
		.test_main_f = test_con2_main
	},
	{
		.test_id = "peripheral_repeat20",
		.test_descr = "Multiple connections test. It expects that a "
			      "central device connects 20 times. The test will "
			      "pass if 20 connections are succeed in less than 22 seconds",
		.test_post_init_f = test_con2_repeat_init,
		.test_tick_f = test_con2_repeat_tick,
		.test_main_f = test_con2_repeat_main
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_connect2_install(struct bst_test_list *tests)
{
	tests = bst_add_tests(tests, test_connect);
	return tests;
}
