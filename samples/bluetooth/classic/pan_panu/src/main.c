/* main.c - Bluetooth PAN User sample */

/*
 * Copyright (c) 2025
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_config.h>
#include <zephyr/net/icmp.h>
#include <zephyr/net/socket.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/assigned_numbers.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/classic/pan.h>

#define PEER_IPV4_ADDR CONFIG_NET_CONFIG_PEER_IPV4_ADDR

BT_PAN_NET_DEVICE_DEFINE(pan_panu0, "pan_panu0", 0x02, 0x00, 0x00, 0x44, 0x00, 0x02);

static struct net_if *pan_iface;
static struct bt_conn *default_conn;
static struct bt_pan *active_pan;
static struct k_work_delayable ping_work;
static struct k_work discover_work;

static struct bt_br_discovery_param br_discover = {
	.length = 10,
	.limited = false,
};

static struct bt_br_discovery_result scan_result[8];

static bool is_nap_device(const struct bt_br_discovery_result *result)
{
	uint8_t major;

	major = (uint8_t)BT_COD_MAJOR_DEVICE_CLASS(result->cod);

	return major == BT_COD_MAJOR_DEVICE_CLASS_LAN_NETWORK;
}

static enum net_verdict ping_reply(struct net_icmp_ctx *ctx, struct net_pkt *pkt,
				   struct net_icmp_ip_hdr *hdr, struct net_icmp_hdr *icmp_hdr,
				   void *user_data)
{
	ARG_UNUSED(ctx);
	ARG_UNUSED(pkt);
	ARG_UNUSED(hdr);
	ARG_UNUSED(icmp_hdr);
	ARG_UNUSED(user_data);

	printk("Ping reply received\n");
	return NET_OK;
}

static struct net_icmp_ctx ping_ctx;

static void send_ping(struct k_work *work)
{
	struct net_sockaddr_in dst = {
		.sin_family = AF_INET,
		.sin_port = 0,
	};
	struct net_icmp_ping_params params = {
		.identifier = 0x5678,
		.sequence = 1,
		.data_size = 32,
	};
	int ret;

	ARG_UNUSED(work);

	if (active_pan == NULL || bt_pan_get_state(active_pan) != BT_PAN_STATE_CONNECTED) {
		return;
	}

	ret = zsock_inet_pton(AF_INET, PEER_IPV4_ADDR, &dst.sin_addr);
	if (ret != 1) {
		printk("Invalid peer address\n");
		return;
	}

	ret = net_icmp_send_echo_request_no_wait(&ping_ctx, pan_iface,
						 (struct net_sockaddr *)&dst, &params, NULL);
	if (ret == 0) {
		printk("Ping sent to %s\n", PEER_IPV4_ADDR);
	} else {
		printk("Ping failed (%d)\n", ret);
	}

	(void)k_work_schedule(&ping_work, K_SECONDS(5));
}

static void pan_connected(struct bt_pan *pan)
{
	printk("PAN connected\n");

	active_pan = pan;
	bt_pan_net_attach(pan, pan_iface);
	(void)k_work_schedule(&ping_work, K_SECONDS(2));
}

static void pan_disconnected(struct bt_pan *pan)
{
	printk("PAN disconnected\n");

	bt_pan_net_detach(pan);
	active_pan = NULL;
	(void)k_work_cancel_delayable(&ping_work);
}

static struct bt_pan_cb pan_cb = {
	.connected = pan_connected,
	.disconnected = pan_disconnected,
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	struct bt_pan *pan;
	int ret;

	if (err != 0) {
		printk("ACL connection failed (err %u)\n", err);
		return;
	}

	printk("ACL %s\n", __func__);

	pan = bt_pan_lookup(conn);
	if (pan == NULL) {
		printk("No PAN session\n");
		return;
	}

	ret = bt_pan_connect(conn, pan);
	if (ret != 0) {
		printk("PAN connect failed (%d)\n", ret);
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("ACL %s (reason %u)\n", __func__, reason);

	if (default_conn == conn) {
		bt_conn_unref(default_conn);
		default_conn = NULL;
	}

	(void)k_work_submit(&discover_work);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static void discovery_timeout_cb(const struct bt_br_discovery_result *results, size_t count)
{
	for (size_t i = 0; i < count; i++) {
		printk("Device[%zu]: %s, rssi %d\n", i, bt_addr_str(&results[i].addr),
		       results[i].rssi);

		if (!is_nap_device(&results[i])) {
			continue;
		}

		(void)k_work_cancel(&discover_work);
		default_conn = bt_conn_create_br(&results[i].addr, BT_BR_CONN_PARAM_DEFAULT);
		if (default_conn == NULL) {
			printk("Failed to create connection\n");
		} else {
			bt_conn_unref(default_conn);
		}
		return;
	}

	(void)k_work_submit(&discover_work);
}

static void discover_work_handler(struct k_work *work)
{
	int err;

	ARG_UNUSED(work);

	err = bt_br_discovery_start(&br_discover, scan_result, ARRAY_SIZE(scan_result));
	if (err != 0) {
		printk("Discovery start failed (%d)\n", err);
	}
}

static struct bt_br_discovery_cb discovery_cb = {
	.timeout = discovery_timeout_cb,
};

static int pan_iface_init(void)
{
	pan_iface = bt_pan_net_get_iface(DEVICE_GET(pan_panu0));
	if (pan_iface == NULL) {
		printk("PAN network interface not found\n");
		return -ENODEV;
	}

	if (!net_if_flag_is_set(pan_iface, NET_IF_UP)) {
		net_if_up(pan_iface);
	}

	return 0;
}

static void bt_ready(int err)
{
	if (err != 0) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth PAN User initialized\n");

	if (pan_iface_init() != 0) {
		return;
	}

	(void)net_config_init_app(NULL, "PAN User network");

	bt_br_discovery_cb_register(&discovery_cb);
	bt_pan_register(BT_PAN_ROLE_PANU, &pan_cb);

	k_work_init(&discover_work, discover_work_handler);
	(void)k_work_submit(&discover_work);
}

int main(void)
{
	int err;

	k_work_init_delayable(&ping_work, send_ping);

	err = net_icmp_init_ctx(&ping_ctx, NET_AF_INET, NET_ICMPV4_ECHO_REPLY, 0, ping_reply);
	if (err != 0) {
		printk("ICMP init failed (%d)\n", err);
		return 0;
	}

	err = bt_enable(bt_ready);
	if (err != 0) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	return 0;
}
