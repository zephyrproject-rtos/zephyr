/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief WiFi shell module
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_wifi_shell, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/printk.h>
#include <zephyr/init.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_event.h>

#include "net_private.h"

#define WIFI_SHELL_MODULE "wifi"

#define WIFI_SHELL_MGMT_EVENTS (NET_EVENT_WIFI_SCAN_RESULT |		\
				NET_EVENT_WIFI_SCAN_DONE |		\
				NET_EVENT_WIFI_CONNECT_RESULT |		\
				NET_EVENT_WIFI_DISCONNECT_RESULT)

static struct {
	const struct shell *sh;

	union {
		struct {

			uint8_t connecting		: 1;
			uint8_t disconnecting	: 1;
			uint8_t _unused		: 6;
		};
		uint8_t all;
	};
} context;

static uint32_t scan_result;

static struct net_mgmt_event_callback wifi_shell_mgmt_cb;

#define print(sh, level, fmt, ...)					\
	do {								\
		if (sh) {						\
			shell_fprintf(sh, level, fmt, ##__VA_ARGS__); \
		} else {						\
			printk(fmt, ##__VA_ARGS__);			\
		}							\
	} while (false)

static void handle_wifi_scan_result(struct net_mgmt_event_callback *cb)
{
	const struct wifi_scan_result *entry =
		(const struct wifi_scan_result *)cb->info;
	uint8_t mac_string_buf[sizeof("xx:xx:xx:xx:xx:xx")];

	scan_result++;

	if (scan_result == 1U) {
		print(context.sh, SHELL_NORMAL,
		      "\n%-4s | %-32s %-5s | %-13s | %-4s | %-15s | %s\n",
		      "Num", "SSID", "(len)", "Chan (Band)", "RSSI", "Security", "BSSID");
	}

	print(context.sh, SHELL_NORMAL, "%-4d | %-32s %-5u | %-4u (%-6s) | %-4d | %-15s | %s\n",
	      scan_result, entry->ssid, entry->ssid_length, entry->channel,
	      wifi_band_txt(entry->band),
	      entry->rssi,
	      wifi_security_txt(entry->security),
	      ((entry->mac_length) ?
		      net_sprint_ll_addr_buf(entry->mac, WIFI_MAC_ADDR_LEN, mac_string_buf,
					     sizeof(mac_string_buf)) : ""));
}

static void handle_wifi_scan_done(struct net_mgmt_event_callback *cb)
{
	const struct wifi_status *status =
		(const struct wifi_status *)cb->info;

	if (status->status) {
		print(context.sh, SHELL_WARNING,
		      "Scan request failed (%d)\n", status->status);
	} else {
		print(context.sh, SHELL_NORMAL, "Scan request done\n");
	}

	scan_result = 0U;
}

static void handle_wifi_connect_result(struct net_mgmt_event_callback *cb)
{
	const struct wifi_status *status =
		(const struct wifi_status *) cb->info;

	if (status->status) {
		print(context.sh, SHELL_WARNING,
		      "Connection request failed (%d)\n", status->status);
	} else {
		print(context.sh, SHELL_NORMAL, "Connected\n");
	}

	context.connecting = false;
}

static void handle_wifi_disconnect_result(struct net_mgmt_event_callback *cb)
{
	const struct wifi_status *status =
		(const struct wifi_status *) cb->info;

	if (context.disconnecting) {
		print(context.sh,
		      status->status ? SHELL_WARNING : SHELL_NORMAL,
		      "Disconnection request %s (%d)\n",
		      status->status ? "failed" : "done",
		      status->status);
		context.disconnecting = false;
	} else {
		print(context.sh, SHELL_NORMAL, "Disconnected\n");
	}
}

static void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb,
				    uint32_t mgmt_event, struct net_if *iface)
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

static int __wifi_args_to_params(size_t argc, char *argv[],
				struct wifi_connect_req_params *params)
{
	char *endptr;
	int idx = 1;

	if (argc < 1) {
		return -EINVAL;
	}

	/* SSID */
	params->ssid = argv[0];
	params->ssid_length = strlen(params->ssid);

	/* Channel (optional) */
	if ((idx < argc) && (strlen(argv[idx]) <= 3)) {
		params->channel = strtol(argv[idx], &endptr, 10);
		if (*endptr != '\0') {
			return -EINVAL;
		}

		if (params->channel == 0U) {
			params->channel = WIFI_CHANNEL_ANY;
		}

		idx++;
	} else {
		params->channel = WIFI_CHANNEL_ANY;
	}

	/* PSK (optional) */
	if (idx < argc) {
		params->psk = argv[idx];
		params->psk_length = strlen(argv[idx]);
		params->security = WIFI_SECURITY_TYPE_PSK;
		idx++;

		/* Security type (optional) */
		if (idx < argc) {
			unsigned int security = strtol(argv[idx], &endptr, 10);

			if (security <= WIFI_SECURITY_TYPE_MAX) {
				params->security = security;
			}
			idx++;
		}
	} else {
		params->security = WIFI_SECURITY_TYPE_NONE;
	}

	/* MFP (optional) */
	params->mfp = WIFI_MFP_OPTIONAL;
	if (idx < argc) {
		unsigned int mfp = strtol(argv[idx], &endptr, 10);

		if (mfp <= WIFI_MFP_REQUIRED) {
			params->mfp = mfp;
		}
		idx++;
	}

	return 0;
}

static int cmd_wifi_connect(const struct shell *sh, size_t argc,
			    char *argv[])
{
	struct net_if *iface = net_if_get_default();
	struct wifi_connect_req_params cnx_params = { 0 };

	if (__wifi_args_to_params(argc - 1, &argv[1], &cnx_params)) {
		shell_help(sh);
		return -ENOEXEC;
	}

	context.connecting = true;
	context.sh = sh;

	if (net_mgmt(NET_REQUEST_WIFI_CONNECT, iface,
		     &cnx_params, sizeof(struct wifi_connect_req_params))) {
		shell_fprintf(sh, SHELL_WARNING,
			      "Connection request failed\n");
		context.connecting = false;

		return -ENOEXEC;
	}

	shell_fprintf(sh, SHELL_NORMAL, "Connection requested\n");

	return 0;
}

static int cmd_wifi_disconnect(const struct shell *sh, size_t argc,
			       char *argv[])
{
	struct net_if *iface = net_if_get_default();
	int status;

	context.disconnecting = true;
	context.sh = sh;

	status = net_mgmt(NET_REQUEST_WIFI_DISCONNECT, iface, NULL, 0);

	if (status) {
		context.disconnecting = false;

		if (status == -EALREADY) {
			shell_fprintf(sh, SHELL_INFO,
				      "Already disconnected\n");
		} else {
			shell_fprintf(sh, SHELL_WARNING,
				      "Disconnect request failed\n");
			return -ENOEXEC;
		}
	} else {
		shell_fprintf(sh, SHELL_NORMAL,
			      "Disconnect requested\n");
	}

	return 0;
}

static int cmd_wifi_scan(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_default();

	context.sh = sh;

	if (net_mgmt(NET_REQUEST_WIFI_SCAN, iface, NULL, 0)) {
		shell_fprintf(sh, SHELL_WARNING, "Scan request failed\n");

		return -ENOEXEC;
	}

	shell_fprintf(sh, SHELL_NORMAL, "Scan requested\n");

	return 0;
}

static int cmd_wifi_status(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_default();
	struct wifi_iface_status status = { 0 };

	context.sh = sh;

	if (net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, iface, &status,
				sizeof(struct wifi_iface_status))) {
		shell_fprintf(sh, SHELL_WARNING, "Status request failed\n");

		return -ENOEXEC;
	}

	shell_fprintf(sh, SHELL_NORMAL, "Status: successful\n");
	shell_fprintf(sh, SHELL_NORMAL, "==================\n");
	shell_fprintf(sh, SHELL_NORMAL, "State: %s\n", wifi_state_txt(status.state));

	if (status.state >= WIFI_STATE_ASSOCIATED) {
		uint8_t mac_string_buf[sizeof("xx:xx:xx:xx:xx:xx")];

		shell_fprintf(sh, SHELL_NORMAL, "Interface Mode: %s\n",
				wifi_mode_txt(status.iface_mode));
		shell_fprintf(sh, SHELL_NORMAL, "Link Mode: %s\n",
				wifi_link_mode_txt(status.link_mode));
		shell_fprintf(sh, SHELL_NORMAL, "SSID: %-32s\n", status.ssid);
		shell_fprintf(sh, SHELL_NORMAL, "BSSID: %s\n",
					  net_sprint_ll_addr_buf(status.bssid,
					  WIFI_MAC_ADDR_LEN, mac_string_buf,
					  sizeof(mac_string_buf))
					 );
		shell_fprintf(sh, SHELL_NORMAL, "Band: %s\n",
				wifi_band_txt(status.band));
		shell_fprintf(sh, SHELL_NORMAL, "Channel: %d\n", status.channel);
		shell_fprintf(sh, SHELL_NORMAL, "Security: %s\n",
				wifi_security_txt(status.security));
		shell_fprintf(sh, SHELL_NORMAL, "MFP: %s\n",
				wifi_mfp_txt(status.mfp));
		shell_fprintf(sh, SHELL_NORMAL, "RSSI: %d\n", status.rssi);
	}

	return 0;
}


#if defined(CONFIG_NET_STATISTICS_WIFI) && \
					defined(CONFIG_NET_STATISTICS_USER_API)
static void print_wifi_stats(struct net_if *iface, struct net_stats_wifi *data,
			    const struct shell *sh)
{
	shell_fprintf(sh, SHELL_NORMAL, "Statistics for Wi-Fi interface %p [%d]\n", iface,
	       net_if_get_by_iface(iface));

	shell_fprintf(sh, SHELL_NORMAL, "Bytes received   : %u\n", data->bytes.received);
	shell_fprintf(sh, SHELL_NORMAL, "Bytes sent       : %u\n", data->bytes.sent);
	shell_fprintf(sh, SHELL_NORMAL, "Packets received : %u\n", data->pkts.rx);
	shell_fprintf(sh, SHELL_NORMAL, "Packets sent     : %u\n", data->pkts.tx);
	shell_fprintf(sh, SHELL_NORMAL, "Bcast received   : %u\n", data->broadcast.rx);
	shell_fprintf(sh, SHELL_NORMAL, "Bcast sent       : %u\n", data->broadcast.tx);
	shell_fprintf(sh, SHELL_NORMAL, "Mcast received   : %u\n", data->multicast.rx);
	shell_fprintf(sh, SHELL_NORMAL, "Mcast sent       : %u\n", data->multicast.tx);
	shell_fprintf(sh, SHELL_NORMAL, "Beacons received : %u\n",	data->sta_mgmt.beacons_rx);
	shell_fprintf(sh, SHELL_NORMAL, "Beacons missed   : %u\n",
				data->sta_mgmt.beacons_miss);
}
#endif /* CONFIG_NET_STATISTICS_WIFI && CONFIG_NET_STATISTICS_USER_API */

static int cmd_wifi_stats(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_STATISTICS_WIFI) && \
					defined(CONFIG_NET_STATISTICS_USER_API)
	struct net_if *iface = net_if_get_default();
	struct net_stats_wifi stats = { 0 };
	int ret;

	ret = net_mgmt(NET_REQUEST_STATS_GET_WIFI, iface,
				&stats, sizeof(stats));
	if (!ret) {
		print_wifi_stats(iface, &stats, sh);
	}
#else
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_fprintf(sh, SHELL_INFO, "Set %s to enable %s support.\n",
		"CONFIG_NET_STATISTICS_WIFI and CONFIG_NET_STATISTICS_USER_API",
		"statistics");
#endif /* CONFIG_NET_STATISTICS_WIFI && CONFIG_NET_STATISTICS_USER_API */

	return 0;
}


static int cmd_wifi_ap_enable(const struct shell *sh, size_t argc,
			      char *argv[])
{
	struct net_if *iface = net_if_get_default();
	static struct wifi_connect_req_params cnx_params;

	if (__wifi_args_to_params(argc - 1, &argv[1], &cnx_params)) {
		shell_help(sh);
		return -ENOEXEC;
	}

	context.sh = sh;

	if (net_mgmt(NET_REQUEST_WIFI_AP_ENABLE, iface,
		     &cnx_params, sizeof(struct wifi_connect_req_params))) {
		shell_fprintf(sh, SHELL_WARNING, "AP mode failed\n");
		return -ENOEXEC;
	}

	shell_fprintf(sh, SHELL_NORMAL, "AP mode enabled\n");

	return 0;
}

static int cmd_wifi_ap_disable(const struct shell *sh, size_t argc,
			       char *argv[])
{
	struct net_if *iface = net_if_get_default();

	if (net_mgmt(NET_REQUEST_WIFI_AP_DISABLE, iface, NULL, 0)) {
		shell_fprintf(sh, SHELL_WARNING, "AP mode disable failed\n");

		return -ENOEXEC;
	}

	shell_fprintf(sh, SHELL_NORMAL, "AP mode disabled\n");

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(wifi_cmd_ap,
	SHELL_CMD(enable, NULL, "<SSID> <SSID length> [channel] [PSK]",
		  cmd_wifi_ap_enable),
	SHELL_CMD(disable, NULL,
		  "Disable Access Point mode",
		  cmd_wifi_ap_disable),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(wifi_commands,
	SHELL_CMD(connect, NULL,
		  "Connect to a Wi-Fi AP"
		  "\"<SSID>\"\n<channel number (optional), "
		  "0 means all>\n"
		  "<PSK (optional: valid only for secure SSIDs)>\n"
		  "<Security type (optional: valid only for secure SSIDs)>\n"
		  "0:None, 1:PSK, 2:PSK-256, 3:SAE\n"
		  "<MFP (optional): 0:Disable, 1:Optional, 2:Required",
		  cmd_wifi_connect),
	SHELL_CMD(disconnect, NULL, "Disconnect from the Wi-Fi AP",
		  cmd_wifi_disconnect),
	SHELL_CMD(scan, NULL, "Scan for Wi-Fi APs", cmd_wifi_scan),
	SHELL_CMD(status, NULL, "Status of the Wi-Fi interface", cmd_wifi_status),
	SHELL_CMD(statistics, NULL, "Wi-Fi interface statistics", cmd_wifi_stats),
	SHELL_CMD(ap, &wifi_cmd_ap, "Access Point mode commands", NULL),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(wifi, &wifi_commands, "Wi-Fi commands", NULL);

static int wifi_shell_init(const struct device *unused)
{
	ARG_UNUSED(unused);

	context.sh = NULL;
	context.all = 0U;
	scan_result = 0U;

	net_mgmt_init_event_callback(&wifi_shell_mgmt_cb,
				     wifi_mgmt_event_handler,
				     WIFI_SHELL_MGMT_EVENTS);

	net_mgmt_add_event_callback(&wifi_shell_mgmt_cb);

	return 0;
}

SYS_INIT(wifi_shell_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
