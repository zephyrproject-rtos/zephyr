/*
 * Copyright (c) 2023-2025 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdint.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys_clock.h>
#include <zephyr/toolchain.h>

#include "babblekit/testcase.h"
#include "babblekit/flags.h"
#include "bstests.h"
#include "common.h"

#define ENQUEUE_COUNT 2

extern enum bst_result_t bst_result;
static struct bt_iso_chan iso_chans[CONFIG_BT_ISO_MAX_CHAN];
static struct bt_iso_chan *default_chan = &iso_chans[0];
static struct bt_iso_cig *cig;
static uint16_t seq_num;
static volatile size_t enqueue_cnt;
static uint32_t latency_ms = 10U; /* 10ms */
static uint32_t interval_us = 10U * USEC_PER_MSEC; /* 10 ms */
NET_BUF_POOL_FIXED_DEFINE(tx_pool, ENQUEUE_COUNT, BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

BUILD_ASSERT(CONFIG_BT_ISO_MAX_CHAN > 1, "CONFIG_BT_ISO_MAX_CHAN shall be at least 2");

DEFINE_FLAG_STATIC(flag_iso_connected);

static void send_data_cb(struct k_work *work)
{
	static uint8_t buf_data[CONFIG_BT_ISO_TX_MTU];
	static size_t len_to_send = 1;
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

	buf = net_buf_alloc(&tx_pool, K_FOREVER);
	net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);

	net_buf_add_mem(buf, buf_data, len_to_send);

	ret = bt_iso_chan_send(default_chan, buf, seq_num++);
	if (ret < 0) {
		printk("Failed to send ISO data (%d)\n", ret);
		net_buf_unref(buf);

		/* Reschedule for next interval */
		k_work_reschedule(k_work_delayable_from_work(work), K_USEC(interval_us));

		return;
	}

	len_to_send++;
	if (len_to_send > ARRAY_SIZE(buf_data)) {
		len_to_send = 1;
	}

	enqueue_cnt--;
	if (enqueue_cnt > 0U) {
		/* If we have more buffers available, we reschedule the workqueue item immediately
		 * to trigger another encode + TX, but without blocking this call for too long
		 */
		k_work_reschedule(k_work_delayable_from_work(work), K_NO_WAIT);
	}
}
K_WORK_DELAYABLE_DEFINE(iso_send_work, send_data_cb);

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	int err;

	err = bt_le_scan_stop();
	if (err) {
		TEST_FAIL("Failed to stop scanning (err %d)", err);

		return;
	}

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT,
				&default_conn);
	if (err) {
		TEST_FAIL("Failed to create connection (err %d)", err);

		return;
	}
}

static void iso_connected(struct bt_iso_chan *chan)
{
	const struct bt_iso_chan_path hci_path = {
		.pid = BT_ISO_DATA_PATH_HCI,
		.format = BT_HCI_CODING_FORMAT_TRANSPARENT,
	};
	int err;

	printk("ISO Channel %p connected\n", chan);

	seq_num = 0U;
	enqueue_cnt = ENQUEUE_COUNT;

	if (chan == default_chan) {
		/* Start send timer */
		k_work_schedule(&iso_send_work, K_MSEC(0));

		SET_FLAG(flag_iso_connected);
	}

	err = bt_iso_setup_data_path(chan, BT_HCI_DATAPATH_DIR_HOST_TO_CTLR, &hci_path);
	TEST_ASSERT(err == 0, "Failed to set ISO data path: %d", err);
}

static void iso_disconnected(struct bt_iso_chan *chan, uint8_t reason)
{
	int err;

	printk("ISO Channel %p disconnected (reason 0x%02x)\n", chan, reason);

	if (chan == default_chan) {
		k_work_cancel_delayable(&iso_send_work);

		UNSET_FLAG(flag_iso_connected);
	}

	err = bt_iso_remove_data_path(chan, BT_HCI_DATAPATH_DIR_HOST_TO_CTLR);
	TEST_ASSERT(err == 0, "Failed to remove ISO data path: %d", err);
}

static void sdu_sent_cb(struct bt_iso_chan *chan)
{
	int err;

	enqueue_cnt++;

	if (!IS_FLAG_SET(flag_iso_connected)) {
		/* TX has been aborted */
		return;
	}

	err = k_work_schedule(&iso_send_work, K_NO_WAIT);
	if (err < 0) {
		TEST_FAIL("Failed to schedule TX for chan %p: %d", chan, err);
	}
}

static void init(void)
{
	static struct bt_iso_chan_ops iso_ops = {
		.connected = iso_connected,
		.disconnected = iso_disconnected,
		.sent = sdu_sent_cb,
	};
	static struct bt_iso_chan_io_qos iso_tx = {
		.sdu = CONFIG_BT_ISO_TX_MTU,
		.phy = BT_GAP_LE_PHY_2M,
		.rtn = 1,
	};
	static struct bt_iso_chan_qos iso_qos = {
		.tx = &iso_tx,
		.rx = NULL,
	};
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		TEST_FAIL("Bluetooth enable failed (err %d)", err);

		return;
	}

	for (size_t i = 0U; i < ARRAY_SIZE(iso_chans); i++) {
		iso_chans[i].ops = &iso_ops;
		iso_chans[i].qos = &iso_qos;
#if defined(CONFIG_BT_SMP)
		iso_chans[i].required_sec_level = BT_SECURITY_L2;
#endif /* CONFIG_BT_SMP */
	}
}

static void set_cig_defaults(struct bt_iso_cig_param *param)
{
	param->cis_channels = &default_chan;
	param->num_cis = 1U;
	param->sca = BT_GAP_SCA_UNKNOWN;
	param->packing = BT_ISO_PACKING_SEQUENTIAL;
	param->framing = BT_ISO_FRAMING_UNFRAMED;
	param->c_to_p_latency = latency_ms;   /* ms */
	param->p_to_c_latency = latency_ms;   /* ms */
	param->c_to_p_interval = interval_us; /* us */
	param->p_to_c_interval = interval_us; /* us */

}

static void create_cig(size_t iso_channels)
{
	struct bt_iso_chan *channels[ARRAY_SIZE(iso_chans)];
	struct bt_iso_cig_param param;
	int err;

	for (size_t i = 0U; i < iso_channels; i++) {
		channels[i] = &iso_chans[i];
	}

	set_cig_defaults(&param);
	param.num_cis = iso_channels;
	param.cis_channels = channels;

	err = bt_iso_cig_create(&param, &cig);
	if (err != 0) {
		TEST_FAIL("Failed to create CIG (%d)", err);

		return;
	}
}

static int reconfigure_cig_interval(struct bt_iso_cig_param *param)
{
	int err;

	/* Test modifying CIG parameter without any CIS */
	param->num_cis = 0U;
	param->c_to_p_interval = 7500; /* us */
	param->p_to_c_interval = param->c_to_p_interval;
	err = bt_iso_cig_reconfigure(cig, param);
	if (err != 0) {
		TEST_FAIL("Failed to reconfigure CIG to new interval (%d)", err);

		return err;
	}

	err = bt_iso_cig_reconfigure(cig, param);
	if (err != 0) {
		TEST_FAIL("Failed to reconfigure CIG to same interval (%d)", err);

		return err;
	}

	/* Test modifying to different values for both intervals */
	param->c_to_p_interval = 5000; /* us */
	param->p_to_c_interval = 2500; /* us */
	err = bt_iso_cig_reconfigure(cig, param);
	if (err != 0) {
		TEST_FAIL("Failed to reconfigure CIG to new interval (%d)", err);

		return err;
	}

	return 0;
}

static int reconfigure_cig_latency(struct bt_iso_cig_param *param)
{
	int err;

	/* Test modifying CIG latency without any CIS */
	param->num_cis = 0U;
	param->c_to_p_latency = 20; /* ms */
	param->p_to_c_latency = param->c_to_p_latency;
	err = bt_iso_cig_reconfigure(cig, param);
	if (err != 0) {
		TEST_FAIL("Failed to reconfigure CIG latency (%d)", err);

		return err;
	}

	param->c_to_p_latency = 30; /* ms */
	param->p_to_c_latency = 40; /* ms */
	err = bt_iso_cig_reconfigure(cig, param);
	if (err != 0) {
		TEST_FAIL("Failed to reconfigure CIG for different latencies (%d)", err);

		return err;
	}

	return 0;
}

static void reconfigure_cig(void)
{
	struct bt_iso_chan *channels[2];
	struct bt_iso_cig_param param;
	int err;

	for (size_t i = 0U; i < ARRAY_SIZE(channels); i++) {
		channels[i] = &iso_chans[i];
	}

	set_cig_defaults(&param);

	/* Test modifying existing CIS */
	default_chan->qos->tx->rtn++;

	err = bt_iso_cig_reconfigure(cig, &param);
	if (err != 0) {
		TEST_FAIL("Failed to reconfigure CIS to new RTN (%d)", err);

		return;
	}

	/* Test modifying interval parameter */
	err = reconfigure_cig_interval(&param);
	if (err != 0) {
		return;
	}

	/* Test modifying latency parameter */
	err = reconfigure_cig_latency(&param);
	if (err != 0) {
		return;
	}

	/* Add CIS to the CIG and restore all other parameters */
	set_cig_defaults(&param);
	param.cis_channels = &channels[1];

	err = bt_iso_cig_reconfigure(cig, &param);
	if (err != 0) {
		TEST_FAIL("Failed to reconfigure CIG with new CIS and original parameters (%d)",
			  err);

		return;
	}
}

static void connect_acl(void)
{
	int err;

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	if (err != 0) {
		TEST_FAIL("Scanning failed to start (err %d)", err);

		return;
	}

	WAIT_FOR_FLAG(flag_connected);
}

static void connect_cis(void)
{
	const struct bt_iso_connect_param connect_param = {
		.acl = default_conn,
		.iso_chan = default_chan,
	};
	int err;

	err = bt_iso_chan_connect(&connect_param, 1);
	if (err) {
		TEST_FAIL("Failed to connect ISO (%d)", err);

		return;
	}

	WAIT_FOR_FLAG(flag_iso_connected);
}

static void disconnect_cis(void)
{
	int err;

	err = bt_iso_chan_disconnect(default_chan);
	if (err) {
		TEST_FAIL("Failed to disconnect ISO (err %d)", err);

		return;
	}

	WAIT_FOR_FLAG_UNSET(flag_iso_connected);
}

static void disconnect_acl(void)
{
	int err;

	err = bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	if (err) {
		TEST_FAIL("Failed to disconnect ACL (err %d)", err);

		return;
	}

	WAIT_FOR_FLAG_UNSET(flag_connected);
}

static void terminate_cig(void)
{
	int err;

	err = bt_iso_cig_terminate(cig);
	if (err != 0) {
		TEST_FAIL("Failed to terminate CIG (%d)", err);

		return;
	}

	cig = NULL;
}

static void reset_bluetooth(void)
{
	int err;

	printk("Resetting Bluetooth\n");

	err = bt_disable();
	if (err != 0) {
		TEST_FAIL("Failed to disable (%d)", err);

		return;
	}

	/* After a disable, all CIGs and BIGs are removed */
	cig = NULL;

	err = bt_enable(NULL);
	if (err != 0) {
		TEST_FAIL("Failed to re-enable (%d)", err);

		return;
	}
}

static void test_main(void)
{
	init();
	create_cig(1);
	reconfigure_cig();
	connect_acl();
	connect_cis();

	while (seq_num < 100U) {
		k_sleep(K_USEC(interval_us));
	}

	disconnect_cis();
	disconnect_acl();
	terminate_cig();

	TEST_PASS("Test passed");
}

static void test_main_disable(void)
{
	init();

	/* Setup and connect before disabling */
	create_cig(ARRAY_SIZE(iso_chans));
	connect_acl();
	connect_cis();

	/* Reset BT to see if we can set it up again */
	reset_bluetooth();

	/* Set everything up again to see if everything still works as expected */
	create_cig(ARRAY_SIZE(iso_chans));
	connect_acl();
	connect_cis();

	while (seq_num < 100U) {
		k_sleep(K_USEC(interval_us));
	}

	disconnect_cis();
	disconnect_acl();
	terminate_cig();

	TEST_PASS("Disable test passed");
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "central",
		.test_descr = "Central",
		.test_main_f = test_main,
	},
	{
		.test_id = "central_disable",
		.test_descr = "CIS central that tests bt_disable for ISO",
		.test_main_f = test_main_disable,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_main_cis_central_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}
