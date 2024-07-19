/*
 * Copyright 2020 - 2021 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/classic/a2dp.h>
#include "app_connect.h"

extern void app_sdp_discover_a2dp_sink(void);
static void connected(struct bt_conn *conn, uint8_t err);
static void disconnected(struct bt_conn *conn, uint8_t reason);
static void security_changed(struct bt_conn *conn, bt_security_t level,
				 enum bt_security_err err);

struct bt_conn *default_conn;
bt_addr_t default_peer_addr;
static uint8_t default_connect_initialized;

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed,
};
extern struct bt_a2dp *default_a2dp;
static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		if (default_conn != NULL) {
			bt_conn_unref(default_conn);
			default_conn = NULL;
		}
		printk("Connection failed (err 0x%02x)\n", err);
	} else {
		if (1U == default_connect_initialized) {
			struct bt_conn_info info;

			default_connect_initialized = 0U;
			bt_conn_get_info(conn, &info);
			if (info.type == BT_CONN_TYPE_LE) {
				return;
			}

			default_conn = bt_conn_ref(conn);
			/*
			 * Do an SDP Query on Successful ACL connection complete with the
			 * required device
			 */
			if (0 == memcmp(info.br.dst, &default_peer_addr, 6U)) {
				app_sdp_discover_a2dp_sink();
			}
			printk("Connected\n");
		}
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected (reason 0x%02x)\n", reason);

	if (default_conn != conn) {
		return;
	}

	if (default_conn) {
		bt_conn_unref(default_conn);
		default_conn = NULL;
	} else {
		return;
	}
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
				 enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
		printk("Security changed: %s level %u\n", addr, level);
	} else {
		if (err == BT_SECURITY_ERR_PIN_OR_KEY_MISSING) {
			printk("\n");

			printk("___________________________________________________________\n");
			printk("The peer device seems to have lost the bonding information.\n");
			printk("Kindly delete the bonding information of the peer from the\n");
			printk("and try again.\n\n");

			printk("\n");
		}
		printk("Security failed: %s level %u err %d\n", addr, level, err);
	}
}

void app_connect(uint8_t *addr)
{
	default_connect_initialized = 1U;
	memcpy(&default_peer_addr, addr, 6U);
	default_conn = bt_conn_create_br(&default_peer_addr, BT_BR_CONN_PARAM_DEFAULT);
	if (!default_conn) {
		default_connect_initialized = 0U;
		printk("Connection failed\r\n");
	} else {
		/* unref connection obj in advance as app user */
		bt_conn_unref(default_conn);
		printk("Connection pending\r\n");
	}
}

void app_disconnect(void)
{
	if (bt_conn_disconnect(default_conn, 0x13U)) {
		printk("Disconnection failed\r\n");
	}
}

void app_connect_init(void)
{
	bt_conn_cb_register(&conn_callbacks);
}
