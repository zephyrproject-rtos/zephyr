/* main.c - Bluetooth PAN Access Point sample */

/*
 * Copyright (c) 2025
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_config.h>
#include <zephyr/net/icmp.h>
#include <zephyr/net/socket.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/classic/pan.h>

#define PEER_IPV4_ADDR CONFIG_NET_CONFIG_PEER_IPV4_ADDR

BT_PAN_NET_DEVICE_DEFINE(pan_ap0, "pan_ap0", 0x02, 0x00, 0x00, 0x44, 0x00, 0x01);

static struct net_if *pan_iface;
static struct bt_pan *active_pan;
static struct k_work_delayable ping_work;

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
		.identifier = 0x1234,
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

static int pan_accept(struct bt_conn *conn, struct bt_pan **pan)
{
	ARG_UNUSED(conn);

	*pan = bt_pan_lookup(conn);
	if (*pan == NULL) {
		return -ENOMEM;
	}

	return 0;
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
	.accept = pan_accept,
	.connected = pan_connected,
	.disconnected = pan_disconnected,
};

static int pan_iface_init(void)
{
	pan_iface = bt_pan_net_get_iface(DEVICE_GET(pan_ap0));
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

	printk("Bluetooth PAN AP initialized\n");

	if (pan_iface_init() != 0) {
		return;
	}

	(void)net_config_init_app(NULL, "PAN AP network");

	bt_pan_register(BT_PAN_ROLE_NAP, &pan_cb);

	err = bt_br_set_connectable(true, NULL);
	if (err != 0) {
		printk("Set connectable failed (err %d)\n", err);
		return;
	}

	err = bt_br_set_discoverable(true, false);
	if (err != 0) {
		printk("Set discoverable failed (err %d)\n", err);
		return;
	}

	printk("Waiting for PANU connection...\n");
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
