/*
 * Copyright (c) 2024-2025 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "babblekit/flags.h"
#include "babblekit/sync.h"
#include "babblekit/testcase.h"

#include "common.h"

LOG_MODULE_REGISTER(bis_receiver, LOG_LEVEL_INF);

#define PA_SYNC_INTERVAL_TO_TIMEOUT_RATIO 5U /* Set the timeout relative to interval */
/* The broadcaster will send SDUs from 0 to CONFIG_BT_ISO_RX_MTU in the SDU data length. We want to
 * receive at least 2 of each size to ensure correctness
 */
#define RX_CNT_TO_PASS                    (CONFIG_BT_ISO_RX_MTU * 2)

extern enum bst_result_t bst_result;

DEFINE_FLAG_STATIC(flag_broadcaster_found);
DEFINE_FLAG_STATIC(flag_iso_connected);
DEFINE_FLAG_STATIC(flag_data_received);
DEFINE_FLAG_STATIC(flag_pa_synced);
DEFINE_FLAG_STATIC(flag_biginfo);

static struct bt_iso_chan iso_chans[CONFIG_BT_ISO_MAX_CHAN];
static struct bt_le_scan_recv_info broadcaster_info;
static bt_addr_le_t broadcaster_addr;
static uint8_t broadcaster_num_bis;

/** Log data as d_0 d_1 d_2 ... d_(n-2) d_(n-1) d_(n) to show the 3 first and 3 last octets
 *
 * Examples:
 * 01
 * 0102
 * 010203
 * 01020304
 * 0102030405
 * 010203040506
 * 010203...050607
 * 010203...060708
 * etc.
 */
static void iso_log_data(uint8_t *data, size_t data_len)
{
	/* Maximum number of octets from each end of the data */
	const uint8_t max_octets = 3;
	char data_str[35];
	size_t str_len;

	str_len = bin2hex(data, MIN(max_octets, data_len), data_str, sizeof(data_str));
	if (data_len > max_octets) {
		if (data_len > (max_octets * 2)) {
			static const char dots[] = "...";

			strcat(&data_str[str_len], dots);
			str_len += strlen(dots);
		}

		str_len += bin2hex(data + (data_len - MIN(max_octets, data_len - max_octets)),
				   MIN(max_octets, data_len - max_octets), data_str + str_len,
				   sizeof(data_str) - str_len);
	}

	LOG_DBG("\t %s", data_str);
}

static void iso_recv(struct bt_iso_chan *chan, const struct bt_iso_recv_info *info,
		     struct net_buf *buf)
{
	if (IS_FLAG_SET(flag_data_received)) {
		return;
	}

	if (info->flags & BT_ISO_FLAGS_VALID) {
		static uint16_t last_buf_len;
		static uint32_t last_ts;
		static size_t rx_cnt;

		LOG_DBG("Incoming data channel %p len %u", chan, buf->len);
		iso_log_data(buf->data, buf->len);

		if (memcmp(buf->data, mock_iso_data, buf->len) != 0) {
			TEST_FAIL("Unexpected data received");
		} else if (last_buf_len != 0U && buf->len != 1U && buf->len != last_buf_len + 1) {
			TEST_FAIL("Unexpected data length (%u) received (expected 1 or %u)",
				  buf->len, last_buf_len);
		} else if (last_ts != 0U && info->ts > last_ts + 2 * SDU_INTERVAL_US) {
			TEST_FAIL("Unexpected timestamp (%u) received (expected %u)", info->ts,
				  last_ts + SDU_INTERVAL_US);
		} else if (rx_cnt++ > RX_CNT_TO_PASS) {
			LOG_INF("Data received");
			SET_FLAG(flag_data_received);
		}

		last_buf_len = buf->len;
		last_ts = info->ts;
	}
}

static void iso_connected(struct bt_iso_chan *chan)
{
	const struct bt_iso_chan_path hci_path = {
		.pid = BT_ISO_DATA_PATH_HCI,
		.format = BT_HCI_CODING_FORMAT_TRANSPARENT,
	};
	struct bt_iso_info info;
	int err;

	LOG_INF("ISO Channel %p connected", chan);

	err = bt_iso_chan_get_info(chan, &info);
	TEST_ASSERT(err == 0, "Failed to get BIS info: %d", err);

	TEST_ASSERT(info.can_recv);
	TEST_ASSERT(!info.can_send);
	TEST_ASSERT(info.type == BT_ISO_CHAN_TYPE_SYNC_RECEIVER);
	TEST_ASSERT(IN_RANGE(info.iso_interval, BT_ISO_ISO_INTERVAL_MIN, BT_ISO_ISO_INTERVAL_MAX),
		    "Invalid ISO interval 0x%04x", info.iso_interval);
	TEST_ASSERT(IN_RANGE(info.max_subevent, BT_ISO_NSE_MIN, BT_ISO_NSE_MAX),
		    "Invalid subevent number 0x%02x", info.max_subevent);
	TEST_ASSERT(IN_RANGE(info.sync_receiver.latency, BT_HCI_LE_TRANSPORT_LATENCY_BIG_MIN,
			     BT_HCI_LE_TRANSPORT_LATENCY_BIG_MAX),
		    "Invalid transport latency 0x%06x", info.sync_receiver.latency);
	TEST_ASSERT((info.sync_receiver.pto % info.iso_interval) == 0U,
		    "PTO in ms %u shall be a multiple of the ISO interval %u",
		    info.sync_receiver.pto, info.iso_interval);
	TEST_ASSERT(IN_RANGE((info.sync_receiver.pto / info.iso_interval), BT_ISO_PTO_MIN,
			     BT_ISO_PTO_MAX),
		    "Invalid PTO 0x%x", (info.sync_receiver.pto / info.iso_interval));
	TEST_ASSERT(IN_RANGE(info.sync_receiver.bn, BT_ISO_BN_MIN, BT_ISO_BN_MAX),
		    "Invalid BN 0x%02x", info.sync_receiver.bn);
	TEST_ASSERT(IN_RANGE(info.sync_receiver.irc, BT_ISO_IRC_MIN, BT_ISO_IRC_MAX),
		    "Invalid IRC 0x%02x", info.sync_receiver.irc);
	TEST_ASSERT(info.sync_receiver.big_handle != 0xFF /* invalid BIG handle */,
		    "Invalid BIG handle 0x%02x", info.sync_receiver.big_handle);
	TEST_ASSERT(
		IN_RANGE(info.sync_receiver.bis_number, BT_ISO_BIS_INDEX_MIN, BT_ISO_BIS_INDEX_MAX),
		"Invalid BIS number 0x%02x", info.sync_receiver.bis_number);

	SET_FLAG(flag_iso_connected);

	err = bt_iso_setup_data_path(chan, BT_HCI_DATAPATH_DIR_CTLR_TO_HOST, &hci_path);
	TEST_ASSERT(err == 0, "Failed to setup ISO RX data path: %d\n", err);
}

static void iso_disconnected(struct bt_iso_chan *chan, uint8_t reason)
{
	LOG_INF("ISO Channel %p disconnected (reason 0x%02x)", chan, reason);

	UNSET_FLAG(flag_iso_connected);
}

static void broadcast_scan_recv(const struct bt_le_scan_recv_info *info, struct net_buf_simple *ad)
{
	if (IS_FLAG_SET(flag_broadcaster_found)) {
		/* no-op*/
		return;
	}

	LOG_INF("Broadcaster found");

	if (info->interval != 0U) {
		memcpy(&broadcaster_info, info, sizeof(broadcaster_info));
		bt_addr_le_copy(&broadcaster_addr, info->addr);
		SET_FLAG(flag_broadcaster_found);
	}
}

static void pa_synced_cb(struct bt_le_per_adv_sync *sync,
			 struct bt_le_per_adv_sync_synced_info *info)
{
	LOG_INF("PA synced");

	SET_FLAG(flag_pa_synced);
}

static void pa_term_cb(struct bt_le_per_adv_sync *sync,
		       const struct bt_le_per_adv_sync_term_info *info)
{
	LOG_INF("PA terminated");

	UNSET_FLAG(flag_pa_synced);
}

static void pa_biginfo_cb(struct bt_le_per_adv_sync *sync, const struct bt_iso_biginfo *biginfo)
{
	if (IS_FLAG_SET(flag_biginfo)) {
		/* no-op*/
		return;
	}

	LOG_INF("BIGInfo received");

	broadcaster_num_bis = biginfo->num_bis;
	SET_FLAG(flag_biginfo);
}

static void init(void)
{
	static struct bt_le_per_adv_sync_cb pa_sync_cbs = {
		.biginfo = pa_biginfo_cb,
		.synced = pa_synced_cb,
		.term = pa_term_cb,
	};
	static struct bt_le_scan_cb bap_scan_cb = {
		.recv = broadcast_scan_recv,
	};
	static struct bt_iso_chan_io_qos iso_rx = {
		.sdu = CONFIG_BT_ISO_TX_MTU,
	};
	static struct bt_iso_chan_ops iso_ops = {
		.recv = iso_recv,
		.connected = iso_connected,
		.disconnected = iso_disconnected,
	};
	static struct bt_iso_chan_qos iso_qos = {
		.rx = &iso_rx,
	};
	int err;

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth enable failed (err %d)", err);

	for (size_t i = 0U; i < ARRAY_SIZE(iso_chans); i++) {
		iso_chans[i].ops = &iso_ops;
		iso_chans[i].qos = &iso_qos;
	}

	bt_le_per_adv_sync_cb_register(&pa_sync_cbs);
	bt_le_scan_cb_register(&bap_scan_cb);

	bk_sync_init();
}

static uint16_t interval_to_sync_timeout(uint16_t pa_interval)
{
	uint32_t interval_us;
	uint32_t timeout;

	/* Add retries and convert to unit in 10's of ms */
	interval_us = BT_GAP_PER_ADV_INTERVAL_TO_US(pa_interval);
	timeout =
		BT_GAP_US_TO_PER_ADV_SYNC_TIMEOUT(interval_us) * PA_SYNC_INTERVAL_TO_TIMEOUT_RATIO;

	/* Enforce restraints */
	timeout = CLAMP(timeout, BT_GAP_PER_ADV_MIN_TIMEOUT, BT_GAP_PER_ADV_MAX_TIMEOUT);

	return timeout;
}

static void scan_and_sync_pa(struct bt_le_per_adv_sync **out_sync)
{
	struct bt_le_per_adv_sync_param create_params = {0};
	int err;

	LOG_INF("Starting scan");
	err = bt_le_scan_start(BT_LE_SCAN_ACTIVE, NULL);
	TEST_ASSERT(err == 0, "Failed to start scan: %d", err);

	WAIT_FOR_FLAG(flag_broadcaster_found);

	bt_addr_le_copy(&create_params.addr, &broadcaster_addr);
	create_params.options = BT_LE_PER_ADV_SYNC_OPT_FILTER_DUPLICATE;
	create_params.sid = broadcaster_info.sid;
	create_params.skip = 0U;
	create_params.timeout = interval_to_sync_timeout(broadcaster_info.interval);

	LOG_INF("Creating PA sync");
	err = bt_le_per_adv_sync_create(&create_params, out_sync);
	TEST_ASSERT(err == 0, "Failed to sync to PA: %d", err);

	WAIT_FOR_FLAG(flag_pa_synced);

	LOG_INF("Stopping scan");
	err = bt_le_scan_stop();
	TEST_ASSERT(err == 0, "Failed to stop scan: %d", err);
}

static void sync_big(struct bt_le_per_adv_sync *sync, uint8_t cnt, struct bt_iso_big **out_big)
{
	struct bt_iso_chan *bis_channels[CONFIG_BT_ISO_MAX_CHAN];
	struct bt_iso_big_sync_param param = {
		.sync_timeout = interval_to_sync_timeout(broadcaster_info.interval),
		.bis_bitfield = BIT_MASK(cnt),
		.bis_channels = bis_channels,
		.mse = BT_ISO_SYNC_MSE_MIN,
		.encryption = false,
		.num_bis = cnt,
	};
	int err;

	TEST_ASSERT(cnt <= ARRAY_SIZE(bis_channels));
	for (size_t i = 0U; i < cnt; i++) {
		bis_channels[i] = &iso_chans[i];
	}

	LOG_INF("Creating BIG sync");
	err = bt_iso_big_sync(sync, &param, out_big);
	TEST_ASSERT(err == 0, "Failed to create BIG sync: %d");

	WAIT_FOR_FLAG(flag_iso_connected);
}

static void test_main(void)
{
	struct bt_le_per_adv_sync *sync;
	struct bt_iso_big *big;

	init();
	scan_and_sync_pa(&sync);
	WAIT_FOR_FLAG(flag_biginfo);
	sync_big(sync, MIN(broadcaster_num_bis, CONFIG_BT_ISO_MAX_CHAN), &big);

	LOG_INF("Waiting for data");
	WAIT_FOR_FLAG(flag_data_received);
	bk_sync_send();

	LOG_INF("Waiting for sync lost");
	WAIT_FOR_FLAG_UNSET(flag_iso_connected);

	TEST_PASS("Test passed");
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "receiver",
		.test_descr = "receiver",
		.test_main_f = test_main,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_main_bis_receiver_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}
