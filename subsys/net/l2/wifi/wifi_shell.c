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
#include <string.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/printk.h>
#include <zephyr/init.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/wifi_utils.h>
#include <zephyr/posix/unistd.h>

#include "net_private.h"

#define WIFI_SHELL_MODULE "wifi"

#ifdef CONFIG_WIFI_MGMT_RAW_SCAN_RESULTS_ONLY
#define WIFI_SHELL_MGMT_EVENTS (NET_EVENT_WIFI_RAW_SCAN_RESULT |        \
				NET_EVENT_WIFI_SCAN_DONE |              \
				NET_EVENT_WIFI_CONNECT_RESULT |         \
				NET_EVENT_WIFI_DISCONNECT_RESULT |  \
				NET_EVENT_WIFI_TWT)
#else
#define WIFI_SHELL_MGMT_EVENTS (NET_EVENT_WIFI_SCAN_RESULT |		\
				NET_EVENT_WIFI_SCAN_DONE |		\
				NET_EVENT_WIFI_CONNECT_RESULT |		\
				NET_EVENT_WIFI_DISCONNECT_RESULT |  \
				NET_EVENT_WIFI_TWT |		\
				NET_EVENT_WIFI_RAW_SCAN_RESULT)
#endif /* CONFIG_WIFI_MGMT_RAW_SCAN_RESULTS_ONLY */

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

static bool parse_number(const struct shell *sh, long *param, char *str, long min, long max)
{
	char *endptr;
	char *str_tmp = str;
	long num = 0;

	if ((str_tmp[0] == '0') && (str_tmp[1] == 'x')) {
		/* Hexadecimal numbers take base 0 in strtol */
		num = strtol(str_tmp, &endptr, 0);
	} else {
		num = strtol(str_tmp, &endptr, 10);
	}

	if (*endptr != '\0') {
		print(sh, SHELL_ERROR, "Invalid number: %s", str_tmp);
		return false;
	}

	if ((num) < (min) || (num) > (max)) {
		print(sh, SHELL_WARNING, "Value out of range: %s, (%ld-%ld)", str_tmp, min, max);
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
		      "\n%-4s | %-32s %-5s | %-13s | %-4s | %-15s | %-17s | %-8s\n",
		      "Num", "SSID", "(len)", "Chan (Band)", "RSSI", "Security", "BSSID", "MFP");
	}

	print(context.sh, SHELL_NORMAL,
	      "%-4d | %-32s %-5u | %-4u (%-6s) | %-4d | %-15s | %-17s | %-8s\n",
	      scan_result, entry->ssid, entry->ssid_length, entry->channel,
	      wifi_band_txt(entry->band),
	      entry->rssi,
	      wifi_security_txt(entry->security),
	      ((entry->mac_length) ?
		      net_sprint_ll_addr_buf(entry->mac, WIFI_MAC_ADDR_LEN,
					     mac_string_buf,
					     sizeof(mac_string_buf)) : ""),
	      wifi_mfp_txt(entry->mfp));
}

#ifdef CONFIG_WIFI_MGMT_RAW_SCAN_RESULTS
static int wifi_freq_to_channel(int frequency)
{
	int channel = 0;

	if (frequency == 2484) { /* channel 14 */
		channel = 14;
	} else if ((frequency <= 2472) && (frequency >= 2412)) {
		channel = ((frequency - 2412) / 5) + 1;
	} else if ((frequency <= 5320) && (frequency >= 5180)) {
		channel = ((frequency - 5180) / 5) + 36;
	} else if ((frequency <= 5720) && (frequency >= 5500)) {
		channel = ((frequency - 5500) / 5) + 100;
	} else if ((frequency <= 5895) && (frequency >= 5745)) {
		channel = ((frequency - 5745) / 5) + 149;
	} else {
		channel = frequency;
	}

	return channel;
}

static enum wifi_frequency_bands wifi_freq_to_band(int frequency)
{
	enum wifi_frequency_bands band = WIFI_FREQ_BAND_2_4_GHZ;

	if ((frequency  >= 2401) && (frequency <= 2495)) {
		band = WIFI_FREQ_BAND_2_4_GHZ;
	} else if ((frequency  >= 5170) && (frequency <= 5895)) {
		band = WIFI_FREQ_BAND_5_GHZ;
	} else {
		band = WIFI_FREQ_BAND_6_GHZ;
	}

	return band;
}

static void handle_wifi_raw_scan_result(struct net_mgmt_event_callback *cb)
{
	struct wifi_raw_scan_result *raw =
		(struct wifi_raw_scan_result *)cb->info;
	int channel;
	int band;
	int rssi;
	int i = 0;
	uint8_t mac_string_buf[sizeof("xx:xx:xx:xx:xx:xx")];

	scan_result++;

	if (scan_result == 1U) {
		print(context.sh, SHELL_NORMAL,
		      "\n%-4s | %-13s | %-4s |  %-15s | %-15s | %-32s\n",
		      "Num", "Channel (Band)", "RSSI", "BSSID", "Frame length", "Frame Body");
	}

	rssi = raw->rssi;
	channel = wifi_freq_to_channel(raw->frequency);
	band = wifi_freq_to_band(raw->frequency);

	print(context.sh, SHELL_NORMAL, "%-4d | %-4u (%-6s) | %-4d | %s |      %-4d        ",
	      scan_result,
	      channel,
	      wifi_band_txt(band),
	      rssi,
	      net_sprint_ll_addr_buf(raw->data + 10, WIFI_MAC_ADDR_LEN, mac_string_buf,
				     sizeof(mac_string_buf)), raw->frame_length);

	for (i = 0; i < 32; i++) {
		print(context.sh, SHELL_NORMAL, "%02X ", *(raw->data + i));
	}

	print(context.sh, SHELL_NORMAL, "\n");
}
#endif /* CONFIG_WIFI_MGMT_RAW_SCAN_RESULTS */

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

static void print_twt_params(uint8_t dialog_token, uint8_t flow_id,
			     enum wifi_twt_negotiation_type negotiation_type,
			     bool responder, bool implicit, bool announce,
			     bool trigger, uint32_t twt_wake_interval,
			     uint64_t twt_interval)
{
	print(context.sh, SHELL_NORMAL, "TWT Dialog token: %d\n",
	      dialog_token);
	print(context.sh, SHELL_NORMAL, "TWT flow ID: %d\n",
	      flow_id);
	print(context.sh, SHELL_NORMAL, "TWT negotiation type: %s\n",
	      wifi_twt_negotiation_type_txt(negotiation_type));
	print(context.sh, SHELL_NORMAL, "TWT responder: %s\n",
	       responder ? "true" : "false");
	print(context.sh, SHELL_NORMAL, "TWT implicit: %s\n",
	      implicit ? "true" : "false");
	print(context.sh, SHELL_NORMAL, "TWT announce: %s\n",
	      announce ? "true" : "false");
	print(context.sh, SHELL_NORMAL, "TWT trigger: %s\n",
	      trigger ? "true" : "false");
	print(context.sh, SHELL_NORMAL, "TWT wake interval: %d us\n",
	      twt_wake_interval);
	print(context.sh, SHELL_NORMAL, "TWT interval: %lld us\n",
	      twt_interval);
	print(context.sh, SHELL_NORMAL, "========================\n");
}

static void handle_wifi_twt_event(struct net_mgmt_event_callback *cb)
{
	const struct wifi_twt_params *resp =
		(const struct wifi_twt_params *)cb->info;

	if (resp->operation == WIFI_TWT_TEARDOWN) {
		print(context.sh, SHELL_NORMAL, "TWT teardown received for flow ID %d\n",
		      resp->flow_id);
		return;
	}

	if (resp->resp_status == WIFI_TWT_RESP_RECEIVED) {
		print(context.sh, SHELL_NORMAL, "TWT response: %s\n",
		      wifi_twt_setup_cmd_txt(resp->setup_cmd));
		print(context.sh, SHELL_NORMAL, "== TWT negotiated parameters ==\n");
		print_twt_params(resp->dialog_token,
				 resp->flow_id,
				 resp->negotiation_type,
				 resp->setup.responder,
				 resp->setup.implicit,
				 resp->setup.announce,
				 resp->setup.trigger,
				 resp->setup.twt_wake_interval,
				 resp->setup.twt_interval);
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
#ifdef CONFIG_WIFI_MGMT_RAW_SCAN_RESULTS
	case NET_EVENT_WIFI_RAW_SCAN_RESULT:
		handle_wifi_raw_scan_result(cb);
		break;
#endif /* CONFIG_WIFI_MGMT_RAW_SCAN_RESULTS */
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
	if (params->ssid_length > WIFI_SSID_MAX_LEN) {
		return -EINVAL;
	}

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

		if (params->psk_length < WIFI_PSK_MIN_LEN ||
		    (params->security != WIFI_SECURITY_TYPE_SAE &&
		     params->psk_length > WIFI_PSK_MAX_LEN) ||
		    (params->security == WIFI_SECURITY_TYPE_SAE &&
		     params->psk_length > WIFI_SAE_PSWD_MAX_LEN)) {
			return -EINVAL;
		}
	} else {
		params->security = WIFI_SECURITY_TYPE_NONE;
	}


	return 0;
}

static int cmd_wifi_connect(const struct shell *sh, size_t argc,
			    char *argv[])
{
	struct net_if *iface = net_if_get_first_wifi();
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
	struct net_if *iface = net_if_get_first_wifi();
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



static int wifi_scan_args_to_params(const struct shell *sh,
				    size_t argc,
				    char *argv[],
				    struct wifi_scan_params *params,
				    bool *do_scan)
{
	struct getopt_state *state;
	int opt;
	static struct option long_options[] = {{"type", required_argument, 0, 't'},
					       {"bands", required_argument, 0, 'b'},
					       {"dwell_time_active", required_argument, 0, 'a'},
					       {"dwell_time_passive", required_argument, 0, 'p'},
					       {"ssid", required_argument, 0, 's'},
					       {"max_bss", required_argument, 0, 'm'},
					       {"chans", required_argument, 0, 'c'},
					       {"help", no_argument, 0, 'h'},
					       {0, 0, 0, 0}};
	int opt_index = 0;
	int val;
	int opt_num = 0;

	*do_scan = true;

	while ((opt = getopt_long(argc, argv, "t:b:a:p:s:m:c:h", long_options, &opt_index)) != -1) {
		state = getopt_state_get();
		switch (opt) {
		case 't':
			if (!strcmp(optarg, "passive")) {
				params->scan_type = WIFI_SCAN_TYPE_PASSIVE;
			} else if (!strcmp(optarg, "active")) {
				params->scan_type = WIFI_SCAN_TYPE_ACTIVE;
			} else {
				shell_fprintf(sh, SHELL_ERROR, "Invalid scan type %s\n", optarg);
				return -ENOEXEC;
			}

			opt_num++;
			break;
		case 'b':
			if (wifi_utils_parse_scan_bands(optarg, &params->bands)) {
				shell_fprintf(sh, SHELL_ERROR, "Invalid band value(s)\n");
				return -ENOEXEC;
			}

			opt_num++;
			break;
		case 'a':
			val = atoi(optarg);

			if ((val < 5) || (val > 1000)) {
				shell_fprintf(sh, SHELL_ERROR, "Invalid dwell_time_active val\n");
				return -ENOEXEC;
			}

			params->dwell_time_active = val;
			opt_num++;
			break;
		case 'p':
			val = atoi(optarg);

			if ((val < 10) || (val > 1000)) {
				shell_fprintf(sh, SHELL_ERROR, "Invalid dwell_time_passive val\n");
				return -ENOEXEC;
			}

			params->dwell_time_passive = val;
			opt_num++;
			break;
		case 's':
			if (wifi_utils_parse_scan_ssids(optarg,
							params->ssids,
							ARRAY_SIZE(params->ssids))) {
				shell_fprintf(sh, SHELL_ERROR, "Invalid SSID(s)\n");
				return -ENOEXEC;
			}

			opt_num++;
			break;
		case 'm':
			val = atoi(optarg);

			if ((val < 0) || (val > 65535)) {
				shell_fprintf(sh, SHELL_ERROR, "Invalid max_bss val\n");
				return -ENOEXEC;
			}

			params->max_bss_cnt = val;
			opt_num++;
			break;
		case 'c':
			if (wifi_utils_parse_scan_chan(optarg,
						       params->band_chan,
						       ARRAY_SIZE(params->band_chan))) {
				shell_fprintf(sh,
					      SHELL_ERROR,
					      "Invalid band or channel value(s)\n");
				return -ENOEXEC;
			}

			opt_num++;
			break;
		case 'h':
			shell_help(sh);
			*do_scan = false;
			opt_num++;
			break;
		case '?':
		default:
			shell_fprintf(sh, SHELL_ERROR, "Invalid option or option usage: %s\n",
						  argv[opt_index + 1]);
			return -ENOEXEC;
		}
	}

	return opt_num;
}

static int cmd_wifi_scan(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_first_wifi();
	struct wifi_scan_params params = { 0 };
	bool do_scan = true;
	int opt_num;

	context.sh = sh;

	if (argc > 1) {
		opt_num = wifi_scan_args_to_params(sh, argc, argv, &params, &do_scan);

		if (opt_num < 0) {
			shell_help(sh);
			return -ENOEXEC;
		} else if (!opt_num) {
			shell_fprintf(sh, SHELL_WARNING, "No valid option(s) found\n");
			do_scan = false;
		}
	}

	if (do_scan) {
		if (net_mgmt(NET_REQUEST_WIFI_SCAN, iface, &params, sizeof(params))) {
			shell_fprintf(sh, SHELL_WARNING, "Scan request failed\n");
			return -ENOEXEC;
		}

		shell_fprintf(sh, SHELL_NORMAL, "Scan requested\n");

		return 0;
	}

	shell_fprintf(sh, SHELL_WARNING, "Scan not initiated\n");
	return -ENOEXEC;
}

static int cmd_wifi_status(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_first_wifi();
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
	struct net_if *iface = net_if_get_first_wifi();
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
	struct net_if *iface = net_if_get_first_wifi();
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
				wifi_ps_txt(config.ps_params.enabled));
		if (config.ps_params.enabled) {
			shell_fprintf(sh, SHELL_NORMAL, "PS mode: %s\n",
					wifi_ps_mode_txt(config.ps_params.mode));
		}

		shell_fprintf(sh, SHELL_NORMAL, "PS listen_interval: %d\n",
				config.ps_params.listen_interval);

		shell_fprintf(sh, SHELL_NORMAL, "PS wake up mode: %s\n",
				config.ps_params.wakeup_mode ? "Listen interval" : "DTIM");

		if (config.ps_params.timeout_ms) {
			shell_fprintf(sh, SHELL_NORMAL, "PS timeout: %d ms\n",
					config.ps_params.timeout_ms);
		} else {
			shell_fprintf(sh, SHELL_NORMAL, "PS timeout: disabled\n");
		}


		if (config.num_twt_flows == 0) {
			shell_fprintf(sh, SHELL_NORMAL, "No TWT flows\n");
		} else {
			for (int i = 0; i < config.num_twt_flows; i++) {
				print_twt_params(
					config.twt_flows[i].dialog_token,
					config.twt_flows[i].flow_id,
					config.twt_flows[i].negotiation_type,
					config.twt_flows[i].responder,
					config.twt_flows[i].implicit,
					config.twt_flows[i].announce,
					config.twt_flows[i].trigger,
					config.twt_flows[i].twt_wake_interval,
					config.twt_flows[i].twt_interval);
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
		shell_fprintf(sh, SHELL_WARNING,
			      "PS %s failed. Reason: %s\n",
			      params.enabled ? "enable" : "disable",
			      wifi_ps_get_config_err_code_str(params.fail_reason));
		return -ENOEXEC;
	}

	shell_fprintf(sh, SHELL_NORMAL, "%s\n", wifi_ps_txt(params.enabled));

	return 0;
}

static int cmd_wifi_ps_mode(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_first_wifi();
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
		shell_fprintf(sh, SHELL_WARNING, "%s failed Reason : %s\n",
			      wifi_ps_mode_txt(params.mode),
			      wifi_ps_get_config_err_code_str(params.fail_reason));
		return -ENOEXEC;
	}

	shell_fprintf(sh, SHELL_NORMAL, "%s\n", wifi_ps_mode_txt(params.mode));

	return 0;
}

static int cmd_wifi_ps_timeout(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_first_wifi();
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
	params.type = WIFI_PS_PARAM_TIMEOUT;

	if (net_mgmt(NET_REQUEST_WIFI_PS, iface, &params, sizeof(params))) {
		shell_fprintf(sh, SHELL_WARNING,
			      "Setting PS timeout failed. Reason : %s\n",
			      wifi_ps_get_config_err_code_str(params.fail_reason));
		return -ENOEXEC;
	}

	if (params.timeout_ms) {
		shell_fprintf(sh, SHELL_NORMAL, "PS timeout: %d ms\n", params.timeout_ms);
	} else {
		shell_fprintf(sh, SHELL_NORMAL, "PS timeout: disabled\n");
	}

	return 0;
}

static int cmd_wifi_twt_setup_quick(const struct shell *sh, size_t argc,
			      char *argv[])
{
	struct net_if *iface = net_if_get_first_wifi();
	struct wifi_twt_params params = { 0 };
	int idx = 1;
	long value;

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
	params.setup.trigger = 0;
	params.setup.announce = 0;

	if (!parse_number(sh, &value, argv[idx++], 1, WIFI_MAX_TWT_WAKE_INTERVAL_US)) {
		return -EINVAL;
	}
	params.setup.twt_wake_interval = (uint32_t)value;

	if (!parse_number(sh, &value, argv[idx++], 1, WIFI_MAX_TWT_INTERVAL_US)) {
		return -EINVAL;
	}
	params.setup.twt_interval = (uint64_t)value;

	if (net_mgmt(NET_REQUEST_WIFI_TWT, iface, &params, sizeof(params))) {
		shell_fprintf(sh, SHELL_WARNING, "%s with %s failed, reason : %s\n",
			wifi_twt_operation_txt(params.operation),
			wifi_twt_negotiation_type_txt(params.negotiation_type),
			wifi_twt_get_err_code_str(params.fail_reason));

		return -ENOEXEC;
	}

	shell_fprintf(sh, SHELL_NORMAL, "TWT operation %s with dg: %d, flow_id: %d requested\n",
		wifi_twt_operation_txt(params.operation),
		params.dialog_token, params.flow_id);

	return 0;
}


static int cmd_wifi_twt_setup(const struct shell *sh, size_t argc,
			      char *argv[])
{
	struct net_if *iface = net_if_get_first_wifi();
	struct wifi_twt_params params = { 0 };
	int idx = 1;
	long value;

	context.sh = sh;

	if (argc != 11) {
		shell_fprintf(sh, SHELL_WARNING, "Invalid number of arguments\n");
		shell_help(sh);
		return -ENOEXEC;
	}

	params.operation = WIFI_TWT_SETUP;

	if (!parse_number(sh, &value, argv[idx++], WIFI_TWT_INDIVIDUAL,
			  WIFI_TWT_WAKE_TBTT)) {
		return -EINVAL;
	}
	params.negotiation_type = (enum wifi_twt_negotiation_type)value;

	if (!parse_number(sh, &value, argv[idx++], WIFI_TWT_SETUP_CMD_REQUEST,
			  WIFI_TWT_SETUP_CMD_DEMAND)) {
		return -EINVAL;
	}
	params.setup_cmd = (enum wifi_twt_setup_cmd)value;

	if (!parse_number(sh, &value, argv[idx++], 1, 255)) {
		return -EINVAL;
	}
	params.dialog_token = (uint8_t)value;

	if (!parse_number(sh, &value, argv[idx++], 0, (WIFI_MAX_TWT_FLOWS - 1))) {
		return -EINVAL;
	}
	params.flow_id = (uint8_t)value;

	if (!parse_number(sh, &value, argv[idx++], 0, 1)) {
		return -EINVAL;
	}
	params.setup.responder = (bool)value;

	if (!parse_number(sh, &value, argv[idx++], 0, 1)) {
		return -EINVAL;
	}
	params.setup.trigger = (bool)value;

	if (!parse_number(sh, &value, argv[idx++], 0, 1)) {
		return -EINVAL;
	}
	params.setup.implicit = (bool)value;

	if (!parse_number(sh, &value, argv[idx++], 0, 1)) {
		return -EINVAL;
	}
	params.setup.announce = (bool)value;

	if (!parse_number(sh, &value, argv[idx++], 1, WIFI_MAX_TWT_WAKE_INTERVAL_US)) {
		return -EINVAL;
	}
	params.setup.twt_wake_interval = (uint32_t)value;

	if (!parse_number(sh, &value, argv[idx++], 1, WIFI_MAX_TWT_INTERVAL_US)) {
		return -EINVAL;
	}
	params.setup.twt_interval = (uint64_t)value;

	if (net_mgmt(NET_REQUEST_WIFI_TWT, iface, &params, sizeof(params))) {
		shell_fprintf(sh, SHELL_WARNING, "%s with %s failed. reason : %s\n",
			wifi_twt_operation_txt(params.operation),
			wifi_twt_negotiation_type_txt(params.negotiation_type),
			wifi_twt_get_err_code_str(params.fail_reason));

		return -ENOEXEC;
	}

	shell_fprintf(sh, SHELL_NORMAL, "TWT operation %s with dg: %d, flow_id: %d requested\n",
		wifi_twt_operation_txt(params.operation),
		params.dialog_token, params.flow_id);

	return 0;
}

static int cmd_wifi_twt_teardown(const struct shell *sh, size_t argc,
			      char *argv[])
{
	struct net_if *iface = net_if_get_first_wifi();
	struct wifi_twt_params params = { 0 };
	long value;

	context.sh = sh;
	int idx = 1;

	if (argc != 5) {
		shell_fprintf(sh, SHELL_WARNING, "Invalid number of arguments\n");
		shell_help(sh);
		return -ENOEXEC;
	}

	params.operation = WIFI_TWT_TEARDOWN;

	if (!parse_number(sh, &value, argv[idx++], WIFI_TWT_INDIVIDUAL,
			  WIFI_TWT_WAKE_TBTT)) {
		return -EINVAL;
	}
	params.negotiation_type = (enum wifi_twt_negotiation_type)value;

	if (!parse_number(sh, &value, argv[idx++], WIFI_TWT_SETUP_CMD_REQUEST,
			  WIFI_TWT_SETUP_CMD_DEMAND)) {
		return -EINVAL;
	}
	params.setup_cmd = (enum wifi_twt_setup_cmd)value;

	if (!parse_number(sh, &value, argv[idx++], 1, 255)) {
		return -EINVAL;
	}
	params.dialog_token = (uint8_t)value;

	if (!parse_number(sh, &value, argv[idx++], 0, (WIFI_MAX_TWT_FLOWS - 1))) {
		return -EINVAL;
	}
	params.flow_id = (uint8_t)value;

	if (net_mgmt(NET_REQUEST_WIFI_TWT, iface, &params, sizeof(params))) {
		shell_fprintf(sh, SHELL_WARNING, "%s with %s failed, reason : %s\n",
			wifi_twt_operation_txt(params.operation),
			wifi_twt_negotiation_type_txt(params.negotiation_type),
			wifi_twt_get_err_code_str(params.fail_reason));

		return -ENOEXEC;
	}

	shell_fprintf(sh, SHELL_NORMAL, "TWT operation %s with dg: %d, flow_id: %d success\n",
		wifi_twt_operation_txt(params.operation),
		params.dialog_token, params.flow_id);

	return 0;
}

static int cmd_wifi_twt_teardown_all(const struct shell *sh, size_t argc,
			      char *argv[])
{
	struct net_if *iface = net_if_get_first_wifi();
	struct wifi_twt_params params = { 0 };

	context.sh = sh;

	params.operation = WIFI_TWT_TEARDOWN;
	params.teardown.teardown_all = 1;

	if (net_mgmt(NET_REQUEST_WIFI_TWT, iface, &params, sizeof(params))) {
		shell_fprintf(sh, SHELL_WARNING, "%s with %s failed, reason : %s\n",
			wifi_twt_operation_txt(params.operation),
			wifi_twt_negotiation_type_txt(params.negotiation_type),
			wifi_twt_get_err_code_str(params.fail_reason));

		return -ENOEXEC;
	}

	shell_fprintf(sh, SHELL_NORMAL, "TWT operation %s all flows success\n",
		wifi_twt_operation_txt(params.operation));

	return 0;
}

static int cmd_wifi_ap_enable(const struct shell *sh, size_t argc,
			      char *argv[])
{
	struct net_if *iface = net_if_get_first_wifi();
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
	struct net_if *iface = net_if_get_first_wifi();
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
	struct net_if *iface = net_if_get_first_wifi();
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
	struct net_if *iface = net_if_get_first_wifi();
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
		if (params.fail_reason ==
		    WIFI_PS_PARAM_LISTEN_INTERVAL_RANGE_INVALID) {
			shell_fprintf(sh, SHELL_WARNING,
			      "Setting listen interval failed. Reason :%s\n",
			      wifi_ps_get_config_err_code_str(params.fail_reason));
			shell_fprintf(sh, SHELL_WARNING,
				"Hardware support valid range : 3 - 65535\n");
		} else  {
			shell_fprintf(sh, SHELL_WARNING,
				"Setting listen interval failed. Reason :%s\n",
				wifi_ps_get_config_err_code_str(params.fail_reason));
		}
		return -ENOEXEC;
	}

	shell_fprintf(sh, SHELL_NORMAL,
		"Listen interval %hu\n", params.listen_interval);

	return 0;
}

static int cmd_wifi_ps_wakeup_mode(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_first_wifi();
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
		shell_fprintf(sh, SHELL_WARNING,
			      "Setting PS wake up mode to %s failed..Reason :%s\n",
			      params.wakeup_mode ? "Listen interval" : "DTIM interval",
			      wifi_ps_get_config_err_code_str(params.fail_reason));
		return -ENOEXEC;
	}

	shell_fprintf(sh, SHELL_NORMAL, "%s\n",
		      wifi_ps_wakeup_mode_txt(params.wakeup_mode));

	return 0;
}

void parse_mode_args_to_params(const struct shell *sh, int argc,
			       char *argv[], struct wifi_mode_info *mode,
			       bool *do_mode_oper)
{
	int opt;
	int option_index = 0;

	static struct option long_options[] = {{"if_index", optional_argument, 0, 'i'},
					       {"sta", no_argument, 0, 's'},
					       {"monitor", no_argument, 0, 'm'},
					       {"TX-injection", no_argument, 0, 't'},
					       {"promiscuous", no_argument, 0, 'p'},
					       {"ap", no_argument, 0, 'a'},
					       {"softap", no_argument, 0, 'k'},
					       {"get", no_argument, 0, 'g'},
					       {"help", no_argument, 0, 'h'},
					       {0, 0, 0, 0}};

	while ((opt = getopt_long(argc, argv, "i:smtpakgh", long_options, &option_index)) != -1) {
		switch (opt) {
		case 's':
			mode->mode |= WIFI_STA_MODE;
			break;
		case 'm':
			mode->mode |= WIFI_MONITOR_MODE;
			break;
		case 't':
			mode->mode |= WIFI_TX_INJECTION_MODE;
			break;
		case 'p':
			mode->mode |= WIFI_PROMISCUOUS_MODE;
			break;
		case 'a':
			mode->mode |= WIFI_AP_MODE;
			break;
		case 'k':
			mode->mode |= WIFI_SOFTAP_MODE;
			break;
		case 'g':
			mode->oper = WIFI_MGMT_GET;
			break;
		case 'i':
			mode->if_index = (uint8_t)atoi(optarg);
			break;
		case 'h':
			shell_help(sh);
			*do_mode_oper = false;
			break;
		case '?':
		default:
			break;
		}
	}
}

static int cmd_wifi_mode(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface;
	struct wifi_mode_info mode_info = {0};
	int ret;
	bool do_mode_oper = true;

	if (argc > 1) {
		mode_info.oper = WIFI_MGMT_SET;
		parse_mode_args_to_params(sh, argc, argv, &mode_info, &do_mode_oper);
	} else {
		shell_fprintf(sh, SHELL_ERROR, "Invalid number of arguments\n");
		return -EINVAL;
	}

	if (do_mode_oper) {
		/* Check interface index value. Mode validation must be performed by
		 * lower layer
		 */
		if (mode_info.if_index == 0) {
			iface = net_if_get_first_wifi();
			if (iface == NULL) {
				shell_fprintf(sh, SHELL_ERROR,
					      "Cannot find the default wifi interface\n");
				return -ENOEXEC;
			}
			mode_info.if_index = net_if_get_by_iface(iface);
		} else {
			iface = net_if_get_by_index(mode_info.if_index);
			if (iface == NULL) {
				shell_fprintf(sh, SHELL_ERROR,
					      "Cannot find interface for if_index %d\n",
					      mode_info.if_index);
				return -ENOEXEC;
			}
		}

		ret = net_mgmt(NET_REQUEST_WIFI_MODE, iface, &mode_info, sizeof(mode_info));

		if (ret) {
			shell_fprintf(sh, SHELL_ERROR, "mode %s operation failed with reason %d\n",
					mode_info.oper == WIFI_MGMT_GET ? "get" : "set", ret);
			return -ENOEXEC;
		}

		if (mode_info.oper == WIFI_MGMT_GET) {
			shell_fprintf(sh, SHELL_NORMAL, "Wi-Fi current mode is %x\n",
					mode_info.mode);
		} else {
			shell_fprintf(sh, SHELL_NORMAL, "Wi-Fi mode set to %x\n", mode_info.mode);
		}
	}
	return 0;
}

void parse_channel_args_to_params(const struct shell *sh, int argc,
				  char *argv[], struct wifi_channel_info *channel,
				  bool *do_channel_oper)
{
	int opt;
	int option_index = 0;

	static struct option long_options[] = {{"if_index", optional_argument, 0, 'i'},
					       {"channel", required_argument, 0, 'c'},
					       {"get", no_argument, 0, 'g'},
					       {"help", no_argument, 0, 'h'},
					       {0, 0, 0, 0}};

	while ((opt = getopt_long(argc, argv, "i:c:gh", long_options, &option_index)) != -1)  {
		switch (opt) {
		case 'c':
			channel->channel = (uint16_t)atoi(optarg);
			break;
		case 'i':
			channel->if_index = (uint8_t)atoi(optarg);
			break;
		case 'g':
			channel->oper = WIFI_MGMT_GET;
			break;
		case 'h':
			shell_help(sh);
			*do_channel_oper = false;
			break;
		case '?':
		default:
			break;
		}
	}
}

static int cmd_wifi_channel(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface;
	struct wifi_channel_info channel_info = {0};
	int ret;
	bool do_channel_oper = true;

	if (argc > 1) {
		channel_info.oper = WIFI_MGMT_SET;
		parse_channel_args_to_params(sh, argc, argv, &channel_info, &do_channel_oper);
	} else {
		shell_fprintf(sh, SHELL_ERROR, "Invalid number of arguments\n");
		return -EINVAL;
	}

	if (do_channel_oper) {
		/*
		 * Validate parameters before sending to lower layer.
		 * Do it here instead of parse_channel_args_to_params
		 * as this is right before sending the parameters to
		 * the lower layer.
		 */

		if (channel_info.if_index == 0) {
			iface = net_if_get_first_wifi();
			if (iface == NULL) {
				shell_fprintf(sh, SHELL_ERROR,
					      "Cannot find the default wifi interface\n");
				return -ENOEXEC;
			}
			channel_info.if_index = net_if_get_by_iface(iface);
		} else {
			iface = net_if_get_by_index(channel_info.if_index);
			if (iface == NULL) {
				shell_fprintf(sh, SHELL_ERROR,
					      "Cannot find interface for if_index %d\n",
					      channel_info.if_index);
				return -ENOEXEC;
			}
		}

		if (channel_info.oper == WIFI_MGMT_SET) {
			if ((channel_info.channel < WIFI_CHANNEL_MIN) ||
			    (channel_info.channel > WIFI_CHANNEL_MAX)) {
				shell_fprintf(sh, SHELL_ERROR,
						"Invalid channel number. Range is (1-233)\n");
				return -ENOEXEC;
			}
		}

		ret = net_mgmt(NET_REQUEST_WIFI_CHANNEL, iface,
				&channel_info, sizeof(channel_info));

		if (ret) {
			shell_fprintf(sh, SHELL_ERROR,
					"channel %s operation failed with reason %d\n",
					channel_info.oper == WIFI_MGMT_GET ? "get" : "set", ret);
			return -ENOEXEC;
		}

		if (channel_info.oper == WIFI_MGMT_GET) {
			shell_fprintf(sh, SHELL_NORMAL, "Wi-Fi current channel is: %d\n",
					channel_info.channel);
		} else {
			shell_fprintf(sh, SHELL_NORMAL, "Wi-Fi channel set to %d\n",
					channel_info.channel);
		}
	}
	return 0;
}

void parse_filter_args_to_params(const struct shell *sh, int argc,
				 char *argv[], struct wifi_filter_info *filter,
				 bool *do_filter_oper)
{
	int opt;
	int option_index = 0;

	static struct option long_options[] = {{"if_index", optional_argument, 0, 'i'},
					       {"capture_len", optional_argument, 0, 'b'},
					       {"all", no_argument, 0, 'a'},
					       {"mgmt", no_argument, 0, 'm'},
					       {"ctrl", no_argument, 0, 'c'},
					       {"data", no_argument, 0, 'd'},
					       {"get", no_argument, 0, 'g'},
					       {"help", no_argument, 0, 'h'},
					       {0, 0, 0, 0}};

	while ((opt = getopt_long(argc, argv, "i:b:amcdgh", long_options, &option_index)) != -1)  {
		switch (opt) {
		case 'a':
			filter->filter |= WIFI_PACKET_FILTER_ALL;
			break;
		case 'm':
			filter->filter |= WIFI_PACKET_FILTER_MGMT;
			break;
		case 'c':
			filter->filter |= WIFI_PACKET_FILTER_DATA;
			break;
		case 'd':
			filter->filter |= WIFI_PACKET_FILTER_CTRL;
			break;
		case 'i':
			filter->if_index = (uint8_t)atoi(optarg);
			break;
		case 'b':
			filter->buffer_size = (uint16_t)atoi(optarg);
			break;
		case 'h':
			shell_help(sh);
			*do_filter_oper = false;
			break;
		case 'g':
			filter->oper = WIFI_MGMT_GET;
			break;
		case '?':
		default:
			break;
		}
	}
}

static int cmd_wifi_packet_filter(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface;
	struct wifi_filter_info packet_filter = {0};
	int ret;
	bool do_filter_oper = true;

	if (argc > 1) {
		packet_filter.oper = WIFI_MGMT_SET;
		parse_filter_args_to_params(sh, argc, argv, &packet_filter, &do_filter_oper);
	} else {
		shell_fprintf(sh, SHELL_ERROR, "Invalid number of arguments\n");
		return -EINVAL;
	}

	if (do_filter_oper) {
		/*
		 * Validate parameters before sending to lower layer.
		 * Do it here instead of parse_filter_args_to_params
		 * as this is right before sending the parameters to
		 * the lower layer. filter and packet capture length
		 * value to be verified by the lower layer.
		 */
		if (packet_filter.if_index == 0) {
			iface = net_if_get_first_wifi();
			if (iface == NULL) {
				shell_fprintf(sh, SHELL_ERROR,
					      "Cannot find the default wifi interface\n");
				return -ENOEXEC;
			}
			packet_filter.if_index = net_if_get_by_iface(iface);
		} else {
			iface = net_if_get_by_index(packet_filter.if_index);
			if (iface == NULL) {
				shell_fprintf(sh, SHELL_ERROR,
					      "Cannot find interface for if_index %d\n",
					      packet_filter.if_index);
				return -ENOEXEC;
			}
		}

		ret = net_mgmt(NET_REQUEST_WIFI_PACKET_FILTER, iface,
					&packet_filter, sizeof(packet_filter));

		if (ret) {
			shell_fprintf(sh, SHELL_ERROR,
					"Wi-Fi packet filter %s operation failed with reason %d\n",
					packet_filter.oper == WIFI_MGMT_GET ? "get" : "set", ret);
			return -ENOEXEC;
		}

		if (packet_filter.oper == WIFI_MGMT_GET) {
			shell_fprintf(sh, SHELL_NORMAL, "Wi-Fi current mode packet filter is %d\n",
					packet_filter.filter);
		} else {
			shell_fprintf(sh, SHELL_NORMAL, "Wi-Fi mode packet filter set to %d\n",
					packet_filter.filter);
		}
	}
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(wifi_cmd_ap,
	SHELL_CMD(disable, NULL,
		  "Disable Access Point mode",
		  cmd_wifi_ap_disable),
	SHELL_CMD(enable, NULL, "<SSID> [channel] [PSK]",
		  cmd_wifi_ap_enable),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(wifi_twt_ops,
	SHELL_CMD(quick_setup, NULL, " Start a TWT flow with defaults:\n"
		"<twt_wake_interval: 1-262144us> <twt_interval: 1us-2^31us>\n",
		cmd_wifi_twt_setup_quick),
	SHELL_CMD(setup, NULL, " Start a TWT flow:\n"
		"<negotiation_type, 0: Individual, 1: Broadcast, 2: Wake TBTT>\n"
		"<setup_cmd: 0: Request, 1: Suggest, 2: Demand>\n"
		"<dialog_token: 1-255> <flow_id: 0-7> <responder: 0/1> <trigger: 0/1> <implicit:0/1> "
		"<announce: 0/1> <twt_wake_interval: 1-262144us> <twt_interval: 1us-2^31us>\n",
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
	SHELL_CMD(scan, NULL,
		  "Scan for Wi-Fi APs\n"
		    "OPTIONAL PARAMETERS:\n"
		    "[-t, --type <active/passive>] : Preferred mode of scan. The actual mode of scan can depend on factors such as the Wi-Fi chip implementation, regulatory domain restrictions. Default type is active.\n"
		    "[-b, --bands <Comma separated list of band values (2/5/6)>] : Bands to be scanned where 2: 2.4 GHz, 5: 5 GHz, 6: 6 GHz.\n"
		    "[-a, --dwell_time_active <val_in_ms>] : Active scan dwell time (in ms) on a channel. Range 5 ms to 1000 ms.\n"
		    "[-p, --dwell_time_passive <val_in_ms>] : Passive scan dwell time (in ms) on a channel. Range 10 ms to 1000 ms.\n"
		    "[-s, --ssid : SSID to scan for. Can be provided multiple times.\n"
		    "[-m, --max_bss <val>] : Maximum BSSes to scan for. Range 1 - 65535.\n"
		    "[-c, --chans <Comma separated list of channel ranges>] : Channels to be scanned. The channels must be specified in the form band1:chan1,chan2_band2:chan3,..etc. band1, band2 must be valid band values and chan1, chan2, chan3 must be specified as a list of comma separated values where each value is either a single channel or a channel range specified as chan_start-chan_end. Each band channel set has to be separated by a _. For example, a valid channel specification can be 2:1,6-11,14_5:36,149-165,44\n"
		    "[-h, --help] : Print out the help for the scan command.",
		  cmd_wifi_scan),
	SHELL_CMD(statistics, NULL, "Wi-Fi interface statistics", cmd_wifi_stats),
	SHELL_CMD(status, NULL, "Status of the Wi-Fi interface", cmd_wifi_status),
	SHELL_CMD(twt, &wifi_twt_ops, "Manage TWT flows", NULL),
	SHELL_CMD(reg_domain, NULL,
		"Set or Get Wi-Fi regulatory domain\n"
		"Usage: wifi reg_domain [ISO/IEC 3166-1 alpha2] [-f]\n"
		"-f: Force to use this regulatory hint over any other regulatory hints\n"
		"Note: This may cause regulatory compliance issues, use it at your own risk.",
		cmd_wifi_reg_domain),
	SHELL_CMD(mode, NULL, "mode operational setting\n"
		"This command may be used to set the Wi-Fi device into a specific mode of operation\n"
		"parameters:"
		"[-i : Interface index - optional argument\n"
		"[-s : Station mode.\n"
		"[-m : Monitor mode.\n"
		"[-p : Promiscuous mode.\n"
		"[-t : TX-Injection mode.\n"
		"[-a : AP mode.\n"
		"[-k : Softap mode.\n"
		"[-h : Help.\n"
		"[-g : Get current mode for a specific interface index.\n"
		"Usage: Get operation example for interface index 1\n"
		"wifi mode -g -i1\n"
		"Set operation example for interface index 1 - set station+promiscuous\n"
		"wifi mode -i1 -sp\n",
		cmd_wifi_mode),
	SHELL_CMD(packet_filter, NULL, "mode filter setting\n"
		"This command is used to set packet filter setting when\n"
		"monitor, TX-Injection and promiscuous mode is enabled.\n"
		"The different packet filter modes are control, management, data and enable all filters\n"
		"parameters:"
		"[-i : Interface index - optional argument.\n"
		"[-a : Enable all packet filter modes\n"
		"[-m : Enable management packets to allowed up the stack.\n"
		"[-c : Enable control packets to be allowed up the stack.\n"
		"[-d : Enable Data packets to be allowed up the stack.\n"
		"[-g : Get current filter settings for a specific interface index.\n"
		"<-b : Capture length buffer size for each packet to be captured - optional argument.\n"
		"<-h : Help.\n"
		"Usage: Get operation example for interface index 1\n"
		"wifi packet_filter -g -i1\n"
		"Set operation example for interface index 1 - set data+management frame filter\n"
		"wifi packet_filter -i1 -md\n",
		cmd_wifi_packet_filter),
	SHELL_CMD(channel, NULL, "wifi channel setting\n"
		"This command is used to set the channel when\n"
		"monitor or TX-Injection mode is enabled.\n"
		"Currently 20 MHz is only supported and no BW parameter is provided\n"
		"parameters:"
		"[-i : Interface index - optional argument.\n"
		"[-c : Set a specific channel number to the lower layer.\n"
		"[-g : Get current set channel number from the lower layer.\n"
		"[-h : Help.\n"
		"Usage: Get operation example for interface index 1\n"
		"wifi channel -g -i1\n"
		"Set operation example for interface index 1 (setting channel 5)\n"
		"wifi -i1 -c5\n",
		cmd_wifi_channel),
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
