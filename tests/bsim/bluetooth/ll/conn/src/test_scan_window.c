/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include "babblekit/testcase.h"
#include "bstests.h"

/* An initiator may answer an ADV_IND with a CONNECT_IND only if the CONNECT_IND
 * is over before the scan window is closed. In every round the central has one
 * scan window and the peripheral one advertising event, which comes later from
 * round to round and crosses the end of the scan window. A connection that the
 * central reports has to be one that the peripheral has as well.
 */

#define ROUNDS         64U
#define ROUND_MS       400U
#define FIRST_ROUND_MS 100U
#define WAIT_MS        (ROUND_MS / 2U)

/* One scan window in a round: the central gives up before the next one */
#define SCAN_INTERVAL  0x00a0 /* 100 ms */
#define SCAN_WINDOW    0x0020 /* 20 ms */
#define CREATE_TIMEOUT 6U     /* 60 ms */

/* One advertising event in a round: advertising is stopped before the next
 * one. The event is on air some 2 ms after it was started, which puts it at
 * the end of the scan window in the middle of the rounds.
 */
#define ADV_INTERVAL 0x0800 /* 1.28 s */
#define ADV_TIME_MS  50U
#define ADV_START_US 17600U
#define ADV_STEP_US  50U

/* A connection that the peripheral does not have is lost after six
 * connection intervals.
 */
#define CONN_INTERVAL 0x0006 /* 7.5 ms */
#define CONN_HOLD_MS  100U

static const bt_addr_le_t peripheral_addr = {
	.type = BT_ADDR_LE_RANDOM,
	.a.val = {0x01, 0x00, 0x00, 0x00, 0x00, 0xc0},
};

static K_SEM_DEFINE(connected_sem, 0, 1);
static K_SEM_DEFINE(disconnected_sem, 0, 1);
static uint8_t connected_err;
static uint8_t disconnected_reason;

static void connected(struct bt_conn *conn, uint8_t err)
{
	connected_err = err;
	k_sem_give(&connected_sem);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	disconnected_reason = reason;
	k_sem_give(&disconnected_sem);
}

static void round_wait(uint32_t round)
{
	k_sleep(K_TIMEOUT_ABS_MS(FIRST_ROUND_MS + (round * ROUND_MS)));
}

static void test_central_scan_window_main(void)
{
	const struct bt_conn_le_create_param create_param = {
		.options = BT_CONN_LE_OPT_NONE,
		.interval = SCAN_INTERVAL,
		.window = SCAN_WINDOW,
		.timeout = CREATE_TIMEOUT,
	};
	static struct bt_conn_cb conn_callbacks = {
		.connected = connected,
		.disconnected = disconnected,
	};
	uint32_t established = 0U;
	uint32_t missed = 0U;
	int err;

	TEST_START("central_scan_window");

	bt_conn_cb_register(&conn_callbacks);

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth init failed (err %d)", err);

	for (uint32_t round = 0U; round < ROUNDS; round++) {
		struct bt_conn *conn = NULL;

		round_wait(round);

		err = bt_conn_le_create(&peripheral_addr, &create_param,
					BT_LE_CONN_PARAM(CONN_INTERVAL, CONN_INTERVAL, 0, 100),
					&conn);
		TEST_ASSERT(err == 0, "Round %u: create failed (err %d)", round, err);

		err = k_sem_take(&connected_sem, K_MSEC(WAIT_MS));
		TEST_ASSERT(err == 0, "Round %u: no result of the create", round);

		if (connected_err == BT_HCI_ERR_UNKNOWN_CONN_ID) {
			missed++;
			TEST_PRINT("Round %u: no connection", round);
		} else {
			TEST_ASSERT(connected_err == 0U, "Round %u: connection failed (0x%02x)",
				    round, connected_err);

			err = k_sem_take(&disconnected_sem, K_MSEC(CONN_HOLD_MS));
			TEST_ASSERT(err != 0, "Round %u: connection lost (reason 0x%02x)", round,
				    disconnected_reason);

			err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
			TEST_ASSERT(err == 0, "Round %u: disconnect failed (err %d)", round, err);

			err = k_sem_take(&disconnected_sem, K_MSEC(WAIT_MS));
			TEST_ASSERT(err == 0, "Round %u: no disconnection", round);

			established++;
			TEST_PRINT("Round %u: connected", round);
		}

		bt_conn_drop(&conn);
	}

	TEST_ASSERT(established != 0U, "No advertising event was early enough");
	TEST_ASSERT(missed != 0U, "No advertising event was late enough");

	TEST_PASS("central_scan_window: %u connected, %u not", established, missed);
}

static void test_peripheral_scan_window_main(void)
{
	const struct bt_le_adv_param adv_param = {
		.id = BT_ID_DEFAULT,
		.options = BT_LE_ADV_OPT_CONN | BT_LE_ADV_OPT_USE_IDENTITY,
		.interval_min = ADV_INTERVAL,
		.interval_max = ADV_INTERVAL,
	};
	const struct bt_data ad[] = {
		BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	};
	bt_addr_le_t addr = peripheral_addr;
	uint8_t irk[16] = {0x01};
	int err;

	TEST_START("peripheral_scan_window");

	/* An identity that is created before bt_enable() needs its IRK */
	err = bt_id_create(&addr, IS_ENABLED(CONFIG_BT_PRIVACY) ? irk : NULL);
	TEST_ASSERT(err == BT_ID_DEFAULT, "Identity creation failed (err %d)", err);

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth init failed (err %d)", err);

	for (uint32_t round = 0U; round < ROUNDS; round++) {
		round_wait(round);
		k_busy_wait(ADV_START_US + (round * ADV_STEP_US));

		err = bt_le_adv_start(&adv_param, ad, ARRAY_SIZE(ad), NULL, 0);
		TEST_ASSERT(err == 0, "Round %u: advertising failed to start (err %d)", round, err);

		k_sleep(K_MSEC(ADV_TIME_MS));

		err = bt_le_adv_stop();
		TEST_ASSERT(err == 0, "Round %u: advertising failed to stop (err %d)", round, err);
	}

	TEST_PASS("peripheral_scan_window");
}

static const struct bst_test_instance test_scan_window[] = {
	{
		.test_id = "central_scan_window",
		.test_descr = "Initiates once per round and expects every connection to last.",
		.test_main_f = test_central_scan_window_main,
	},
	{
		.test_id = "peripheral_scan_window",
		.test_descr = "Advertises once per round, later in every round.",
		.test_main_f = test_peripheral_scan_window_main,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_scan_window_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_scan_window);
}
