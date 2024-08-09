/*
 * Copyright (c) 2024 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/logging/log.h>

#include "babblekit/flags.h"
#include "babblekit/testcase.h"

LOG_MODULE_REGISTER(broadcaster, LOG_LEVEL_INF);

static struct bt_iso_chan iso_chans[CONFIG_BT_ISO_MAX_CHAN];
static struct bt_iso_chan *default_chan = &iso_chans[0];

NET_BUF_POOL_FIXED_DEFINE(tx_pool, CONFIG_BT_ISO_TX_BUF_COUNT,
			  BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static DEFINE_FLAG(iso_connected);
static DEFINE_FLAG(sdu_sent);

static int send_data(struct bt_iso_chan *chan, bool ts)
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

	net_buf_add_le32(buf, 0xdeadbeef);
	net_buf_add_u8(buf, 0x11);

	LOG_INF("Sending SDU with%s timestamp (headroom %d)", ts ? "" : "out",
		net_buf_headroom(buf));
	LOG_HEXDUMP_INF(buf->data, buf->len, "SDU payload");

	if (ts) {
		err = bt_iso_chan_send_ts(default_chan, buf, seq++, 0x00eeeee);
	} else {
		err = bt_iso_chan_send(default_chan, buf, seq++);
	}

	return err;
}

static void iso_connected_cb(struct bt_iso_chan *chan)
{
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

static void init(void)
{
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

	create_ext_adv(&adv);
	create_big(adv, 1U, &big);
	start_ext_adv(adv);
}

void entrypoint_broadcaster(void)
{
	/* Test purpose:
	 *
	 * Verifies that we are able to send an ISO SDU that exactly fits the
	 * configured TX MTU, without any HCI fragmentation.
	 *
	 * One device:
	 * - `broadcaster`: sends two ISO SDUs
	 *
	 * Procedure:
	 * - initialize Bluetooth and a BIS
	 * - send an SDU without timestamp
	 * - send an SDU with timestamp
	 *
	 * [verdict]
	 * - no fragmentation is observed on the HCI layer
	 */
	int err;

	LOG_INF("Starting ISO HCI fragmentation test");

	init();

	/* Send an SDU without timestamp */
	err = send_data(default_chan, 0);
	TEST_ASSERT(!err, "Failed to send data w/o TS (err %d)", err);

	/* Wait until we have sent the SDU.
	 * Using linker wrapping, we verify that no fragmentation happens.
	 */
	WAIT_FOR_FLAG(sdu_sent);

	/* Send an SDU with timestamp */
	err = send_data(default_chan, 1);
	TEST_ASSERT(!err, "Failed to send data w/ TS (err %d)", err);

	/* Wait until we have sent the SDU.
	 * Using linker wrapping, we verify that no fragmentation happens.
	 */
	WAIT_FOR_FLAG(sdu_sent);

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
	LOG_HEXDUMP_DBG(buf->data, buf->len, "h->c");

	if (bt_buf_get_type(buf) == BT_BUF_ISO_OUT) {
		validate_no_iso_frag(buf);
	}

	return __real_bt_send(buf);
}
