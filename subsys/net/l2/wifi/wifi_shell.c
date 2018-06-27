/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief WiFi shell module
 */

#include <zephyr.h>
#include <stdio.h>
#include <stdlib.h>
#include <shell/shell.h>
#include <misc/printk.h>
#include <init.h>

#include <net/net_if.h>
#include <net/wifi_mgmt.h>
#include <net/net_event.h>

#define WIFI_SHELL_MODULE "wifi"

#define WIFI_SHELL_MGMT_EVENTS (NET_EVENT_WIFI_SCAN_RESULT |		\
				NET_EVENT_WIFI_SCAN_DONE |		\
				NET_EVENT_WIFI_CONNECT_RESULT |		\
				NET_EVENT_WIFI_DISCONNECT_RESULT)

static union {
	struct {
		u8_t connecting		: 1;
		u8_t disconnecting	: 1;
		u8_t _unused		: 6;
	};
	u8_t all;
} context;

static u32_t scan_result;

static struct net_mgmt_event_callback wifi_shell_mgmt_cb;

static void handle_wifi_scan_result(struct net_mgmt_event_callback *cb)
{
	const struct wifi_scan_result *entry =
		(const struct wifi_scan_result *)cb->info;

	scan_result++;

	if (scan_result == 1) {
		printk("\n%-4s | %-32s %-5s | %-4s | %-4s | %-5s\n",
		       "Num", "SSID", "(len)", "Chan", "RSSI", "Sec");
	}

	printk("%-4d | %-32s %-5u | %-4u | %-4d | %-5s\n",
	       scan_result, entry->ssid, entry->ssid_length,
	       entry->channel, entry->rssi,
	       (entry->security == WIFI_SECURITY_TYPE_PSK ?
		"WPA/WPA2" : "Open"));
}

static void handle_wifi_scan_done(struct net_mgmt_event_callback *cb)
{
	const struct wifi_status *status =
		(const struct wifi_status *)cb->info;

	if (status->status) {
		printk("\nScan request failed (%d)\n", status->status);
	} else {
		printk("----------\n");
		printk("Scan request done\n");
	}

	scan_result = 0;
}

static void handle_wifi_connect_result(struct net_mgmt_event_callback *cb)
{
	const struct wifi_status *status =
		(const struct wifi_status *) cb->info;

	if (status->status) {
		printk("\nConnection request failed (%d)\n", status->status);
	} else {
		printk("\nConnected\n");
	}

	context.connecting = false;
}

static void handle_wifi_disconnect_result(struct net_mgmt_event_callback *cb)
{
	const struct wifi_status *status =
		(const struct wifi_status *) cb->info;

	if (context.disconnecting) {
		printk("\nDisconnection request %s (%d)\n",
		       status->status ? "failed" : "done",
		       status->status);
		context.disconnecting = false;
	} else {
		printk("\nDisconnected\n");
	}
}

static void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb,
				    u32_t mgmt_event, struct net_if *iface)
{
	switch (mgmt_event) {
	case NET_EVENT_WIFI_SCAN_RESULT:
		handle_wifi_scan_result(cb);
		break;
	case NET_EVENT_WIFI_SCAN_DONE:
		handle_wifi_scan_done(cb);
		break;
	case NET_EVENT_WIFI_CONNECT_RESULT:
		handle_wifi_connect_result(cb);
		break;
	case NET_EVENT_WIFI_DISCONNECT_RESULT:
		handle_wifi_disconnect_result(cb);
		break;
	default:
		break;
	}
}

static int shell_cmd_connect(int argc, char *argv[])
{
	struct net_if *iface = net_if_get_default();
	static struct wifi_connect_req_params cnx_params;
	int idx = 3;

	if (argc < 3) {
		return -EINVAL;
	}

	cnx_params.ssid_length = atoi(argv[2]);
	if (cnx_params.ssid_length <= 2) {
		return -EINVAL;
	}

	cnx_params.ssid = &argv[1][1];

	argv[1][cnx_params.ssid_length + 1] = '\0';

	if ((idx < argc) && (strlen(argv[idx]) <= 2)) {
		cnx_params.channel = atoi(argv[3]);
		if (cnx_params.channel == 0) {
			cnx_params.channel = WIFI_CHANNEL_ANY;
		}

		idx++;
	} else {
		cnx_params.channel = WIFI_CHANNEL_ANY;
	}

	if (idx < argc) {
		cnx_params.psk = argv[idx];
		cnx_params.psk_length = strlen(argv[idx]);
		cnx_params.security = WIFI_SECURITY_TYPE_PSK;
	} else {
		cnx_params.security = WIFI_SECURITY_TYPE_NONE;
	}

	context.connecting = true;

	if (net_mgmt(NET_REQUEST_WIFI_CONNECT, iface,
		     &cnx_params, sizeof(struct wifi_connect_req_params))) {
		printk("Connection request failed\n");
		context.connecting = false;
	} else {
		printk("Connection requested\n");
	}

	return 0;
}

static int shell_cmd_disconnect(int argc, char *argv[])
{
	struct net_if *iface = net_if_get_default();
	int status;

	context.disconnecting = true;

	status = net_mgmt(NET_REQUEST_WIFI_DISCONNECT, iface, NULL, 0);

	if (status) {
		context.disconnecting = false;

		if (status == -EALREADY) {
			printk("Already disconnected\n");
		} else {
			printk("Disconnect request failed\n");
		}
	} else {
		printk("Disconnect requested\n");
	}

	return 0;
}

static int shell_cmd_scan(int argc, char *argv[])
{
	struct net_if *iface = net_if_get_default();

	if (net_mgmt(NET_REQUEST_WIFI_SCAN, iface, NULL, 0)) {
		printk("Scan request failed\n");
	} else {
		printk("Scan requested\n");
	}

	return 0;
}

static struct shell_cmd wifi_commands[] = {
	{ "connect",		shell_cmd_connect,
	  "\"<SSID>\" <SSID length> <channel number (optional), 0 means all> "
	  "<PSK (optional: valid only for secured SSIDs)>" },
	{ "disconnect",		shell_cmd_disconnect,
	  NULL },
	{ "scan",		shell_cmd_scan,
	  NULL },
	{ NULL, NULL, NULL },
};

static int wifi_shell_init(struct device *unused)
{
	ARG_UNUSED(unused);

	context.all = 0;
	scan_result = 0;

	net_mgmt_init_event_callback(&wifi_shell_mgmt_cb,
				     wifi_mgmt_event_handler,
				     WIFI_SHELL_MGMT_EVENTS);

	net_mgmt_add_event_callback(&wifi_shell_mgmt_cb);

	SHELL_REGISTER(WIFI_SHELL_MODULE, wifi_commands);

	return 0;
}

SYS_INIT(wifi_shell_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
