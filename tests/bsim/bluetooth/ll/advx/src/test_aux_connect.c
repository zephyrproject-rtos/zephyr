/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/kernel.h>

#include "babblekit/testcase.h"
#include "bstests.h"

/* An extended advertiser allocates the node rx for a connection when it accepts
 * an AUX_CONNECT_REQ, before it transmits the AUX_CONNECT_RSP. In the sweep
 * rounds the peripheral stops advertising a little later after the start from
 * round to round, so that the stop moves across the AUX_CONNECT_REQ and
 * AUX_CONNECT_RSP of the central's connection attempt. In the last round, which
 * does not stop advertising, both have to be connected.
 */

/* Round 0 measures when the peripheral is connected after the start of
 * advertising, and the sweep rounds stop advertising from STOP_BEFORE_US before
 * that time on, STOP_STEP_US later from round to round.
 */
#define SWEEP_ROUNDS   34U
#define ROUNDS         (SWEEP_ROUNDS + 2U)
#define ROUND_MS       100U
#define FIRST_ROUND_MS 100U
#define ADV_DELAY_MS   10U
#define STOP_BEFORE_US 700U
#define STOP_STEP_US   25U

/* The central initiates before the peripheral starts advertising, and gives up
 * before the next round. A stop right after the AUX_CONNECT_RSP can leave the
 * central with a connection that fails after six connection intervals.
 */
#define SCAN_INTERVAL  0x0060 /* 60 ms */
#define CREATE_TIMEOUT 4U     /* 40 ms */
#define CONN_INTERVAL  0x0006 /* 7.5 ms */
#define WAIT_MS        70U
#define ADV_INTERVAL   0x0020 /* 20 ms */

static const bt_addr_le_t peripheral_addr = {
	.type = BT_ADDR_LE_RANDOM,
	.a.val = {0x02, 0x00, 0x00, 0x00, 0x00, 0xc0},
};

static K_SEM_DEFINE(connected_sem, 0, 1);
static K_SEM_DEFINE(disconnected_sem, 0, 1);
static uint8_t disconnected_reason;
static uint32_t connected_cycles;
static uint8_t connected_err;

static void connected(struct bt_conn *conn, uint8_t err)
{
	connected_cycles = k_cycle_get_32();
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
	/* Both devices start each round at the same time, however long the last one took */
	(void)k_sleep(K_TIMEOUT_ABS_MS(FIRST_ROUND_MS + (round * ROUND_MS)));
}

static void test_central_aux_connect_main(void)
{
	const struct bt_conn_le_create_param create_param = {
		.options = BT_CONN_LE_OPT_NONE,
		.interval = SCAN_INTERVAL,
		.window = SCAN_INTERVAL,
		.timeout = CREATE_TIMEOUT,
	};
	static struct bt_conn_cb conn_callbacks = {
		.connected = connected,
		.disconnected = disconnected,
	};
	uint32_t established = 0U;
	uint32_t missed = 0U;
	int err;

	TEST_START("central_aux_connect");

	err = bt_conn_cb_register(&conn_callbacks);
	TEST_ASSERT(err == 0, "Callback registration failed (err %d)", err);

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth init failed (err %d)", err);

	for (uint32_t round = 0U; round < ROUNDS; round++) {
		const bool is_sweep = (round != 0U) && (round != (ROUNDS - 1U));
		struct bt_conn *conn = NULL;

		round_wait(round);

		err = bt_conn_le_create(&peripheral_addr, &create_param,
					BT_LE_CONN_PARAM(CONN_INTERVAL, CONN_INTERVAL, 0, 100),
					&conn);
		TEST_ASSERT(err == 0, "Round %u: create failed (err %d)", round, err);

		err = k_sem_take(&connected_sem, K_MSEC(WAIT_MS));
		TEST_ASSERT(err == 0, "Round %u: no result of the create", round);

		if (connected_err == BT_HCI_ERR_UNKNOWN_CONN_ID) {
			TEST_ASSERT(is_sweep, "Round %u: no connection without a stop", round);

			missed++;
			TEST_PRINT("Round %u: no connection", round);
		} else {
			TEST_ASSERT(connected_err == 0U, "Round %u: connection failed (0x%02x)",
				    round, connected_err);

			err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
			TEST_ASSERT(err == 0, "Round %u: disconnect failed (err %d)", round, err);

			err = k_sem_take(&disconnected_sem, K_MSEC(WAIT_MS));
			TEST_ASSERT(err == 0, "Round %u: no disconnection", round);

			if (is_sweep) {
				established++;
			} else {
				/* Only a connection that the peripheral has as well
				 * ends by the termination.
				 */
				TEST_ASSERT(disconnected_reason == BT_HCI_ERR_LOCALHOST_TERM_CONN,
					    "Round %u: disconnected with reason 0x%02x", round,
					    disconnected_reason);
			}

			TEST_PRINT("Round %u: connected", round);
		}

		bt_conn_drop(&conn);
	}

	TEST_ASSERT(missed != 0U, "No stop was early enough");
	TEST_ASSERT(established != 0U, "No stop was late enough");

	TEST_PASS_AND_EXIT("central_aux_connect: %u connected, %u not", established, missed);
}

static void test_peripheral_aux_connect_main(void)
{
	static struct bt_conn_cb conn_callbacks = {
		.connected = connected,
	};
	const struct bt_le_adv_param adv_param = {
		.id = BT_ID_DEFAULT,
		.options = BT_LE_ADV_OPT_CONN | BT_LE_ADV_OPT_EXT_ADV | BT_LE_ADV_OPT_USE_IDENTITY,
		.interval_min = ADV_INTERVAL,
		.interval_max = ADV_INTERVAL,
	};
	bt_addr_le_t addr = peripheral_addr;
	uint32_t connected_us = 0U;
	struct bt_le_ext_adv *adv;
	int err;

	TEST_START("peripheral_aux_connect");

	err = bt_conn_cb_register(&conn_callbacks);
	TEST_ASSERT(err == 0, "Callback registration failed (err %d)", err);

	err = bt_id_create(&addr, NULL);
	TEST_ASSERT(err == BT_ID_DEFAULT, "Identity creation failed (err %d)", err);

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth init failed (err %d)", err);

	err = bt_le_ext_adv_create(&adv_param, NULL, &adv);
	TEST_ASSERT(err == 0, "Advertising set creation failed (err %d)", err);

	for (uint32_t round = 0U; round < ROUNDS; round++) {
		const bool is_sweep = (round != 0U) && (round != (ROUNDS - 1U));
		uint32_t start_cycles;

		round_wait(round);

		/* Start after the central initiates, so that its connection attempt
		 * falls in the first advertising event, the one that round 0 measures.
		 */
		(void)k_sleep(K_MSEC(ADV_DELAY_MS));

		/* A connection of an earlier sweep round does not count */
		k_sem_reset(&connected_sem);

		err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
		TEST_ASSERT(err == 0, "Round %u: advertising failed to start (err %d)", round, err);

		start_cycles = k_cycle_get_32();

		if (is_sweep) {
			k_busy_wait(connected_us - STOP_BEFORE_US + ((round - 1U) * STOP_STEP_US));

			err = bt_le_ext_adv_stop(adv);
			TEST_ASSERT(err == 0, "Round %u: advertising failed to stop (err %d)",
				    round, err);

			continue;
		}

		err = k_sem_take(&connected_sem, K_MSEC(WAIT_MS));
		TEST_ASSERT((err == 0) && (connected_err == 0U), "Round %u: not connected", round);

		if (round == 0U) {
			connected_us = k_cyc_to_us_floor32(connected_cycles - start_cycles);
			TEST_ASSERT(connected_us > STOP_BEFORE_US,
				    "Round 0: connected %u us after the start of advertising",
				    connected_us);

			TEST_PRINT("Connected %u us after the start of advertising", connected_us);
		}
	}

	TEST_PASS("peripheral_aux_connect");
}

static const struct bst_test_instance test_aux_connect[] = {
	{
		.test_id = "central_aux_connect",
		.test_descr = "Initiates once per round, expects a connection without a stop.",
		.test_main_f = test_central_aux_connect_main,
	},
	{
		.test_id = "peripheral_aux_connect",
		.test_descr = "Advertises once per round, stops later from sweep round to round.",
		.test_main_f = test_peripheral_aux_connect_main,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_aux_connect_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_aux_connect);
}
