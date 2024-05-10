/*
 * Copyright (c) 2021 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/byteorder.h>

#define LATENCY_MS      10U                   /* 10.0 ms */
#define SDU_INTERVAL_US (10U * USEC_PER_MSEC) /* 10.0 ms */
#define NUM_RETRIES     1U                    /* SDU tx retries, influences ISO interval length */

static void start_scan(void);

static struct bt_conn *default_conn;
static struct bt_iso_chan iso_chan;
static uint32_t seq_num;
NET_BUF_POOL_FIXED_DEFINE(tx_pool, 1, BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static void iso_send_sdu(struct k_work *work);

static K_WORK_DEFINE(iso_send_sdu_work, iso_send_sdu);

static void iso_timer_timeout(struct k_timer *timer);

static K_TIMER_DEFINE(iso_timer, iso_timer_timeout, NULL);

/**
 * @brief Send ISO data SDU
 *
 * This will send an increasing amount of ISO data, starting from 1 octet.
 *
 * First iteration : 0x00
 * Second iteration: 0x00 0x01
 * Third iteration : 0x00 0x01 0x02
 *
 * And so on, until it wraps around the configured ISO TX MTU (CONFIG_BT_ISO_TX_MTU)
 *
 * @param work Pointer to the work structure
 */
static void iso_send_sdu(struct k_work *work)
{
	static uint8_t buf_data[CONFIG_BT_ISO_TX_MTU];
	static bool is_first_sdu = true;
	static size_t len_to_send = 1;

	struct net_buf *buf;
	int ret;

	ARG_UNUSED(work);

	if (unlikely(is_first_sdu)) {
		/* initialize data */
		for (int i = 0; i < ARRAY_SIZE(buf_data); i++) {
			buf_data[i] = (uint8_t)i;
		}

		is_first_sdu = false;
	}

	buf = net_buf_alloc(&tx_pool, K_FOREVER);
	net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);

	net_buf_add_mem(buf, buf_data, len_to_send);

	ret = bt_iso_chan_send(&iso_chan, buf, seq_num++);

	if (ret < 0) {
		printk("Failed to send ISO data (%d)\n", ret);
		net_buf_unref(buf);
	}

	len_to_send++;
	if (len_to_send > ARRAY_SIZE(buf_data)) {
		len_to_send = 1;
	}
}

static void iso_timer_timeout(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	k_work_submit(&iso_send_sdu_work);
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	char addr_str[BT_ADDR_LE_STR_LEN];
	int err;

	if (default_conn) {
		/* Already connected */
		return;
	}

	/* We're only interested in connectable events */
	if (type != BT_GAP_ADV_TYPE_ADV_IND && type != BT_GAP_ADV_TYPE_ADV_DIRECT_IND) {
		return;
	}

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
	printk("Device found: %s (RSSI %d)\n", addr_str, rssi);

	/* connect only to devices in close proximity */
	if (rssi < -50) {
		return;
	}

	if (bt_le_scan_stop()) {
		return;
	}

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT,
				&default_conn);
	if (err) {
		printk("Create conn to %s failed (%u)\n", addr_str, err);
		start_scan();
	}
}

static void start_scan(void)
{
	int err;

	/* This demo doesn't require active scan */
	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	if (err) {
		printk("Scanning failed to start (err %d)\n", err);
		return;
	}

	printk("Scanning successfully started\n");
}

static void iso_connected(struct bt_iso_chan *chan)
{
	printk("ISO Channel %p connected\n", chan);

	seq_num = 0U;

	/* Start send timer */
	k_timer_start(&iso_timer, K_NO_WAIT, K_USEC(SDU_INTERVAL_US));
}

static void iso_disconnected(struct bt_iso_chan *chan, uint8_t reason)
{
	printk("ISO Channel %p disconnected (reason 0x%02x)\n", chan, reason);
	k_timer_stop(&iso_timer);
}

static struct bt_iso_chan_ops iso_ops = {
	.connected	= iso_connected,
	.disconnected	= iso_disconnected,
};

static struct bt_iso_chan_io_qos iso_tx = {
	.sdu = CONFIG_BT_ISO_TX_MTU,
	.phy = BT_GAP_LE_PHY_2M,
	.rtn = NUM_RETRIES,
	.path = NULL,
};

static struct bt_iso_chan_qos iso_qos = {
	.tx = &iso_tx,
	.rx = NULL,
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	struct bt_iso_connect_param connect_param;
	char addr[BT_ADDR_LE_STR_LEN];
	int iso_err;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		printk("Failed to connect to %s (%u)\n", addr, err);

		bt_conn_unref(default_conn);
		default_conn = NULL;

		start_scan();
		return;
	}

	if (conn != default_conn) {
		return;
	}

	printk("Connected: %s\n", addr);

	connect_param.acl = conn;
	connect_param.iso_chan = &iso_chan;

	iso_err = bt_iso_chan_connect(&connect_param, 1);

	if (iso_err) {
		printk("Failed to connect iso (%d)\n", iso_err);
		return;
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (conn != default_conn) {
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s (reason 0x%02x)\n", addr, reason);

	bt_conn_unref(default_conn);
	default_conn = NULL;

	start_scan();
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

int main(void)
{
	struct bt_iso_chan *channels[1];
	struct bt_iso_cig_param param;
	struct bt_iso_cig *cig;
	int err;

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}


	printk("Bluetooth initialized\n");

	iso_chan.ops = &iso_ops;
	iso_chan.qos = &iso_qos;
#if defined(CONFIG_BT_SMP)
	iso_chan.required_sec_level = BT_SECURITY_L2,
#endif /* CONFIG_BT_SMP */

	channels[0] = &iso_chan;
	param.cis_channels = channels;
	param.num_cis = ARRAY_SIZE(channels);
	param.sca = BT_GAP_SCA_UNKNOWN;
	param.packing = BT_ISO_PACKING_SEQUENTIAL;
	param.framing = BT_ISO_FRAMING_UNFRAMED;
	param.c_to_p_latency = LATENCY_MS;
	param.p_to_c_latency = LATENCY_MS;
	param.c_to_p_interval = SDU_INTERVAL_US;
	param.p_to_c_interval = SDU_INTERVAL_US;

	err = bt_iso_cig_create(&param, &cig);

	if (err != 0) {
		printk("Failed to create CIG (%d)\n", err);
		return 0;
	}

	start_scan();

	return 0;
}
