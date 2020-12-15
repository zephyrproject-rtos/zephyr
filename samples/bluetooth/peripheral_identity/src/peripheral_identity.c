/* main.c - Application main entry point */

/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <sys/printk.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>

static struct k_work work_adv_start;
static uint8_t volatile conn_count;
static uint8_t id_current;

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
};

static void adv_start(struct k_work *work)
{
	struct bt_le_adv_param adv_param = {
		.id = BT_ID_DEFAULT,
		.sid = 0,
		.secondary_max_skip = 0,
		.options = (BT_LE_ADV_OPT_CONNECTABLE |
			    BT_LE_ADV_OPT_USE_NAME |
			    BT_LE_ADV_OPT_ONE_TIME),
		.interval_min = 0x0020, /* 20 ms */
		.interval_max = 0x0020, /* 20 ms */
		.peer = NULL,
	};
	size_t id_count = 0xFF;
	int err;

	bt_id_get(NULL, &id_count);
	if (id_current == id_count) {
		int id;

		id = bt_id_create(NULL, NULL);
		if (id < 0) {
			printk("Create id failed (%d)\n", id);
		} else {
			printk("New id: %d\n", id);
		}
	}

	printk("Using current id: %u\n", id_current);
	adv_param.id = id_current;

	err = bt_le_adv_start(&adv_param, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	id_current++;
	if (id_current == CONFIG_BT_MAX_CONN) {
		id_current = 0;
	}

	printk("Advertising successfully started\n");
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (err) {
		printk("Connection failed (err 0x%02x)\n", err);
		return;
	}

	conn_count++;
	if (conn_count < CONFIG_BT_MAX_CONN) {
		k_work_submit(&work_adv_start);
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Connected (%u): %s\n", conn_count, addr);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected %s (reason 0x%02x)\n", addr, reason);

	if (conn_count == CONFIG_BT_MAX_CONN) {
		k_work_submit(&work_adv_start);
	}
	conn_count--;
}

static bool le_param_req(struct bt_conn *conn, struct bt_le_conn_param *param)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("LE conn  param req: %s int (0x%04x, 0x%04x) lat %d to %d\n",
	       addr, param->interval_min, param->interval_max, param->latency,
	       param->timeout);

	return true;
}

static void le_param_updated(struct bt_conn *conn, uint16_t interval,
			     uint16_t latency, uint16_t timeout)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("LE conn param updated: %s int 0x%04x lat %d to %d\n",
	       addr, interval, latency, timeout);
}

#if defined(CONFIG_BT_SMP)
static void security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
		printk("Security changed: %s level %u\n", addr, level);
	} else {
		printk("Security failed: %s level %u err %d\n", addr, level,
		       err);
	}
}

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing cancelled: %s\n", addr);
}

static struct bt_conn_auth_cb auth_callbacks = {
	.cancel = auth_cancel,
};
#endif /* CONFIG_BT_SMP */

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
	.le_param_req = le_param_req,
	.le_param_updated = le_param_updated,
#if defined(CONFIG_BT_SMP)
	.security_changed = security_changed,
#endif /* CONFIG_BT_SMP */
};

int init_peripheral(void)
{
	size_t id_count;
	int err;

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return err;
	}

	bt_conn_cb_register(&conn_callbacks);

#if defined(CONFIG_BT_SMP)
	bt_conn_auth_cb_register(&auth_callbacks);
#endif /* CONFIG_BT_SMP */

	printk("Bluetooth initialized\n");

	k_work_init(&work_adv_start, adv_start);
	k_work_submit(&work_adv_start);

	/* wait for connection attempts on all identities */
	do {
		k_sleep(K_MSEC(100));

		id_count = 0xFF;
		bt_id_get(NULL, &id_count);
	} while (id_count != CONFIG_BT_MAX_CONN);

	/* rotate identities so reconnections are attempted in case of any
	 * disconnections
	 */
	while (1) {
		k_sleep(K_SECONDS(1));

		if (conn_count == CONFIG_BT_MAX_CONN) {
			break;
		}

		printk("Stop advertising...\n");
		err = bt_le_adv_stop();
		if (err) {
			printk("Failed to stop advertising (%d)\n", err);

			return err;
		}

		k_work_submit(&work_adv_start);
	}

	return 0;
}
