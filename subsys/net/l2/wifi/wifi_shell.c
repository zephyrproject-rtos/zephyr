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
				NET_EVENT_WIFI_DISCONNECT_RESULT |  \
				NET_EVENT_WIFI_TWT)

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

static bool parse_number(const struct shell *sh, long *param, char *str, int min, int max)
{
	char *endptr;
	char *str_tmp = str;
	long num = strtol(str_tmp, &endptr, 10);

	if (*endptr != '\0') {
		print(sh, SHELL_ERROR, "Invalid number: %s", str_tmp);
		return false;
	}
	if ((num) < (min) || (num) > (max)) {
		print(sh, SHELL_WARNING, "Value out of range: %s, (%d-%d)", str_tmp, min, max);
		return false;
	}
	*param = num;
	return true;
}

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

static void handle_wifi_twt_event(struct net_mgmt_event_callback *cb)
{
	const struct wifi_twt_params *resp =
		(const struct wifi_twt_params *)cb->info;

	if (resp->resp_status == WIFI_TWT_RESP_RECEIVED) {
		print(context.sh, SHELL_NORMAL, "TWT response: %s for dialog: %d and flow: %d\n",
		      wifi_twt_setup_cmd2str[resp->setup_cmd], resp->dialog_token, resp->flow_id);

		/* If accepted, then no need to print TWT params */
		if (resp->setup_cmd != WIFI_TWT_SETUP_CMD_ACCEPT) {
			print(context.sh, SHELL_NORMAL,
			      "TWT parameters: trigger: %s wake_interval: %d us, interval: %lld us\n",
			      resp->setup.trigger ? "trigger" : "no_trigger",
			      resp->setup.twt_wake_interval,
			      resp->setup.twt_interval);
		}
	} else {
		print(context.sh, SHELL_NORMAL, "TWT response timed out\n");
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
	case NET_EVENT_WIFI_TWT:
		handle_wifi_twt_event(cb);
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
		/* Defaults */
		params->security = WIFI_SECURITY_TYPE_PSK;
		params->mfp = WIFI_MFP_OPTIONAL;
		idx++;

		/* Security type (optional) */
		if (idx < argc) {
			unsigned int security = strtol(argv[idx], &endptr, 10);

			if (security <= WIFI_SECURITY_TYPE_MAX) {
				params->security = security;
			}
			idx++;

			/* MFP (optional) */
			if (idx < argc) {
				unsigned int mfp = strtol(argv[idx], &endptr, 10);

				if (mfp <= WIFI_MFP_REQUIRED) {
					params->mfp = mfp;
				}
				idx++;
			}
		}
	} else {
		params->security = WIFI_SECURITY_TYPE_NONE;
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
		shell_fprintf(sh, SHELL_NORMAL, "Beacon Interval: %d\n", status.beacon_interval);
		shell_fprintf(sh, SHELL_NORMAL, "DTIM: %d\n", status.dtim_period);
		shell_fprintf(sh, SHELL_NORMAL, "TWT: %s\n",
				status.twt_capable ? "Supported" : "Not supported");
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
	shell_fprintf(sh, SHELL_NORMAL, "Receive errors   : %u\n", data->errors.rx);
	shell_fprintf(sh, SHELL_NORMAL, "Send errors      : %u\n", data->errors.tx);
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

static int cmd_wifi_ps(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_default();
	struct wifi_ps_params params = { 0 };

	context.sh = sh;

	if (argc > 2) {
		shell_fprintf(sh, SHELL_WARNING, "Invalid number of arguments\n");
		return -ENOEXEC;
	}

	if (argc == 1) {
		struct wifi_ps_config config = { 0 };

		if (net_mgmt(NET_REQUEST_WIFI_PS_CONFIG, iface,
				&config, sizeof(config))) {
			shell_fprintf(sh, SHELL_WARNING, "Failed to get PS config\n");
			return -ENOEXEC;
		}

		shell_fprintf(sh, SHELL_NORMAL, "PS status: %s\n",
				wifi_ps2str[config.ps_params.enabled]);
		if (config.ps_params.enabled) {
			shell_fprintf(sh, SHELL_NORMAL, "PS mode: %s\n",
					wifi_ps_mode2str[config.ps_params.mode]);
		}

		shell_fprintf(sh, SHELL_NORMAL, "PS listen_interval: %d\n",
				config.ps_params.listen_interval);

		shell_fprintf(sh, SHELL_NORMAL, "PS wake up mode: %s\n",
				config.ps_params.wakeup_mode ? "Listen interval" : "DTIM");

		shell_fprintf(sh, SHELL_NORMAL, "PS timeout: %d ms\n",
				config.ps_params.timeout_ms);

		if (config.num_twt_flows == 0) {
			shell_fprintf(sh, SHELL_NORMAL, "No TWT flows\n");
		} else {
			for (int i = 0; i < config.num_twt_flows; i++) {
				shell_fprintf(sh, SHELL_NORMAL, "TWT Dialog token: %d\n",
					config.twt_flows[i].dialog_token);
				shell_fprintf(sh, SHELL_NORMAL, "TWT flow ID: %d\n",
					config.twt_flows[i].flow_id);
				shell_fprintf(sh, SHELL_NORMAL, "TWT negotiation type: %s\n",
					wifi_twt_negotiation_type2str[
					config.twt_flows[i].negotiation_type]);
				shell_fprintf(sh, SHELL_NORMAL, "TWT responder: %s\n",
					config.twt_flows[i].responder ? "true" : "false");
				shell_fprintf(sh, SHELL_NORMAL, "TWT implicit: %s\n",
					config.twt_flows[i].implicit ? "true" : "false");
				shell_fprintf(sh, SHELL_NORMAL, "TWT trigger: %s\n",
					config.twt_flows[i].trigger ? "true" : "false");
				shell_fprintf(sh, SHELL_NORMAL, "TWT announce: %s\n",
					config.twt_flows[i].announce ? "true" : "false");
				shell_fprintf(sh, SHELL_NORMAL, "TWT wake interval: %d us\n",
					config.twt_flows[i].twt_wake_interval);
				shell_fprintf(sh, SHELL_NORMAL, "TWT interval: %lld us\n",
					config.twt_flows[i].twt_interval);
				shell_fprintf(sh, SHELL_NORMAL, "========================\n");
			}
		}
		return 0;
	}

	if (!strncmp(argv[1], "on", 2)) {
		params.enabled = WIFI_PS_ENABLED;
	} else if (!strncmp(argv[1], "off", 3)) {
		params.enabled = WIFI_PS_DISABLED;
	} else {
		shell_fprintf(sh, SHELL_WARNING, "Invalid argument\n");
		return -ENOEXEC;
	}

	params.type = WIFI_PS_PARAM_STATE;

	if (net_mgmt(NET_REQUEST_WIFI_PS, iface, &params, sizeof(params))) {
		shell_fprintf(sh, SHELL_WARNING, "Power save %s failed\n",
			params.enabled ? "enable" : "disable");
		return -ENOEXEC;
	}

	shell_fprintf(sh, SHELL_NORMAL, "%s\n", wifi_ps2str[params.enabled]);

	return 0;
}

static int cmd_wifi_ps_mode(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_default();
	struct wifi_ps_params params = { 0 };

	context.sh = sh;

	if (!strncmp(argv[1], "legacy", 6)) {
		params.mode = WIFI_PS_MODE_LEGACY;
	} else if (!strncmp(argv[1], "wmm", 3)) {
		params.mode = WIFI_PS_MODE_WMM;
	} else {
		shell_fprintf(sh, SHELL_WARNING, "Invalid PS mode\n");
		return -ENOEXEC;
	}

	params.type = WIFI_PS_PARAM_MODE;

	if (net_mgmt(NET_REQUEST_WIFI_PS, iface, &params, sizeof(params))) {
		shell_fprintf(sh, SHELL_WARNING, "%s failed\n", wifi_ps_mode2str[params.mode]);
		return -ENOEXEC;
	}

	shell_fprintf(sh, SHELL_NORMAL, "%s\n", wifi_ps_mode2str[params.mode]);

	return 0;
}

static int cmd_wifi_ps_timeout(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_default();
	struct wifi_ps_params params = { 0 };
	long timeout_ms = 0;
	int err = 0;

	context.sh = sh;

	timeout_ms = shell_strtol(argv[1], 10, &err);

	if (err) {
		shell_error(sh, "Unable to parse input (err %d)", err);
		return err;
	}

	params.timeout_ms = timeout_ms;
	params.type = WIFI_PS_PARAM_MODE;

	if (net_mgmt(NET_REQUEST_WIFI_PS, iface, &params, sizeof(params))) {
		shell_fprintf(sh, SHELL_WARNING, "Setting PS timeout failed\n");
		return -ENOEXEC;
	}

	shell_fprintf(sh, SHELL_NORMAL,
		"PS timeout %d ms\n", params.timeout_ms);

	return 0;
}

static int cmd_wifi_twt_setup_quick(const struct shell *sh, size_t argc,
			      char *argv[])
{
	struct net_if *iface = net_if_get_default();
	struct wifi_twt_params params = { 0 };
	int idx = 1;

	context.sh = sh;

	if (argc != 3) {
		shell_fprintf(sh, SHELL_WARNING, "Invalid number of arguments\n");
		shell_help(sh);
		return -ENOEXEC;
	}

	/* Sensible defaults */
	params.operation = WIFI_TWT_SETUP;
	params.negotiation_type = WIFI_TWT_INDIVIDUAL;
	params.setup_cmd = WIFI_TWT_SETUP_CMD_REQUEST;
	params.dialog_token = 1;
	params.flow_id = 0;
	params.setup.responder = 0;
	params.setup.implicit = 1;
	params.setup.trigger = 1;
	params.setup.announce = 0;

	if (!parse_number(sh, (long *)&params.setup.twt_wake_interval, argv[idx++],
			  1, WIFI_MAX_TWT_WAKE_INTERVAL_US) ||
	    !parse_number(sh, (long *)&params.setup.twt_interval, argv[idx++], 1,
			  WIFI_MAX_TWT_INTERVAL_US))
		return -EINVAL;

	if (net_mgmt(NET_REQUEST_WIFI_TWT, iface, &params, sizeof(params))) {
		shell_fprintf(sh, SHELL_WARNING, "%s with %s failed, reason : %s\n",
			wifi_twt_operation2str[params.operation],
			wifi_twt_negotiation_type2str[params.negotiation_type],
			get_twt_err_code_str(params.fail_reason));

		return -ENOEXEC;
	}

	shell_fprintf(sh, SHELL_NORMAL, "TWT operation %s with dg: %d, flow_id: %d requested\n",
		wifi_twt_operation2str[params.operation],
		params.dialog_token, params.flow_id);

	return 0;
}


static int cmd_wifi_twt_setup(const struct shell *sh, size_t argc,
			      char *argv[])
{
	struct net_if *iface = net_if_get_default();
	struct wifi_twt_params params = { 0 };
	int idx = 1;
	long neg_type;
	long setup_cmd;

	context.sh = sh;

	if (argc != 11) {
		shell_fprintf(sh, SHELL_WARNING, "Invalid number of arguments\n");
		shell_help(sh);
		return -ENOEXEC;
	}

	params.operation = WIFI_TWT_SETUP;

	if (!parse_number(sh, &neg_type, argv[idx++], WIFI_TWT_INDIVIDUAL,
			  WIFI_TWT_WAKE_TBTT) ||
	    !parse_number(sh, &setup_cmd, argv[idx++], WIFI_TWT_SETUP_CMD_REQUEST,
			  WIFI_TWT_SETUP_CMD_DEMAND) ||
	    !parse_number(sh, (long *)&params.dialog_token, argv[idx++], 1, 255) ||
	    !parse_number(sh, (long *)&params.flow_id, argv[idx++], 0,
			  (WIFI_MAX_TWT_FLOWS - 1)) ||
	    !parse_number(sh, (long *)&params.setup.responder, argv[idx++], 0, 1) ||
	    !parse_number(sh, (long *)&params.setup.trigger, argv[idx++], 0, 1) ||
	    !parse_number(sh, (long *)&params.setup.implicit, argv[idx++], 0, 1) ||
	    !parse_number(sh, (long *)&params.setup.announce, argv[idx++], 0, 1) ||
	    !parse_number(sh, (long *)&params.setup.twt_wake_interval, argv[idx++], 1,
			  WIFI_MAX_TWT_WAKE_INTERVAL_US) ||
	    !parse_number(sh, (long *)&params.setup.twt_interval, argv[idx++], 1,
			  WIFI_MAX_TWT_INTERVAL_US))
		return -EINVAL;

	params.negotiation_type = neg_type;
	params.setup_cmd = setup_cmd;

	if (net_mgmt(NET_REQUEST_WIFI_TWT, iface, &params, sizeof(params))) {
		shell_fprintf(sh, SHELL_WARNING, "%s with %s failed. reason : %s\n",
			wifi_twt_operation2str[params.operation],
			wifi_twt_negotiation_type2str[params.negotiation_type],
			get_twt_err_code_str(params.fail_reason));

		return -ENOEXEC;
	}

	shell_fprintf(sh, SHELL_NORMAL, "TWT operation %s with dg: %d, flow_id: %d requested\n",
		wifi_twt_operation2str[params.operation],
		params.dialog_token, params.flow_id);

	return 0;
}

static int cmd_wifi_twt_teardown(const struct shell *sh, size_t argc,
			      char *argv[])
{
	struct net_if *iface = net_if_get_default();
	struct wifi_twt_params params = { 0 };
	long neg_type = 0;
	long setup_cmd = 0;

	context.sh = sh;
	int idx = 1;

	if (argc != 5) {
		shell_fprintf(sh, SHELL_WARNING, "Invalid number of arguments\n");
		shell_help(sh);
		return -ENOEXEC;
	}

	params.operation = WIFI_TWT_TEARDOWN;
	neg_type = params.negotiation_type;
	setup_cmd = params.setup_cmd;

	if (!parse_number(sh, &neg_type, argv[idx++], WIFI_TWT_INDIVIDUAL,
			  WIFI_TWT_WAKE_TBTT) ||
	    !parse_number(sh, &setup_cmd, argv[idx++], WIFI_TWT_SETUP_CMD_REQUEST,
			  WIFI_TWT_SETUP_CMD_DEMAND) ||
	    !parse_number(sh, (long *)&params.dialog_token, argv[idx++], 1, 255) ||
	    !parse_number(sh, (long *)&params.flow_id, argv[idx++], 0,
			  (WIFI_MAX_TWT_FLOWS - 1)))
		return -EINVAL;

	if (net_mgmt(NET_REQUEST_WIFI_TWT, iface, &params, sizeof(params))) {
		shell_fprintf(sh, SHELL_WARNING, "%s with %s failed, reason : %s\n",
			wifi_twt_operation2str[params.operation],
			wifi_twt_negotiation_type2str[params.negotiation_type],
			get_twt_err_code_str(params.fail_reason));

		return -ENOEXEC;
	}

	shell_fprintf(sh, SHELL_NORMAL, "TWT operation %s with dg: %d, flow_id: %d requested\n",
		wifi_twt_operation2str[params.operation],
		params.dialog_token, params.flow_id);

	return 0;
}

static int cmd_wifi_twt_teardown_all(const struct shell *sh, size_t argc,
			      char *argv[])
{
	struct net_if *iface = net_if_get_default();
	struct wifi_twt_params params = { 0 };

	context.sh = sh;

	params.operation = WIFI_TWT_TEARDOWN;
	params.teardown.teardown_all = 1;

	if (net_mgmt(NET_REQUEST_WIFI_TWT, iface, &params, sizeof(params))) {
		shell_fprintf(sh, SHELL_WARNING, "%s with %s failed, reason : %s\n",
			wifi_twt_operation2str[params.operation],
			wifi_twt_negotiation_type2str[params.negotiation_type],
			get_twt_err_code_str(params.fail_reason));

		return -ENOEXEC;
	}

	shell_fprintf(sh, SHELL_NORMAL, "TWT operation %s all flows\n",
		wifi_twt_operation2str[params.operation]);

	return 0;
}

static int cmd_wifi_ap_enable(const struct shell *sh, size_t argc,
			      char *argv[])
{
	struct net_if *iface = net_if_get_default();
	static struct wifi_connect_req_params cnx_params;
	int ret;

	if (__wifi_args_to_params(argc - 1, &argv[1], &cnx_params)) {
		shell_help(sh);
		return -ENOEXEC;
	}

	context.sh = sh;

	ret = net_mgmt(NET_REQUEST_WIFI_AP_ENABLE, iface, &cnx_params,
		sizeof(struct wifi_connect_req_params));
	if (ret) {
		shell_fprintf(sh, SHELL_WARNING, "AP mode enable failed: %s\n", strerror(-ret));
		return -ENOEXEC;
	}

	shell_fprintf(sh, SHELL_NORMAL, "AP mode enabled\n");

	return 0;
}

static int cmd_wifi_ap_disable(const struct shell *sh, size_t argc,
			       char *argv[])
{
	struct net_if *iface = net_if_get_default();
	int ret;

	ret = net_mgmt(NET_REQUEST_WIFI_AP_DISABLE, iface, NULL, 0);
	if (ret) {
		shell_fprintf(sh, SHELL_WARNING, "AP mode disable failed: %s\n", strerror(-ret));
		return -ENOEXEC;
	}

	shell_fprintf(sh, SHELL_NORMAL, "AP mode disabled\n");

	return 0;
}


static int cmd_wifi_reg_domain(const struct shell *sh, size_t argc,
			       char *argv[])
{
	struct net_if *iface = net_if_get_default();
	struct wifi_reg_domain regd = {0};
	int ret;

	if (argc == 1) {
		regd.oper = WIFI_MGMT_GET;
	} else if (argc >= 2 && argc <= 3) {
		regd.oper = WIFI_MGMT_SET;
		if (strlen(argv[1]) != 2) {
			shell_fprintf(sh, SHELL_WARNING,
				"Invalid reg domain: Length should be two letters/digits\n");
			return -ENOEXEC;
		}

		/* Two letter country code with special case of 00 for WORLD */
		if (((argv[1][0] < 'A' || argv[1][0] > 'Z') ||
			(argv[1][1] < 'A' || argv[1][1] > 'Z')) &&
			(argv[1][0] != '0' || argv[1][1] != '0')) {
			shell_fprintf(sh, SHELL_WARNING, "Invalid reg domain %c%c\n", argv[1][0],
				argv[1][1]);
			return -ENOEXEC;
		}
		regd.country_code[0] = argv[1][0];
		regd.country_code[1] = argv[1][1];

		if (argc == 3) {
			if (strncmp(argv[2], "-f", 2) == 0) {
				regd.force = true;
			} else {
				shell_fprintf(sh, SHELL_WARNING, "Invalid option %s\n", argv[2]);
				return -ENOEXEC;
			}
		}
	} else {
		shell_help(sh);
		return -ENOEXEC;
	}

	ret = net_mgmt(NET_REQUEST_WIFI_REG_DOMAIN, iface,
		       &regd, sizeof(regd));
	if (ret) {
		shell_fprintf(sh, SHELL_WARNING, "Cannot %s Regulatory domain: %d\n",
			regd.oper == WIFI_MGMT_GET ? "get" : "set", ret);
		return -ENOEXEC;
	}

	if (regd.oper == WIFI_MGMT_GET) {
		shell_fprintf(sh, SHELL_NORMAL, "Wi-Fi Regulatory domain is: %c%c\n",
			regd.country_code[0], regd.country_code[1]);
	} else {
		shell_fprintf(sh, SHELL_NORMAL, "Wi-Fi Regulatory domain set to: %c%c\n",
			regd.country_code[0], regd.country_code[1]);
	}

	return 0;
}

static int cmd_wifi_listen_interval(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_default();
	struct wifi_ps_params params = { 0 };
	long interval = 0;

	context.sh = sh;

	if (!parse_number(sh, &interval, argv[1],
			  WIFI_LISTEN_INTERVAL_MIN,
			  WIFI_LISTEN_INTERVAL_MAX)) {
		return -EINVAL;
	}

	params.listen_interval = interval;
	params.type = WIFI_PS_PARAM_LISTEN_INTERVAL;

	if (net_mgmt(NET_REQUEST_WIFI_PS, iface, &params, sizeof(params))) {
		shell_fprintf(sh, SHELL_WARNING, "Setting listen interval failed\n");
		return -ENOEXEC;
	}

	shell_fprintf(sh, SHELL_NORMAL,
		"Listen interval %hu\n", params.listen_interval);

	return 0;
}

static int cmd_wifi_ps_wakeup_mode(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_default();
	struct wifi_ps_params params = { 0 };

	context.sh = sh;

	if (!strncmp(argv[1], "dtim", 4)) {
		params.wakeup_mode = WIFI_PS_WAKEUP_MODE_DTIM;
	} else if (!strncmp(argv[1], "listen_interval", 15)) {
		params.wakeup_mode = WIFI_PS_WAKEUP_MODE_LISTEN_INTERVAL;
	} else {
		shell_fprintf(sh, SHELL_WARNING, "Invalid argument\n");
		shell_fprintf(sh, SHELL_INFO,
			      "Valid argument : <dtim> / <listen_interval>\n");
		return -ENOEXEC;
	}

	params.type = WIFI_PS_PARAM_WAKEUP_MODE;

	if (net_mgmt(NET_REQUEST_WIFI_PS, iface, &params, sizeof(params))) {
		shell_fprintf(sh, SHELL_WARNING, "Setting PS wake up mode to %s failed\n",
			params.wakeup_mode ? "Listen interval" : "DTIM interval");
		return -ENOEXEC;
	}

	shell_fprintf(sh, SHELL_NORMAL, "%s\n",
		      wifi_ps_wakeup_mode2str[params.wakeup_mode]);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(wifi_cmd_ap,
	SHELL_CMD(disable, NULL,
		  "Disable Access Point mode",
		  cmd_wifi_ap_disable),
	SHELL_CMD(enable, NULL, "<SSID> <SSID length> [channel] [PSK]",
		  cmd_wifi_ap_enable),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(wifi_twt_ops,
	SHELL_CMD(quick_setup, NULL, " Start a TWT flow with defaults:\n"
		"<twt_wake_interval: 1-262144us> <twt_interval: 1us-2^64us>\n",
		cmd_wifi_twt_setup_quick),
	SHELL_CMD(setup, NULL, " Start a TWT flow:\n"
		"<negotiation_type, 0: Individual, 1: Broadcast, 2: Wake TBTT>\n"
		"<setup_cmd: 0: Request, 1: Suggest, 2: Demand>\n"
		"<dialog_token: 1-255> <flow_id: 0-7> <responder: 0/1> <trigger: 0/1> <implicit:0/1> "
		"<announce: 0/1> <twt_wake_interval: 1-262144us> <twt_interval: 1us-2^64us>\n",
		cmd_wifi_twt_setup),
	SHELL_CMD(teardown, NULL, " Teardown a TWT flow:\n"
		"<negotiation_type, 0: Individual, 1: Broadcast, 2: Wake TBTT>\n"
		"<setup_cmd: 0: Request, 1: Suggest, 2: Demand>\n"
		"<dialog_token: 1-255> <flow_id: 0-7>\n",
		cmd_wifi_twt_teardown),
	SHELL_CMD(teardown_all, NULL, " Teardown all TWT flows\n",
		cmd_wifi_twt_teardown_all),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(wifi_commands,
	SHELL_CMD(ap, &wifi_cmd_ap, "Access Point mode commands", NULL),
	SHELL_CMD(connect, NULL,
		  "Connect to a Wi-Fi AP\n"
		  "\"<SSID>\"\n"
		  "<channel number (optional), 0 means all>\n"
		  "<PSK (optional: valid only for secure SSIDs)>\n"
		  "<Security type (optional: valid only for secure SSIDs)>\n"
		  "0:None, 1:PSK, 2:PSK-256, 3:SAE\n"
		  "<MFP (optional: needs security type to be specified)>\n"
		  ": 0:Disable, 1:Optional, 2:Required",
		  cmd_wifi_connect),
	SHELL_CMD(disconnect, NULL, "Disconnect from the Wi-Fi AP",
		  cmd_wifi_disconnect),
	SHELL_CMD(ps, NULL, "Configure Wi-F PS on/off, no arguments will dump config",
		  cmd_wifi_ps),
	SHELL_CMD_ARG(ps_mode,
		      NULL,
		      "<legacy>\n"
		      "<wmm>",
		      cmd_wifi_ps_mode,
		      2,
		      0),
	SHELL_CMD(scan, NULL, "Scan for Wi-Fi APs", cmd_wifi_scan),
	SHELL_CMD(statistics, NULL, "Wi-Fi interface statistics", cmd_wifi_stats),
	SHELL_CMD(status, NULL, "Status of the Wi-Fi interface", cmd_wifi_status),
	SHELL_CMD(twt, &wifi_twt_ops, "Manage TWT flows", NULL),
	SHELL_CMD(ap, &wifi_cmd_ap, "Access Point mode commands", NULL),
	SHELL_CMD(reg_domain, NULL,
		"Set or Get Wi-Fi regulatory domain\n"
		"Usage: wifi reg_domain [ISO/IEC 3166-1 alpha2] [-f]\n"
		"-f: Force to use this regulatory hint over any other regulatory hints\n"
		"Note: This may cause regulatory compliance issues, use it at your own risk.",
		cmd_wifi_reg_domain),
	SHELL_CMD_ARG(ps_timeout,
		      NULL,
		      "<val> - PS inactivity timer(in ms)",
		      cmd_wifi_ps_timeout,
		      2,
		      0),
	SHELL_CMD_ARG(ps_listen_interval,
		      NULL,
		      "<val> - Listen interval in the range of <0-65535>",
		      cmd_wifi_listen_interval,
		      2,
		      0),
	SHELL_CMD_ARG(ps_wakeup_mode,
		     NULL,
		     "<dtim> : Set PS wake up mode to DTIM interval\n"
		     "<listen_interval> : Set PS wake up mode to listen interval",
		     cmd_wifi_ps_wakeup_mode,
		     2,
		     0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(wifi, &wifi_commands, "Wi-Fi commands", NULL);

static int wifi_shell_init(void)
{

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
