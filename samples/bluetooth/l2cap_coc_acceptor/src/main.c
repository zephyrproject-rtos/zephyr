/*
 * Copyright (c) 2025 The Zephyr Project Contributors
 * Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/sys/printk.h>

/* doc l2cap server start */
#define PSM 0x29 /* example PSM */

static int l2cap_recv(struct bt_l2cap_chan *chan, struct net_buf *buf);
static void l2cap_connected(struct bt_l2cap_chan *chan);
static void l2cap_disconnected(struct bt_l2cap_chan *chan);

/* ops used for every channel instance */
static struct bt_l2cap_chan_ops l2cap_ops = {
	.recv       = l2cap_recv,
	.connected  = l2cap_connected,
	.disconnected = l2cap_disconnected,
};

/* Fixed array of channel instances, one per possible connection */
static struct bt_l2cap_le_chan l2cap_chans[CONFIG_BT_MAX_CONN];

static int accept_cb(struct bt_conn *conn, struct bt_l2cap_server *server,
		     struct bt_l2cap_chan **chan)
{
	uint8_t conn_index = bt_conn_index(conn);

	/* initialize the chosen entry */
	memset(&l2cap_chans[conn_index], 0, sizeof(l2cap_chans[conn_index]));
	l2cap_chans[conn_index].chan.ops = &l2cap_ops;

	*chan = &l2cap_chans[conn_index].chan;

	printk("L2CAP channel accepted, assigned chan[%d]\n", conn_index);

	return 0; /* accept */
}

static struct bt_l2cap_server server = {
	.psm = PSM,
	.sec_level = BT_SECURITY_L1,
	.accept = accept_cb,
};
/* doc l2cap server end */

static void l2cap_connected(struct bt_l2cap_chan *chan)
{
	printk("L2CAP channel connected\n");
}

static void l2cap_disconnected(struct bt_l2cap_chan *chan)
{
	printk("L2CAP channel disconnected\n");
}

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static struct k_work adv_work;

static void adv_work_handler(struct k_work *work)
{
	int err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), NULL, 0);

	if (err) {
		printk("Advertising failed to restart (err %d)\n", err);
	} else {
		printk("Advertising successfully restarted\n");
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected (reason %u)\n", reason);
	k_work_submit(&adv_work);
}

static struct bt_conn_cb conn_callbacks = {
	.disconnected = disconnected,
};

static int l2cap_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	printk("L2CAP channel received %u bytes\n", buf->len);
	/* For synchronous processing, return 0 and the stack frees buf. */
	return 0;
}

int main(void)
{
	int err = bt_enable(NULL);

	if (err) {
		printk("Bluetooth init failed: %d\n", err);
		return err;
	}

	bt_conn_cb_register(&conn_callbacks);
	k_work_init(&adv_work, adv_work_handler);

	err = bt_l2cap_server_register(&server);
	if (err) {
		printk("L2CAP server registration failed: %d\n", err);
		return err;
	}

	printk("L2CAP server registered, PSM %u\n", PSM);

	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return err;
	}

	printk("Advertising successfully started\n");
	return 0;
}
