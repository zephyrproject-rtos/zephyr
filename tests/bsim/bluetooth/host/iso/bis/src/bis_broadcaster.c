/*
 * Copyright (c) 2024 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common.h"

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/logging/log.h>

#include "babblekit/flags.h"
#include "babblekit/sync.h"
#include "babblekit/testcase.h"

LOG_MODULE_REGISTER(bis_broadcaster, LOG_LEVEL_INF);

#define LATENCY_MS      10U                 /* 10ms */
#define SDU_INTERVAL_US 10U * USEC_PER_MSEC /* 10 ms */

extern enum bst_result_t bst_result;
static struct bt_iso_chan iso_chans[CONFIG_BT_ISO_MAX_CHAN];
static struct bt_iso_chan *default_chan = &iso_chans[0];
static uint16_t seq_num;
NET_BUF_POOL_FIXED_DEFINE(tx_pool, CONFIG_BT_ISO_TX_BUF_COUNT,
			  BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static DEFINE_FLAG(flag_iso_connected);

static void send_data_cb(struct k_work *work);
K_WORK_DELAYABLE_DEFINE(iso_send_work, send_data_cb);

static void send_data(struct bt_iso_chan *chan)
{
	static uint8_t buf_data[CONFIG_BT_ISO_TX_MTU];
	static size_t len_to_send = 1U;
	static bool data_initialized;
	struct net_buf *buf;
	int ret;

	if (!IS_FLAG_SET(flag_iso_connected)) {
		/* TX has been aborted */
		return;
	}

	if (!data_initialized) {
		for (int i = 0; i < ARRAY_SIZE(buf_data); i++) {
			buf_data[i] = (uint8_t)i;
		}

		data_initialized = true;
	}

	buf = net_buf_alloc(&tx_pool, K_NO_WAIT);
	TEST_ASSERT(buf != NULL, "Failed to allocate buffer");

	net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);

	net_buf_add_mem(buf, buf_data, len_to_send);

	ret = bt_iso_chan_send(default_chan, buf, seq_num++);
	if (ret < 0) {
		LOG_DBG("Failed to send ISO data: %d", ret);
		net_buf_unref(buf);

		/* Reschedule for next interval */
		k_work_reschedule(&iso_send_work, K_USEC(SDU_INTERVAL_US));

		return;
	}

	len_to_send++;
	if (len_to_send > ARRAY_SIZE(buf_data)) {
		len_to_send = 1;
	}
}

static void send_data_cb(struct k_work *work)
{
	const uint16_t tx_pool_cnt = tx_pool.uninit_count;

	/* Send/enqueue as many as we can */
	for (uint16_t i = 0U; i < tx_pool_cnt; i++) {
		send_data(default_chan);
	}
}

static void iso_connected_cb(struct bt_iso_chan *chan)
{
	LOG_INF("ISO Channel %p connected", chan);

	if (chan == default_chan) {
		seq_num = 0U;

		SET_FLAG(flag_iso_connected);
	}
}

static void iso_disconnected_cb(struct bt_iso_chan *chan, uint8_t reason)
{
	LOG_INF("ISO Channel %p disconnected (reason 0x%02x)", chan, reason);

	if (chan == default_chan) {
		k_work_cancel_delayable(&iso_send_work);

		UNSET_FLAG(flag_iso_connected);
	}
}

static void sdu_sent_cb(struct bt_iso_chan *chan)
{
	if (!IS_FLAG_SET(flag_iso_connected)) {
		/* TX has been aborted */
		return;
	}

	send_data(chan);
}

static void init(void)
{
	static struct bt_iso_chan_ops iso_ops = {
		.disconnected = iso_disconnected_cb,
		.connected = iso_connected_cb,
		.sent = sdu_sent_cb,
	};
	static struct bt_iso_chan_io_qos iso_tx = {
		.sdu = CONFIG_BT_ISO_TX_MTU,
		.phy = BT_GAP_LE_PHY_2M,
		.rtn = 1,
		.path = NULL,
	};
	static struct bt_iso_chan_qos iso_qos = {
		.tx = &iso_tx,
		.rx = NULL,
	};
	int err;

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth enable failed: %d", err);

	for (size_t i = 0U; i < ARRAY_SIZE(iso_chans); i++) {
		iso_chans[i].ops = &iso_ops;
		iso_chans[i].qos = &iso_qos;
	}

	bk_sync_init();
}

static void create_ext_adv(struct bt_le_ext_adv **adv)
{
	int err;

	LOG_INF("Creating extended advertising set with periodic advertising");

	/* Create a non-connectable advertising set */
	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_NCONN, NULL, adv);
	TEST_ASSERT(err == 0, "Unable to create extended advertising set: %d", err);

	/* Set periodic advertising parameters */
	err = bt_le_per_adv_set_param(*adv, BT_LE_PER_ADV_PARAM(BT_GAP_PER_ADV_FAST_INT_MIN_2,
								BT_GAP_PER_ADV_FAST_INT_MAX_2,
								BT_LE_PER_ADV_OPT_NONE));
	TEST_ASSERT(err == 0, "Failed to set periodic advertising parameters: %d", err);
}

static void start_ext_adv(struct bt_le_ext_adv *adv)
{
	int err;

	LOG_INF("Starting extended and periodic advertising");

	/* Start extended advertising */
	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	TEST_ASSERT(err == 0, "Failed to start extended advertising: %d", err);

	/* FIXME: Temporary workaround to get around an assert in the controller
	 * Open issue: https://github.com/zephyrproject-rtos/zephyr/issues/72852
	 */
	k_sleep(K_MSEC(100));

	/* Enable Periodic Advertising */
	err = bt_le_per_adv_start(adv);
	TEST_ASSERT(err == 0, "Failed to enable periodic advertising: %d", err);
}

static void create_big(struct bt_le_ext_adv *adv, size_t cnt, struct bt_iso_big **out_big)
{
	struct bt_iso_chan *channels[ARRAY_SIZE(iso_chans)];
	struct bt_iso_big_create_param param = {
		.packing = BT_ISO_PACKING_SEQUENTIAL,
		.framing = BT_ISO_FRAMING_UNFRAMED,
		.interval = SDU_INTERVAL_US,
		.bis_channels = channels,
		.latency = LATENCY_MS,
		.encryption = false,
		.num_bis = cnt,
	};
	int err;

	for (size_t i = 0U; i < cnt; i++) {
		channels[i] = &iso_chans[i];
	}

	LOG_INF("Creating BIG");

	err = bt_iso_big_create(adv, &param, out_big);
	TEST_ASSERT(err == 0, "Failed to create BIG: %d", err);

	WAIT_FOR_FLAG(flag_iso_connected);
}

static void start_tx(void)
{
	const uint16_t tx_pool_cnt = tx_pool.uninit_count;

	LOG_INF("Starting TX");

	/* Send/enqueue as many as we can */
	for (uint16_t i = 0U; i < tx_pool_cnt; i++) {
		send_data(default_chan);
	}
}

static void terminate_big(struct bt_iso_big *big)
{
	int err;

	LOG_INF("Terminating BIG");

	err = bt_iso_big_terminate(big);
	TEST_ASSERT(err == 0, "Failed to terminate BIG: %d", err);

	big = NULL;
}

static void reset_bluetooth(void)
{
	int err;

	LOG_INF("Resetting Bluetooth");

	err = bt_disable();
	TEST_ASSERT(err == 0, "Failed to disable: %d", err);

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Failed to re-enable: %d", err);
}

static void test_main(void)
{
	struct bt_le_ext_adv *adv;
	struct bt_iso_big *big;

	init();

	/* Create advertising set and BIG and start it and starting TXing */
	create_ext_adv(&adv);
	create_big(adv, 1U, &big);
	start_ext_adv(adv);
	start_tx();

	/* Wait for receiver to tell us to terminate */
	bk_sync_wait();

	terminate_big(big);
	big = NULL;

	TEST_PASS("Test passed");
}

static void test_main_disable(void)
{
	struct bt_le_ext_adv *adv;
	struct bt_iso_big *big;

	init();

	/* Create advertising set and BIG */
	create_ext_adv(&adv);
	create_big(adv, ARRAY_SIZE(iso_chans), &big);

	/* Reset BT to see if we can set it up again */
	reset_bluetooth();

	/* After a disable, all advertising sets and BIGs are removed */
	big = NULL;
	adv = NULL;

	/* Set everything up again to see if everything still works as expected */
	create_ext_adv(&adv);
	create_big(adv, ARRAY_SIZE(iso_chans), &big);
	start_ext_adv(adv);
	start_tx();

	/* Wait for receiver to tell us to terminate */
	bk_sync_wait();

	terminate_big(big);
	big = NULL;

	TEST_PASS("Disable test passed");
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "broadcaster",
		.test_descr = "Minimal BIS broadcaster that broadcast ISO data",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main,
	},
	{
		.test_id = "broadcaster_disable",
		.test_descr = "BIS broadcaster that tests bt_disable for ISO",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main_disable,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_main_bis_broadcaster_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}
