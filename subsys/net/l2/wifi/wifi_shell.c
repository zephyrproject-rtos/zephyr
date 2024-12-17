/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright 2024 NXP
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
#include <math.h>
#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE
static const char ca_cert_test[] = {
	#include <wifi_enterprise_test_certs/ca.pem.inc>
	'\0'
};

static const char client_cert_test[] = {
	#include <wifi_enterprise_test_certs/client.pem.inc>
	'\0'
};

static const char client_key_test[] = {
	#include <wifi_enterprise_test_certs/client-key.pem.inc>
	'\0'
};

static const char ca_cert2_test[] = {
	#include <wifi_enterprise_test_certs/ca2.pem.inc>
	'\0'};

static const char client_cert2_test[] = {
	#include <wifi_enterprise_test_certs/client2.pem.inc>
	'\0'};

static const char client_key2_test[] = {
	#include <wifi_enterprise_test_certs/client-key2.pem.inc>
	'\0'};

static const char server_cert_test[] = {
	#include <wifi_enterprise_test_certs/server.pem.inc>
	'\0'
};

static const char server_key_test[] = {
	#include <wifi_enterprise_test_certs/server-key.pem.inc>
	'\0'
};
#endif

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


#if defined CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE || \
	defined CONFIG_WIFI_NM_HOSTAPD_CRYPTO_ENTERPRISE
static int cmd_wifi_set_enterprise_creds(const struct shell *sh, struct net_if *iface)
{
	struct wifi_enterprise_creds_params params = {0};

	params.ca_cert = (uint8_t *)ca_cert_test;
	params.ca_cert_len = ARRAY_SIZE(ca_cert_test);
	params.client_cert = (uint8_t *)client_cert_test;
	params.client_cert_len = ARRAY_SIZE(client_cert_test);
	params.client_key = (uint8_t *)client_key_test;
	params.client_key_len = ARRAY_SIZE(client_key_test);
	params.ca_cert2 = (uint8_t *)ca_cert2_test;
	params.ca_cert2_len = ARRAY_SIZE(ca_cert2_test);
	params.client_cert2 = (uint8_t *)client_cert2_test;
	params.client_cert2_len = ARRAY_SIZE(client_cert2_test);
	params.client_key2 = (uint8_t *)client_key2_test;
	params.client_key2_len = ARRAY_SIZE(client_key2_test);
	params.server_cert = (uint8_t *)server_cert_test;
	params.server_cert_len = ARRAY_SIZE(server_cert_test);
	params.server_key = (uint8_t *)server_key_test;
	params.server_key_len = ARRAY_SIZE(server_key_test);

	if (net_mgmt(NET_REQUEST_WIFI_ENTERPRISE_CREDS, iface, &params, sizeof(params))) {
		PR_WARNING("Set enterprise credentials failed\n");
		return -1;
	}

	return 0;
}
#endif

static bool parse_number(const struct shell *sh, long *param, char *str,
			 char *pname, long min, long max)
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
		PR_ERROR("Invalid number: %s\n", str_tmp);
		return false;
	}

	if ((num) < (min) || (num) > (max)) {
		if (pname) {
			PR_WARNING("%s value out of range: %s, (%ld-%ld)\n",
				   pname, str_tmp, min, max);
		} else {
			PR_WARNING("Value out of range: %s, (%ld-%ld)\n",
				   str_tmp, min, max);
		}
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

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_ROAMING
static void handle_wifi_signal_change(struct net_mgmt_event_callback *cb)
{
	struct net_if *iface = net_if_get_wifi_sta();
	const struct shell *sh = context.sh;
	int ret;

	ret = net_mgmt(NET_REQUEST_WIFI_START_ROAMING, iface, NULL, 0);
	if (ret) {
		PR_WARNING("Start roaming failed\n");
		return;
	}

	PR("Start roaming requested\n");
}

static void handle_wifi_neighbor_rep_complete(struct net_mgmt_event_callback *cb)
{
	struct net_if *iface = net_if_get_wifi_sta();
	const struct shell *sh = context.sh;
	int ret;

	ret = net_mgmt(NET_REQUEST_WIFI_NEIGHBOR_REP_COMPLETE, iface, NULL, 0);
	if (ret) {
		PR_WARNING("Neighbor report complete failed\n");
		return;
	}

	PR("Neighbor report complete requested\n");
}
#endif

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
#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_ROAMING
	case NET_EVENT_WIFI_SIGNAL_CHANGE:
		handle_wifi_signal_change(cb);
		break;
	case NET_EVENT_WIFI_NEIGHBOR_REP_COMP:
		handle_wifi_neighbor_rep_complete(cb);
		break;
#endif
	default:
		break;
	}
}

static int __wifi_args_to_params(const struct shell *sh, size_t argc, char *argv[],
				 struct wifi_connect_req_params *params,
				 enum wifi_iface_mode iface_mode)
{
	int opt;
	int opt_index = 0;
	struct getopt_state *state;
	static const struct option long_options[] = {
		{"ssid", required_argument, 0, 's'},
		{"passphrase", required_argument, 0, 'p'},
		{"key-mgmt", required_argument, 0, 'k'},
		{"ieee-80211w", required_argument, 0, 'w'},
		{"bssid", required_argument, 0, 'm'},
		{"band", required_argument, 0, 'b'},
		{"channel", required_argument, 0, 'c'},
		{"timeout", required_argument, 0, 't'},
		{"anon-id", required_argument, 0, 'a'},
		{"bandwidth", required_argument, 0, 'B'},
		{"key1-pwd", required_argument, 0, 'K'},
		{"key2-pwd", required_argument, 0, 'K'},
		{"suiteb-type", required_argument, 0, 'S'},
		{"TLS-cipher", required_argument, 0, 'T'},
		{"eap-version", required_argument, 0, 'V'},
		{"eap-id1", required_argument, 0, 'I'},
		{"eap-id2", required_argument, 0, 'I'},
		{"eap-id3", required_argument, 0, 'I'},
		{"eap-id4", required_argument, 0, 'I'},
		{"eap-id5", required_argument, 0, 'I'},
		{"eap-id6", required_argument, 0, 'I'},
		{"eap-id7", required_argument, 0, 'I'},
		{"eap-id8", required_argument, 0, 'I'},
		{"eap-pwd1", required_argument, 0, 'P'},
		{"eap-pwd2", required_argument, 0, 'P'},
		{"eap-pwd3", required_argument, 0, 'P'},
		{"eap-pwd4", required_argument, 0, 'P'},
		{"eap-pwd5", required_argument, 0, 'P'},
		{"eap-pwd6", required_argument, 0, 'P'},
		{"eap-pwd7", required_argument, 0, 'P'},
		{"eap-pwd8", required_argument, 0, 'P'},
		{"ignore-broadcast-ssid", required_argument, 0, 'i'},
		{"ieee-80211r", no_argument, 0, 'R'},
		{"help", no_argument, 0, 'h'},
		{0, 0, 0, 0}};
	char *endptr;
	int idx = 1;
	bool secure_connection = false;
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
	int key_passwd_cnt = 0;
	int ret = 0;

	/* Defaults */
	params->band = WIFI_FREQ_BAND_UNKNOWN;
	params->channel = WIFI_CHANNEL_ANY;
	params->security = WIFI_SECURITY_TYPE_NONE;
	params->mfp = WIFI_MFP_OPTIONAL;
	params->eap_ver = 1;
	params->ignore_broadcast_ssid = 0;
	params->bandwidth = WIFI_FREQ_BANDWIDTH_20MHZ;

	while ((opt = getopt_long(argc, argv, "s:p:k:e:w:b:c:m:t:a:B:K:S:T:V:I:P:i:Rh",
				  long_options, &opt_index)) != -1) {
		state = getopt_state_get();
		switch (opt) {
		case 's':
			params->ssid = state->optarg;
			params->ssid_length = strlen(params->ssid);
			if (params->ssid_length > WIFI_SSID_MAX_LEN) {
				PR_WARNING("SSID too long (max %d characters)\n",
					   WIFI_SSID_MAX_LEN);
				return -EINVAL;
			}
			break;
		case 'k':
			params->security = atoi(state->optarg);
			if (params->security) {
				secure_connection = true;
			}
			break;
		case 'p':
			params->psk = state->optarg;
			params->psk_length = strlen(params->psk);
			break;
		case 'c':
			channel = strtol(state->optarg, &endptr, 10);
#ifdef CONFIG_WIFI_NM_HOSTAPD_AP
			if (iface_mode == WIFI_MODE_AP && channel == 0) {
				params->channel = channel;
				break;
			}
#endif
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
			if (iface_mode == WIFI_MODE_INFRA ||
				iface_mode == WIFI_MODE_AP) {
				switch (atoi(state->optarg)) {
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
					PR_ERROR("Invalid band: %d\n", atoi(state->optarg));
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
			params->mfp = atoi(state->optarg);
			break;
		case 'm':
			if (net_bytes_from_str(params->bssid, sizeof(params->bssid),
					       state->optarg) < 0) {
				PR_WARNING("Invalid MAC address\n");
				return -EINVAL;
			}
			break;
		case 't':
			if (iface_mode == WIFI_MODE_INFRA) {
				params->timeout = strtol(state->optarg, &endptr, 10);
				if (*endptr != '\0') {
					PR_ERROR("Invalid timeout: %s\n", state->optarg);
					return -EINVAL;
				}
			}
			break;
		case 'a':
			params->anon_id = state->optarg;
			params->aid_length = strlen(params->anon_id);
			if (params->aid_length > WIFI_ENT_IDENTITY_MAX_LEN) {
				PR_WARNING("anon_id too long (max %d characters)\n",
					    WIFI_ENT_IDENTITY_MAX_LEN);
				return -EINVAL;
			}
			break;
		case 'B':
			if (iface_mode == WIFI_MODE_AP) {
				switch (atoi(state->optarg)) {
				case 1:
					params->bandwidth = WIFI_FREQ_BANDWIDTH_20MHZ;
					break;
				case 2:
					params->bandwidth = WIFI_FREQ_BANDWIDTH_40MHZ;
					break;
				case 3:
					params->bandwidth = WIFI_FREQ_BANDWIDTH_80MHZ;
					break;
				default:
					PR_ERROR("Invalid bandwidth: %d\n", atoi(state->optarg));
					return -EINVAL;
				}
			}
			break;
		case 'K':
			if (key_passwd_cnt >= 2) {
				PR_WARNING("too many key_passwd (max 2 key_passwd)\n");
				return -EINVAL;
			}

			if (key_passwd_cnt == 0) {
				params->key_passwd = state->optarg;
				params->key_passwd_length = strlen(params->key_passwd);
				if (params->key_passwd_length > WIFI_ENT_PSWD_MAX_LEN) {
					PR_WARNING("key_passwd too long (max %d characters)\n",
							WIFI_ENT_PSWD_MAX_LEN);
					return -EINVAL;
				}
			} else if (key_passwd_cnt == 1) {
				params->key2_passwd = state->optarg;
				params->key2_passwd_length = strlen(params->key2_passwd);
				if (params->key2_passwd_length > WIFI_ENT_PSWD_MAX_LEN) {
					PR_WARNING("key2_passwd too long (max %d characters)\n",
							WIFI_ENT_PSWD_MAX_LEN);
					return -EINVAL;
				}
			}
			key_passwd_cnt++;
			break;
		case 'S':
			params->suiteb_type = atoi(state->optarg);
			break;
		case 'T':
			params->TLS_cipher = atoi(state->optarg);
			break;
		case 'V':
			params->eap_ver = atoi(state->optarg);
			if (params->eap_ver != 0U && params->eap_ver != 1U) {
				PR_WARNING("eap_ver error %d\n", params->eap_ver);
				return -EINVAL;
			}
			break;
		case 'I':
			if (params->nusers >= WIFI_ENT_IDENTITY_MAX_USERS) {
				PR_WARNING("too many eap identities (max %d identities)\n",
					    WIFI_ENT_IDENTITY_MAX_USERS);
				return -EINVAL;
			}

			params->eap_identity = state->optarg;
			params->eap_id_length = strlen(params->eap_identity);

			params->identities[params->nusers] = state->optarg;
			params->nusers++;
			if (params->eap_id_length > WIFI_ENT_IDENTITY_MAX_LEN) {
				PR_WARNING("eap identity too long (max %d characters)\n",
					    WIFI_ENT_IDENTITY_MAX_LEN);
				return -EINVAL;
			}
			break;
		case 'P':
			if (params->passwds >= WIFI_ENT_IDENTITY_MAX_USERS) {
				PR_WARNING("too many eap passwds (max %d passwds)\n",
					    WIFI_ENT_IDENTITY_MAX_USERS);
				return -EINVAL;
			}

			params->eap_password = state->optarg;
			params->eap_passwd_length = strlen(params->eap_password);

			params->passwords[params->passwds] = state->optarg;
			params->passwds++;
			if (params->eap_passwd_length > WIFI_ENT_PSWD_MAX_LEN) {
				PR_WARNING("eap password length too long (max %d characters)\n",
					    WIFI_ENT_PSWD_MAX_LEN);
				return -EINVAL;
			}
			break;
		case 'R':
			params->ft_used = true;
			break;
		case 'i':
			params->ignore_broadcast_ssid = shell_strtol(state->optarg, 10, &ret);
			break;
		case 'h':
			return -ENOEXEC;
		default:
			PR_ERROR("Invalid option %c\n", state->optopt);
			return -EINVAL;
		}

		if (ret) {
			PR_ERROR("Invalid argument %d ret %d\n", opt_index, ret);
			return -EINVAL;
		}
	}

	if (params->psk && !secure_connection) {
		PR_WARNING("Passphrase provided without security configuration\n");
	}

	if (!params->ssid) {
		PR_ERROR("SSID not provided\n");
		return -EINVAL;
	}

	if (iface_mode == WIFI_MODE_AP && params->channel == WIFI_CHANNEL_ANY) {
		PR_ERROR("Channel not provided\n");
		return -EINVAL;
	}

#ifdef CONFIG_WIFI_NM_HOSTAPD_AP
	if (iface_mode == WIFI_MODE_AP) {
		if (params->channel == 0 && params->band == WIFI_FREQ_BAND_UNKNOWN) {
			PR_ERROR("Band not provided when channel is 0\n");
			return -EINVAL;
		}

		if (params->channel > 0 && params->channel <= 14 &&
		    (params->band != WIFI_FREQ_BAND_2_4_GHZ &&
		     params->band != WIFI_FREQ_BAND_UNKNOWN)) {
			PR_ERROR("Band and channel mismatch\n");
			return -EINVAL;
		}

		if (params->channel >= 36 &&
		    (params->band != WIFI_FREQ_BAND_5_GHZ &&
		     params->band != WIFI_FREQ_BAND_UNKNOWN)) {
			PR_ERROR("Band and channel mismatch\n");
			return -EINVAL;
		}
	}
#endif
	if (params->ignore_broadcast_ssid > 2) {
		PR_ERROR("Invalid ignore_broadcast_ssid value\n");
		return -EINVAL;
	}

	return 0;
}

static int cmd_wifi_connect(const struct shell *sh, size_t argc,
			    char *argv[])
{
	struct net_if *iface = net_if_get_wifi_sta();
	struct wifi_connect_req_params cnx_params = { 0 };
	int ret;

	context.sh = sh;
	if (__wifi_args_to_params(sh, argc, argv, &cnx_params, WIFI_MODE_INFRA)) {
		shell_help(sh);
		return -ENOEXEC;
	}

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE
	/* Load the enterprise credentials if needed */
	if (cnx_params.security == WIFI_SECURITY_TYPE_EAP_TLS ||
	    cnx_params.security == WIFI_SECURITY_TYPE_EAP_PEAP_MSCHAPV2 ||
	    cnx_params.security == WIFI_SECURITY_TYPE_EAP_PEAP_GTC ||
	    cnx_params.security == WIFI_SECURITY_TYPE_EAP_TTLS_MSCHAPV2 ||
	    cnx_params.security == WIFI_SECURITY_TYPE_EAP_PEAP_TLS ||
	    cnx_params.security == WIFI_SECURITY_TYPE_EAP_TLS_SHA256) {
		cmd_wifi_set_enterprise_creds(sh, iface);
	}
#endif

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
	struct net_if *iface = net_if_get_wifi_sta();
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
	int opt;
	int opt_index = 0;
	struct getopt_state *state;
	static const struct option long_options[] = {
		{"type", required_argument, 0, 't'},
		{"bands", required_argument, 0, 'b'},
		{"dwell_time_active", required_argument, 0, 'a'},
		{"dwell_time_passive", required_argument, 0, 'p'},
		{"ssid", required_argument, 0, 's'},
		{"max_bss", required_argument, 0, 'm'},
		{"chans", required_argument, 0, 'c'},
		{"help", no_argument, 0, 'h'},
		{0, 0, 0, 0}};
	int val;
	int opt_num = 0;

	*do_scan = true;

	while ((opt = getopt_long(argc, argv, "t:b:a:p:s:m:c:h",
				  long_options, &opt_index)) != -1) {
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
	struct net_if *iface = net_if_get_wifi_sta();
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
	struct net_if *iface = net_if_get_wifi_sta();
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
		PR("Current PHY TX rate (Mbps) : %d.%03d\n",
		   status.current_phy_tx_rate / 1000,
		   status.current_phy_tx_rate % 1000);
	}

	return 0;
}

static int cmd_wifi_ap_status(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_wifi_sap();
	struct wifi_iface_status status = {0};
	uint8_t mac_string_buf[sizeof("xx:xx:xx:xx:xx:xx")];

	context.sh = sh;

	if (net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, iface, &status,
		     sizeof(struct wifi_iface_status))) {
		PR_WARNING("Status request failed\n");

		return -ENOEXEC;
	}

	switch (status.state) {
	case WIFI_SAP_IFACE_UNINITIALIZED:
		PR("State: %s\n", "UNINITIALIZED");
		return 0;
	case WIFI_SAP_IFACE_DISABLED:
		PR("State: %s\n", "DISABLED");
		return 0;
	case WIFI_SAP_IFACE_COUNTRY_UPDATE:
		PR("State: %s\n", "COUNTRY_UPDATE");
		return 0;
	case WIFI_SAP_IFACE_ACS:
		PR("State: %s\n", "ACS");
		return 0;
	case WIFI_SAP_IFACE_HT_SCAN:
		PR("State: %s\n", "HT_SCAN");
		return 0;
	case WIFI_SAP_IFACE_DFS:
		PR("State: %s\n", "DFS");
		break;
	case WIFI_SAP_IFACE_NO_IR:
		PR("State: %s\n", "NO_IR");
		break;
	case WIFI_SAP_IFACE_ENABLED:
		PR("State: %s\n", "ENABLED");
		break;
	default:
		return 0;
	}

	PR("Interface Mode: %s\n", wifi_mode_txt(status.iface_mode));
	PR("Link Mode: %s\n", wifi_link_mode_txt(status.link_mode));
	PR("SSID: %.32s\n", status.ssid);
	PR("BSSID: %s\n", net_sprint_ll_addr_buf(status.bssid, WIFI_MAC_ADDR_LEN, mac_string_buf,
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
	PR("TWT: %s\n", status.twt_capable ? "Supported" : "Not supported");

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
	PR("Unicast received : %u\n", data->unicast.rx);
	PR("Unicast sent     : %u\n", data->unicast.tx);
	PR("Overrun count    : %u\n", data->overrun_count);
}
#endif /* CONFIG_NET_STATISTICS_WIFI && CONFIG_NET_STATISTICS_USER_API */

static int cmd_wifi_stats(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_STATISTICS_WIFI) && \
					defined(CONFIG_NET_STATISTICS_USER_API)
	struct net_if *iface = net_if_get_wifi_sta();
	struct net_stats_wifi stats = { 0 };
	int ret;

	context.sh = sh;

	if (argc == 1) {
		ret = net_mgmt(NET_REQUEST_STATS_GET_WIFI, iface,
			&stats, sizeof(stats));
		if (!ret) {
			print_wifi_stats(iface, &stats, sh);
		}
	}

	if (argc > 1) {
		if (!strncasecmp(argv[1], "reset", 5)) {
			ret = net_mgmt(NET_REQUEST_STATS_RESET_WIFI, iface,
					&stats, sizeof(stats));
			if (!ret) {
				PR("Wi-Fi interface statistics have been reset.\n");
			}
		} else if (!strncasecmp(argv[1], "help", 4)) {
			shell_help(sh);
		} else {
			PR_WARNING("Invalid argument\n");
			shell_help(sh);
			return -ENOEXEC;
		}
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

static int cmd_wifi_11k(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_wifi_sta();
	struct wifi_11k_params params = { 0 };

	context.sh = sh;

	if (argc > 2) {
		PR_WARNING("Invalid number of arguments\n");
		return -ENOEXEC;
	}

	if (argc == 1) {
		params.oper = WIFI_MGMT_GET;
	} else {
		params.oper = WIFI_MGMT_SET;
		if (!strncasecmp(argv[1], "enable", 2)) {
			params.enable_11k = true;
		} else if (!strncasecmp(argv[1], "disable", 3)) {
			params.enable_11k = false;
		} else {
			PR_WARNING("Invalid argument\n");
			return -ENOEXEC;
		}
	}

	if (net_mgmt(NET_REQUEST_WIFI_11K_CONFIG, iface, &params, sizeof(params))) {
		PR_WARNING("11k enable/disable failed\n");
		return -ENOEXEC;
	}

	if (params.oper == WIFI_MGMT_GET) {
		PR("11k is %s\n", params.enable_11k ? "enabled" : "disabled");
	} else {
		PR("%s %s requested\n", argv[0], argv[1]);
	}

	return 0;
}


static int cmd_wifi_11k_neighbor_request(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_wifi_sta();
	struct wifi_11k_params params = { 0 };

	context.sh = sh;

	if ((argc != 1 && argc != 3) || (argc == 3 && !strncasecmp("ssid", argv[1], 4))) {
		PR_WARNING("Invalid input arguments\n");
		PR_WARNING("Usage: %s\n", argv[0]);
		PR_WARNING("or	 %s ssid <ssid>\n", argv[0]);
		return -ENOEXEC;
	}

	if (argc == 3) {
		if (strlen(argv[2]) > (sizeof(params.ssid) - 1)) {
			PR_WARNING("Error: ssid too long\n");
			return -ENOEXEC;
		}
		(void)memcpy((void *)params.ssid, (const void *)argv[2],
			     (size_t)strlen(argv[2]));
	}

	if (net_mgmt(NET_REQUEST_WIFI_11K_NEIGHBOR_REQUEST, iface, &params, sizeof(params))) {
		PR_WARNING("11k neighbor request failed\n");
		return -ENOEXEC;
	}

	if (argc == 3) {
		PR("%s %s %s requested\n", argv[0], argv[1], argv[2]);
	} else {
		PR("%s requested\n", argv[0]);
	}

	return 0;
}

static int cmd_wifi_ps(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_wifi_sta();
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

		shell_fprintf(sh, SHELL_NORMAL, "PS exit strategy: %s\n",
				wifi_ps_exit_strategy_txt(config.ps_params.exit_strategy));

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
	struct net_if *iface = net_if_get_wifi_sta();
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
	struct net_if *iface = net_if_get_wifi_sta();
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
	struct net_if *iface = net_if_get_wifi_sta();
	struct wifi_twt_params params = { 0 };
	int idx = 1;
	long value;
	double twt_mantissa_scale = 0.0;
	double twt_interval_scale = 0.0;
	uint16_t scale = 1000;
	int exponent = 0;

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

	if (!parse_number(sh, &value, argv[idx++], NULL, 1, WIFI_MAX_TWT_WAKE_INTERVAL_US)) {
		return -EINVAL;
	}
	params.setup.twt_wake_interval = (uint32_t)value;

	if (!parse_number(sh, &value, argv[idx++], NULL, 1, WIFI_MAX_TWT_INTERVAL_US)) {
		return -EINVAL;
	}
	params.setup.twt_interval = (uint64_t)value;

	/* control the region of mantissa filed */
	twt_interval_scale = (double)(params.setup.twt_interval / scale);
	/* derive mantissa and exponent from interval */
	twt_mantissa_scale = frexp(twt_interval_scale, &exponent);
	params.setup.twt_mantissa = ceil(twt_mantissa_scale * scale);
	params.setup.twt_exponent = exponent;

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

static int cmd_wifi_btwt_setup(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_wifi_sap();
	struct wifi_twt_params params = {0};
	int idx = 1;
	long value;
	int ret = 0;

	context.sh = sh;

	params.btwt.sub_id = (uint16_t)shell_strtol(argv[idx++], 10, &ret);
	params.btwt.nominal_wake = (uint8_t)shell_strtol(argv[idx++], 10, &ret);
	params.btwt.max_sta_support = (uint8_t)shell_strtol(argv[idx++], 10, &ret);

	if (!parse_number(sh, &value, argv[idx++], NULL, 1, 0xFFFF)) {
		return -EINVAL;
	}
	params.btwt.twt_mantissa = (uint16_t)value;

	params.btwt.twt_offset = (uint16_t)shell_strtol(argv[idx++], 10, &ret);

	if (!parse_number(sh, &value, argv[idx++], NULL, 0, WIFI_MAX_TWT_EXPONENT)) {
		return -EINVAL;
	}
	params.btwt.twt_exponent = (uint8_t)value;

	params.btwt.sp_gap = (uint8_t)shell_strtol(argv[idx++], 10, &ret);

	if (ret) {
		PR_ERROR("Invalid argument (ret %d)\n", ret);
		return -EINVAL;
	}

	if (net_mgmt(NET_REQUEST_WIFI_BTWT, iface, &params, sizeof(params))) {
		PR_WARNING("Failed reason : %s\n",
			   wifi_twt_get_err_code_str(params.fail_reason));

		return -ENOEXEC;
	}

	PR("BTWT setup\n");

	return 0;
}

static int twt_args_to_params(const struct shell *sh, size_t argc, char *argv[],
				 struct wifi_twt_params *params)
{
	int opt;
	int opt_index = 0;
	struct getopt_state *state;
	long value;
	double twt_mantissa_scale = 0.0;
	double twt_interval_scale = 0.0;
	uint16_t scale = 1000;
	int exponent = 0;
	static const struct option long_options[] = {
		{"negotiation-type", required_argument, 0, 'n'},
		{"setup-cmd", required_argument, 0, 'c'},
		{"dialog-token", required_argument, 0, 't'},
		{"flow-id", required_argument, 0, 'f'},
		{"responder", required_argument, 0, 'r'},
		{"trigger", required_argument, 0, 'T'},
		{"implicit", required_argument, 0, 'I'},
		{"announce", required_argument, 0, 'a'},
		{"wake-interval", required_argument, 0, 'w'},
		{"interval", required_argument, 0, 'i'},
		{"wake-ahead-duration", required_argument, 0, 'D'},
		{"info-disable", required_argument, 0, 'd'},
		{"exponent", required_argument, 0, 'e'},
		{"mantissa", required_argument, 0, 'm'},
		{"help", no_argument, 0, 'h'},
		{0, 0, 0, 0}};

	params->operation = WIFI_TWT_SETUP;

	while ((opt = getopt_long(argc, argv, "n:c:t:f:r:T:I:a:t:w:i:D:d:e:m:h",
				  long_options, &opt_index)) != -1) {
		state = getopt_state_get();
		switch (opt) {
		case 'n':
			if (!parse_number(sh, &value, state->optarg, NULL,
					  WIFI_TWT_INDIVIDUAL,
					  WIFI_TWT_WAKE_TBTT)) {
				return -EINVAL;
			}
			params->negotiation_type = (enum wifi_twt_negotiation_type)value;
			break;

		case 'c':
			if (!parse_number(sh, &value, state->optarg, NULL,
					  WIFI_TWT_SETUP_CMD_REQUEST,
					  WIFI_TWT_SETUP_CMD_DEMAND)) {
				return -EINVAL;
			}
			params->setup_cmd = (enum wifi_twt_setup_cmd)value;
			break;

		case 't':
			if (!parse_number(sh, &value, state->optarg, NULL, 1, 255)) {
				return -EINVAL;
			}
			params->dialog_token = (uint8_t)value;
			break;

		case 'f':
			if (!parse_number(sh, &value, state->optarg, NULL, 0,
					  (WIFI_MAX_TWT_FLOWS - 1))) {
				return -EINVAL;
			}
			params->flow_id = (uint8_t)value;
			break;

		case 'r':
			if (!parse_number(sh, &value, state->optarg, NULL, 0, 1)) {
				return -EINVAL;
			}
			params->setup.responder = (bool)value;
			break;

		case 'T':
			if (!parse_number(sh, &value, state->optarg, NULL, 0, 1)) {
				return -EINVAL;
			}
			params->setup.trigger = (bool)value;
			break;

		case 'I':
			if (!parse_number(sh, &value, state->optarg, NULL, 0, 1)) {
				return -EINVAL;
			}
			params->setup.implicit = (bool)value;
			break;

		case 'a':
			if (!parse_number(sh, &value, state->optarg, NULL, 0, 1)) {
				return -EINVAL;
			}
			params->setup.announce = (bool)value;
			break;

		case 'w':
			if (!parse_number(sh, &value, state->optarg, NULL, 1,
					  WIFI_MAX_TWT_WAKE_INTERVAL_US)) {
				return -EINVAL;
			}
			params->setup.twt_wake_interval = (uint32_t)value;
			break;

		case 'i':
			if (!parse_number(sh, &value, state->optarg, NULL, 1,
					  WIFI_MAX_TWT_INTERVAL_US)) {
				return -EINVAL;
			}
			params->setup.twt_interval = (uint64_t)value;
			break;

		case 'D':
			if (!parse_number(sh, &value, state->optarg, NULL, 0,
					  WIFI_MAX_TWT_WAKE_AHEAD_DURATION_US)) {
				return -EINVAL;
			}
			params->setup.twt_wake_ahead_duration = (uint32_t)value;
			break;

		case 'd':
			if (!parse_number(sh, &value, state->optarg, NULL, 0, 1)) {
				return -EINVAL;
			}
			params->setup.twt_info_disable = (bool)value;
			break;

		case 'e':
			if (!parse_number(sh, &value, state->optarg, NULL, 0,
					  WIFI_MAX_TWT_EXPONENT)) {
				return -EINVAL;
			}
			params->setup.twt_exponent = (uint8_t)value;
			break;

		case 'm':
			if (!parse_number(sh, &value, state->optarg, NULL, 0, 0xFFFF)) {
				return -EINVAL;
			}
			params->setup.twt_mantissa = (uint16_t)value;
			break;

		case 'h':
			return -ENOEXEC;
		}
	}

	if ((params->setup.twt_interval != 0) &&
	   ((params->setup.twt_exponent != 0) ||
	   (params->setup.twt_mantissa != 0))) {
		PR_ERROR("Only one of TWT internal or (mantissa, exponent) should be used\n");
		return -EINVAL;
	}

	if (params->setup.twt_interval) {
		/* control the region of mantissa filed */
		twt_interval_scale = (double)(params->setup.twt_interval / scale);
		/* derive mantissa and exponent from interval */
		twt_mantissa_scale = frexp(twt_interval_scale, &exponent);
		params->setup.twt_mantissa = ceil(twt_mantissa_scale * scale);
		params->setup.twt_exponent = exponent;
	} else if ((params->setup.twt_exponent != 0) ||
		   (params->setup.twt_mantissa != 0)) {
		params->setup.twt_interval = floor(ldexp(params->setup.twt_mantissa,
							 params->setup.twt_exponent));
	} else {
		PR_ERROR("Either TWT interval or (mantissa, exponent) is needed\n");
		return -EINVAL;
	}

	return 0;
}

static int cmd_wifi_twt_setup(const struct shell *sh, size_t argc,
			      char *argv[])
{
	struct net_if *iface = net_if_get_wifi_sta();
	struct wifi_twt_params params = { 0 };

	context.sh = sh;

	if (twt_args_to_params(sh, argc, argv, &params)) {
		shell_help(sh);
		return -ENOEXEC;
	}

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
	struct net_if *iface = net_if_get_wifi_sta();
	struct wifi_twt_params params = { 0 };
	long value;

	context.sh = sh;
	int idx = 1;

	params.operation = WIFI_TWT_TEARDOWN;

	if (!parse_number(sh, &value, argv[idx++], NULL, WIFI_TWT_INDIVIDUAL,
			  WIFI_TWT_WAKE_TBTT)) {
		return -EINVAL;
	}
	params.negotiation_type = (enum wifi_twt_negotiation_type)value;

	if (!parse_number(sh, &value, argv[idx++], NULL, WIFI_TWT_SETUP_CMD_REQUEST,
			  WIFI_TWT_SETUP_CMD_DEMAND)) {
		return -EINVAL;
	}
	params.setup_cmd = (enum wifi_twt_setup_cmd)value;

	if (!parse_number(sh, &value, argv[idx++], NULL, 1, 255)) {
		return -EINVAL;
	}
	params.dialog_token = (uint8_t)value;

	if (!parse_number(sh, &value, argv[idx++], NULL, 0, (WIFI_MAX_TWT_FLOWS - 1))) {
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
	struct net_if *iface = net_if_get_wifi_sta();
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
	struct net_if *iface = net_if_get_wifi_sap();
	struct wifi_connect_req_params cnx_params = {0};
	int ret;

	context.sh = sh;
	if (__wifi_args_to_params(sh, argc, &argv[0], &cnx_params, WIFI_MODE_AP)) {
		shell_help(sh);
		return -ENOEXEC;
	}

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE
	/* Load the enterprise credentials if needed */
	if (cnx_params.security == WIFI_SECURITY_TYPE_EAP_TLS ||
	    cnx_params.security == WIFI_SECURITY_TYPE_EAP_PEAP_MSCHAPV2 ||
	    cnx_params.security == WIFI_SECURITY_TYPE_EAP_PEAP_GTC ||
	    cnx_params.security == WIFI_SECURITY_TYPE_EAP_TTLS_MSCHAPV2 ||
	    cnx_params.security == WIFI_SECURITY_TYPE_EAP_PEAP_TLS ||
	    cnx_params.security == WIFI_SECURITY_TYPE_EAP_TLS_SHA256) {
		cmd_wifi_set_enterprise_creds(sh, iface);
	}
#endif

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
	struct net_if *iface = net_if_get_wifi_sap();
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
	struct net_if *iface = net_if_get_wifi_sap();
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

static int wifi_ap_config_args_to_params(const struct shell *sh, size_t argc, char *argv[],
					 struct wifi_ap_config_params *params)
{
	int opt;
	int opt_index = 0;
	struct getopt_state *state;
	static const struct option long_options[] = {
		{"max_inactivity", required_argument, 0, 'i'},
		{"max_num_sta", required_argument, 0, 's'},
#if defined(CONFIG_WIFI_NM_HOSTAPD_AP)
		{"ht_capab", required_argument, 0, 'n'},
		{"vht_capab", required_argument, 0, 'c'},
#endif
		{"help", no_argument, 0, 'h'},
		{0, 0, 0, 0}};
	long val;

	while ((opt = getopt_long(argc, argv, "i:s:n:c:h",
				  long_options, &opt_index)) != -1) {
		state = getopt_state_get();
		switch (opt) {
		case 'i':
			if (!parse_number(sh, &val, state->optarg, "max_inactivity",
					  0, WIFI_AP_STA_MAX_INACTIVITY)) {
				return -EINVAL;
			}
			params->max_inactivity = (uint32_t)val;
			params->type |= WIFI_AP_CONFIG_PARAM_MAX_INACTIVITY;
			break;
		case 's':
			if (!parse_number(sh, &val, state->optarg, "max_num_sta",
					  0, CONFIG_WIFI_MGMT_AP_MAX_NUM_STA)) {
				return -EINVAL;
			}
			params->max_num_sta = (uint32_t)val;
			params->type |= WIFI_AP_CONFIG_PARAM_MAX_NUM_STA;
			break;
#if defined(CONFIG_WIFI_NM_HOSTAPD_AP)
		case 'n':
			strncpy(params->ht_capab, state->optarg, WIFI_AP_IEEE_80211_CAPAB_MAX_LEN);
			params->type |= WIFI_AP_CONFIG_PARAM_HT_CAPAB;
			break;
		case 'c':
			strncpy(params->vht_capab, state->optarg, WIFI_AP_IEEE_80211_CAPAB_MAX_LEN);
			params->type |= WIFI_AP_CONFIG_PARAM_VHT_CAPAB;
			break;
#endif
		case 'h':
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		default:
			PR_ERROR("Invalid option %c\n", state->optopt);
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
	}

	return 0;
}

static int cmd_wifi_ap_config_params(const struct shell *sh, size_t argc,
				     char *argv[])
{
	struct net_if *iface = net_if_get_wifi_sap();
	struct wifi_ap_config_params ap_config_params = { 0 };
	int ret = -1;

	context.sh = sh;

	if (wifi_ap_config_args_to_params(sh, argc, argv, &ap_config_params)) {
		return -ENOEXEC;
	}

	ret = net_mgmt(NET_REQUEST_WIFI_AP_CONFIG_PARAM, iface,
		       &ap_config_params, sizeof(struct wifi_ap_config_params));
	if (ret) {
		PR_WARNING("Setting AP parameter failed: %s\n",
			   strerror(-ret));
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_wifi_ap_set_rts_threshold(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_wifi_sap();
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

	if (net_mgmt(NET_REQUEST_WIFI_AP_RTS_THRESHOLD, iface,
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

static int cmd_wifi_reg_domain(const struct shell *sh, size_t argc,
			       char *argv[])
{
	struct net_if *iface = net_if_get_wifi_sta();
	struct wifi_reg_domain regd = {0};
	int ret, chan_idx = 0;
	int opt;
	bool force = false;
	bool verbose = false;
	int opt_index = 0;
	static const struct option long_options[] = {
		{"force", no_argument, 0, 'f'},
		{"verbose", no_argument, 0, 'v'},
		{NULL, 0, NULL, 0}
	};

	while ((opt = getopt_long(argc, argv, "fv", long_options, &opt_index)) != -1) {
		switch (opt) {
		case 'f':
			force = true;
			break;
		case 'v':
			verbose = true;
			break;
		default:
			return -ENOEXEC;
		}
	}

	if (optind == argc) {
		regd.chan_info = &chan_info[0];
		regd.oper = WIFI_MGMT_GET;
	} else if (optind == argc - 1) {
		if (strlen(argv[optind]) != 2) {
			PR_WARNING("Invalid reg domain: Length should be two letters/digits\n");
			return -ENOEXEC;
		}

		/* Two letter country code with special case of 00 for WORLD */
		if (((argv[optind][0] < 'A' || argv[optind][0] > 'Z') ||
			(argv[optind][1] < 'A' || argv[optind][1] > 'Z')) &&
			(argv[optind][0] != '0' || argv[optind][1] != '0')) {
			PR_WARNING("Invalid reg domain %c%c\n", argv[optind][0], argv[optind][1]);
			return -ENOEXEC;
		}
		regd.country_code[0] = argv[optind][0];
		regd.country_code[1] = argv[optind][1];
		regd.force = force;
		regd.oper = WIFI_MGMT_SET;
	} else {
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
		if (!verbose) {
			return 0;
		}
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
	struct net_if *iface = net_if_get_wifi_sta();
	struct wifi_ps_params params = { 0 };
	long interval;

	context.sh = sh;

	if (!parse_number(sh, &interval, argv[1], NULL,
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
		} else {
			PR_WARNING("Setting listen interval failed. Reason :%s\n",
				   wifi_ps_get_config_err_code_str(params.fail_reason));
		}
		return -ENOEXEC;
	}

	PR("Listen interval %hu\n", params.listen_interval);

	return 0;
}

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_WNM
static int cmd_wifi_btm_query(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_wifi_sta();
	uint8_t query_reason = 0;
	long tmp = 0;

	context.sh = sh;

	if (!parse_number(sh, &tmp, argv[1], NULL,
			  WIFI_BTM_QUERY_REASON_UNSPECIFIED, WIFI_BTM_QUERY_REASON_LEAVING_ESS)) {
		return -EINVAL;
	}

	query_reason = tmp;

	if (net_mgmt(NET_REQUEST_WIFI_BTM_QUERY, iface, &query_reason, sizeof(query_reason))) {
		PR_WARNING("Setting BTM query Reason failed. Reason : %d\n", query_reason);
		return -ENOEXEC;
	}

	PR("Query reason %d\n", query_reason);

	return 0;
}
#endif

static int cmd_wifi_wps_pbc(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_wifi_sta();
	struct wifi_wps_config_params params = {0};

	context.sh = sh;

	if (argc == 1) {
		params.oper = WIFI_WPS_PBC;
	} else {
		shell_help(sh);
		return -ENOEXEC;
	}

	if (net_mgmt(NET_REQUEST_WIFI_WPS_CONFIG, iface, &params, sizeof(params))) {
		PR_WARNING("Start wps pbc connection failed\n");
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_wifi_wps_pin(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_wifi_sta();
	struct wifi_wps_config_params params = {0};

	context.sh = sh;

	if (argc == 1) {
		params.oper = WIFI_WPS_PIN_GET;
	} else if (argc == 2) {
		params.oper = WIFI_WPS_PIN_SET;
		strncpy(params.pin, argv[1], WIFI_WPS_PIN_MAX_LEN);
	} else {
		shell_help(sh);
		return -ENOEXEC;
	}

	if (net_mgmt(NET_REQUEST_WIFI_WPS_CONFIG, iface, &params, sizeof(params))) {
		PR_WARNING("Start wps pin connection failed\n");
		return -ENOEXEC;
	}

	if (params.oper == WIFI_WPS_PIN_GET) {
		PR("WPS PIN is: %s\n", params.pin);
	}

	return 0;
}

static int cmd_wifi_ap_wps_pbc(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_wifi_sap();
	struct wifi_wps_config_params params = {0};

	context.sh = sh;

	if (argc == 1) {
		params.oper = WIFI_WPS_PBC;
	} else {
		shell_help(sh);
		return -ENOEXEC;
	}

	if (net_mgmt(NET_REQUEST_WIFI_WPS_CONFIG, iface, &params, sizeof(params))) {
		PR_WARNING("Start AP WPS PBC failed\n");
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_wifi_ap_wps_pin(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_wifi_sap();
	struct wifi_wps_config_params params = {0};

	context.sh = sh;

	if (argc == 1) {
		params.oper = WIFI_WPS_PIN_GET;
	} else if (argc == 2) {
		params.oper = WIFI_WPS_PIN_SET;
		strncpy(params.pin, argv[1], WIFI_WPS_PIN_MAX_LEN);
	} else {
		shell_help(sh);
		return -ENOEXEC;
	}

	if (net_mgmt(NET_REQUEST_WIFI_WPS_CONFIG, iface, &params, sizeof(params))) {
		PR_WARNING("Start AP WPS PIN failed\n");
		return -ENOEXEC;
	}

	if (params.oper == WIFI_WPS_PIN_GET) {
		PR("WPS PIN is: %s\n", params.pin);
	}

	return 0;
}

static int cmd_wifi_ps_wakeup_mode(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_wifi_sta();
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
	struct net_if *iface = net_if_get_wifi_sta();
	unsigned int rts_threshold = -1; /* Default value if user supplies "off" argument */
	int err = 0;

	context.sh = sh;

	if (argc > 2) {
		PR_WARNING("Invalid number of arguments\n");
		return -ENOEXEC;
	}

	if (argc == 1) {
		if (net_mgmt(NET_REQUEST_WIFI_RTS_THRESHOLD_CONFIG, iface,
			     &rts_threshold, sizeof(rts_threshold))) {
			PR_WARNING("Failed to get rts_threshold\n");
			return -ENOEXEC;
		}

		if ((int)rts_threshold < 0) {
			PR("RTS threshold is off\n");
		} else {
			PR("RTS threshold: %d\n", rts_threshold);
		}
	}

	if (argc == 2) {
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

		if ((int)rts_threshold >= 0) {
			shell_fprintf(sh, SHELL_NORMAL, "RTS threshold: %d\n", rts_threshold);
		} else {
			shell_fprintf(sh, SHELL_NORMAL, "RTS threshold is off\n");
		}
	}

	return 0;
}

static int cmd_wifi_ps_exit_strategy(const struct shell *sh, size_t argc,
			    char *argv[])
{
	struct net_if *iface = net_if_get_wifi_sta();
	struct wifi_ps_params params = { 0 };

	context.sh = sh;

	if (!strncmp(argv[1], "tim", 3)) {
		params.exit_strategy = WIFI_PS_EXIT_EVERY_TIM;
	} else if (!strncmp(argv[1], "custom", 6)) {
		params.exit_strategy = WIFI_PS_EXIT_CUSTOM_ALGO;
	} else {
		shell_fprintf(sh, SHELL_WARNING, "Invalid argument\n");
		shell_fprintf(sh, SHELL_INFO, "Valid argument : <tim> / <custom>\n");
		return -ENOEXEC;
	}

	params.type = WIFI_PS_PARAM_EXIT_STRATEGY;

	if (net_mgmt(NET_REQUEST_WIFI_PS, iface, &params, sizeof(params))) {
		shell_fprintf(sh, SHELL_WARNING,
			      "Setting PS exit strategy to %s failed..Reason :%s\n",
			      wifi_ps_exit_strategy_txt(params.exit_strategy),
			      wifi_ps_get_config_err_code_str(params.fail_reason));
		return -ENOEXEC;
	}

	shell_fprintf(sh, SHELL_NORMAL, "%s\n",
		      wifi_ps_exit_strategy_txt(params.exit_strategy));

	return 0;
}

void parse_mode_args_to_params(const struct shell *sh, int argc,
			       char *argv[], struct wifi_mode_info *mode,
			       bool *do_mode_oper)
{
	int opt;
	int opt_index = 0;
	struct getopt_state *state;
	static const struct option long_options[] = {
		{"if-index", optional_argument, 0, 'i'},
		{"sta", no_argument, 0, 's'},
		{"monitor", no_argument, 0, 'm'},
		{"ap", no_argument, 0, 'a'},
		{"softap", no_argument, 0, 'k'},
		{"get", no_argument, 0, 'g'},
		{"help", no_argument, 0, 'h'},
		{0, 0, 0, 0}};

	while ((opt = getopt_long(argc, argv, "i:smtpakgh",
				  long_options, &opt_index)) != -1) {
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
			iface = net_if_get_wifi_sta();
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
	int opt_index = 0;
	struct getopt_state *state;
	static const struct option long_options[] = {
		{"if-index", optional_argument, 0, 'i'},
		{"channel", required_argument, 0, 'c'},
		{"get", no_argument, 0, 'g'},
		{"help", no_argument, 0, 'h'},
		{0, 0, 0, 0}};

	while ((opt = getopt_long(argc, argv, "i:c:gh",
				  long_options, &opt_index)) != -1) {
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
			iface = net_if_get_wifi_sta();
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
	int opt_index = 0;
	struct getopt_state *state;
	static const struct option long_options[] = {
		{"if-index", optional_argument, 0, 'i'},
		{"capture-len", optional_argument, 0, 'b'},
		{"all", no_argument, 0, 'a'},
		{"mgmt", no_argument, 0, 'm'},
		{"ctrl", no_argument, 0, 'c'},
		{"data", no_argument, 0, 'd'},
		{"get", no_argument, 0, 'g'},
		{"help", no_argument, 0, 'h'},
		{0, 0, 0, 0}};

	while ((opt = getopt_long(argc, argv, "i:b:amcdgh",
				  long_options, &opt_index)) != -1) {
		state = getopt_state_get();
		switch (opt) {
		case 'a':
			filter->filter |= WIFI_PACKET_FILTER_ALL;
			break;
		case 'm':
			filter->filter |= WIFI_PACKET_FILTER_MGMT;
			break;
		case 'c':
			filter->filter |= WIFI_PACKET_FILTER_CTRL;
			break;
		case 'd':
			filter->filter |= WIFI_PACKET_FILTER_DATA;
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
			iface = net_if_get_wifi_sta();
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
	struct net_if *iface = net_if_get_wifi_sta();
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

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_DPP
static int parse_dpp_args_auth_init(const struct shell *sh, size_t argc, char *argv[],
				    struct wifi_dpp_params *params)
{
	int opt;
	int opt_index = 0;
	struct getopt_state *state;
	static const struct option long_options[] = {
		{"peer", required_argument, 0, 'p'},
		{"role", required_argument, 0, 'r'},
		{"configurator", required_argument, 0, 'c'},
		{"mode", required_argument, 0, 'm'},
		{"ssid", required_argument, 0, 's'},
		{0, 0, 0, 0}};
	int ret = 0;

	while ((opt = getopt_long(argc, argv, "p:r:c:m:s:",
				  long_options, &opt_index)) != -1) {
		state = getopt_state_get();
		switch (opt) {
		case 'p':
			params->auth_init.peer = shell_strtol(state->optarg, 10, &ret);
			break;
		case 'r':
			params->auth_init.role = shell_strtol(state->optarg, 10, &ret);
			break;
		case 'c':
			params->auth_init.configurator = shell_strtol(state->optarg, 10, &ret);
			break;
		case 'm':
			params->auth_init.conf = shell_strtol(state->optarg, 10, &ret);
			break;
		case 's':
			strncpy(params->auth_init.ssid, state->optarg, WIFI_SSID_MAX_LEN);
			break;
		default:
			PR_ERROR("Invalid option %c\n", state->optopt);
			return -EINVAL;
		}

		if (ret) {
			PR_ERROR("Invalid argument %d ret %d\n", opt_index, ret);
			return -EINVAL;
		}
	}

	return 0;
}

static int parse_dpp_args_chirp(const struct shell *sh, size_t argc, char *argv[],
				struct wifi_dpp_params *params)
{
	int opt;
	int opt_index = 0;
	struct getopt_state *state;
	static const struct option long_options[] = {
		{"own", required_argument, 0, 'i'},
		{"freq", required_argument, 0, 'f'},
		{0, 0, 0, 0}};
	int ret = 0;

	while ((opt = getopt_long(argc, argv, "i:f:",
				  long_options, &opt_index)) != -1) {
		state = getopt_state_get();
		switch (opt) {
		case 'i':
			params->chirp.id = shell_strtol(state->optarg, 10, &ret);
			break;
		case 'f':
			params->chirp.freq = shell_strtol(state->optarg, 10, &ret);
			break;
		default:
			PR_ERROR("Invalid option %c\n", state->optopt);
			return -EINVAL;
		}

		if (ret) {
			PR_ERROR("Invalid argument %d ret %d\n", opt_index, ret);
			return -EINVAL;
		}
	}

	return 0;
}

static int parse_dpp_args_listen(const struct shell *sh, size_t argc, char *argv[],
				 struct wifi_dpp_params *params)
{
	int opt;
	int opt_index = 0;
	struct getopt_state *state;
	static const struct option long_options[] = {
		{"role", required_argument, 0, 'r'},
		{"freq", required_argument, 0, 'f'},
		{0, 0, 0, 0}};
	int ret = 0;

	while ((opt = getopt_long(argc, argv, "r:f:",
				  long_options, &opt_index)) != -1) {
		state = getopt_state_get();
		switch (opt) {
		case 'r':
			params->listen.role = shell_strtol(state->optarg, 10, &ret);
			break;
		case 'f':
			params->listen.freq = shell_strtol(state->optarg, 10, &ret);
			break;
		default:
			PR_ERROR("Invalid option %c\n", state->optopt);
			return -EINVAL;
		}

		if (ret) {
			PR_ERROR("Invalid argument %d ret %d\n", opt_index, ret);
			return -EINVAL;
		}
	}

	return 0;
}

static int parse_dpp_args_btstrap_gen(const struct shell *sh, size_t argc, char *argv[],
				      struct wifi_dpp_params *params)
{
	int opt;
	int opt_index = 0;
	struct getopt_state *state;
	static const struct option long_options[] = {
		{"type", required_argument, 0, 't'},
		{"opclass", required_argument, 0, 'o'},
		{"channel", required_argument, 0, 'h'},
		{"mac", required_argument, 0, 'a'},
		{0, 0, 0, 0}};
	int ret = 0;

	while ((opt = getopt_long(argc, argv, "t:o:h:a:",
				  long_options, &opt_index)) != -1) {
		state = getopt_state_get();
		switch (opt) {
		case 't':
			params->bootstrap_gen.type = shell_strtol(state->optarg, 10, &ret);
			break;
		case 'o':
			params->bootstrap_gen.op_class = shell_strtol(state->optarg, 10, &ret);
			break;
		case 'h':
			params->bootstrap_gen.chan = shell_strtol(state->optarg, 10, &ret);
			break;
		case 'a':
			ret = net_bytes_from_str(params->bootstrap_gen.mac,
						 WIFI_MAC_ADDR_LEN, state->optarg);
			break;
		default:
			PR_ERROR("Invalid option %c\n", state->optopt);
			return -EINVAL;
		}

		if (ret) {
			PR_ERROR("Invalid argument %d ret %d\n", opt_index, ret);
			return -EINVAL;
		}
	}

	/* DPP bootstrap type currently only support qr_code */
	if (params->bootstrap_gen.type == 0) {
		params->bootstrap_gen.type = WIFI_DPP_BOOTSTRAP_TYPE_QRCODE;
	}

	if (params->bootstrap_gen.type != WIFI_DPP_BOOTSTRAP_TYPE_QRCODE) {
		PR_ERROR("DPP bootstrap type currently only support qr_code\n");
		return -ENOTSUP;
	}

	/* operating class should be set alongside with channel */
	if ((params->bootstrap_gen.op_class && !params->bootstrap_gen.chan) ||
	    (!params->bootstrap_gen.op_class && params->bootstrap_gen.chan)) {
		PR_ERROR("Operating class should be set alongside with channel\n");
		return -EINVAL;
	}

	return 0;
}

static int parse_dpp_args_set_config_param(const struct shell *sh, size_t argc, char *argv[],
					   struct wifi_dpp_params *params)
{
	int opt;
	int opt_index = 0;
	struct getopt_state *state;
	static const struct option long_options[] = {
		{"configurator", required_argument, 0, 'c'},
		{"mode", required_argument, 0, 'm'},
		{"ssid", required_argument, 0, 's'},
		{0, 0, 0, 0}};
	int ret = 0;

	while ((opt = getopt_long(argc, argv, "p:r:c:m:s:",
				  long_options, &opt_index)) != -1) {
		state = getopt_state_get();
		switch (opt) {
		case 'c':
			params->configurator_set.configurator =
				shell_strtol(state->optarg, 10, &ret);
			break;
		case 'm':
			params->configurator_set.conf = shell_strtol(state->optarg, 10, &ret);
			break;
		case 's':
			strncpy(params->configurator_set.ssid, state->optarg, WIFI_SSID_MAX_LEN);
			break;
		default:
			PR_ERROR("Invalid option %c\n", state->optopt);
			return -EINVAL;
		}

		if (ret) {
			PR_ERROR("Invalid argument %d ret %d\n", opt_index, ret);
			return -EINVAL;
		}
	}

	return 0;
}

static int cmd_wifi_dpp_configurator_add(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_wifi_sta();
	struct wifi_dpp_params params = {0};

	params.action = WIFI_DPP_CONFIGURATOR_ADD;

	if (net_mgmt(NET_REQUEST_WIFI_DPP, iface, &params, sizeof(params))) {
		PR_WARNING("Failed to request DPP action\n");
		return -ENOEXEC;
	}
	return 0;
}

static int cmd_wifi_dpp_auth_init(const struct shell *sh, size_t argc, char *argv[])
{
	int ret;
	struct net_if *iface = net_if_get_wifi_sta();
	struct wifi_dpp_params params = {0};

	params.action = WIFI_DPP_AUTH_INIT;

	ret = parse_dpp_args_auth_init(sh, argc, argv, &params);
	if (ret) {
		PR_ERROR("parse DPP args fail\n");
		return -EINVAL;
	}

	if (net_mgmt(NET_REQUEST_WIFI_DPP, iface, &params, sizeof(params))) {
		PR_WARNING("Failed to request DPP action\n");
		return -ENOEXEC;
	}
	return 0;
}

static int cmd_wifi_dpp_qr_code(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_wifi_sta();
	struct wifi_dpp_params params = {0};

	params.action = WIFI_DPP_QR_CODE;

	if (argc >= 2) {
		strncpy(params.dpp_qr_code, argv[1], WIFI_DPP_QRCODE_MAX_LEN);
	}

	if (net_mgmt(NET_REQUEST_WIFI_DPP, iface, &params, sizeof(params))) {
		PR_WARNING("Failed to request DPP action\n");
		return -ENOEXEC;
	}
	return 0;
}

static int cmd_wifi_dpp_chirp(const struct shell *sh, size_t argc, char *argv[])
{
	int ret;
	struct net_if *iface = net_if_get_wifi_sta();
	struct wifi_dpp_params params = {0};

	params.action = WIFI_DPP_CHIRP;

	ret = parse_dpp_args_chirp(sh, argc, argv, &params);
	if (ret) {
		PR_ERROR("parse DPP args fail\n");
		return -EINVAL;
	}

	if (net_mgmt(NET_REQUEST_WIFI_DPP, iface, &params, sizeof(params))) {
		PR_WARNING("Failed to request DPP action\n");
		return -ENOEXEC;
	}
	return 0;
}

static int cmd_wifi_dpp_listen(const struct shell *sh, size_t argc, char *argv[])
{
	int ret;
	struct net_if *iface = net_if_get_wifi_sta();
	struct wifi_dpp_params params = {0};

	params.action = WIFI_DPP_LISTEN;

	ret = parse_dpp_args_listen(sh, argc, argv, &params);
	if (ret) {
		PR_ERROR("parse DPP args fail\n");
		return -EINVAL;
	}

	if (net_mgmt(NET_REQUEST_WIFI_DPP, iface, &params, sizeof(params))) {
		PR_WARNING("Failed to request DPP action\n");
		return -ENOEXEC;
	}
	return 0;
}

static int cmd_wifi_dpp_btstrap_gen(const struct shell *sh, size_t argc, char *argv[])
{
	int ret;
	struct net_if *iface = net_if_get_wifi_sta();
	struct wifi_dpp_params params = {0};

	params.action = WIFI_DPP_BOOTSTRAP_GEN;

	ret = parse_dpp_args_btstrap_gen(sh, argc, argv, &params);
	if (ret) {
		PR_ERROR("parse DPP args fail\n");
		return -EINVAL;
	}

	if (net_mgmt(NET_REQUEST_WIFI_DPP, iface, &params, sizeof(params))) {
		PR_WARNING("Failed to request DPP action\n");
		return -ENOEXEC;
	}
	return 0;
}

static int cmd_wifi_dpp_btstrap_get_uri(const struct shell *sh, size_t argc, char *argv[])
{
	int ret = 0;
	struct net_if *iface = net_if_get_wifi_sta();
	struct wifi_dpp_params params = {0};

	params.action = WIFI_DPP_BOOTSTRAP_GET_URI;

	if (argc >= 2) {
		params.id = shell_strtol(argv[1], 10, &ret);
	}

	if (ret) {
		PR_ERROR("parse DPP args fail\n");
		return -EINVAL;
	}

	if (net_mgmt(NET_REQUEST_WIFI_DPP, iface, &params, sizeof(params))) {
		PR_WARNING("Failed to request DPP action\n");
		return -ENOEXEC;
	}
	return 0;
}

static int cmd_wifi_dpp_configurator_set(const struct shell *sh, size_t argc, char *argv[])
{
	int ret;
	struct net_if *iface = net_if_get_wifi_sta();
	struct wifi_dpp_params params = {0};

	params.action = WIFI_DPP_SET_CONF_PARAM;

	ret = parse_dpp_args_set_config_param(sh, argc, argv, &params);
	if (ret) {
		PR_ERROR("parse DPP args fail\n");
		return -EINVAL;
	}

	if (net_mgmt(NET_REQUEST_WIFI_DPP, iface, &params, sizeof(params))) {
		PR_WARNING("Failed to request DPP action\n");
		return -ENOEXEC;
	}
	return 0;
}

static int cmd_wifi_dpp_resp_timeout_set(const struct shell *sh, size_t argc, char *argv[])
{
	int ret = 0;
	struct net_if *iface = net_if_get_wifi_sta();
	struct wifi_dpp_params params = {0};

	params.action = WIFI_DPP_SET_WAIT_RESP_TIME;

	if (argc >= 2) {
		params.dpp_resp_wait_time = shell_strtol(argv[1], 10, &ret);
	}

	if (ret) {
		PR_ERROR("parse DPP args fail");
		return -EINVAL;
	}

	if (net_mgmt(NET_REQUEST_WIFI_DPP, iface, &params, sizeof(params))) {
		PR_WARNING("Failed to request DPP action\n");
		return -ENOEXEC;
	}
	return 0;
}

static int cmd_wifi_dpp_ap_btstrap_gen(const struct shell *sh, size_t argc, char *argv[])
{
	int ret;
	struct net_if *iface = net_if_get_wifi_sap();
	struct wifi_dpp_params params = {0};

	params.action = WIFI_DPP_BOOTSTRAP_GEN;

	ret = parse_dpp_args_btstrap_gen(sh, argc, argv, &params);
	if (ret) {
		PR_ERROR("parse DPP args fail\n");
		return -EINVAL;
	}

	if (net_mgmt(NET_REQUEST_WIFI_DPP, iface, &params, sizeof(params))) {
		PR_WARNING("Failed to request DPP action\n");
		return -ENOEXEC;
	}
	return 0;
}

static int cmd_wifi_dpp_ap_btstrap_get_uri(const struct shell *sh, size_t argc, char *argv[])
{
	int ret = 0;
	struct net_if *iface = net_if_get_wifi_sap();
	struct wifi_dpp_params params = {0};

	params.action = WIFI_DPP_BOOTSTRAP_GET_URI;

	if (argc >= 2) {
		params.id = shell_strtol(argv[1], 10, &ret);
	}

	if (ret) {
		PR_ERROR("parse DPP args fail\n");
		return -EINVAL;
	}

	if (net_mgmt(NET_REQUEST_WIFI_DPP, iface, &params, sizeof(params))) {
		PR_WARNING("Failed to request DPP action\n");
		return -ENOEXEC;
	}
	return 0;
}

static int cmd_wifi_dpp_ap_qr_code(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_wifi_sap();
	struct wifi_dpp_params params = {0};

	params.action = WIFI_DPP_QR_CODE;

	if (argc >= 2) {
		strncpy(params.dpp_qr_code, argv[1], WIFI_DPP_QRCODE_MAX_LEN);
	}

	if (net_mgmt(NET_REQUEST_WIFI_DPP, iface, &params, sizeof(params))) {
		PR_WARNING("Failed to request DPP action\n");
		return -ENOEXEC;
	}
	return 0;
}

static int cmd_wifi_dpp_ap_auth_init(const struct shell *sh, size_t argc, char *argv[])
{
	int opt;
	int opt_index = 0;
	struct getopt_state *state;
	static const struct option long_options[] = {
		{"peer", required_argument, 0, 'p'},
		{0, 0, 0, 0}};
	int ret = 0;
	struct net_if *iface = net_if_get_wifi_sap();
	struct wifi_dpp_params params = {0};

	params.action = WIFI_DPP_AUTH_INIT;

	while ((opt = getopt_long(argc, argv, "p:",
				  long_options, &opt_index)) != -1) {
		state = getopt_state_get();
		switch (opt) {
		case 'p':
			params.auth_init.peer = shell_strtol(state->optarg, 10, &ret);
			break;
		default:
			PR_ERROR("Invalid option %c\n", state->optopt);
			return -EINVAL;
		}

		if (ret) {
			PR_ERROR("Invalid argument %d ret %d\n", opt_index, ret);
			return -EINVAL;
		}
	}

	/* AP DPP auth only act as enrollee */
	params.auth_init.role = WIFI_DPP_ROLE_ENROLLEE;

	if (net_mgmt(NET_REQUEST_WIFI_DPP, iface, &params, sizeof(params))) {
		PR_WARNING("Failed to request DPP action\n");
		return -ENOEXEC;
	}
	return 0;
}

static int cmd_wifi_dpp_reconfig(const struct shell *sh, size_t argc, char *argv[])
{
	int ret = 0;
	struct net_if *iface = net_if_get_wifi_sta();
	struct wifi_dpp_params params = {0};

	params.action = WIFI_DPP_RECONFIG;

	if (argc >= 2) {
		params.network_id = shell_strtol(argv[1], 10, &ret);
	}

	if (ret) {
		PR_ERROR("parse DPP args fail\n");
		return -EINVAL;
	}

	if (net_mgmt(NET_REQUEST_WIFI_DPP, iface, &params, sizeof(params))) {
		PR_WARNING("Failed to request DPP action\n");
		return -ENOEXEC;
	}
	return 0;
}

#endif /* CONFIG_WIFI_NM_WPA_SUPPLICANT_DPP */
static int cmd_wifi_pmksa_flush(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_wifi_sta();

	context.sh = sh;

	if (net_mgmt(NET_REQUEST_WIFI_PMKSA_FLUSH, iface, NULL, 0)) {
		PR_WARNING("Flush PMKSA cache entries failed\n");
		return -ENOEXEC;
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	wifi_cmd_ap,
	SHELL_CMD_ARG(disable, NULL, "Disable Access Point mode.\n", cmd_wifi_ap_disable, 1, 0),
	SHELL_CMD_ARG(enable, NULL,
		      "-s --ssid=<SSID>\n"
		      "-c --channel=<channel number>\n"
		      "-p --passphrase=<PSK> (valid only for secure SSIDs)\n"
		      "-k --key-mgmt=<Security type> (valid only for secure SSIDs)\n"
		      "0:None, 1:WPA2-PSK, 2:WPA2-PSK-256, 3:SAE-HNP, 4:SAE-H2E, 5:SAE-AUTO, 6:WAPI,"
		      "7:EAP-TLS, 8:WEP, 9: WPA-PSK, 10: WPA-Auto-Personal, 11: DPP\n"
		      "12: EAP-PEAP-MSCHAPv2, 13: EAP-PEAP-GTC, 14: EAP-TTLS-MSCHAPv2,\n"
		      "15: EAP-PEAP-TLS, 16:EAP_TLS_SHA256\n"
		      "-w --ieee-80211w=<MFP> (optional: needs security type to be specified)\n"
		      "0:Disable, 1:Optional, 2:Required\n"
		      "-b --band=<band> (2 -2.6GHz, 5 - 5Ghz, 6 - 6GHz)\n"
		      "-m --bssid=<BSSID>\n"
		      "-i --ignore-broadcast-ssid=<type>. Hide SSID in AP mode.\n"
		      "0: disabled (default)\n"
		      "1: send empty (length=0) SSID in beacon and ignore probe request for "
		      "broadcast SSID.\n"
		      "2: clear SSID (ASCII 0), but keep the original length and ignore "
		      "probe requests for broadcast SSID.\n"
		      "[-B, --bandwidth=<bandwidth>]: 1:20MHz, 2:40MHz, 3:80MHz\n"
		      "[-K, --key1-pwd for eap phase1 or --key2-pwd for eap phase2]:\n"
		      "Private key passwd for enterprise mode. Default no password for private key.\n"
		      "[-S, --suiteb-type]: 1:suiteb, 2:suiteb-192. Default 0: not suiteb mode.\n"
		      "[-V, --eap-version]: 0 or 1. Default 1: eap version 1.\n"
		      "[-I, --eap-id1...--eap-id8]: Client Identity. Default no eap identity.\n"
		      "[-P, --eap-pwd1...--eap-pwd8]: Client Password.\n"
		      "Default no password for eap user.\n"
		      "-h --help (prints help)",
		      cmd_wifi_ap_enable, 2, 45),
	SHELL_CMD_ARG(stations, NULL, "List stations connected to the AP", cmd_wifi_ap_stations, 1,
		      0),
	SHELL_CMD_ARG(disconnect, NULL,
		      "Disconnect a station from the AP\n"
		      "<MAC address of the station>\n",
		      cmd_wifi_ap_sta_disconnect, 2, 0),
	SHELL_CMD_ARG(config, NULL,
		      "Configure AP parameters.\n"
		      "-i --max_inactivity=<time duration (in seconds)>\n"
		      "-s --max_num_sta=<maximum number of stations>\n"
#if defined(CONFIG_WIFI_NM_HOSTAPD_AP)
		      "Please refer to hostapd.conf to set the following options,\n"
		      "============ IEEE 802.11 related configuration ============\n"
		      "-n --ht_capab=<HT capabilities (string)>\n"
		      "(e.g. \"ht_capab=[HT40+]\" is that \"-n [HT40+]\")\n"
		      "-c --vht_capab=<VHT capabilities (string)>\n"
		      "(e.g. \"vht_capab=[SU-BEAMFORMEE][BF-ANTENNA-4]\" is that\n"
		      "\"-c [SU-BEAMFORMEE][BF-ANTENNA-4]\")\n"
		      "===========================================================\n"
#endif
		      "-h --help (prints help)",
		      cmd_wifi_ap_config_params, 2, 10),
	SHELL_CMD_ARG(wps_pbc, NULL,
		      "Start AP WPS PBC session.\n",
		      cmd_wifi_ap_wps_pbc, 1, 0),
	SHELL_CMD_ARG(wps_pin, NULL,
		      "Get or Set AP WPS PIN.\n"
		      "[pin] Only applicable for set.\n",
		      cmd_wifi_ap_wps_pin, 1, 1),
	SHELL_CMD_ARG(status, NULL, "Status of Wi-Fi SAP\n", cmd_wifi_ap_status, 1, 0),
	SHELL_CMD_ARG(rts_threshold,
		  NULL,
		  "<rts_threshold: rts threshold/off>.\n",
		  cmd_wifi_ap_set_rts_threshold,
		  2,
		  0),
	SHELL_SUBCMD_SET_END);

SHELL_SUBCMD_ADD((wifi), ap, &wifi_cmd_ap,
		 "Access Point mode commands.",
		 NULL,
		 0, 0);

SHELL_STATIC_SUBCMD_SET_CREATE(wifi_twt_ops,
	SHELL_CMD_ARG(quick_setup, NULL, " Start a TWT flow with defaults:\n"
		"<twt_wake_interval: 1-262144us> <twt_interval: 1us-2^31us>.\n",
		cmd_wifi_twt_setup_quick,
		3, 0),
	SHELL_CMD_ARG(setup, NULL, " Start a TWT flow:\n"
		"<-n --negotiation-type>: 0: Individual, 1: Broadcast, 2: Wake TBTT\n"
		"<-c --setup-cmd>: 0: Request, 1: Suggest, 2: Demand\n"
		"<-t --dialog-token>: 1-255\n"
		"<-f --flow-id>: 0-7\n"
		"<-r --responder>: 0/1\n"
		"<-T --trigger>: 0/1\n"
		"<-I --implicit>:0/1\n"
		"<-a --announce>: 0/1\n"
		"<-w --wake-interval>: 1-262144us\n"
		"<-i --interval>: 1us-2^31us\n"
		"<-D --wake-ahead-duration>: 0us-2^31us\n"
		"<-d --info-disable>: 0/1\n"
		"<-e --exponent>: 0-31\n"
		"<-m --mantissa>: 1-2^16\n"
		"[-h, --help]: Print out command usage.\n",
		cmd_wifi_twt_setup,
		25, 5),
	SHELL_CMD_ARG(
		btwt_setup, NULL,
		" Start a BTWT flow:\n"
		"<sub_id: Broadcast TWT AP config> <nominal_wake: 64-255> <max_sta_support>"
		"<twt_mantissa:0-sizeof(UINT16)> <twt_offset> <twt_exponent: 0-31> <sp_gap>.\n",
		cmd_wifi_btwt_setup,
		8, 0),
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

SHELL_SUBCMD_ADD((wifi), twt, &wifi_twt_ops,
		 "Manage TWT flows.",
		 NULL,
		 0, 0);

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_DPP
SHELL_STATIC_SUBCMD_SET_CREATE(
	wifi_cmd_dpp,
	SHELL_CMD_ARG(configurator_add, NULL,
		      " Add DPP configurator\n",
		      cmd_wifi_dpp_configurator_add, 1, 0),
	SHELL_CMD_ARG(auth_init, NULL,
		      "DPP start auth request:\n"
		      "-p --peer <peer_bootstrap_id>\n"
		      "[-r --role <1/2>]: DPP role default 1. 1: configurator, 2: enrollee\n"
		      "Optional args for configurator:\n"
		      "[-c --configurator <configurator_id>]\n"
		      "[-m --mode <1/2>]: Peer mode. 1: STA, 2: AP\n"
		      "[-s --ssid <SSID>]: SSID\n",
		      cmd_wifi_dpp_auth_init, 3, 8),
	SHELL_CMD_ARG(qr_code, NULL,
		      " Input QR code:\n"
		      "<qr_code_string>\n",
		      cmd_wifi_dpp_qr_code, 2, 0),
	SHELL_CMD_ARG(chirp, NULL,
		      " DPP chirp:\n"
		      "-i --own <own_bootstrap_id>\n"
		      "-f --freq <listen_freq>\n",
		      cmd_wifi_dpp_chirp, 5, 0),
	SHELL_CMD_ARG(listen, NULL,
		      " DPP listen:\n"
		      "-f --freq <listen_freq>\n"
		      "-r --role <1/2>: DPP role. 1: configurator, 2: enrollee\n",
		      cmd_wifi_dpp_listen, 5, 0),
	SHELL_CMD_ARG(btstrap_gen, NULL,
		      " DPP bootstrap generate:\n"
		      "[-t --type <1/2/3>]: Bootstrap type. 1: qr_code, 2: pkex, 3: nfc."
		      " Currently only support qr_code\n"
		      "[-o --opclass <operating_class>]\n"
		      "[-h --channel <channel>]\n"
		      "[-a --mac <mac_addr>]\n",
		      cmd_wifi_dpp_btstrap_gen, 1, 8),
	SHELL_CMD_ARG(btstrap_get_uri, NULL,
		      " Get DPP bootstrap uri by id:\n"
		      "<bootstrap_id>\n",
		      cmd_wifi_dpp_btstrap_get_uri, 2, 0),
	SHELL_CMD_ARG(configurator_set, NULL,
		      " Set DPP configurator parameters:\n"
		      "-c --configurator <configurator_id>\n"
		      "[-m --mode <1/2>]: Peer mode. 1: STA, 2: AP\n"
		      "[-s --ssid <SSID>]: SSID\n",
		      cmd_wifi_dpp_configurator_set, 3, 4),
	SHELL_CMD_ARG(resp_timeout_set, NULL,
		      " Set DPP RX response wait timeout ms:\n"
		      "<timeout_ms>\n",
		      cmd_wifi_dpp_resp_timeout_set, 2, 0),
	SHELL_CMD_ARG(ap_btstrap_gen, NULL,
		      " AP DPP bootstrap generate:\n"
		      "[-t --type <1/2/3>]: Bootstrap type. 1: qr_code, 2: pkex, 3: nfc."
		      " Currently only support qr_code\n"
		      "[-o --opclass <operating_class>]\n"
		      "[-h --channel <channel>]\n"
		      "[-a --mac <mac_addr>]\n",
		      cmd_wifi_dpp_ap_btstrap_gen, 1, 8),
	SHELL_CMD_ARG(ap_btstrap_get_uri, NULL,
		      " AP get DPP bootstrap uri by id:\n"
		      "<bootstrap_id>\n",
		      cmd_wifi_dpp_ap_btstrap_get_uri, 2, 0),
	SHELL_CMD_ARG(ap_qr_code, NULL,
		      " AP Input QR code:\n"
		      "<qr_code_string>\n",
		      cmd_wifi_dpp_ap_qr_code, 2, 0),
	SHELL_CMD_ARG(ap_auth_init, NULL,
		      "AP DPP start auth request as enrollee:\n"
		      "-p --peer <peer_bootstrap_id>\n",
		      cmd_wifi_dpp_ap_auth_init, 3, 0),
	SHELL_CMD_ARG(reconfig, NULL,
		      " reconfig network by id:\n"
		      "<network_id>\n",
		      cmd_wifi_dpp_reconfig, 2, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_SUBCMD_ADD((wifi), dpp, &wifi_cmd_dpp,
		 "DPP actions.",
		 NULL,
		 0, 0);
#endif /* CONFIG_WIFI_NM_WPA_SUPPLICANT_DPP */

SHELL_SUBCMD_SET_CREATE(wifi_commands, (wifi));

SHELL_SUBCMD_ADD((wifi), 11k, NULL,
		 "Configure 11k or get 11k status.\n"
		 "[enable/disable]\n",
		 cmd_wifi_11k,
		 1, 1);

SHELL_SUBCMD_ADD((wifi), 11k_neighbor_request, NULL,
		 "Send Neighbor Report Request frame.\n"
		 "[ssid <ssid>]\n",
		 cmd_wifi_11k_neighbor_request,
		 1, 2);

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_WNM
SHELL_SUBCMD_ADD((wifi), 11v_btm_query, NULL,
		 "<query_reason: The reason code for a BSS transition management query>.\n",
		 cmd_wifi_btm_query,
		 2, 0);
#endif

SHELL_SUBCMD_ADD((wifi), channel, NULL,
		 "wifi channel setting\n"
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
		 2, 4);

SHELL_SUBCMD_ADD((wifi), connect, NULL,
		  "Connect to a Wi-Fi AP\n"
		  "<-s --ssid \"<SSID>\">: SSID.\n"
		  "[-c --channel]: Channel that needs to be scanned for connection. "
		 "0:any channel.\n"
		  "[-b, --band] 0: any band (2:2.4GHz, 5:5GHz, 6:6GHz]\n"
		  "[-p, --psk]: Passphrase (valid only for secure SSIDs)\n"
		  "[-k, --key-mgmt]: Key Management type (valid only for secure SSIDs)\n"
		  "0:None, 1:WPA2-PSK, 2:WPA2-PSK-256, 3:SAE-HNP, 4:SAE-H2E, 5:SAE-AUTO, 6:WAPI,"
		  "7:EAP-TLS, 8:WEP, 9: WPA-PSK, 10: WPA-Auto-Personal, 11: DPP\n"
		  "12: EAP-PEAP-MSCHAPv2, 13: EAP-PEAP-GTC, 14: EAP-TTLS-MSCHAPv2,\n"
		  "15: EAP-PEAP-TLS, 16:EAP_TLS_SHA256\n"
		  "[-w, --ieee-80211w]: MFP (optional: needs security type to be specified)\n"
		  ": 0:Disable, 1:Optional, 2:Required.\n"
		  "[-m, --bssid]: MAC address of the AP (BSSID).\n"
		  "[-t, --timeout]: Timeout for the connection attempt (in seconds).\n"
		  "[-a, --anon-id]: Anonymous identity for enterprise mode.\n"
		  "[-K, --key1-pwd for eap phase1 or --key2-pwd for eap phase2]:\n"
		  "Private key passwd for enterprise mode. Default no password for private key.\n"
		  "[-S, --suiteb-type]: 1:suiteb, 2:suiteb-192. Default 0: not suiteb mode.\n"
		  "[-T, --TLS-cipher]: 0:TLS-NONE, 1:TLS-ECC-P384, 2:TLS-RSA-3K.\n"
		  "[-V, --eap-version]: 0 or 1. Default 1: eap version 1.\n"
		  "[-I, --eap-id1]: Client Identity. Default no eap identity.\n"
		  "[-P, --eap-pwd1]: Client Password.\n"
		  "Default no password for eap user.\n"
		  "[-R, --ieee-80211r]: Use IEEE80211R fast BSS transition connect."
		  "[-h, --help]: Print out the help for the connect command.\n",
		  cmd_wifi_connect,
		 2, 19);

SHELL_SUBCMD_ADD((wifi), disconnect, NULL,
		 "Disconnect from the Wi-Fi AP.\n",
		 cmd_wifi_disconnect,
		 1, 0);

SHELL_SUBCMD_ADD((wifi), mode, NULL,
		 "mode operational setting\n"
		 "This command may be used to set the Wi-Fi device into a specific "
		 "mode of operation\n"
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
		 1, 9);

SHELL_SUBCMD_ADD((wifi), packet_filter, NULL,
		 "mode filter setting\n"
		 "This command is used to set packet filter setting when\n"
		 "monitor, TX-Injection and promiscuous mode is enabled\n"
		 "The different packet filter modes are control, management, "
		 "data and enable all filters\n"
		 "[-i, --if-index <idx>] : Interface index\n"
		 "[-a, --all] : Enable all packet filter modes\n"
		 "[-m, --mgmt] : Enable management packets to allowed up the stack\n"
		 "[-c, --ctrl] : Enable control packets to be allowed up the stack\n"
		 "[-d, --data] : Enable Data packets to be allowed up the stack\n"
		 "[-g, --get] : Get current filter settings for a specific interface index\n"
		 "[-b, --capture-len <len>] : Capture length buffer size for each packet "
		 "to be captured\n"
		 "[-h, --help] : Help\n"
		 "Usage: Get operation example for interface index 1\n"
		 "wifi packet_filter -g -i1\n"
		 "Set operation example for interface index 1 - set data+management frame filter\n"
		 "wifi packet_filter -i1 -md.\n",
		 cmd_wifi_packet_filter,
		 2, 8);

SHELL_SUBCMD_ADD((wifi), pmksa_flush, NULL,
		 "Flush PMKSA cache entries.\n",
		 cmd_wifi_pmksa_flush,
		 1, 0);

SHELL_SUBCMD_ADD((wifi), ps, NULL,
		 "Configure or display Wi-Fi power save state.\n[on/off]\n",
		 cmd_wifi_ps,
		 1, 1);

SHELL_SUBCMD_ADD((wifi), ps_listen_interval, NULL,
		 "<val> - Listen interval in the range of <0-65535>.\n",
		 cmd_wifi_listen_interval,
		 2, 0);

SHELL_SUBCMD_ADD((wifi), ps_mode, NULL,
		 "<mode: legacy/WMM>.\n",
		 cmd_wifi_ps_mode,
		 2, 0);

SHELL_SUBCMD_ADD((wifi), ps_timeout, NULL,
		 "<val> - PS inactivity timer(in ms).\n",
		 cmd_wifi_ps_timeout,
		 2, 0);

SHELL_SUBCMD_ADD((wifi), ps_wakeup_mode, NULL,
		 "<wakeup_mode: DTIM/Listen Interval>.\n",
		 cmd_wifi_ps_wakeup_mode,
		 2, 0);

SHELL_SUBCMD_ADD((wifi), reg_domain, NULL,
		 "Set or Get Wi-Fi regulatory domain\n"
		 "[ISO/IEC 3166-1 alpha2]: Regulatory domain\n"
		 "[-f]: Force to use this regulatory hint over any other regulatory hints\n"
		 "Note1: The behavior of this command is dependent on the Wi-Fi driver/chipset implementation\n"
		 "Note2: This may cause regulatory compliance issues, use it at your own risk.\n"
		 "[-v]: Verbose, display the per-channel regulatory information\n",
		 cmd_wifi_reg_domain,
		 1, 3);

SHELL_SUBCMD_ADD((wifi), rts_threshold, NULL,
		 "<rts_threshold: rts threshold/off>.\n",
		 cmd_wifi_set_rts_threshold,
		 1, 1);

SHELL_SUBCMD_ADD((wifi), scan, NULL,
		 "Scan for Wi-Fi APs\n"
		 "[-t, --type <active/passive>] : Preferred mode of scan. "
		 "The actual mode of scan can depend on factors such as the Wi-Fi chip "
		 "implementation, regulatory domain restrictions. Default type is active\n"
		 "[-b, --bands <Comma separated list of band values (2/5/6)>] : "
		 "Bands to be scanned where 2: 2.4 GHz, 5: 5 GHz, 6: 6 GHz\n"
		 "[-a, --dwell_time_active <val_in_ms>] : "
		 "Active scan dwell time (in ms) on a channel. Range 5 ms to 1000 ms\n"
		 "[-p, --dwell_time_passive <val_in_ms>] : "
		 "Passive scan dwell time (in ms) on a channel. Range 10 ms to 1000 ms\n"
		 "[-s, --ssid] : SSID to scan for. Can be provided multiple times\n"
		 "[-m, --max_bss <val>] : Maximum BSSes to scan for. Range 1 - 65535\n"
		 "[-c, --chans <Comma separated list of channel ranges>] : "
		 "Channels to be scanned. The channels must be specified in the form "
		 "band1:chan1,chan2_band2:chan3,..etc. band1, band2 must be valid band "
		 "values and chan1, chan2, chan3 must be specified as a list of comma "
		 "separated values where each value is either a single channel or a "
		 "channel range specified as chan_start-chan_end. Each band channel "
		 "set has to be separated by a _. For example, a valid channel "
		 "specification can be 2:1,6_5:36 or 2:1,6-11,14_5:36,163-177,52. "
		 "Care should be taken to ensure that configured channels don't exceed "
		 "CONFIG_WIFI_MGMT_SCAN_CHAN_MAX_MANUAL\n"
		 "[-h, --help] : Print out the help for the scan command.\n",
		 cmd_wifi_scan,
		 1, 8);

SHELL_SUBCMD_ADD((wifi), statistics, NULL,
		 "Wi-Fi interface statistics.\n"
		 "[reset] : Reset Wi-Fi interface statistics\n"
		 "[help] :  Print out the help for the statistics command.",
		 cmd_wifi_stats,
		 1, 1);

SHELL_SUBCMD_ADD((wifi), status, NULL,
		 "Status of the Wi-Fi interface.\n",
		 cmd_wifi_status,
		 1, 0);

SHELL_SUBCMD_ADD((wifi), version, NULL,
		 "Print Wi-Fi Driver and Firmware versions\n",
		 cmd_wifi_version,
		 1, 0);

SHELL_SUBCMD_ADD((wifi), wps_pbc, NULL,
		 "Start a WPS PBC connection.\n",
		 cmd_wifi_wps_pbc,
		 1, 0);

SHELL_SUBCMD_ADD((wifi), wps_pin, NULL,
		 "Set and get WPS pin.\n"
		 "[pin] Only applicable for set.\n",
		 cmd_wifi_wps_pin,
		 1, 1);

SHELL_SUBCMD_ADD((wifi), ps_exit_strategy, NULL,
		 "<tim> : Set PS exit strategy to Every TIM\n"
		 "<custom> : Set PS exit strategy to Custom",
		 cmd_wifi_ps_exit_strategy,
		 2, 0);

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
