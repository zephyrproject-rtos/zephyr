/*
 * Copyright (c) 2024-2025 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include "babblekit/flags.h"
#include "babblekit/testcase.h"

LOG_MODULE_REGISTER(broadcaster, LOG_LEVEL_INF);

static struct bt_iso_chan iso_chans[CONFIG_BT_ISO_MAX_CHAN];
static struct bt_iso_chan *default_chan = &iso_chans[0];

NET_BUF_POOL_FIXED_DEFINE(tx_pool, CONFIG_BT_ISO_TX_BUF_COUNT,
			  BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

DEFINE_FLAG_STATIC(iso_connected);
DEFINE_FLAG_STATIC(first_frag);
DEFINE_FLAG_STATIC(sdu_sent);

extern void bt_conn_suspend_tx(bool suspend);
extern void bt_testing_set_iso_mtu(uint16_t mtu);

static int send_data(struct bt_iso_chan *chan)
{
	static uint16_t seq;
	struct net_buf *buf;
	int err;

	if (!IS_FLAG_SET(iso_connected)) {
		/* TX has been aborted */
		return -ENOTCONN;
	}

	buf = net_buf_alloc(&tx_pool, K_NO_WAIT);
	TEST_ASSERT(buf != NULL, "Failed to allocate buffer");

	net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);

	net_buf_add(buf, 40);

	LOG_INF("Sending SDU (headroom %d)", net_buf_headroom(buf));
	LOG_HEXDUMP_INF(buf->data, buf->len, "SDU payload");

	err = bt_iso_chan_send(default_chan, buf, seq++);

	return err;
}

static void iso_connected_cb(struct bt_iso_chan *chan)
{
	const struct bt_iso_chan_path hci_path = {
		.pid = BT_ISO_DATA_PATH_HCI,
		.format = BT_HCI_CODING_FORMAT_TRANSPARENT,
	};
	int err;

	err = bt_iso_setup_data_path(chan, BT_HCI_DATAPATH_DIR_HOST_TO_CTLR, &hci_path);
	TEST_ASSERT(err == 0, "Unable to setup ISO TX path: %d", err);

	LOG_INF("ISO Channel %p connected", chan);

	SET_FLAG(iso_connected);
}

static void iso_disconnected_cb(struct bt_iso_chan *chan, uint8_t reason)
{
	LOG_INF("ISO Channel %p disconnected (reason 0x%02x)", chan, reason);

	UNSET_FLAG(iso_connected);
}

static void sdu_sent_cb(struct bt_iso_chan *chan)
{
	SET_FLAG(sdu_sent);
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
	const uint16_t latency_ms = 10U;
	const uint16_t sdu_interval_us = 10U * USEC_PER_MSEC;

	struct bt_iso_chan *channels[ARRAY_SIZE(iso_chans)];
	struct bt_iso_big_create_param param = {
		.packing = BT_ISO_PACKING_SEQUENTIAL,
		.framing = BT_ISO_FRAMING_UNFRAMED,
		.interval = sdu_interval_us,
		.bis_channels = channels,
		.latency = latency_ms,
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

	WAIT_FOR_FLAG(iso_connected);
}

struct bt_le_ext_adv *adv;
struct bt_iso_big *big;

static struct bt_iso_chan_ops iso_ops = {
	.disconnected = iso_disconnected_cb,
	.connected = iso_connected_cb,
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

static void init(void)
{
	int err;

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth enable failed: %d", err);
}

static void connect_iso(void)
{
	bt_testing_set_iso_mtu(10);

	for (size_t i = 0U; i < ARRAY_SIZE(iso_chans); i++) {
		iso_chans[i].ops = &iso_ops;
		iso_chans[i].qos = &iso_qos;
	}

	create_ext_adv(&adv);
	create_big(adv, 1U, &big);
	start_ext_adv(adv);
}

static void disconnect_iso(void)
{
	int err;

	err = bt_iso_big_terminate(big);
	TEST_ASSERT(err == 0, "bt_iso_big_terminate failed (%d)", err);
	err = bt_le_per_adv_stop(adv);
	TEST_ASSERT(err == 0, "bt_le_per_adv_stop failed (%d)", err);
	k_msleep(100);
	err = bt_le_ext_adv_stop(adv);
	TEST_ASSERT(err == 0, "bt_le_ext_adv_stop failed (%d)", err);
	k_msleep(100);
	err = bt_le_ext_adv_delete(adv);
	TEST_ASSERT(err == 0, "bt_le_ext_adv_delete failed (%d)", err);

	big = NULL;
	adv = NULL;
}

void entrypoint_broadcaster(void)
{
	/* Test purpose:
	 *
	 * Verifies that we are not leaking buffers when getting disconnected
	 * while sending fragmented ISO SDU.
	 *
	 * One device:
	 * - `broadcaster`: sends fragmented ISO SDUs
	 *
	 * Procedure:
	 * - initialize Bluetooth and a BIS
	 * - send a fragmented SDU
	 * - disconnect when the first fragment is sent
	 * - repeat TEST_ITERATIONS time
	 *
	 * [verdict]
	 * - no buffer is leaked and repeating the operation success
	 */
	int err;
	uint8_t TEST_ITERATIONS = 4;

	LOG_INF("Starting ISO HCI fragmentation test 2");

	init();

	for (size_t i = 0; i < TEST_ITERATIONS; i++) {
		connect_iso();

		/* Send an SDU */
		err = send_data(default_chan);
		TEST_ASSERT(!err, "Failed to send data w/o TS (err %d)", err);

		/* Wait until we have sent the first SDU fragment. */
		WAIT_FOR_FLAG(first_frag);

		disconnect_iso();
		bt_conn_suspend_tx(false);

		k_msleep(1000);
	}

	TEST_PASS_AND_EXIT("Test passed");
}

void validate_no_iso_frag(struct net_buf *buf)
{
	struct bt_hci_iso_hdr *hci_hdr = (void *)buf->data;

	uint16_t handle = sys_le16_to_cpu(hci_hdr->handle);
	uint8_t flags = bt_iso_flags(handle);
	uint8_t pb_flag = bt_iso_flags_pb(flags);

	TEST_ASSERT(pb_flag == BT_ISO_SINGLE, "Packet was fragmented");
}

int __real_bt_send(struct net_buf *buf);

int __wrap_bt_send(struct net_buf *buf)
{
	struct bt_hci_iso_hdr *hci_hdr = (void *)buf->data;

	if (bt_buf_get_type(buf) == BT_BUF_ISO_OUT) {
		uint16_t handle = sys_le16_to_cpu(hci_hdr->handle);
		uint8_t flags = bt_iso_flags(handle);
		uint8_t pb_flag = bt_iso_flags_pb(flags);

		if (pb_flag == BT_ISO_START) {
			SET_FLAG(first_frag);
			bt_conn_suspend_tx(true);
		}
	}

	return __real_bt_send(buf);
}
