/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief WiFi shell module
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_wifi_shell, LOG_LEVEL_INF);

#include <zephyr.h>
#include <stdio.h>
#include <stdlib.h>
#include <shell/shell.h>
#include <shell/shell_uart.h>
#include <sys/printk.h>
#include <init.h>

#include <net/net_if.h>
#include <net/wifi_mgmt.h>
#include <net/net_event.h>

#define WIFI_SHELL_MODULE "wifi"

#define WIFI_SHELL_MGMT_EVENTS (NET_EVENT_WIFI_SCAN_RESULT |		\
				NET_EVENT_WIFI_SCAN_DONE |		\
				NET_EVENT_WIFI_CONNECT_RESULT |		\
				NET_EVENT_WIFI_DISCONNECT_RESULT)

static struct {
	const struct shell *shell;

	union {
		struct {

			u8_t connecting		: 1;
			u8_t disconnecting	: 1;
			u8_t _unused		: 6;
		};
		u8_t all;
	};
} context;

static u32_t scan_result;

static struct net_mgmt_event_callback wifi_shell_mgmt_cb;

#define print(shell, level, fmt, ...)					\
	do {								\
		if (shell) {						\
			shell_fprintf(shell, level, fmt, ##__VA_ARGS__); \
		} else {						\
			printk(fmt, ##__VA_ARGS__);			\
		}							\
	} while (false)

static void handle_wifi_scan_result(struct net_mgmt_event_callback *cb)
{
	const struct wifi_scan_result *entry =
		(const struct wifi_scan_result *)cb->info;

	scan_result++;

	if (scan_result == 1U) {
		print(context.shell, SHELL_NORMAL,
		      "%-4s | %-32s %-5s | %-4s | %-4s | %-5s\n",
		      "Num", "SSID", "(len)", "Chan", "RSSI", "Sec");
	}

	print(context.shell, SHELL_NORMAL,
	      "%-4d | %-32s %-5u | %-4u | %-4d | %-5s\n",
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
		print(context.shell, SHELL_WARNING,
		      "Scan request failed (%d)\n", status->status);
	} else {
		print(context.shell, SHELL_NORMAL, "Scan request done\n");
	}

	scan_result = 0U;
}

static void handle_wifi_connect_result(struct net_mgmt_event_callback *cb)
{
	const struct wifi_status *status =
		(const struct wifi_status *) cb->info;

	if (status->status) {
		print(context.shell, SHELL_WARNING,
		      "Connection request failed (%d)\n", status->status);
	} else {
		print(context.shell, SHELL_NORMAL, "Connected\n");
	}

	context.connecting = false;
}

static void handle_wifi_disconnect_result(struct net_mgmt_event_callback *cb)
{
	const struct wifi_status *status =
		(const struct wifi_status *) cb->info;

	if (context.disconnecting) {
		print(context.shell,
		      status->status ? SHELL_WARNING : SHELL_NORMAL,
		      "Disconnection request %s (%d)\n",
		      status->status ? "failed" : "done",
		      status->status);
		context.disconnecting = false;
	} else {
		print(context.shell, SHELL_NORMAL, "Disconnected\n");
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

static int __wifi_args_to_params(size_t argc, char *argv[],
				struct wifi_connect_req_params *params)
{
	char *endptr;
	int idx = 2;

	if (argc < 2) {
		return -EINVAL;
	}

	/* SSID */
	params->ssid = argv[0];

	/* SSID length */
	params->ssid_length = strtol(argv[1], &endptr, 10);
	if (*endptr != '\0' || params->ssid_length <= 2U) {
		return -EINVAL;
	}

	/* Channel (optional) */
	if ((idx < argc) && (strlen(argv[idx]) <= 2)) {
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
	} else {
		params->security = WIFI_SECURITY_TYPE_NONE;
	}

	return 0;
}

static int cmd_wifi_connect(const struct shell *shell, size_t argc,
			    char *argv[])
{
	struct net_if *iface = net_if_get_default();
	static struct wifi_connect_req_params cnx_params;

	if (__wifi_args_to_params(argc - 1, &argv[1], &cnx_params)) {
		shell_help(shell);
		return -ENOEXEC;
	}

	context.connecting = true;
	context.shell = shell;

	if (net_mgmt(NET_REQUEST_WIFI_CONNECT, iface,
		     &cnx_params, sizeof(struct wifi_connect_req_params))) {
		shell_fprintf(shell, SHELL_WARNING,
			      "Connection request failed\n");
		context.connecting = false;

		return -ENOEXEC;
	} else {
		shell_fprintf(shell, SHELL_NORMAL,
			      "Connection requested\n");
	}

	return 0;
}

static int cmd_wifi_disconnect(const struct shell *shell, size_t argc,
			       char *argv[])
{
	struct net_if *iface = net_if_get_default();
	int status;

	context.disconnecting = true;
	context.shell = shell;

	status = net_mgmt(NET_REQUEST_WIFI_DISCONNECT, iface, NULL, 0);

	if (status) {
		context.disconnecting = false;

		if (status == -EALREADY) {
			shell_fprintf(shell, SHELL_INFO,
				      "Already disconnected\n");
		} else {
			shell_fprintf(shell, SHELL_WARNING,
				      "Disconnect request failed\n");
			return -ENOEXEC;
		}
	} else {
		shell_fprintf(shell, SHELL_NORMAL,
			      "Disconnect requested\n");
	}

	return 0;
}

static int cmd_wifi_scan(const struct shell *shell, size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_default();

	if (net_mgmt(NET_REQUEST_WIFI_SCAN, iface, NULL, 0)) {
		shell_fprintf(shell, SHELL_WARNING, "Scan request failed\n");

		return -ENOEXEC;
	} else {
		shell_fprintf(shell, SHELL_NORMAL, "Scan requested\n");
	}

	return 0;
}

static int cmd_wifi_ap_enable(const struct shell *shell, size_t argc,
			      char *argv[])
{
	struct net_if *iface = net_if_get_default();
	static struct wifi_connect_req_params cnx_params;

	if (__wifi_args_to_params(argc - 1, &argv[1], &cnx_params)) {
		shell_help(shell);
		return -ENOEXEC;
	}

	context.shell = shell;

	if (net_mgmt(NET_REQUEST_WIFI_AP_ENABLE, iface,
		     &cnx_params, sizeof(struct wifi_connect_req_params))) {
		shell_fprintf(shell, SHELL_WARNING, "AP mode failed\n");
		return -ENOEXEC;
	} else {
		shell_fprintf(shell, SHELL_NORMAL, "AP mode enabled\n");
	}

	return 0;
}

static int cmd_wifi_ap_disable(const struct shell *shell, size_t argc,
			       char *argv[])
{
	struct net_if *iface = net_if_get_default();

	if (net_mgmt(NET_REQUEST_WIFI_AP_DISABLE, iface, NULL, 0)) {
		shell_fprintf(shell, SHELL_WARNING, "AP mode disable failed\n");

		return -ENOEXEC;
	} else {
		shell_fprintf(shell, SHELL_NORMAL, "AP mode disabled\n");
	}

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
		  "\"<SSID>\"\n<SSID length>\n<channel number (optional), "
		  "0 means all>\n"
		  "<PSK (optional: valid only for secured SSIDs)>",
		  cmd_wifi_connect),
	SHELL_CMD(disconnect, NULL, "Disconnect from Wifi AP",
		  cmd_wifi_disconnect),
	SHELL_CMD(scan, NULL, "Scan Wifi AP", cmd_wifi_scan),
	SHELL_CMD(ap, &wifi_cmd_ap, "Access Point mode commands", NULL),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(wifi, &wifi_commands, "Wifi commands", NULL);

static int wifi_shell_init(struct device *unused)
{
	ARG_UNUSED(unused);

	context.shell = NULL;
	context.all = 0U;
	scan_result = 0U;

	net_mgmt_init_event_callback(&wifi_shell_mgmt_cb,
				     wifi_mgmt_event_handler,
				     WIFI_SHELL_MGMT_EVENTS);

	net_mgmt_add_event_callback(&wifi_shell_mgmt_cb);

	return 0;
}

SYS_INIT(wifi_shell_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
