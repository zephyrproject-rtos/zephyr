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

#define TIMEOUT_ISO_RECV_MIN 1U /* seconds */

static void start_scan(void);

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	char addr_str[BT_ADDR_LE_STR_LEN];
	struct bt_conn *conn;
	int err;

	/* We're only interested in connectable events */
	if (type != BT_GAP_ADV_TYPE_ADV_IND && type != BT_GAP_ADV_TYPE_ADV_DIRECT_IND) {
		return;
	}

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
	printk("Device found: %s (RSSI %d)\n", addr_str, rssi);

	/* connect only to devices in close proximity */
	if (rssi < -70) {
		return;
	}

	if (bt_le_scan_stop()) {
		return;
	}

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN,
				BT_LE_CONN_PARAM_DEFAULT, &conn);
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

NET_BUF_POOL_FIXED_DEFINE(tx_pool, 1, BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU), 8,
			  NULL);

static uint32_t interval_us = 10U * USEC_PER_MSEC; /* 10 ms */
static struct k_work_delayable iso_send_work;

static struct bt_iso_chan iso_chan[CONFIG_BT_ISO_MAX_CHAN];
static uint16_t seq_num[CONFIG_BT_ISO_MAX_CHAN];

static K_SEM_DEFINE(sem_iso_chan_connected, 0, 1);
static K_SEM_DEFINE(sem_iso_chan_disconnected, 0, 1);
static K_SEM_DEFINE(sem_conn_disconnected, 0, 1);

static uint8_t timeout_iso_recv = TIMEOUT_ISO_RECV_MIN;

/**
 * @brief Send ISO data on timeout
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
static void iso_timer_timeout(struct k_work *work)
{
	int ret;
	static uint8_t buf_data[CONFIG_BT_ISO_TX_MTU];
	static bool data_initialized;
	struct net_buf *buf;
	static size_t len_to_send = 1;

	if (!data_initialized) {
		for (int i = 0; i < ARRAY_SIZE(buf_data); i++) {
			buf_data[i] = (uint8_t)i;
		}

		data_initialized = true;
	}

	buf = net_buf_alloc(&tx_pool, K_FOREVER);
	net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);

	net_buf_add_mem(buf, buf_data, len_to_send);

	k_work_schedule(&iso_send_work, K_USEC(interval_us));

	printk("Send ISO Data Seq Num %u len %u...\n", seq_num[0], len_to_send);

	ret = bt_iso_chan_send(&iso_chan[0], buf, seq_num[0]++,
			       BT_ISO_TIMESTAMP_NONE);
	if (ret < 0) {
		net_buf_unref(buf);
		printk("Failed to send ISO data (%d)\n", ret);

		return;
	}

	len_to_send++;
	if (len_to_send > ARRAY_SIZE(buf_data)) {
		len_to_send = 1;
	}
}

/** Print data as d_0 d_1 d_2 ... d_(n-2) d_(n-1) d_(n) to show the 3 first and 3 last octets
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
static void iso_print_data(uint8_t *data, size_t data_len)
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
				   MIN(max_octets, data_len - max_octets),
				   data_str + str_len,
				   sizeof(data_str) - str_len);
	}

	printk("\t %s\n", data_str);
}

static void iso_recv(struct bt_iso_chan *chan, const struct bt_iso_recv_info *info,
		struct net_buf *buf)
{
	printk("Incoming data channel %p flags 0x%x seq_num %u ts %u len %u\n",
	       chan, info->flags, info->seq_num, info->ts, buf->len);
	iso_print_data(buf->data, buf->len);
}

static void iso_connected(struct bt_iso_chan *chan)
{
	printk("ISO Channel %p connected\n", chan);

	seq_num[0] = 0U;

	k_sem_give(&sem_iso_chan_connected);

	/* Start send timer */
	k_work_schedule(&iso_send_work, K_MSEC(0));
}

static void iso_disconnected(struct bt_iso_chan *chan, uint8_t reason)
{
	printk("ISO Channel %p disconnected (reason 0x%02x)\n", chan, reason);

	if (reason != BT_HCI_ERR_REMOTE_USER_TERM_CONN) {
		timeout_iso_recv = TIMEOUT_ISO_RECV_MIN + 1U;
	} else {
		timeout_iso_recv = TIMEOUT_ISO_RECV_MIN;
	}

	k_work_cancel_delayable(&iso_send_work);

	k_sem_give(&sem_iso_chan_disconnected);
}

static struct bt_iso_chan_ops iso_ops = {
	.recv		= iso_recv,
	.connected	= iso_connected,
	.disconnected	= iso_disconnected,
};

static struct bt_iso_chan_io_qos iso_tx = {
	.sdu = CONFIG_BT_ISO_TX_MTU,
	.phy = BT_GAP_LE_PHY_2M,
	.rtn = 1U,
	.path = NULL,
};

static struct bt_iso_chan_io_qos iso_rx = {
	.sdu = CONFIG_BT_ISO_TX_MTU,
	.phy = BT_GAP_LE_PHY_2M,
	.rtn = 1U,
	.path = NULL,
};

static struct bt_iso_chan_qos iso_qos = {
	.tx = &iso_tx,
	.rx = &iso_rx,
};

static void iso_chan_connect(struct bt_conn *conn, void *data)
{
	struct bt_iso_connect_param connect_param[CONFIG_BT_ISO_MAX_CHAN];
	int err;

	for (int i = 0; i < CONFIG_BT_ISO_MAX_CHAN; i++) {
		connect_param[i].acl = conn;
		connect_param[i].iso_chan = &iso_chan[i];
	}

	printk("Connecting ISO Channels...\n");
	err = bt_iso_chan_connect(connect_param, ARRAY_SIZE(connect_param));
	if (err) {
		printk("Failed to connect iso (%d)\n", err);
	}
}

static void conn_disconnect(struct bt_conn *conn, void *data)
{
	int err;

	printk("%s: Disconnecting...\n", __func__);
	err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	if (err) {
		printk("Failed to disconnect (%d)\n", err);
	}
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		printk("Failed to connect to %s (%u)\n", addr, err);

		bt_conn_unref(conn);

		start_scan();

		return;
	}

	printk("Connected: %s\n", addr);

	(void)iso_chan_connect(conn, NULL);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s (reason 0x%02x)\n", addr, reason);

	bt_conn_unref(conn);

	k_sem_give(&sem_conn_disconnected);

	start_scan();
}

static bool le_param_req(struct bt_conn *conn, struct bt_le_conn_param *param)
{
	printk("%s: int (0x%04x, 0x%04x) lat %u to %u\n", __func__,
	       param->interval_min, param->interval_max, param->latency,
	       param->timeout);

	return true;
}

static void le_param_updated(struct bt_conn *conn, uint16_t interval,
			     uint16_t latency, uint16_t timeout)
{
	printk("%s: int 0x%04x lat %u to %u\n", __func__, interval,
	       latency, timeout);
}

#if defined(CONFIG_BT_SMP)
static void security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err err)
{
	printk("%s: to level %u (err %u)\n", __func__, level, err);
}
#endif

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.le_param_req = le_param_req,
	.le_param_updated = le_param_updated,
#if defined(CONFIG_BT_SMP)
	.security_changed = security_changed,
#endif
};

void main(void)
{
	int err;
	struct bt_iso_chan *channels[CONFIG_BT_ISO_MAX_CHAN];
	struct bt_iso_cig_param param;
	struct bt_iso_cig *cig;

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}


	printk("Bluetooth initialized\n");

	for (int i = 0; i < CONFIG_BT_ISO_MAX_CHAN; i++) {
		iso_chan[i].ops = &iso_ops;
		iso_chan[i].qos = &iso_qos;
#if defined(CONFIG_BT_SMP)
		iso_chan[i].required_sec_level = BT_SECURITY_L2;
#endif /* CONFIG_BT_SMP */

		channels[i] = &iso_chan[i];
	}

	param.cis_channels = channels;
	param.num_cis = ARRAY_SIZE(channels);
	param.sca = BT_GAP_SCA_UNKNOWN;
	param.packing = 0;
	param.framing = 0;
	param.latency = 10; /* ms */
	param.interval = interval_us; /* us */

	err = bt_iso_cig_create(&param, &cig);

	if (err != 0) {
		printk("Failed to create CIG (%d)\n", err);
		return;
	}

	start_scan();

	k_work_init_delayable(&iso_send_work, iso_timer_timeout);

	while (1) {
		printk("Waiting ISO Channel Connection...\n");
		err = k_sem_take(&sem_iso_chan_connected, K_FOREVER);
		if (err) {
			printk("Wait ISO channel connect failed (err %d)\n",
			       err);
			break;
		}
		printk("ISO Channel Connected.\n");

		printk("Waiting for %d seconds...\n", timeout_iso_recv);
		err = k_sem_take(&sem_iso_chan_disconnected,
				 K_SECONDS(timeout_iso_recv));
		if (err) {
#if defined(TEST_ISO_CHAN_DISCONNECT)
			printk("Disconnecting ISO Channel...\n");
			err = bt_iso_chan_disconnect(&iso_chan[0]);
			if (err) {
				printk("ISO Channel disconnect failed (err %d)\n",
				       err);
				break;
			}

			printk("Waiting ISO Channel disconnect...\n");
			err = k_sem_take(&sem_iso_chan_disconnected,
					 K_FOREVER);
			if (err) {
				printk("Disconnecting ISO Channel failed (err %d)\n",
				       err);
				break;
			}

#else /* test using bt_conn_disconnect() */
			bt_conn_foreach(BT_CONN_TYPE_LE, conn_disconnect, NULL);

			printk("Waiting ISO Channel disconnect...\n");
			err = k_sem_take(&sem_iso_chan_disconnected,
					 K_FOREVER);
			if (err) {
				printk("Disconnecting ISO Channel failed (err %d)\n",
				       err);
				break;
			}

			printk("Waiting disconnect...\n");
			err = k_sem_take(&sem_conn_disconnected, K_FOREVER);
			if (err) {
				printk("Disconnecting failed (err %d)\n",
				       err);
				break;
			}
#endif /* !TEST_ISO_CHAN_DISCONNECT */
		}

#if defined(TEST_ISO_CHAN_DISCONNECT)
		bt_conn_foreach(BT_CONN_TYPE_ISO, iso_chan_connect, NULL);
#endif /* TEST_ISO_CHAN_DISCONNECT */
	}

	printk("%s: Exit\n", __func__);
}
