/*
 * Copyright (c) 2025 The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/sys/byteorder.h>

#define PSM              0x29
#define SEND_INTERVAL_MS 2000
#define DATA_MTU         23

NET_BUF_POOL_FIXED_DEFINE(data_pool, 1, DATA_MTU, 8, NULL);

K_THREAD_STACK_DEFINE(send_thread_stack, 1024);
static struct k_thread send_thread_data;
static struct bt_conn *default_conn;
static volatile bool channel_connected;

static void start_scan(void);

static int client_chan_recv(struct bt_l2cap_chan *chan, struct net_buf *buf);
static void client_chan_connected(struct bt_l2cap_chan *chan);
static void client_chan_disconnected(struct bt_l2cap_chan *chan);

static struct bt_l2cap_chan_ops client_ops = {
	.recv = client_chan_recv,
	.connected = client_chan_connected,
	.disconnected = client_chan_disconnected,
};

static struct bt_l2cap_le_chan client_chan = {
	.chan.ops = &client_ops,
};

static void send_task(void *p1, void *p2, void *p3)
{
	struct bt_l2cap_chan *chan = p1;

	while (channel_connected) {
		const char *msg = "Hello from client";
		struct net_buf *buf;

		buf = net_buf_alloc(&data_pool, K_NO_WAIT);
		if (!buf) {
			printk("Failed to allocate tx buffer\n");
			k_sleep(K_MSEC(SEND_INTERVAL_MS));
			continue;
		}

		net_buf_reserve(buf, BT_L2CAP_SDU_CHAN_SEND_RESERVE);
		net_buf_add_mem(buf, msg, strlen(msg));

		if (bt_l2cap_chan_send(chan, buf) < 0) {
			printk("Failed to send L2CAP data\n");
			net_buf_unref(buf);
		} else {
			printk("Sent: %s\n", msg);
		}

		k_sleep(K_MSEC(SEND_INTERVAL_MS));
	}
}

static int client_chan_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	printk("L2CAP channel received %u bytes\n", buf->len);
	return 0;
}

static void client_chan_connected(struct bt_l2cap_chan *chan)
{
	printk("L2CAP channel connected\n");
	channel_connected = true;

	k_thread_create(&send_thread_data, send_thread_stack,
			K_THREAD_STACK_SIZEOF(send_thread_stack), send_task, chan, NULL, NULL,
			K_PRIO_PREEMPT(7), 0, K_NO_WAIT);
}

static void client_chan_disconnected(struct bt_l2cap_chan *chan)
{
	printk("L2CAP channel disconnected\n");
	channel_connected = false;
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		printk("Connection failed (err %u)\n", err);
		bt_conn_unref(conn);
		start_scan();
		return;
	}

	printk("Connected\n");
	default_conn = bt_conn_ref(conn);

	int rc = bt_l2cap_chan_connect(default_conn, &client_chan.chan, PSM);

	if (rc) {
		printk("L2CAP channel connection failed: %d\n", rc);

	} else {
		printk("L2CAP channel connection initiated\n");
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected (reason %u)\n", reason);
	if (default_conn) {
		bt_conn_unref(default_conn);
		default_conn = NULL;
	}
	channel_connected = false;
	start_scan();
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	char addr_str[BT_ADDR_LE_STR_LEN];
	int err;

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
	printk("Found device: %s (RSSI %d)\n", addr_str, rssi);

	err = bt_le_scan_stop();
	if (err) {
		printk("Failed to stop scanning: %d\n", err);
		return;
	}

	struct bt_le_conn_param *param = BT_LE_CONN_PARAM_DEFAULT;

	int rc = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, param, &default_conn);

	if (rc) {
		printk("Failed to create connection: %d\n", rc);
		start_scan();
	} else {
		printk("Connecting to %s ...\n", addr_str);
	}
}

static void start_scan(void)
{
	int err;
	struct bt_le_scan_param scan_param = {
		.type = BT_HCI_LE_SCAN_ACTIVE,
		.options = BT_LE_SCAN_OPT_NONE,
		.interval = BT_GAP_SCAN_FAST_INTERVAL,
		.window = BT_GAP_SCAN_FAST_WINDOW,
	};

	err = bt_le_scan_start(&scan_param, device_found);
	if (err) {
		printk("Starting scanning failed (err %d)\n", err);
	} else {
		printk("Scanning started\n");
	}
}

int main(void)
{
	int err = bt_enable(NULL);

	if (err) {
		printk("Bluetooth init failed: %d\n", err);
		return err;
	}
	printk("Bluetooth initialized\n");

	bt_conn_cb_register(&conn_callbacks);

	start_scan();
	return 0;
}
