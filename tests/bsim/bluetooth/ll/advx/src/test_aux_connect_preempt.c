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
#include <zephyr/sys/util.h>

#include "subsys/bluetooth/controller/util/memq.h"
#include "subsys/bluetooth/controller/util/mayfly.h"
#include "subsys/bluetooth/controller/ticker/ticker.h"
#include "subsys/bluetooth/controller/ll_sw/nordic/hal/nrf5/ticker.h"
#include "subsys/bluetooth/controller/ll_sw/pdu_df.h"
#include "subsys/bluetooth/controller/ll_sw/nordic/lll/pdu_vendor.h"
#include "subsys/bluetooth/controller/ll_sw/pdu.h"
#include "subsys/bluetooth/controller/ll_sw/lll.h"
#include "subsys/bluetooth/controller/ll_sw/lll_clock.h"
#include "subsys/bluetooth/controller/ll_sw/nordic/lll/lll_internal.h"

#include "babblekit/testcase.h"
#include "bstests.h"

/* In round 1, right after the peripheral's Controller has accepted the
 * AUX_CONNECT_REQ, the test prepares an event of its own whose preempt timeout
 * has already passed, so that it preempts the auxiliary event before the
 * AUX_CONNECT_RSP. That prepare has to be cancelled and the AUX_CONNECT_RSP has
 * to establish the connection, as in round 0. In round 2 the same prepare, made
 * while no event runs, has to run.
 */
#define ROUNDS         3U
#define ROUND_MS       200U
#define FIRST_ROUND_MS 100U
#define ADV_DELAY_MS   10U
#define ADV_INTERVAL   0x0020 /* 20 ms */
#define SCAN_INTERVAL  0x0060 /* 60 ms */
#define CREATE_TIMEOUT 10U    /* 100 ms */
#define CONN_INTERVAL  0x0006 /* 7.5 ms */
#define WAIT_MS        150U

/* Far enough back for any preempt timeout of the Controller to have passed */
#define PREPARE_BACK_US 10000U

static const bt_addr_le_t peripheral_addr = {
	.type = BT_ADDR_LE_RANDOM,
	.a.val = {0x02, 0x00, 0x00, 0x00, 0x00, 0xc0},
};

static K_SEM_DEFINE(connected_sem, 0, 1);
static K_SEM_DEFINE(disconnected_sem, 0, 1);
static uint8_t disconnected_reason;
static uint32_t connected_cycles;
static uint8_t connected_err;

static struct ull_hdr preemptor_ull;
static struct lll_hdr preemptor_lll = {
	.parent = &preemptor_ull,
};
static struct lll_prepare_param preemptor_prepare_param;
static uint32_t volatile preemptor_cancelled;
static uint32_t volatile preemptor_ran;
static uint32_t volatile rsp_count;
static int volatile preemptor_err;
static bool volatile preemptor_armed;

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

static int preemptor_prepare_cb(struct lll_prepare_param *prepare_param)
{
	preemptor_ran++;

	/* End the event at once, as an event that finds itself stopped does */
	lll_abort_cb(NULL, prepare_param->param);

	return 0;
}

static void preemptor_abort_cb(struct lll_prepare_param *prepare_param, void *param)
{
	if (prepare_param != NULL) {
		preemptor_cancelled++;
	}

	lll_abort_cb(prepare_param, param);
}

static void preemptor_prepare(void *param)
{
	int err;

	err = lll_hfclock_on();
	if (err < 0) {
		preemptor_err = err;

		return;
	}

	/* Taken back by the done event, as for the prepare of a role */
	preemptor_ull.ref++;

	/* With its preempt timeout passed, lll.c asks the current event at once */
	preemptor_prepare_param.ticks_at_expire =
		(ticker_ticks_now_get() - HAL_TICKER_US_TO_TICKS(PREPARE_BACK_US)) &
		HAL_TICKER_CNTR_MASK;
	preemptor_prepare_param.param = &preemptor_lll;

	err = lll_prepare(lll_is_abort_cb, preemptor_abort_cb, preemptor_prepare_cb, 0,
			  &preemptor_prepare_param);
	if ((err != 0) && (err != -EINPROGRESS)) {
		preemptor_err = err;
	}
}

static memq_link_t preemptor_link;
static struct mayfly preemptor_mfy = {0, 0, &preemptor_link, NULL, preemptor_prepare};

/* Only declared: the linker option --wrap=radio_pkt_tx_set (see CMakeLists.txt)
 * resolves it to the HAL's radio_pkt_tx_set() and routes the Controller's calls
 * of radio_pkt_tx_set() to __wrap_radio_pkt_tx_set() below.
 */
void __real_radio_pkt_tx_set(void *tx_packet);

void __wrap_radio_pkt_tx_set(void *tx_packet)
{
	const struct pdu_adv *pdu = tx_packet;

	/* The radio ISR of an accepted AUX_CONNECT_REQ sets up the AUX_CONNECT_RSP:
	 * prepare the preemptor right after that ISR, from the same context.
	 */
	if (pdu->type == PDU_ADV_TYPE_AUX_CONNECT_RSP) {
		rsp_count++;

		if (preemptor_armed) {
			uint32_t ret;

			preemptor_armed = false;

			ret = mayfly_enqueue(TICKER_USER_ID_LLL, TICKER_USER_ID_LLL, 1U,
					     &preemptor_mfy);
			if (ret != 0U) {
				preemptor_err = -EBUSY;
			}
		}
	}

	__real_radio_pkt_tx_set(tx_packet);
}

static void test_central_aux_connect_preempt_main(void)
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
	int err;

	TEST_START("central_aux_connect_preempt");

	err = bt_conn_cb_register(&conn_callbacks);
	TEST_ASSERT(err == 0, "Callback registration failed (err %d)", err);

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
		TEST_ASSERT((err == 0) && (connected_err == 0U), "Round %u: not connected", round);

		err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		TEST_ASSERT(err == 0, "Round %u: disconnect failed (err %d)", round, err);

		/* Only a connection that the peripheral has as well ends by the
		 * termination.
		 */
		err = k_sem_take(&disconnected_sem, K_MSEC(WAIT_MS));
		TEST_ASSERT((err == 0) && (disconnected_reason == BT_HCI_ERR_LOCALHOST_TERM_CONN),
			    "Round %u: disconnected with reason 0x%02x", round,
			    disconnected_reason);

		bt_conn_drop(&conn);
	}

	TEST_PASS_AND_EXIT("central_aux_connect_preempt");
}

static void test_peripheral_aux_connect_preempt_main(void)
{
	static struct bt_conn_cb conn_callbacks = {
		.connected = connected,
		.disconnected = disconnected,
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

	TEST_START("peripheral_aux_connect_preempt");

	err = bt_conn_cb_register(&conn_callbacks);
	TEST_ASSERT(err == 0, "Callback registration failed (err %d)", err);

	err = bt_id_create(&addr, NULL);
	TEST_ASSERT(err == BT_ID_DEFAULT, "Identity creation failed (err %d)", err);

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth init failed (err %d)", err);

	err = bt_le_ext_adv_create(&adv_param, NULL, &adv);
	TEST_ASSERT(err == 0, "Advertising set creation failed (err %d)", err);

	for (uint32_t round = 0U; round < ROUNDS; round++) {
		uint32_t start_cycles;
		k_timeout_t timeout;
		uint32_t at_us;

		round_wait(round);

		/* Start after the central initiates, so that it connects in the first
		 * advertising event, the one that round 0 measures.
		 */
		(void)k_sleep(K_MSEC(ADV_DELAY_MS));

		if (round == 1U) {
			preemptor_armed = true;
		} else if (round == 2U) {
			uint32_t ret;

			ret = mayfly_enqueue(TICKER_USER_ID_THREAD, TICKER_USER_ID_LLL, 0U,
					     &preemptor_mfy);
			TEST_ASSERT(ret == 0U, "Round 2: prepare not enqueued (ret %u)", ret);

			/* Let the LLL run the prepare, which ends its event at once */
			(void)k_sleep(K_MSEC(1));

			TEST_ASSERT(preemptor_ran == 1U, "Round 2: a later prepare did not run");
		}

		k_sem_reset(&connected_sem);
		rsp_count = 0U;

		err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
		TEST_ASSERT(err == 0, "Round %u: advertising failed to start (err %d)", round, err);

		start_cycles = k_cycle_get_32();

		/* After round 0, fail before a second AUX_CONNECT_RSP could come, an
		 * advertising interval later.
		 */
		timeout = (round == 0U) ? K_MSEC(WAIT_MS) : K_USEC(connected_us + 10000U);

		err = k_sem_take(&connected_sem, timeout);

		if (round == 1U) {
			TEST_ASSERT((preemptor_cancelled == 1U) && (preemptor_ran == 0U),
				    "Round 1: preempting prepare cancelled %u, run %u",
				    preemptor_cancelled, preemptor_ran);
		}

		TEST_ASSERT((err == 0) && (connected_err == 0U), "Round %u: not connected", round);

		/* The connection came with the AUX_CONNECT_RSP that the preemptor met */
		if (round == 1U) {
			TEST_ASSERT(rsp_count == 1U, "Round 1: connected after %u AUX_CONNECT_RSP",
				    rsp_count);
		}

		at_us = k_cyc_to_us_floor32(connected_cycles - start_cycles);
		if (round == 0U) {
			connected_us = at_us;
		}

		TEST_PRINT("Round %u: connected %u us after the start, %u AUX_CONNECT_RSP", round,
			   at_us, rsp_count);

		/* The central ends the simulation after the last round */
		if (round < (ROUNDS - 1U)) {
			err = k_sem_take(&disconnected_sem, K_MSEC(WAIT_MS));
			TEST_ASSERT(err == 0, "Round %u: not disconnected", round);
		}
	}

	TEST_ASSERT(preemptor_err == 0, "Preempting prepare failed (err %d)", preemptor_err);
	TEST_ASSERT(preemptor_ull.ref == 0U, "Preempting prepare left %u references",
		    preemptor_ull.ref);

	TEST_PASS("peripheral_aux_connect_preempt");
}

static const struct bst_test_instance test_aux_connect_preempt[] = {
	{
		.test_id = "central_aux_connect_preempt",
		.test_descr = "Initiates once per round, expects a connection on both sides.",
		.test_main_f = test_central_aux_connect_preempt_main,
	},
	{
		.test_id = "peripheral_aux_connect_preempt",
		.test_descr = "Advertises once per round, preempts the AUX_CONNECT_RSP in round 1.",
		.test_main_f = test_peripheral_aux_connect_preempt_main,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_aux_connect_preempt_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_aux_connect_preempt);
}
