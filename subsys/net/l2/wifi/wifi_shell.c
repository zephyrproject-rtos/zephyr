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
#include <strings.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/printk.h>
#include <zephyr/init.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/wifi_utils.h>
#include <zephyr/posix/unistd.h>
#include <zephyr/sys/slist.h>

#include "net_shell_private.h"

#define WIFI_SHELL_MODULE "wifi"

#define WIFI_SHELL_MGMT_EVENTS_COMMON (NET_EVENT_WIFI_SCAN_DONE   |\
				NET_EVENT_WIFI_CONNECT_RESULT     |\
				NET_EVENT_WIFI_DISCONNECT_RESULT  |\
				NET_EVENT_WIFI_TWT                |\
				NET_EVENT_WIFI_RAW_SCAN_RESULT    |\
				NET_EVENT_WIFI_AP_ENABLE_RESULT   |\
				NET_EVENT_WIFI_AP_DISABLE_RESULT  |\
				NET_EVENT_WIFI_AP_STA_CONNECTED   |\
				NET_EVENT_WIFI_AP_STA_DISCONNECTED)

#ifdef CONFIG_WIFI_MGMT_RAW_SCAN_RESULTS_ONLY
#define WIFI_SHELL_MGMT_EVENTS (WIFI_SHELL_MGMT_EVENTS_COMMON)
#else
#define WIFI_SHELL_MGMT_EVENTS (WIFI_SHELL_MGMT_EVENTS_COMMON |\
				NET_EVENT_WIFI_SCAN_RESULT)
#endif /* CONFIG_WIFI_MGMT_RAW_SCAN_RESULTS_ONLY */

#define MAX_BANDS_STR_LEN 64

static struct {
	const struct shell *sh;
	uint32_t scan_result;

	union {
		struct {

			uint8_t connecting: 1;
			uint8_t disconnecting: 1;
			uint8_t _unused: 6;
		};
		uint8_t all;
	};
} context;

static struct net_mgmt_event_callback wifi_shell_mgmt_cb;
static struct wifi_reg_chan_info chan_info[MAX_REG_CHAN_NUM];

static K_MUTEX_DEFINE(wifi_ap_sta_list_lock);
struct wifi_ap_sta_node {
	bool valid;
	struct wifi_ap_sta_info sta_info;
};
static struct wifi_ap_sta_node sta_list[CONFIG_WIFI_SHELL_MAX_AP_STA];

static bool parse_number(const struct shell *sh, long *param, char *str, long min, long max)
{
	char *endptr;
	char *str_tmp = str;
	long num;

	if ((str_tmp[0] == '0') && (str_tmp[1] == 'x')) {
		/* Hexadecimal numbers take base 0 in strtol */
		num = strtol(str_tmp, &endptr, 0);
	} else {
		num = strtol(str_tmp, &endptr, 10);
	}

	if (*endptr != '\0') {
		PR_ERROR("Invalid number: %s", str_tmp);
		return false;
	}

	if ((num) < (min) || (num) > (max)) {
		PR_WARNING("Value out of range: %s, (%ld-%ld)", str_tmp, min, max);
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
	const struct shell *sh = context.sh;
	uint8_t ssid_print[WIFI_SSID_MAX_LEN + 1];

	context.scan_result++;

	if (context.scan_result == 1U) {
		PR("\n%-4s | %-32s %-5s | %-13s | %-4s | %-15s | %-17s | %-8s\n",
		   "Num", "SSID", "(len)", "Chan (Band)", "RSSI", "Security", "BSSID", "MFP");
	}

	strncpy(ssid_print, entry->ssid, sizeof(ssid_print) - 1);
	ssid_print[sizeof(ssid_print) - 1] = '\0';

	PR("%-4d | %-32s %-5u | %-4u (%-6s) | %-4d | %-15s | %-17s | %-8s\n",
	   context.scan_result, ssid_print, entry->ssid_length, entry->channel,
	   wifi_band_txt(entry->band),
	   entry->rssi,
	   wifi_security_txt(entry->security),
	   ((entry->mac_length) ?
		   net_sprint_ll_addr_buf(entry->mac, WIFI_MAC_ADDR_LEN,
					  mac_string_buf,
					  sizeof(mac_string_buf)) : ""),
	   wifi_mfp_txt(entry->mfp));
}

static int wifi_freq_to_channel(int frequency)
{
	int channel;

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

#ifdef CONFIG_WIFI_MGMT_RAW_SCAN_RESULTS
static enum wifi_frequency_bands wifi_freq_to_band(int frequency)
{
	enum wifi_frequency_bands band = WIFI_FREQ_BAND_2_4_GHZ;

	if ((frequency >= 2401) && (frequency <= 2495)) {
		band = WIFI_FREQ_BAND_2_4_GHZ;
	} else if ((frequency >= 5170) && (frequency <= 5895)) {
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
	uint8_t mac_string_buf[sizeof("xx:xx:xx:xx:xx:xx")];
	const struct shell *sh = context.sh;

	context.scan_result++;

	if (context.scan_result == 1U) {
		PR("\n%-4s | %-13s | %-4s |  %-15s | %-15s | %-32s\n",
		   "Num", "Channel (Band)", "RSSI", "BSSID", "Frame length", "Frame Body");
	}

	rssi = raw->rssi;
	channel = wifi_freq_to_channel(raw->frequency);
	band = wifi_freq_to_band(raw->frequency);

	PR("%-4d | %-4u (%-6s) | %-4d | %s |      %-4d        ",
	   context.scan_result,
	   channel,
	   wifi_band_txt(band),
	   rssi,
	   net_sprint_ll_addr_buf(raw->data + 10, WIFI_MAC_ADDR_LEN, mac_string_buf,
				  sizeof(mac_string_buf)), raw->frame_length);

	for (int i = 0; i < 32; i++) {
		PR("%02X ", *(raw->data + i));
	}

	PR("\n");
}
#endif /* CONFIG_WIFI_MGMT_RAW_SCAN_RESULTS */

static void handle_wifi_scan_done(struct net_mgmt_event_callback *cb)
{
	const struct wifi_status *status =
		(const struct wifi_status *)cb->info;
	const struct shell *sh = context.sh;

	if (status->status) {
		PR_WARNING("Scan request failed (%d)\n", status->status);
	} else {
		PR("Scan request done\n");
	}

	context.scan_result = 0U;
}

static void handle_wifi_connect_result(struct net_mgmt_event_callback *cb)
{
	const struct wifi_status *status =
		(const struct wifi_status *) cb->info;
	const struct shell *sh = context.sh;

	if (status->status) {
		PR_WARNING("Connection request failed (%d)\n", status->status);
	} else {
		PR("Connected\n");
	}

	context.connecting = false;
}

static void handle_wifi_disconnect_result(struct net_mgmt_event_callback *cb)
{
	const struct wifi_status *status =
		(const struct wifi_status *) cb->info;
	const struct shell *sh = context.sh;

	if (context.disconnecting) {
		if (status->status) {
			PR_WARNING("Disconnection request failed (%d)\n", status->status);
		} else {
			PR("Disconnection request done (%d)\n", status->status);
		}
		context.disconnecting = false;
	} else {
		PR("Disconnected\n");
	}
}

static void print_twt_params(uint8_t dialog_token, uint8_t flow_id,
			     enum wifi_twt_negotiation_type negotiation_type,
			     bool responder, bool implicit, bool announce,
			     bool trigger, uint32_t twt_wake_interval,
			     uint64_t twt_interval)
{
	const struct shell *sh = context.sh;

	PR("TWT Dialog token: %d\n",
	      dialog_token);
	PR("TWT flow ID: %d\n",
	      flow_id);
	PR("TWT negotiation type: %s\n",
	      wifi_twt_negotiation_type_txt(negotiation_type));
	PR("TWT responder: %s\n",
	       responder ? "true" : "false");
	PR("TWT implicit: %s\n",
	      implicit ? "true" : "false");
	PR("TWT announce: %s\n",
	      announce ? "true" : "false");
	PR("TWT trigger: %s\n",
	      trigger ? "true" : "false");
	PR("TWT wake interval: %d us\n",
	      twt_wake_interval);
	PR("TWT interval: %lld us\n",
	      twt_interval);
	PR("========================\n");
}

static void handle_wifi_twt_event(struct net_mgmt_event_callback *cb)
{
	const struct wifi_twt_params *resp =
		(const struct wifi_twt_params *)cb->info;
	const struct shell *sh = context.sh;

	if (resp->operation == WIFI_TWT_TEARDOWN) {
		if (resp->teardown_status == WIFI_TWT_TEARDOWN_SUCCESS) {
			PR("TWT teardown succeeded for flow ID %d\n",
			      resp->flow_id);
		} else {
			PR("TWT teardown failed for flow ID %d\n",
			      resp->flow_id);
		}
		return;
	}

	if (resp->resp_status == WIFI_TWT_RESP_RECEIVED) {
		PR("TWT response: %s\n",
		      wifi_twt_setup_cmd_txt(resp->setup_cmd));
		PR("== TWT negotiated parameters ==\n");
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
		PR("TWT response timed out\n");
	}
}

static void handle_wifi_ap_enable_result(struct net_mgmt_event_callback *cb)
{
	const struct wifi_status *status =
		(const struct wifi_status *)cb->info;
	const struct shell *sh = context.sh;

	if (status->status) {
		PR_WARNING("AP enable request failed (%d)\n", status->status);
	} else {
		PR("AP enabled\n");
	}
}

static void handle_wifi_ap_disable_result(struct net_mgmt_event_callback *cb)
{
	const struct wifi_status *status =
		(const struct wifi_status *)cb->info;
	const struct shell *sh = context.sh;

	if (status->status) {
		PR_WARNING("AP disable request failed (%d)\n", status->status);
	} else {
		PR("AP disabled\n");
	}

	k_mutex_lock(&wifi_ap_sta_list_lock, K_FOREVER);
	memset(&sta_list, 0, sizeof(sta_list));
	k_mutex_unlock(&wifi_ap_sta_list_lock);
}

static void handle_wifi_ap_sta_connected(struct net_mgmt_event_callback *cb)
{
	const struct wifi_ap_sta_info *sta_info =
		(const struct wifi_ap_sta_info *)cb->info;
	const struct shell *sh = context.sh;
	uint8_t mac_string_buf[sizeof("xx:xx:xx:xx:xx:xx")];
	int i;

	PR("Station connected: %s\n",
	   net_sprint_ll_addr_buf(sta_info->mac, WIFI_MAC_ADDR_LEN,
				  mac_string_buf, sizeof(mac_string_buf)));

	k_mutex_lock(&wifi_ap_sta_list_lock, K_FOREVER);
	for (i = 0; i < CONFIG_WIFI_SHELL_MAX_AP_STA; i++) {
		if (!sta_list[i].valid) {
			sta_list[i].sta_info = *sta_info;
			sta_list[i].valid = true;
			break;
		}
	}
	if (i == CONFIG_WIFI_SHELL_MAX_AP_STA) {
		PR_WARNING("No space to store station info: "
			   "Increase CONFIG_WIFI_SHELL_MAX_AP_STA\n");
	}
	k_mutex_unlock(&wifi_ap_sta_list_lock);
}

static void handle_wifi_ap_sta_disconnected(struct net_mgmt_event_callback *cb)
{
	const struct wifi_ap_sta_info *sta_info =
		(const struct wifi_ap_sta_info *)cb->info;
	const struct shell *sh = context.sh;
	uint8_t mac_string_buf[sizeof("xx:xx:xx:xx:xx:xx")];

	PR("Station disconnected: %s\n",
	   net_sprint_ll_addr_buf(sta_info->mac, WIFI_MAC_ADDR_LEN,
				  mac_string_buf, sizeof(mac_string_buf)));

	k_mutex_lock(&wifi_ap_sta_list_lock, K_FOREVER);
	for (int i = 0; i < CONFIG_WIFI_SHELL_MAX_AP_STA; i++) {
		if (!sta_list[i].valid) {
			continue;
		}

		if (!memcmp(sta_list[i].sta_info.mac, sta_info->mac,
			    WIFI_MAC_ADDR_LEN)) {
			sta_list[i].valid = false;
			break;
		}
	}
	k_mutex_unlock(&wifi_ap_sta_list_lock);
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
	case NET_EVENT_WIFI_AP_ENABLE_RESULT:
		handle_wifi_ap_enable_result(cb);
		break;
	case NET_EVENT_WIFI_AP_DISABLE_RESULT:
		handle_wifi_ap_disable_result(cb);
		break;
	case NET_EVENT_WIFI_AP_STA_CONNECTED:
		handle_wifi_ap_sta_connected(cb);
		break;
	case NET_EVENT_WIFI_AP_STA_DISCONNECTED:
		handle_wifi_ap_sta_disconnected(cb);
		break;
	default:
		break;
	}
}

static int __wifi_args_to_params(const struct shell *sh, size_t argc, char *argv[],
				 struct wifi_connect_req_params *params,
				 enum wifi_iface_mode iface_mode)
{
	char *endptr;
	int idx = 1;
	struct getopt_state *state;
	int opt;
	bool secure_connection = false;
	static struct option long_options[] = {{"ssid", required_argument, 0, 's'},
					       {"passphrase", required_argument, 0, 'p'},
					       {"key-mgmt", required_argument, 0, 'k'},
					       {"ieee-80211w", required_argument, 0, 'w'},
					       {"bssid", required_argument, 0, 'm'},
					       {"band", required_argument, 0, 'b'},
					       {"channel", required_argument, 0, 'c'},
					       {"help", no_argument, 0, 'h'},
					       {0, 0, 0, 0}};
	int opt_index = 0;
	uint8_t band;
	const uint8_t all_bands[] = {
		WIFI_FREQ_BAND_2_4_GHZ,
		WIFI_FREQ_BAND_5_GHZ,
		WIFI_FREQ_BAND_6_GHZ
	};
	bool found = false;
	char bands_str[MAX_BANDS_STR_LEN] = {0};
	size_t offset = 0;
	long channel;

	/* Defaults */
	params->band = WIFI_FREQ_BAND_UNKNOWN;
	params->channel = WIFI_CHANNEL_ANY;
	params->security = WIFI_SECURITY_TYPE_NONE;
	params->mfp = WIFI_MFP_OPTIONAL;

	while ((opt = getopt_long(argc, argv, "s:p:k:w:b:c:m:h", long_options, &opt_index)) != -1) {
		state = getopt_state_get();
		switch (opt) {
		case 's':
			params->ssid = optarg;
			params->ssid_length = strlen(params->ssid);
			if (params->ssid_length > WIFI_SSID_MAX_LEN) {
				PR_WARNING("SSID too long (max %d characters)\n",
					    WIFI_SSID_MAX_LEN);
				return -EINVAL;
			}
			break;
		case 'k':
			params->security = atoi(optarg);
			if (params->security) {
				secure_connection = true;
			}
			break;
		case 'p':
			params->psk = optarg;
			params->psk_length = strlen(params->psk);
			break;
		case 'c':
			channel = strtol(optarg, &endptr, 10);
			for (band = 0; band < ARRAY_SIZE(all_bands); band++) {
				offset += snprintf(bands_str + offset,
						   sizeof(bands_str) - offset,
						   "%s%s",
						   band ? "," : "",
						   wifi_band_txt(all_bands[band]));
				if (offset >= sizeof(bands_str)) {
					PR_ERROR("Failed to parse channel: %s: "
						 "band string too long\n",
						 argv[idx]);
					return -EINVAL;
				}

				if (wifi_utils_validate_chan(all_bands[band],
							     channel)) {
					found = true;
					break;
				}
			}

			if (!found) {
				PR_ERROR("Invalid channel: %ld, checked bands: %s\n",
					 channel,
					 bands_str);
				return -EINVAL;
			}

			params->channel = channel;
			break;
		case 'b':
			if (iface_mode == WIFI_MODE_INFRA) {
				switch (atoi(optarg)) {
				case 2:
					params->band = WIFI_FREQ_BAND_2_4_GHZ;
					break;
				case 5:
					params->band = WIFI_FREQ_BAND_5_GHZ;
					break;
				case 6:
					params->band = WIFI_FREQ_BAND_6_GHZ;
					break;
				default:
					PR_ERROR("Invalid band: %d\n", atoi(optarg));
					return -EINVAL;
				}
			}
			break;
		case 'w':
			if (params->security == WIFI_SECURITY_TYPE_NONE ||
				params->security == WIFI_SECURITY_TYPE_WPA_PSK) {
				PR_ERROR("MFP not supported for security type %s\n",
						 wifi_security_txt(params->security));
				return -EINVAL;
			}
			params->mfp = atoi(optarg);
			break;
		case 'm':
			sscanf(optarg, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
				&params->bssid[0], &params->bssid[1],
				&params->bssid[2], &params->bssid[3],
				&params->bssid[4], &params->bssid[5]);
			break;
		case 'h':
			return -ENOEXEC;
		default:
			PR_ERROR("Invalid option %c\n", opt);
			shell_help(sh);
			return -EINVAL;
		}
	}
	if (params->psk && !secure_connection) {
		PR_WARNING("Passphrase provided without security configuration\n");
	}
	return 0;
}

static int cmd_wifi_connect(const struct shell *sh, size_t argc,
			    char *argv[])
{
	struct net_if *iface = net_if_get_first_wifi();
	struct wifi_connect_req_params cnx_params = { 0 };
	int ret;

	context.sh = sh;
	if (__wifi_args_to_params(sh, argc, argv, &cnx_params, WIFI_MODE_INFRA)) {
		shell_help(sh);
		return -ENOEXEC;
	}

	context.connecting = true;
	ret = net_mgmt(NET_REQUEST_WIFI_CONNECT, iface,
		       &cnx_params, sizeof(struct wifi_connect_req_params));
	if (ret) {
		printk("Connection request failed with error: %d\n", ret);
		context.connecting = false;
		return -ENOEXEC;
	}

	PR("Connection requested\n");

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
			PR_INFO("Already disconnected\n");
		} else {
			PR_WARNING("Disconnect request failed: %d\n", status);
			return -ENOEXEC;
		}
	} else {
		PR("Disconnect requested\n");
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
	static const struct option long_options[] = {{"type", required_argument, 0, 't'},
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
			if (!strncasecmp(state->optarg, "passive", 7)) {
				params->scan_type = WIFI_SCAN_TYPE_PASSIVE;
			} else if (!strncasecmp(state->optarg, "active", 6)) {
				params->scan_type = WIFI_SCAN_TYPE_ACTIVE;
			} else {
				PR_ERROR("Invalid scan type %s\n", state->optarg);
				return -ENOEXEC;
			}

			opt_num++;
			break;
		case 'b':
			if (wifi_utils_parse_scan_bands(state->optarg, &params->bands)) {
				PR_ERROR("Invalid band value(s)\n");
				return -ENOEXEC;
			}

			opt_num++;
			break;
		case 'a':
			val = atoi(state->optarg);

			if (val < 0) {
				PR_ERROR("Invalid dwell_time_active val\n");
				return -ENOEXEC;
			}

			params->dwell_time_active = val;
			opt_num++;
			break;
		case 'p':
			val = atoi(state->optarg);

			if (val < 0) {
				PR_ERROR("Invalid dwell_time_passive val\n");
				return -ENOEXEC;
			}

			params->dwell_time_passive = val;
			opt_num++;
			break;
		case 's':
			if (wifi_utils_parse_scan_ssids(state->optarg,
							params->ssids,
							ARRAY_SIZE(params->ssids))) {
				PR_ERROR("Invalid SSID(s)\n");
				return -ENOEXEC;
			}

			opt_num++;
			break;
		case 'm':
			val = atoi(state->optarg);

			if ((val < 0) || (val > WIFI_MGMT_SCAN_MAX_BSS_CNT)) {
				PR_ERROR("Invalid max_bss val\n");
				return -ENOEXEC;
			}

			params->max_bss_cnt = val;
			opt_num++;
			break;
		case 'c':
			if (wifi_utils_parse_scan_chan(state->optarg,
						       params->band_chan,
						       ARRAY_SIZE(params->band_chan))) {
				PR_ERROR("Invalid band or channel value(s)\n");
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
			PR_ERROR("Invalid option or option usage: %s\n",
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
			PR_WARNING("No valid option(s) found\n");
			do_scan = false;
		}
	}

	if (do_scan) {
		if (net_mgmt(NET_REQUEST_WIFI_SCAN, iface, &params, sizeof(params))) {
			PR_WARNING("Scan request failed\n");
			return -ENOEXEC;
		}

		PR("Scan requested\n");

		return 0;
	}

	PR_WARNING("Scan not initiated\n");
	return -ENOEXEC;
}

static int cmd_wifi_status(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_first_wifi();
	struct wifi_iface_status status = { 0 };

	context.sh = sh;

	if (net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, iface, &status,
		     sizeof(struct wifi_iface_status))) {
		PR_WARNING("Status request failed\n");

		return -ENOEXEC;
	}

	PR("Status: successful\n");
	PR("==================\n");
	PR("State: %s\n", wifi_state_txt(status.state));

	if (status.state >= WIFI_STATE_ASSOCIATED) {
		uint8_t mac_string_buf[sizeof("xx:xx:xx:xx:xx:xx")];

		PR("Interface Mode: %s\n", wifi_mode_txt(status.iface_mode));
		PR("Link Mode: %s\n", wifi_link_mode_txt(status.link_mode));
		PR("SSID: %.32s\n", status.ssid);
		PR("BSSID: %s\n",
		   net_sprint_ll_addr_buf(status.bssid,
					  WIFI_MAC_ADDR_LEN, mac_string_buf,
					  sizeof(mac_string_buf)));
		PR("Band: %s\n", wifi_band_txt(status.band));
		PR("Channel: %d\n", status.channel);
		PR("Security: %s\n", wifi_security_txt(status.security));
		PR("MFP: %s\n", wifi_mfp_txt(status.mfp));
		if (status.iface_mode == WIFI_MODE_INFRA) {
			PR("RSSI: %d\n", status.rssi);
		}
		PR("Beacon Interval: %d\n", status.beacon_interval);
		PR("DTIM: %d\n", status.dtim_period);
		PR("TWT: %s\n",
		   status.twt_capable ? "Supported" : "Not supported");
	}

	return 0;
}

#if defined(CONFIG_NET_STATISTICS_WIFI) && \
					defined(CONFIG_NET_STATISTICS_USER_API)
static void print_wifi_stats(struct net_if *iface, struct net_stats_wifi *data,
			     const struct shell *sh)
{
	PR("Statistics for Wi-Fi interface %p [%d]\n", iface,
	   net_if_get_by_iface(iface));

	PR("Bytes received   : %u\n", data->bytes.received);
	PR("Bytes sent       : %u\n", data->bytes.sent);
	PR("Packets received : %u\n", data->pkts.rx);
	PR("Packets sent     : %u\n", data->pkts.tx);
	PR("Receive errors   : %u\n", data->errors.rx);
	PR("Send errors      : %u\n", data->errors.tx);
	PR("Bcast received   : %u\n", data->broadcast.rx);
	PR("Bcast sent       : %u\n", data->broadcast.tx);
	PR("Mcast received   : %u\n", data->multicast.rx);
	PR("Mcast sent       : %u\n", data->multicast.tx);
	PR("Beacons received : %u\n", data->sta_mgmt.beacons_rx);
	PR("Beacons missed   : %u\n", data->sta_mgmt.beacons_miss);
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

	PR_INFO("Set %s to enable %s support.\n",
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
		PR_WARNING("Invalid number of arguments\n");
		return -ENOEXEC;
	}

	if (argc == 1) {
		struct wifi_ps_config config = { 0 };

		if (net_mgmt(NET_REQUEST_WIFI_PS_CONFIG, iface,
				&config, sizeof(config))) {
			PR_WARNING("Failed to get PS config\n");
			return -ENOEXEC;
		}

		PR("PS status: %s\n",
		   wifi_ps_txt(config.ps_params.enabled));
		if (config.ps_params.enabled) {
			PR("PS mode: %s\n",
			   wifi_ps_mode_txt(config.ps_params.mode));
		}

		PR("PS listen_interval: %d\n",
		   config.ps_params.listen_interval);

		PR("PS wake up mode: %s\n",
		   config.ps_params.wakeup_mode ? "Listen interval" : "DTIM");

		if (config.ps_params.timeout_ms) {
			PR("PS timeout: %d ms\n",
			   config.ps_params.timeout_ms);
		} else {
			PR("PS timeout: disabled\n");
		}

		if (config.num_twt_flows == 0) {
			PR("No TWT flows\n");
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
				PR("TWT Wake ahead duration : %d us\n",
				   config.twt_flows[i].twt_wake_ahead_duration);
			}
		}
		return 0;
	}

	if (!strncasecmp(argv[1], "on", 2)) {
		params.enabled = WIFI_PS_ENABLED;
	} else if (!strncasecmp(argv[1], "off", 3)) {
		params.enabled = WIFI_PS_DISABLED;
	} else {
		PR_WARNING("Invalid argument\n");
		return -ENOEXEC;
	}

	params.type = WIFI_PS_PARAM_STATE;

	if (net_mgmt(NET_REQUEST_WIFI_PS, iface, &params, sizeof(params))) {
		PR_WARNING("PS %s failed. Reason: %s\n",
			   params.enabled ? "enable" : "disable",
			   wifi_ps_get_config_err_code_str(params.fail_reason));
		return -ENOEXEC;
	}

	PR("%s\n", wifi_ps_txt(params.enabled));

	return 0;
}

static int cmd_wifi_ps_mode(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_first_wifi();
	struct wifi_ps_params params = { 0 };

	context.sh = sh;

	if (!strncasecmp(argv[1], "legacy", 6)) {
		params.mode = WIFI_PS_MODE_LEGACY;
	} else if (!strncasecmp(argv[1], "WMM", 3)) {
		params.mode = WIFI_PS_MODE_WMM;
	} else {
		PR_WARNING("Invalid PS mode\n");
		return -ENOEXEC;
	}

	params.type = WIFI_PS_PARAM_MODE;

	if (net_mgmt(NET_REQUEST_WIFI_PS, iface, &params, sizeof(params))) {
		PR_WARNING("%s failed Reason : %s\n",
			   wifi_ps_mode_txt(params.mode),
			   wifi_ps_get_config_err_code_str(params.fail_reason));
		return -ENOEXEC;
	}

	PR("%s\n", wifi_ps_mode_txt(params.mode));

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
		PR_WARNING("Setting PS timeout failed. Reason : %s\n",
			   wifi_ps_get_config_err_code_str(params.fail_reason));
		return -ENOEXEC;
	}

	if (params.timeout_ms) {
		PR("PS timeout: %d ms\n", params.timeout_ms);
	} else {
		PR("PS timeout: disabled\n");
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
		PR_WARNING("%s with %s failed, reason : %s\n",
			   wifi_twt_operation_txt(params.operation),
			   wifi_twt_negotiation_type_txt(params.negotiation_type),
			   wifi_twt_get_err_code_str(params.fail_reason));

		return -ENOEXEC;
	}

	PR("TWT operation %s with dg: %d, flow_id: %d requested\n",
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

	if (!parse_number(sh, &value, argv[idx++], 0, WIFI_MAX_TWT_WAKE_AHEAD_DURATION_US)) {
		return -EINVAL;
	}
	params.setup.twt_wake_ahead_duration = (uint32_t)value;

	if (net_mgmt(NET_REQUEST_WIFI_TWT, iface, &params, sizeof(params))) {
		PR_WARNING("%s with %s failed. reason : %s\n",
			   wifi_twt_operation_txt(params.operation),
			   wifi_twt_negotiation_type_txt(params.negotiation_type),
			   wifi_twt_get_err_code_str(params.fail_reason));

		return -ENOEXEC;
	}

	PR("TWT operation %s with dg: %d, flow_id: %d requested\n",
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
		PR_WARNING("%s with %s failed, reason : %s\n",
			   wifi_twt_operation_txt(params.operation),
			   wifi_twt_negotiation_type_txt(params.negotiation_type),
			   wifi_twt_get_err_code_str(params.fail_reason));

		return -ENOEXEC;
	}

	PR("TWT operation %s with dg: %d, flow_id: %d success\n",
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
		PR_WARNING("%s with %s failed, reason : %s\n",
			   wifi_twt_operation_txt(params.operation),
			   wifi_twt_negotiation_type_txt(params.negotiation_type),
			   wifi_twt_get_err_code_str(params.fail_reason));

		return -ENOEXEC;
	}

	PR("TWT operation %s all flows success\n",
	   wifi_twt_operation_txt(params.operation));

	return 0;
}

static int cmd_wifi_ap_enable(const struct shell *sh, size_t argc,
			      char *argv[])
{
	struct net_if *iface = net_if_get_first_wifi();
	static struct wifi_connect_req_params cnx_params;
	int ret;

	context.sh = sh;
	if (__wifi_args_to_params(sh, argc, &argv[0], &cnx_params, WIFI_MODE_AP)) {
		shell_help(sh);
		return -ENOEXEC;
	}

	k_mutex_init(&wifi_ap_sta_list_lock);

	ret = net_mgmt(NET_REQUEST_WIFI_AP_ENABLE, iface, &cnx_params,
		       sizeof(struct wifi_connect_req_params));
	if (ret) {
		PR_WARNING("AP mode enable failed: %s\n", strerror(-ret));
		return -ENOEXEC;
	}

	PR("AP mode enable requested\n");

	return 0;
}

static int cmd_wifi_ap_disable(const struct shell *sh, size_t argc,
			       char *argv[])
{
	struct net_if *iface = net_if_get_first_wifi();
	int ret;

	ret = net_mgmt(NET_REQUEST_WIFI_AP_DISABLE, iface, NULL, 0);
	if (ret) {
		PR_WARNING("AP mode disable failed: %s\n", strerror(-ret));
		return -ENOEXEC;
	}

	PR("AP mode disable requested\n");
	return 0;
}

static int cmd_wifi_ap_stations(const struct shell *sh, size_t argc,
				char *argv[])
{
	size_t id = 1;

	ARG_UNUSED(argv);
	ARG_UNUSED(argc);

	PR("AP stations:\n");
	PR("============\n");

	k_mutex_lock(&wifi_ap_sta_list_lock, K_FOREVER);
	for (int i = 0; i < CONFIG_WIFI_SHELL_MAX_AP_STA; i++) {
		struct wifi_ap_sta_info *sta;
		uint8_t mac_string_buf[sizeof("xx:xx:xx:xx:xx:xx")];

		if (!sta_list[i].valid) {
			continue;
		}

		sta = &sta_list[i].sta_info;

		PR("Station %zu:\n", id++);
		PR("==========\n");
		PR("MAC: %s\n",
		   net_sprint_ll_addr_buf(sta->mac,
					  WIFI_MAC_ADDR_LEN,
					  mac_string_buf,
					  sizeof(mac_string_buf)));
		PR("Link mode: %s\n",
		   wifi_link_mode_txt(sta->link_mode));
		PR("TWT: %s\n",
		   sta->twt_capable ? "Supported" : "Not supported");
	}

	if (id == 1) {
		PR("No stations connected\n");
	}
	k_mutex_unlock(&wifi_ap_sta_list_lock);

	return 0;
}

static int cmd_wifi_ap_sta_disconnect(const struct shell *sh, size_t argc,
				      char *argv[])
{
	struct net_if *iface = net_if_get_first_wifi();
	uint8_t mac[6];
	int ret;

	if (net_bytes_from_str(mac, sizeof(mac), argv[1]) < 0) {
		PR_WARNING("Invalid MAC address\n");
		return -ENOEXEC;
	}

	ret = net_mgmt(NET_REQUEST_WIFI_AP_STA_DISCONNECT, iface, mac, sizeof(mac));
	if (ret) {
		PR_WARNING("AP station disconnect failed: %s\n",
			   strerror(-ret));
		return -ENOEXEC;
	}

	PR("AP station disconnect requested\n");
	return 0;
}

static int cmd_wifi_reg_domain(const struct shell *sh, size_t argc,
			       char *argv[])
{
	struct net_if *iface = net_if_get_first_wifi();
	struct wifi_reg_domain regd = {0};
	int ret, chan_idx = 0;

	if (argc == 1) {
		regd.chan_info = &chan_info[0];
		regd.oper = WIFI_MGMT_GET;
	} else if (argc >= 2 && argc <= 3) {
		regd.oper = WIFI_MGMT_SET;
		if (strlen(argv[1]) != 2) {
			PR_WARNING("Invalid reg domain: Length should be two letters/digits\n");
			return -ENOEXEC;
		}

		/* Two letter country code with special case of 00 for WORLD */
		if (((argv[1][0] < 'A' || argv[1][0] > 'Z') ||
			(argv[1][1] < 'A' || argv[1][1] > 'Z')) &&
			(argv[1][0] != '0' || argv[1][1] != '0')) {
			PR_WARNING("Invalid reg domain %c%c\n", argv[1][0], argv[1][1]);
			return -ENOEXEC;
		}
		regd.country_code[0] = argv[1][0];
		regd.country_code[1] = argv[1][1];

		if (argc == 3) {
			if (strncmp(argv[2], "-f", 2) == 0) {
				regd.force = true;
			} else {
				PR_WARNING("Invalid option %s\n", argv[2]);
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
		PR_WARNING("Cannot %s Regulatory domain: %d\n",
			   regd.oper == WIFI_MGMT_GET ? "get" : "set", ret);
		return -ENOEXEC;
	}

	if (regd.oper == WIFI_MGMT_GET) {
		PR("Wi-Fi Regulatory domain is: %c%c\n",
		   regd.country_code[0], regd.country_code[1]);
		PR("<channel>\t<center frequency>\t<supported(y/n)>\t"
		   "<max power(dBm)>\t<passive transmission only(y/n)>\t<DFS supported(y/n)>\n");
		for (chan_idx = 0; chan_idx < regd.num_channels; chan_idx++) {
			PR("  %d\t\t\t%d\t\t\t%s\t\t\t%d\t\t\t%s\t\t\t\t%s\n",
			   wifi_freq_to_channel(chan_info[chan_idx].center_frequency),
			   chan_info[chan_idx].center_frequency,
			   chan_info[chan_idx].supported ? "y" : "n",
			   chan_info[chan_idx].max_power,
			   chan_info[chan_idx].passive_only ? "y" : "n",
			   chan_info[chan_idx].dfs ? "y" : "n");
		}
	} else {
		PR("Wi-Fi Regulatory domain set to: %c%c\n",
		   regd.country_code[0], regd.country_code[1]);
	}

	return 0;
}

static int cmd_wifi_listen_interval(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_first_wifi();
	struct wifi_ps_params params = { 0 };
	long interval;

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
			PR_WARNING("Setting listen interval failed. Reason :%s\n",
				   wifi_ps_get_config_err_code_str(params.fail_reason));
			PR_WARNING("Hardware support valid range : 3 - 65535\n");
		} else {
			PR_WARNING("Setting listen interval failed. Reason :%s\n",
				   wifi_ps_get_config_err_code_str(params.fail_reason));
		}
		return -ENOEXEC;
	}

	PR("Listen interval %hu\n", params.listen_interval);

	return 0;
}

static int cmd_wifi_ps_wakeup_mode(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_first_wifi();
	struct wifi_ps_params params = { 0 };

	context.sh = sh;

	if (!strncasecmp(argv[1], "dtim", 4)) {
		params.wakeup_mode = WIFI_PS_WAKEUP_MODE_DTIM;
	} else if (!strncasecmp(argv[1], "listen_interval", 15)) {
		params.wakeup_mode = WIFI_PS_WAKEUP_MODE_LISTEN_INTERVAL;
	} else {
		PR_WARNING("Invalid argument\n");
		PR_INFO("Valid argument : <dtim> / <listen_interval>\n");
		return -ENOEXEC;
	}

	params.type = WIFI_PS_PARAM_WAKEUP_MODE;

	if (net_mgmt(NET_REQUEST_WIFI_PS, iface, &params, sizeof(params))) {
		PR_WARNING("Setting PS wake up mode to %s failed..Reason :%s\n",
			   params.wakeup_mode ? "Listen interval" : "DTIM interval",
			   wifi_ps_get_config_err_code_str(params.fail_reason));
		return -ENOEXEC;
	}

	PR("%s\n", wifi_ps_wakeup_mode_txt(params.wakeup_mode));

	return 0;
}

static int cmd_wifi_set_rts_threshold(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_first_wifi();
	unsigned int rts_threshold = -1; /* Default value if user supplies "off" argument */
	int err = 0;

	context.sh = sh;

	if (strcmp(argv[1], "off") != 0) {
		long rts_val = shell_strtol(argv[1], 10, &err);

		if (err) {
			shell_error(sh, "Unable to parse input (err %d)", err);
			return err;
		}

		rts_threshold = (unsigned int)rts_val;
	}

	if (net_mgmt(NET_REQUEST_WIFI_RTS_THRESHOLD, iface,
		     &rts_threshold, sizeof(rts_threshold))) {
		shell_fprintf(sh, SHELL_WARNING,
			      "Setting RTS threshold failed.\n");
		return -ENOEXEC;
	}

	if ((int)rts_threshold >= 0)
		shell_fprintf(sh, SHELL_NORMAL, "RTS threshold: %d\n", rts_threshold);
	else
		shell_fprintf(sh, SHELL_NORMAL, "RTS threshold is off\n");

	return 0;
}

void parse_mode_args_to_params(const struct shell *sh, int argc,
			       char *argv[], struct wifi_mode_info *mode,
			       bool *do_mode_oper)
{
	int opt;
	int option_index = 0;
	struct getopt_state *state;

	static const struct option long_options[] = {{"if-index", optional_argument, 0, 'i'},
					       {"sta", no_argument, 0, 's'},
					       {"monitor", no_argument, 0, 'm'},
					       {"ap", no_argument, 0, 'a'},
					       {"softap", no_argument, 0, 'k'},
					       {"get", no_argument, 0, 'g'},
					       {"help", no_argument, 0, 'h'},
					       {0, 0, 0, 0}};

	while ((opt = getopt_long(argc, argv, "i:smtpakgh", long_options, &option_index)) != -1) {
		state = getopt_state_get();
		switch (opt) {
		case 's':
			mode->mode |= WIFI_STA_MODE;
			break;
		case 'm':
			mode->mode |= WIFI_MONITOR_MODE;
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
			mode->if_index = (uint8_t)atoi(state->optarg);
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
		PR_ERROR("Invalid number of arguments\n");
		return -EINVAL;
	}

	if (do_mode_oper) {
		/* Check interface index value. Mode validation must be performed by
		 * lower layer
		 */
		if (mode_info.if_index == 0) {
			iface = net_if_get_first_wifi();
			if (iface == NULL) {
				PR_ERROR("Cannot find the default wifi interface\n");
				return -ENOEXEC;
			}
			mode_info.if_index = net_if_get_by_iface(iface);
		} else {
			iface = net_if_get_by_index(mode_info.if_index);
			if (iface == NULL) {
				PR_ERROR("Cannot find interface for if_index %d\n",
					 mode_info.if_index);
				return -ENOEXEC;
			}
		}

		ret = net_mgmt(NET_REQUEST_WIFI_MODE, iface, &mode_info, sizeof(mode_info));

		if (ret) {
			PR_ERROR("mode %s operation failed with reason %d\n",
				 mode_info.oper == WIFI_MGMT_GET ? "get" : "set", ret);
			return -ENOEXEC;
		}

		if (mode_info.oper == WIFI_MGMT_GET) {
			PR("Wi-Fi current mode is %x\n", mode_info.mode);
		} else {
			PR("Wi-Fi mode set to %x\n", mode_info.mode);
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
	struct getopt_state *state;

	static const struct option long_options[] = {{"if-index", optional_argument, 0, 'i'},
					       {"channel", required_argument, 0, 'c'},
					       {"get", no_argument, 0, 'g'},
					       {"help", no_argument, 0, 'h'},
					       {0, 0, 0, 0}};

	while ((opt = getopt_long(argc, argv, "i:c:gh", long_options, &option_index)) != -1) {
		state = getopt_state_get();
		switch (opt) {
		case 'c':
			channel->channel = (uint16_t)atoi(state->optarg);
			break;
		case 'i':
			channel->if_index = (uint8_t)atoi(state->optarg);
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

	channel_info.oper = WIFI_MGMT_SET;
	parse_channel_args_to_params(sh, argc, argv, &channel_info, &do_channel_oper);

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
				PR_ERROR("Cannot find the default wifi interface\n");
				return -ENOEXEC;
			}
			channel_info.if_index = net_if_get_by_iface(iface);
		} else {
			iface = net_if_get_by_index(channel_info.if_index);
			if (iface == NULL) {
				PR_ERROR("Cannot find interface for if_index %d\n",
					 channel_info.if_index);
				return -ENOEXEC;
			}
		}

		if (channel_info.oper == WIFI_MGMT_SET) {
			if ((channel_info.channel < WIFI_CHANNEL_MIN) ||
			    (channel_info.channel > WIFI_CHANNEL_MAX)) {
				PR_ERROR("Invalid channel number. Range is (1-233)\n");
				return -ENOEXEC;
			}
		}

		ret = net_mgmt(NET_REQUEST_WIFI_CHANNEL, iface, &channel_info,
			       sizeof(channel_info));

		if (ret) {
			PR_ERROR("channel %s operation failed with reason %d\n",
				 channel_info.oper == WIFI_MGMT_GET ? "get" : "set", ret);
			return -ENOEXEC;
		}

		if (channel_info.oper == WIFI_MGMT_GET) {
			PR("Wi-Fi current channel is: %d\n", channel_info.channel);
		} else {
			PR("Wi-Fi channel set to %d\n", channel_info.channel);
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
	struct getopt_state *state;

	static const struct option long_options[] = {{"if-index", optional_argument, 0, 'i'},
					       {"capture-len", optional_argument, 0, 'b'},
					       {"all", no_argument, 0, 'a'},
					       {"mgmt", no_argument, 0, 'm'},
					       {"ctrl", no_argument, 0, 'c'},
					       {"data", no_argument, 0, 'd'},
					       {"get", no_argument, 0, 'g'},
					       {"help", no_argument, 0, 'h'},
					       {0, 0, 0, 0}};

	while ((opt = getopt_long(argc, argv, "i:b:amcdgh", long_options, &option_index)) != -1) {
		state = getopt_state_get();
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
			filter->if_index = (uint8_t)atoi(state->optarg);
			break;
		case 'b':
			filter->buffer_size = (uint16_t)atoi(state->optarg);
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

	packet_filter.oper = WIFI_MGMT_SET;
	parse_filter_args_to_params(sh, argc, argv, &packet_filter, &do_filter_oper);

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
				PR_ERROR("Cannot find the default wifi interface\n");
				return -ENOEXEC;
			}
			packet_filter.if_index = net_if_get_by_iface(iface);
		} else {
			iface = net_if_get_by_index(packet_filter.if_index);
			if (iface == NULL) {
				PR_ERROR("Cannot find interface for if_index %d\n",
					 packet_filter.if_index);
				return -ENOEXEC;
			}
		}

		ret = net_mgmt(NET_REQUEST_WIFI_PACKET_FILTER, iface, &packet_filter,
			       sizeof(packet_filter));

		if (ret) {
			PR_ERROR("Wi-Fi packet filter %s operation failed with reason %d\n",
				 packet_filter.oper == WIFI_MGMT_GET ? "get" : "set", ret);
			return -ENOEXEC;
		}

		if (packet_filter.oper == WIFI_MGMT_GET) {
			PR("Wi-Fi current mode packet filter is %d\n",
			   packet_filter.filter);
		} else {
			PR("Wi-Fi mode packet filter set to %d\n",
			   packet_filter.filter);
		}
	}
	return 0;
}

static int cmd_wifi_version(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_first_wifi();
	struct wifi_version version = {0};

	if (argc > 1) {
		PR_WARNING("Invalid number of arguments\n");
		return -ENOEXEC;
	}

	if (net_mgmt(NET_REQUEST_WIFI_VERSION, iface, &version, sizeof(version))) {
		PR_WARNING("Failed to get Wi-Fi versions\n");
		return -ENOEXEC;
	}

	PR("Wi-Fi Driver Version: %s\n", version.drv_version);
	PR("Wi-Fi Firmware Version: %s\n", version.fw_version);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(wifi_cmd_ap,
	SHELL_CMD_ARG(disable, NULL,
		  "Disable Access Point mode.\n",
		  cmd_wifi_ap_disable,
		  1, 0),
	SHELL_CMD_ARG(enable, NULL,
		  "-s --ssid=<SSID>\n"
		  "-c --channel=<channel number>\n"
		  "-p --passphrase=<PSK> (valid only for secure SSIDs)\n"
		  "-k --key-mgmt=<Security type> (valid only for secure SSIDs)\n"
		  "0:None, 1:WPA2-PSK, 2:WPA2-PSK-256, 3:SAE, 4:WAPI, 5:EAP, 6:WEP, 7: WPA-PSK\n"
		  "-w --ieee-80211w=<MFP> (optional: needs security type to be specified)\n"
		  "0:Disable, 1:Optional, 2:Required\n"
		  "-b --band=<band> (2 -2.6GHz, 5 - 5Ghz, 6 - 6GHz)\n"
		  "-m --bssid=<BSSID>\n"
		  "-h --help (prints help)",
		  cmd_wifi_ap_enable,
		  2, 13),
	SHELL_CMD_ARG(stations, NULL,
		  "List stations connected to the AP",
		  cmd_wifi_ap_stations,
		  1, 0),
	SHELL_CMD_ARG(disconnect, NULL,
		  "Disconnect a station from the AP\n"
		  "<MAC address of the station>\n",
		  cmd_wifi_ap_sta_disconnect,
		  2, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(wifi_twt_ops,
	SHELL_CMD_ARG(quick_setup, NULL, " Start a TWT flow with defaults:\n"
		"<twt_wake_interval: 1-262144us> <twt_interval: 1us-2^31us>.\n",
		cmd_wifi_twt_setup_quick,
		3, 0),
	SHELL_CMD_ARG(setup, NULL, " Start a TWT flow:\n"
		"<negotiation_type, 0: Individual, 1: Broadcast, 2: Wake TBTT>\n"
		"<setup_cmd: 0: Request, 1: Suggest, 2: Demand>\n"
		"<dialog_token: 1-255> <flow_id: 0-7> <responder: 0/1> <trigger: 0/1> <implicit:0/1> "
		"<announce: 0/1> <twt_wake_interval: 1-262144us> <twt_interval: 1us-2^31us>.\n"
		"<twt_wake_ahead_duration>: 0us-2^31us>\n",
		cmd_wifi_twt_setup,
		12, 0),
	SHELL_CMD_ARG(teardown, NULL, " Teardown a TWT flow:\n"
		"<negotiation_type, 0: Individual, 1: Broadcast, 2: Wake TBTT>\n"
		"<setup_cmd: 0: Request, 1: Suggest, 2: Demand>\n"
		"<dialog_token: 1-255> <flow_id: 0-7>.\n",
		cmd_wifi_twt_teardown,
		5, 0),
	SHELL_CMD_ARG(teardown_all, NULL, " Teardown all TWT flows.\n",
		cmd_wifi_twt_teardown_all,
		1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(wifi_commands,
	SHELL_CMD_ARG(version, NULL, "Print Wi-Fi Driver and Firmware versions\n",
		  cmd_wifi_version,
		  1, 0),
	SHELL_CMD(ap, &wifi_cmd_ap, "Access Point mode commands.\n", NULL),
	SHELL_CMD_ARG(connect, NULL,
		  "Connect to a Wi-Fi AP\n"
		  "<-s --ssid \"<SSID>\">: SSID.\n"
		  "[-c --channel]: Channel that needs to be scanned for connection. 0:any channel.\n"
		  "[-b, --band] 0: any band (2:2.4GHz, 5:5GHz, 6:6GHz]\n"
		  "[-p, --psk]: Passphrase (valid only for secure SSIDs)\n"
		  "[-k, --key-mgmt]: Key Management type (valid only for secure SSIDs)\n"
		  "0:None, 1:WPA2-PSK, 2:WPA2-PSK-256, 3:SAE, 4:WAPI, 5:EAP, 6:WEP,"
		  " 7: WPA-PSK, 8: WPA-Auto-Personal\n"
		  "[-w, --ieee-80211w]: MFP (optional: needs security type to be specified)\n"
		  ": 0:Disable, 1:Optional, 2:Required.\n"
		  "[-m, --bssid]: MAC address of the AP (BSSID).\n"
		  "[-h, --help]: Print out the help for the connect command.\n",
		  cmd_wifi_connect,
		  2, 7),
	SHELL_CMD_ARG(disconnect, NULL, "Disconnect from the Wi-Fi AP.\n",
		  cmd_wifi_disconnect,
		  1, 0),
	SHELL_CMD_ARG(ps, NULL, "Configure or display Wi-Fi power save state.\n"
		  "[on/off]\n",
		  cmd_wifi_ps,
		  1, 1),
	SHELL_CMD_ARG(ps_mode,
		      NULL,
		      "<mode: legacy/WMM>.\n",
		      cmd_wifi_ps_mode,
		      2,
		      0),
	SHELL_CMD_ARG(scan, NULL,
		  "Scan for Wi-Fi APs\n"
		    "[-t, --type <active/passive>] : Preferred mode of scan. The actual mode of scan can depend on factors such as the Wi-Fi chip implementation, regulatory domain restrictions. Default type is active\n"
		    "[-b, --bands <Comma separated list of band values (2/5/6)>] : Bands to be scanned where 2: 2.4 GHz, 5: 5 GHz, 6: 6 GHz\n"
		    "[-a, --dwell_time_active <val_in_ms>] : Active scan dwell time (in ms) on a channel. Range 5 ms to 1000 ms\n"
		    "[-p, --dwell_time_passive <val_in_ms>] : Passive scan dwell time (in ms) on a channel. Range 10 ms to 1000 ms\n"
		    "[-s, --ssid] : SSID to scan for. Can be provided multiple times\n"
		    "[-m, --max_bss <val>] : Maximum BSSes to scan for. Range 1 - 65535\n"
		    "[-c, --chans <Comma separated list of channel ranges>] : Channels to be scanned. The channels must be specified in the form band1:chan1,chan2_band2:chan3,..etc. band1, band2 must be valid band values and chan1, chan2, chan3 must be specified as a list of comma separated values where each value is either a single channel or a channel range specified as chan_start-chan_end. Each band channel set has to be separated by a _. For example, a valid channel specification can be 2:1,6_5:36 or 2:1,6-11,14_5:36,163-177,52. Care should be taken to ensure that configured channels don't exceed CONFIG_WIFI_MGMT_SCAN_CHAN_MAX_MANUAL\n"
		    "[-h, --help] : Print out the help for the scan command.\n",
		  cmd_wifi_scan,
		  1, 8),
	SHELL_CMD_ARG(statistics, NULL, "Wi-Fi interface statistics.\n", cmd_wifi_stats, 1, 0),
	SHELL_CMD_ARG(status, NULL, "Status of the Wi-Fi interface.\n", cmd_wifi_status, 1, 0),
	SHELL_CMD(twt, &wifi_twt_ops, "Manage TWT flows.\n", NULL),
	SHELL_CMD_ARG(reg_domain, NULL,
		"Set or Get Wi-Fi regulatory domain\n"
		"[ISO/IEC 3166-1 alpha2]: Regulatory domain\n"
		"[-f]: Force to use this regulatory hint over any other regulatory hints\n"
		"Note: This may cause regulatory compliance issues, use it at your own risk.\n",
		cmd_wifi_reg_domain,
		1, 2),
	SHELL_CMD_ARG(mode, NULL, "mode operational setting\n"
		"This command may be used to set the Wi-Fi device into a specific mode of operation\n"
		"[-i, --if-index <idx>] : Interface index\n"
		"[-s, --sta] : Station mode\n"
		"[-m, --monitor] : Monitor mode\n"
		"[-a, --ap] : AP mode\n"
		"[-k, --softap] : Softap mode\n"
		"[-h, --help] : Help\n"
		"[-g, --get] : Get current mode for a specific interface index\n"
		"Usage: Get operation example for interface index 1\n"
		"wifi mode -g -i1\n"
		"Set operation example for interface index 1 - set station+promiscuous\n"
		"wifi mode -i1 -sp.\n",
		cmd_wifi_mode,
		1, 9),
	SHELL_CMD_ARG(packet_filter, NULL, "mode filter setting\n"
		"This command is used to set packet filter setting when\n"
		"monitor, TX-Injection and promiscuous mode is enabled\n"
		"The different packet filter modes are control, management, data and enable all filters\n"
		"[-i, --if-index <idx>] : Interface index\n"
		"[-a, --all] : Enable all packet filter modes\n"
		"[-m, --mgmt] : Enable management packets to allowed up the stack\n"
		"[-c, --ctrl] : Enable control packets to be allowed up the stack\n"
		"[-d, --data] : Enable Data packets to be allowed up the stack\n"
		"[-g, --get] : Get current filter settings for a specific interface index\n"
		"[-b, --capture-len <len>] : Capture length buffer size for each packet to be captured\n"
		"[-h, --help] : Help\n"
		"Usage: Get operation example for interface index 1\n"
		"wifi packet_filter -g -i1\n"
		"Set operation example for interface index 1 - set data+management frame filter\n"
		"wifi packet_filter -i1 -md.\n",
		cmd_wifi_packet_filter,
		2, 8),
	SHELL_CMD_ARG(channel, NULL, "wifi channel setting\n"
		"This command is used to set the channel when\n"
		"monitor or TX-Injection mode is enabled\n"
		"Currently 20 MHz is only supported and no BW parameter is provided\n"
		"[-i, --if-index <idx>] : Interface index\n"
		"[-c, --channel <chan>] : Set a specific channel number to the lower layer\n"
		"[-g, --get] : Get current set channel number from the lower layer\n"
		"[-h, --help] : Help\n"
		"Usage: Get operation example for interface index 1\n"
		"wifi channel -g -i1\n"
		"Set operation example for interface index 1 (setting channel 5)\n"
		"wifi -i1 -c5.\n",
		cmd_wifi_channel,
		2, 4),
	SHELL_CMD_ARG(ps_timeout,
		      NULL,
		      "<val> - PS inactivity timer(in ms).\n",
		      cmd_wifi_ps_timeout,
		      2,
		      0),
	SHELL_CMD_ARG(ps_listen_interval,
		      NULL,
		      "<val> - Listen interval in the range of <0-65535>.\n",
		      cmd_wifi_listen_interval,
		      2,
		      0),
	SHELL_CMD_ARG(ps_wakeup_mode,
		     NULL,
		     "<wakeup_mode: DTIM/Listen Interval>.\n",
		     cmd_wifi_ps_wakeup_mode,
		     2,
		     0),
	SHELL_CMD_ARG(rts_threshold,
		     NULL,
		     "<rts_threshold: rts threshold/off>.\n",
		     cmd_wifi_set_rts_threshold,
		     2,
		     0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(wifi, &wifi_commands, "Wi-Fi commands", NULL);

static int wifi_shell_init(void)
{

	context.sh = NULL;
	context.all = 0U;
	context.scan_result = 0U;

	net_mgmt_init_event_callback(&wifi_shell_mgmt_cb,
				     wifi_mgmt_event_handler,
				     WIFI_SHELL_MGMT_EVENTS);

	net_mgmt_add_event_callback(&wifi_shell_mgmt_cb);

	return 0;
}

SYS_INIT(wifi_shell_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
