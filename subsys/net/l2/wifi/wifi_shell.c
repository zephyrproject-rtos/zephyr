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
#include <zephyr/sys/slist.h>

#include "net_shell_private.h"
#include <math.h>

#ifdef CONFIG_WIFI_CERTIFICATE_LIB
#include <zephyr/net/wifi_certs.h>
#endif

#define WIFI_SHELL_MODULE "wifi"

#define WIFI_SHELL_MGMT_EVENTS (            \
				NET_EVENT_WIFI_CONNECT_RESULT     |\
				NET_EVENT_WIFI_DISCONNECT_RESULT  |\
				NET_EVENT_WIFI_TWT                |\
				NET_EVENT_WIFI_AP_ENABLE_RESULT   |\
				NET_EVENT_WIFI_AP_DISABLE_RESULT  |\
				NET_EVENT_WIFI_AP_STA_CONNECTED   |\
				NET_EVENT_WIFI_AP_STA_DISCONNECTED|\
				NET_EVENT_WIFI_SIGNAL_CHANGE      |\
				NET_EVENT_WIFI_NEIGHBOR_REP_COMP  |\
				NET_EVENT_WIFI_P2P_DEVICE_FOUND)

#ifdef CONFIG_WIFI_MGMT_RAW_SCAN_RESULTS_ONLY
#define WIFI_SHELL_SCAN_EVENTS (                   \
				NET_EVENT_WIFI_SCAN_DONE          |\
				NET_EVENT_WIFI_RAW_SCAN_RESULT)
#else
#define WIFI_SHELL_SCAN_EVENTS (                   \
				NET_EVENT_WIFI_SCAN_RESULT        |\
				NET_EVENT_WIFI_SCAN_DONE          |\
				NET_EVENT_WIFI_RAW_SCAN_RESULT)
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
static struct net_mgmt_event_callback wifi_shell_scan_cb;
static struct wifi_reg_chan_info chan_info[MAX_REG_CHAN_NUM];

static K_MUTEX_DEFINE(wifi_ap_sta_list_lock);
struct wifi_ap_sta_node {
	bool valid;
	struct wifi_ap_sta_info sta_info;
};
static struct wifi_ap_sta_node sta_list[CONFIG_WIFI_SHELL_MAX_AP_STA];

enum iface_type {
	IFACE_TYPE_STA,
	IFACE_TYPE_SAP,
};

static struct net_if *get_iface(enum iface_type type, int argc, char *argv[])
{
	struct net_if *iface = NULL;
	int iface_index = -1;

	/* Parse arguments manually to find -i or --iface,
	 * it's intentional not to use sys_getopt() here to avoid
	 * permuting the arguments.
	 */
	for (int i = 1; i < argc; i++) {
		if ((strcmp(argv[i], "-i") == 0 ||
			 strcmp(argv[i], "--iface") == 0) &&
			(i + 1 < argc)) {
			int err = 0;

			iface_index = shell_strtol(argv[i + 1], 10, &err);
			if (err || iface_index < 0) {
				LOG_ERR("Invalid interface index: %s", argv[i + 1]);
				return NULL;
			}
			break;
		}
	}

	if (iface_index != -1) {
		iface = net_if_get_by_index(iface_index);
		if (iface == NULL) {
			LOG_WRN("Invalid interface index: %d, using default", iface_index);
		}
	}

	if (iface == NULL) {
		/* Fallback to STA or SAP if user didn't specify a valid interface */
		if (type == IFACE_TYPE_STA) {
			iface = net_if_get_wifi_sta();
		} else if (type == IFACE_TYPE_SAP) {
			iface = net_if_get_wifi_sap();
		}

		if (iface == NULL) {
			LOG_ERR("No default interface found for type: %s",
					type == IFACE_TYPE_STA ? "STA" : "SAP");
			return NULL;
		}
	}

	return iface;
}

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
		PR("\n%-4s | %-32s %-5s | %-13s | %-4s | %-20s | %-17s | %-8s\n",
		   "Num", "SSID", "(len)", "Chan (Band)", "RSSI", "Security", "BSSID", "MFP");
	}

	strncpy(ssid_print, entry->ssid, sizeof(ssid_print) - 1);
	ssid_print[sizeof(ssid_print) - 1] = '\0';

	PR("%-4d | %-32s %-5u | %-4u (%-6s) | %-4d | %-20s | %-17s | %-8s\n",
	   context.scan_result, ssid_print, entry->ssid_length, entry->channel,
	   wifi_band_txt(entry->band),
	   entry->rssi,
	   ((entry->wpa3_ent_type) ?
		wifi_wpa3_enterprise_txt(entry->wpa3_ent_type)
		 : (entry->security == WIFI_SECURITY_TYPE_EAP ? "WPA2 Enterprise"
		 : wifi_security_txt(entry->security))),
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

	net_mgmt_del_event_callback(&wifi_shell_scan_cb);

	context.scan_result = 0U;
}

static void handle_wifi_connect_result(struct net_mgmt_event_callback *cb)
{
	const struct wifi_status *status =
		(const struct wifi_status *) cb->info;
	const struct shell *sh = context.sh;
	int st = status->status;

	if (st) {
		if (st < 0) {
			/* Errno values are negative, try to map to
			 * wifi status values.
			 */
			if (st == -ETIMEDOUT) {
				st = WIFI_STATUS_CONN_TIMEOUT;
			}
		}

		PR_WARNING("Connection request failed (%s/%d)\n",
			   wifi_conn_status_txt(st), st);
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
static void handle_wifi_signal_change(struct net_mgmt_event_callback *cb, struct net_if *iface)
{
	const struct shell *sh = context.sh;
	int ret;

	ret = net_mgmt(NET_REQUEST_WIFI_START_ROAMING, iface, NULL, 0);
	if (ret) {
		PR_WARNING("Start roaming failed\n");
		return;
	}

	PR("Start roaming requested\n");
}

static void handle_wifi_neighbor_rep_complete(struct net_mgmt_event_callback *cb,
				    struct net_if *iface)
{
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

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_P2P
static void handle_wifi_p2p_device_found(struct net_mgmt_event_callback *cb)
{
	const struct wifi_p2p_device_info *peer_info =
		(const struct wifi_p2p_device_info *)cb->info;
	const struct shell *sh = context.sh;

	if (!peer_info || peer_info->device_name[0] == '\0') {
		return;
	}

	PR("Device Name: %-20s MAC Address: %02x:%02x:%02x:%02x:%02x:%02x      "
	   "Config Methods: 0x%x\n",
	   peer_info->device_name,
	   peer_info->mac[0], peer_info->mac[1], peer_info->mac[2],
	   peer_info->mac[3], peer_info->mac[4], peer_info->mac[5],
	   peer_info->config_methods);
}
#endif /* CONFIG_WIFI_NM_WPA_SUPPLICANT_P2P */

static void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb,
				    uint64_t mgmt_event, struct net_if *iface)
{
	switch (mgmt_event) {
	case NET_EVENT_WIFI_CONNECT_RESULT:
		handle_wifi_connect_result(cb);
		break;
	case NET_EVENT_WIFI_DISCONNECT_RESULT:
		handle_wifi_disconnect_result(cb);
		break;
	case NET_EVENT_WIFI_TWT:
		handle_wifi_twt_event(cb);
		break;
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
		handle_wifi_signal_change(cb, iface);
		break;
	case NET_EVENT_WIFI_NEIGHBOR_REP_COMP:
		handle_wifi_neighbor_rep_complete(cb, iface);
		break;
#endif
#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_P2P
	case NET_EVENT_WIFI_P2P_DEVICE_FOUND:
		handle_wifi_p2p_device_found(cb);
		break;
#endif /* CONFIG_WIFI_NM_WPA_SUPPLICANT_P2P */
	default:
		break;
	}
}

static void wifi_mgmt_scan_event_handler(struct net_mgmt_event_callback *cb,
					 uint64_t mgmt_event, struct net_if *iface)
{
	switch (mgmt_event) {
	case NET_EVENT_WIFI_SCAN_RESULT:
		handle_wifi_scan_result(cb);
		break;
	case NET_EVENT_WIFI_SCAN_DONE:
		handle_wifi_scan_done(cb);
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

static int __wifi_args_to_params(const struct shell *sh, size_t argc, char *argv[],
				 struct wifi_connect_req_params *params,
				 enum wifi_iface_mode iface_mode)
{
	int opt;
	int opt_index = 0;
	struct sys_getopt_state *state;
	static const struct sys_getopt_option long_options[] = {
		{"ssid", sys_getopt_required_argument, 0, 's'},
		{"passphrase", sys_getopt_required_argument, 0, 'p'},
		{"key-mgmt", sys_getopt_required_argument, 0, 'k'},
		{"ieee-80211w", sys_getopt_required_argument, 0, 'w'},
		{"bssid", sys_getopt_required_argument, 0, 'm'},
		{"band", sys_getopt_required_argument, 0, 'b'},
		{"channel", sys_getopt_required_argument, 0, 'c'},
		{"timeout", sys_getopt_required_argument, 0, 't'},
		{"anon-id", sys_getopt_required_argument, 0, 'a'},
		{"bandwidth", sys_getopt_required_argument, 0, 'B'},
		{"key1-pwd", sys_getopt_required_argument, 0, 'K'},
		{"key2-pwd", sys_getopt_required_argument, 0, 'K'},
		{"wpa3-enterprise", sys_getopt_required_argument, 0, 'S'},
		{"TLS-cipher", sys_getopt_required_argument, 0, 'T'},
		{"verify-peer-cert", sys_getopt_required_argument, 0, 'A'},
		{"eap-version", sys_getopt_required_argument, 0, 'V'},
		{"eap-id1", sys_getopt_required_argument, 0, 'I'},
		{"eap-id2", sys_getopt_required_argument, 0, 'I'},
		{"eap-id3", sys_getopt_required_argument, 0, 'I'},
		{"eap-id4", sys_getopt_required_argument, 0, 'I'},
		{"eap-id5", sys_getopt_required_argument, 0, 'I'},
		{"eap-id6", sys_getopt_required_argument, 0, 'I'},
		{"eap-id7", sys_getopt_required_argument, 0, 'I'},
		{"eap-id8", sys_getopt_required_argument, 0, 'I'},
		{"eap-pwd1", sys_getopt_required_argument, 0, 'P'},
		{"eap-pwd2", sys_getopt_required_argument, 0, 'P'},
		{"eap-pwd3", sys_getopt_required_argument, 0, 'P'},
		{"eap-pwd4", sys_getopt_required_argument, 0, 'P'},
		{"eap-pwd5", sys_getopt_required_argument, 0, 'P'},
		{"eap-pwd6", sys_getopt_required_argument, 0, 'P'},
		{"eap-pwd7", sys_getopt_required_argument, 0, 'P'},
		{"eap-pwd8", sys_getopt_required_argument, 0, 'P'},
		{"ignore-broadcast-ssid", sys_getopt_required_argument, 0, 'g'},
		{"ieee-80211r", sys_getopt_no_argument, 0, 'R'},
		{"iface", sys_getopt_required_argument, 0, 'i'},
		{"server-cert-domain-exact", sys_getopt_required_argument, 0, 'e'},
		{"server-cert-domain-suffix", sys_getopt_required_argument, 0, 'x'},
		{"help", sys_getopt_no_argument, 0, 'h'},
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
	params->verify_peer_cert = false;

	while ((opt = sys_getopt_long(argc, argv, "s:p:k:e:w:b:c:m:t:a:B:K:S:T:A:V:I:P:g:Rh:i:",
				  long_options, &opt_index)) != -1) {
		state = sys_getopt_state_get();
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
			/* WPA3 security types (SAE) require MFP (802.11w) as required,
			 * if not otherwise set.
			 */
			if (params->security == WIFI_SECURITY_TYPE_SAE_HNP ||
			    params->security == WIFI_SECURITY_TYPE_SAE_H2E ||
			    params->security == WIFI_SECURITY_TYPE_SAE_AUTO) {
				params->mfp = WIFI_MFP_REQUIRED;
			}
			break;
		case 'p':
			params->psk = state->optarg;
			params->psk_length = strlen(params->psk);
			break;
		case 'c':
			channel = strtol(state->optarg, &endptr, 10);
			if (iface_mode == WIFI_MODE_AP && channel == 0) {
				params->channel = channel;
				break;
			}
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
				case 0:
					/* Allow default value when connecting */
					if (iface_mode == WIFI_MODE_INFRA) {
						params->band = WIFI_FREQ_BAND_UNKNOWN;
						break;
					}
					__fallthrough;
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
			params->wpa3_ent_mode = atoi(state->optarg);
			if (params->wpa3_ent_mode != WIFI_WPA3_ENTERPRISE_NA) {
				params->mfp = WIFI_MFP_REQUIRED;
			}
			break;
		case 'T':
			params->TLS_cipher = atoi(state->optarg);
			break;
		case 'A':
			if (iface_mode == WIFI_MODE_INFRA) {
				params->verify_peer_cert = !!atoi(state->optarg);
			}
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
		case 'g':
			params->ignore_broadcast_ssid = shell_strtol(state->optarg, 10, &ret);
			break;
		case 'i':
			/* Unused, but parsing to avoid unknown option error */
			break;
		case 'e':
			params->server_cert_domain_exact = state->optarg;
			params->server_cert_domain_exact_len =
					strlen(params->server_cert_domain_exact);
			break;
		case 'x':
			params->server_cert_domain_suffix = state->optarg;
			params->server_cert_domain_suffix_len =
					strlen(params->server_cert_domain_suffix);
			break;
		case 'h':
			shell_help(sh);
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

	/* For SAE types, use -p passphrase as SAE password and clear PSK. */
	if (params->psk != NULL &&
	    (params->security == WIFI_SECURITY_TYPE_SAE_HNP ||
	     params->security == WIFI_SECURITY_TYPE_SAE_H2E ||
	     params->security == WIFI_SECURITY_TYPE_SAE_AUTO ||
	     params->security == WIFI_SECURITY_TYPE_FT_SAE ||
	     params->security == WIFI_SECURITY_TYPE_SAE_EXT_KEY)) {
		params->sae_password = params->psk;
		params->sae_password_length = params->psk_length;
		params->psk = NULL;
		params->psk_length = 0;
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

	/* AP mode: channel 0 means ACS, band must be specified */
	if (iface_mode == WIFI_MODE_AP && params->channel == 0 &&
	    params->band == WIFI_FREQ_BAND_UNKNOWN) {
		PR_ERROR("Band not provided when channel is 0 (ACS)\n");
		return -EINVAL;
	}

	/* Validate band and channel combination when both are explicitly provided */
	if (params->channel != WIFI_CHANNEL_ANY && params->channel != 0 &&
	    params->band != WIFI_FREQ_BAND_UNKNOWN) {
		if (!wifi_utils_validate_chan(params->band, params->channel)) {
			PR_ERROR("Channel %d is not valid for %s band\n",
				 params->channel, wifi_band_txt(params->band));
			return -EINVAL;
		}
	}

	if (params->ignore_broadcast_ssid > 2) {
		PR_ERROR("Invalid ignore_broadcast_ssid value\n");
		return -EINVAL;
	}

	return 0;
}

static int cmd_wifi_connect(const struct shell *sh, size_t argc,
			    char *argv[])
{
	struct net_if *iface = get_iface(IFACE_TYPE_STA, argc, argv);
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
	    cnx_params.security == WIFI_SECURITY_TYPE_EAP_PEAP_TLS) {
		wifi_set_enterprise_credentials(iface, 0);
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
	struct net_if *iface = get_iface(IFACE_TYPE_STA, argc, argv);
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

#ifdef CONFIG_WIFI_SHELL_RUNTIME_CERTIFICATES
	/* Clear the certificates */
	wifi_clear_enterprise_credentials();
#endif /* CONFIG_WIFI_SHELL_RUNTIME_CERTIFICATES */

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
	struct sys_getopt_state *state;
	static const struct sys_getopt_option long_options[] = {
		{"type", sys_getopt_required_argument, 0, 't'},
		{"bands", sys_getopt_required_argument, 0, 'b'},
		{"dwell_time_active", sys_getopt_required_argument, 0, 'a'},
		{"dwell_time_passive", sys_getopt_required_argument, 0, 'p'},
		{"ssid", sys_getopt_required_argument, 0, 's'},
		{"max_bss", sys_getopt_required_argument, 0, 'm'},
		{"chans", sys_getopt_required_argument, 0, 'c'},
		{"iface", sys_getopt_required_argument, 0, 'i'},
		{"help", sys_getopt_no_argument, 0, 'h'},
		{0, 0, 0, 0}};
	int val;
	int opt_num = 0;

	*do_scan = true;

	while ((opt = sys_getopt_long(argc, argv, "t:b:a:p:s:m:c:i:h",
				  long_options, &opt_index)) != -1) {
		state = sys_getopt_state_get();
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
		case 'i':
			/* Unused, but parsing to avoid unknown option error */
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
	struct net_if *iface = get_iface(IFACE_TYPE_STA, argc, argv);
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
		net_mgmt_add_event_callback(&wifi_shell_scan_cb);

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
	struct net_if *iface = get_iface(IFACE_TYPE_STA, argc, argv);
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
		PR("Security: %s %s\n", wifi_wpa3_enterprise_txt(status.wpa3_ent_type),
								wifi_security_txt(status.security));
		PR("MFP: %s\n", wifi_mfp_txt(status.mfp));
		if (status.iface_mode == WIFI_MODE_INFRA) {
			PR("RSSI: %d\n", status.rssi);
		}
		PR("Beacon Interval: %d\n", status.beacon_interval);
		PR("DTIM: %d\n", status.dtim_period);
		PR("TWT: %s\n",
		   status.twt_capable ? "Supported" : "Not supported");
		PR("Current PHY TX rate (Mbps) : %.1f\n", (double)status.current_phy_tx_rate);
	}

	return 0;
}

static int cmd_wifi_ap_status(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface = get_iface(IFACE_TYPE_SAP, argc, argv);
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
	PR("Security: %s %s\n", wifi_wpa3_enterprise_txt(status.wpa3_ent_type),
							wifi_security_txt(status.security));
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

	PR("Bytes received   : %llu\n", data->bytes.received);
	PR("Bytes sent       : %llu\n", data->bytes.sent);
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
	struct net_if *iface = get_iface(IFACE_TYPE_STA, argc, argv);
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
	struct net_if *iface = get_iface(IFACE_TYPE_STA, argc, argv);
	struct wifi_11k_params params = { 0 };

	context.sh = sh;

	if ((argc != 1 && argc != 3) || (argc == 3 && strncasecmp("ssid", argv[1], 4))) {
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
	struct net_if *iface = get_iface(IFACE_TYPE_STA, argc, argv);
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
	struct net_if *iface = get_iface(IFACE_TYPE_STA, argc, argv);
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
	struct net_if *iface = get_iface(IFACE_TYPE_STA, argc, argv);
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
	struct net_if *iface = get_iface(IFACE_TYPE_STA, argc, argv);
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
	struct net_if *iface = get_iface(IFACE_TYPE_SAP, argc, argv);
	struct wifi_twt_params params = {0};
	int idx = 1;
	int err = 0;

	context.sh = sh;

	params.btwt.btwt_sta_wait = (uint8_t)shell_strtol(argv[idx++], 10, &err);
	if (err) {
		PR_ERROR("Parse btwt sta_wait (err %d)\n", err);
		return -EINVAL;
	}

	params.btwt.btwt_offset = (uint16_t)shell_strtol(argv[idx++], 10, &err);
	if (err) {
		PR_ERROR("Parse btwt offset (err %d)\n", err);
		return -EINVAL;
	}

	params.btwt.btwt_li = (uint8_t)shell_strtol(argv[idx++], 10, &err);
	if (err) {
		PR_ERROR("Parse btwt_li (err %d)\n", err);
		return -EINVAL;
	}

	params.btwt.btwt_count = (uint8_t)shell_strtol(argv[idx++], 10, &err);
	if (err) {
		PR_ERROR("Parse btwt count (err %d)\n", err);
		return -EINVAL;
	}

	if (params.btwt.btwt_count < 2 || params.btwt.btwt_count > 5) {
		PR_ERROR("Invalid broadcast twt count. Count rang: 2-5.\n");
		return -EINVAL;
	}

	if (argc != 5 + params.btwt.btwt_count * 4) {
		PR_ERROR("Invalid number of broadcast parameters.\n");
		return -EINVAL;
	}

	for (int i = 0; i < params.btwt.btwt_count; i++) {
		params.btwt.btwt_set_cfg[i].btwt_id
			= (uint8_t)shell_strtol(argv[idx++], 10, &err);
		if (err) {
			PR_ERROR("Parse btwt [%d] id (err %d)\n", i, err);
			return -EINVAL;
		}

		params.btwt.btwt_set_cfg[i].btwt_mantissa
			= (uint16_t)shell_strtol(argv[idx++], 10, &err);
		if (err) {
			PR_ERROR("Parse btwt [%d] mantissa (err %d)\n", i, err);
			return -EINVAL;
		}

		params.btwt.btwt_set_cfg[i].btwt_exponent
			= (uint8_t)shell_strtol(argv[idx++], 10, &err);
		if (err) {
			PR_ERROR("Parse btwt [%d] exponent (err %d)\n", i, err);
			return -EINVAL;
		}

		params.btwt.btwt_set_cfg[i].btwt_nominal_wake
			= (uint8_t)shell_strtol(argv[idx++], 10, &err);
		if (err) {
			PR_ERROR("Parse btwt [%d] nominal_wake (err %d)\n", i, err);
			return -EINVAL;
		}
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
	struct sys_getopt_state *state;
	long value;
	double twt_mantissa_scale = 0.0;
	double twt_interval_scale = 0.0;
	uint16_t scale = 1000;
	int exponent = 0;
	static const struct sys_getopt_option long_options[] = {
		{"negotiation-type", sys_getopt_required_argument, 0, 'n'},
		{"setup-cmd", sys_getopt_required_argument, 0, 'c'},
		{"dialog-token", sys_getopt_required_argument, 0, 't'},
		{"flow-id", sys_getopt_required_argument, 0, 'f'},
		{"responder", sys_getopt_required_argument, 0, 'r'},
		{"trigger", sys_getopt_required_argument, 0, 'T'},
		{"implicit", sys_getopt_required_argument, 0, 'I'},
		{"announce", sys_getopt_required_argument, 0, 'a'},
		{"wake-interval", sys_getopt_required_argument, 0, 'w'},
		{"interval", sys_getopt_required_argument, 0, 'p'},
		{"wake-ahead-duration", sys_getopt_required_argument, 0, 'D'},
		{"info-disable", sys_getopt_required_argument, 0, 'd'},
		{"exponent", sys_getopt_required_argument, 0, 'e'},
		{"mantissa", sys_getopt_required_argument, 0, 'm'},
		{"iface", sys_getopt_required_argument, 0, 'i'},
		{"help", sys_getopt_no_argument, 0, 'h'},
		{0, 0, 0, 0}};

	params->operation = WIFI_TWT_SETUP;

	while ((opt = sys_getopt_long(argc, argv, "n:c:t:f:r:T:I:a:t:w:p:D:d:e:m:i:h",
				  long_options, &opt_index)) != -1) {
		state = sys_getopt_state_get();
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

		case 'p':
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

		case 'i':
			/* Unused, but parsing to avoid unknown option error */
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
	struct net_if *iface = get_iface(IFACE_TYPE_STA, argc, argv);
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
	struct net_if *iface = get_iface(IFACE_TYPE_STA, argc, argv);
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
	struct net_if *iface = get_iface(IFACE_TYPE_STA, argc, argv);
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
	struct net_if *iface = get_iface(IFACE_TYPE_SAP, argc, argv);
	struct wifi_connect_req_params cnx_params = {0};
	int ret;

	context.sh = sh;
	if (__wifi_args_to_params(sh, argc, &argv[0], &cnx_params, WIFI_MODE_AP)) {
		shell_help(sh);
		return -ENOEXEC;
	}

#ifdef CONFIG_WIFI_NM_HOSTAPD_CRYPTO_ENTERPRISE
	/* Load the enterprise credentials if needed */
	if (cnx_params.security == WIFI_SECURITY_TYPE_EAP_TLS ||
	    cnx_params.security == WIFI_SECURITY_TYPE_EAP_PEAP_MSCHAPV2 ||
	    cnx_params.security == WIFI_SECURITY_TYPE_EAP_PEAP_GTC ||
	    cnx_params.security == WIFI_SECURITY_TYPE_EAP_TTLS_MSCHAPV2 ||
	    cnx_params.security == WIFI_SECURITY_TYPE_EAP_PEAP_TLS) {
		wifi_set_enterprise_credentials(iface, 1);
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
	struct net_if *iface = get_iface(IFACE_TYPE_SAP, argc, argv);
	int ret;

	ret = net_mgmt(NET_REQUEST_WIFI_AP_DISABLE, iface, NULL, 0);
	if (ret) {
		PR_WARNING("AP mode disable failed: %s\n", strerror(-ret));
		return -ENOEXEC;
	}

	PR("AP mode disable requested\n");

#ifdef CONFIG_WIFI_SHELL_RUNTIME_CERTIFICATES
	/* Clear the certificates */
	wifi_clear_enterprise_credentials();
#endif /* CONFIG_WIFI_SHELL_RUNTIME_CERTIFICATES */

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
	struct net_if *iface = get_iface(IFACE_TYPE_SAP, argc, argv);
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
	struct sys_getopt_state *state;
	static const struct sys_getopt_option long_options[] = {
		{"max_inactivity", sys_getopt_required_argument, 0, 't'},
		{"max_num_sta", sys_getopt_required_argument, 0, 's'},
#if defined(CONFIG_WIFI_NM_HOSTAPD_AP)
		{"ht_capab", sys_getopt_required_argument, 0, 'n'},
		{"vht_capab", sys_getopt_required_argument, 0, 'c'},
#endif
		{"iface", sys_getopt_required_argument, 0, 'i'},
		{"help", sys_getopt_no_argument, 0, 'h'},
		{0, 0, 0, 0}};
	long val;

	while ((opt = sys_getopt_long(argc, argv, "t:s:n:c:i:h",
				  long_options, &opt_index)) != -1) {
		state = sys_getopt_state_get();
		switch (opt) {
		case 't':
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
		case 'i':
			/* Unused, but parsing to avoid unknown option error */
			break;
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
	struct net_if *iface = get_iface(IFACE_TYPE_SAP, argc, argv);
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
	struct net_if *iface = get_iface(IFACE_TYPE_SAP, argc, argv);
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
	struct net_if *iface = get_iface(IFACE_TYPE_STA, argc, argv);
	struct wifi_reg_domain regd = {0};
	int ret, chan_idx = 0;
	int opt;
	bool force = false;
	bool verbose = false;
	int opt_index = 0;
	static const struct sys_getopt_option long_options[] = {
		{"force", sys_getopt_no_argument, 0, 'f'},
		{"verbose", sys_getopt_no_argument, 0, 'v'},
		{"iface", sys_getopt_required_argument, 0, 'i'},
		{NULL, 0, NULL, 0}
	};

	while ((opt = sys_getopt_long(argc, argv, "fvi:", long_options, &opt_index)) != -1) {
		switch (opt) {
		case 'f':
			force = true;
			break;
		case 'v':
			verbose = true;
			break;
		case 'i':
			/* Unused, but parsing to avoid unknown option error */
			break;
		default:
			return -ENOEXEC;
		}
	}

	if (sys_getopt_optind == argc) {
		regd.chan_info = &chan_info[0];
		regd.oper = WIFI_MGMT_GET;
	} else if (sys_getopt_optind == argc - 1) {
		if (strlen(argv[sys_getopt_optind]) != 2) {
			PR_WARNING("Invalid reg domain: Length should be two letters/digits\n");
			return -ENOEXEC;
		}

		/* Two letter country code with special case of 00 for WORLD */
		if (((argv[sys_getopt_optind][0] < 'A' || argv[sys_getopt_optind][0] > 'Z') ||
			(argv[sys_getopt_optind][1] < 'A' || argv[sys_getopt_optind][1] > 'Z')) &&
			(argv[sys_getopt_optind][0] != '0' || argv[sys_getopt_optind][1] != '0')) {
			PR_WARNING("Invalid reg domain %c%c\n", argv[sys_getopt_optind][0],
				   argv[sys_getopt_optind][1]);
			return -ENOEXEC;
		}
		regd.country_code[0] = argv[sys_getopt_optind][0];
		regd.country_code[1] = argv[sys_getopt_optind][1];
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
	struct net_if *iface = get_iface(IFACE_TYPE_STA, argc, argv);
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

static int cmd_wifi_btm_query(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface = get_iface(IFACE_TYPE_STA, argc, argv);
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

static int cmd_wifi_wps_pbc(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface = get_iface(IFACE_TYPE_STA, argc, argv);
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
	struct net_if *iface = get_iface(IFACE_TYPE_STA, argc, argv);
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
	struct net_if *iface = get_iface(IFACE_TYPE_SAP, argc, argv);
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
	struct net_if *iface = get_iface(IFACE_TYPE_SAP, argc, argv);
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
	struct net_if *iface = get_iface(IFACE_TYPE_STA, argc, argv);
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
	struct net_if *iface = get_iface(IFACE_TYPE_STA, argc, argv);
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
	struct net_if *iface = get_iface(IFACE_TYPE_STA, argc, argv);
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
	int opt_num = 0;
	struct sys_getopt_state *state;
	static const struct sys_getopt_option long_options[] = {
		{"iface", sys_getopt_required_argument, 0, 'i'},
		{"sta", sys_getopt_no_argument, 0, 's'},
		{"monitor", sys_getopt_no_argument, 0, 'm'},
		{"ap", sys_getopt_no_argument, 0, 'a'},
		{"softap", sys_getopt_no_argument, 0, 'k'},
		{"help", sys_getopt_no_argument, 0, 'h'},
		{0, 0, 0, 0}};

	mode->oper = WIFI_MGMT_GET;
	while ((opt = sys_getopt_long(argc, argv, "i:smtpakh",
				  long_options, &opt_index)) != -1) {
		state = sys_getopt_state_get();
		switch (opt) {
		case 's':
			mode->mode |= WIFI_STA_MODE;
			opt_num++;
			break;
		case 'm':
			mode->mode |= WIFI_MONITOR_MODE;
			opt_num++;
			break;
		case 'a':
			mode->mode |= WIFI_AP_MODE;
			opt_num++;
			break;
		case 'k':
			mode->mode |= WIFI_SOFTAP_MODE;
			opt_num++;
			break;
		case 'i':
			mode->if_index = (uint8_t)atoi(state->optarg);
			/* Don't count iface as it's common for both get and set */
			break;
		case 'h':
			shell_help(sh);
			*do_mode_oper = false;
			opt_num++;
			break;
		case '?':
		default:
			break;
		}
	}

	if (opt_num != 0) {
		mode->oper = WIFI_MGMT_SET;
	}
}

static int cmd_wifi_mode(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface;
	struct wifi_mode_info mode_info = {0};
	int ret;
	bool do_mode_oper = true;

	parse_mode_args_to_params(sh, argc, argv, &mode_info, &do_mode_oper);

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
	struct sys_getopt_state *state;
	static const struct sys_getopt_option long_options[] = {
		{"iface", sys_getopt_optional_argument, 0, 'i'},
		{"channel", sys_getopt_required_argument, 0, 'c'},
		{"get", sys_getopt_no_argument, 0, 'g'},
		{"help", sys_getopt_no_argument, 0, 'h'},
		{0, 0, 0, 0}};

	while ((opt = sys_getopt_long(argc, argv, "i:c:gh",
				  long_options, &opt_index)) != -1) {
		state = sys_getopt_state_get();
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
	struct sys_getopt_state *state;
	static const struct sys_getopt_option long_options[] = {
		{"iface", sys_getopt_required_argument, 0, 'i'},
		{"capture-len", sys_getopt_optional_argument, 0, 'b'},
		{"all", sys_getopt_no_argument, 0, 'a'},
		{"mgmt", sys_getopt_no_argument, 0, 'm'},
		{"ctrl", sys_getopt_no_argument, 0, 'c'},
		{"data", sys_getopt_no_argument, 0, 'd'},
		{"get", sys_getopt_no_argument, 0, 'g'},
		{"help", sys_getopt_no_argument, 0, 'h'},
		{0, 0, 0, 0}};

	while ((opt = sys_getopt_long(argc, argv, "i:b:amcdgh",
				  long_options, &opt_index)) != -1) {
		state = sys_getopt_state_get();
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
	struct net_if *iface = get_iface(IFACE_TYPE_STA, argc, argv);
	struct wifi_version version = {0};

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
	struct sys_getopt_state *state;
	static const struct sys_getopt_option long_options[] = {
		{"peer", sys_getopt_required_argument, 0, 'p'},
		{"role", sys_getopt_required_argument, 0, 'r'},
		{"configurator", sys_getopt_required_argument, 0, 'c'},
		{"mode", sys_getopt_required_argument, 0, 'm'},
		{"ssid", sys_getopt_required_argument, 0, 's'},
		{"iface", sys_getopt_required_argument, 0, 'i'},
		{0, 0, 0, 0}};
	int ret = 0;

	while ((opt = sys_getopt_long(argc, argv, "p:r:c:m:s:i:",
				  long_options, &opt_index)) != -1) {
		state = sys_getopt_state_get();
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
		case 'i':
			/* Unused, but parsing to avoid unknown option error */
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
	struct sys_getopt_state *state;
	static const struct sys_getopt_option long_options[] = {
		{"own", sys_getopt_required_argument, 0, 'o'},
		{"freq", sys_getopt_required_argument, 0, 'f'},
		{"iface", sys_getopt_required_argument, 0, 'i'},
		{0, 0, 0, 0}};
	int ret = 0;

	while ((opt = sys_getopt_long(argc, argv, "o:f:i:",
				  long_options, &opt_index)) != -1) {
		state = sys_getopt_state_get();
		switch (opt) {
		case 'o':
			params->chirp.id = shell_strtol(state->optarg, 10, &ret);
			break;
		case 'f':
			params->chirp.freq = shell_strtol(state->optarg, 10, &ret);
			break;
		case 'i':
			/* Unused, but parsing to avoid unknown option error */
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
	struct sys_getopt_state *state;
	static const struct sys_getopt_option long_options[] = {
		{"role", sys_getopt_required_argument, 0, 'r'},
		{"freq", sys_getopt_required_argument, 0, 'f'},
		{"iface", sys_getopt_required_argument, 0, 'i'},
		{0, 0, 0, 0}};
	int ret = 0;

	while ((opt = sys_getopt_long(argc, argv, "r:f:i:",
				  long_options, &opt_index)) != -1) {
		state = sys_getopt_state_get();
		switch (opt) {
		case 'r':
			params->listen.role = shell_strtol(state->optarg, 10, &ret);
			break;
		case 'f':
			params->listen.freq = shell_strtol(state->optarg, 10, &ret);
			break;
		case 'i':
			/* Unused, but parsing to avoid unknown option error */
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
	struct sys_getopt_state *state;
	static const struct sys_getopt_option long_options[] = {
		{"type", sys_getopt_required_argument, 0, 't'},
		{"opclass", sys_getopt_required_argument, 0, 'o'},
		{"channel", sys_getopt_required_argument, 0, 'h'},
		{"mac", sys_getopt_required_argument, 0, 'a'},
		{"iface", sys_getopt_required_argument, 0, 'i'},
		{0, 0, 0, 0}};
	int ret = 0;

	while ((opt = sys_getopt_long(argc, argv, "t:o:h:a:i:",
				  long_options, &opt_index)) != -1) {
		state = sys_getopt_state_get();
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
		case 'i':
			/* Unused, but parsing to avoid unknown option error */
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
	struct sys_getopt_state *state;
	static const struct sys_getopt_option long_options[] = {
		{"configurator", sys_getopt_required_argument, 0, 'c'},
		{"mode", sys_getopt_required_argument, 0, 'm'},
		{"ssid", sys_getopt_required_argument, 0, 's'},
		{"iface", sys_getopt_required_argument, 0, 'i'},
		{0, 0, 0, 0}};
	int ret = 0;

	while ((opt = sys_getopt_long(argc, argv, "p:r:c:m:s:i:",
				  long_options, &opt_index)) != -1) {
		state = sys_getopt_state_get();
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
		case 'i':
			/* Unused, but parsing to avoid unknown option error */
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
	struct net_if *iface = get_iface(IFACE_TYPE_STA, argc, argv);
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
	struct net_if *iface = get_iface(IFACE_TYPE_STA, argc, argv);
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
	struct net_if *iface = get_iface(IFACE_TYPE_STA, argc, argv);
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
	struct net_if *iface = get_iface(IFACE_TYPE_STA, argc, argv);
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
	struct net_if *iface = get_iface(IFACE_TYPE_STA, argc, argv);
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
	struct net_if *iface = get_iface(IFACE_TYPE_STA, argc, argv);
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
	struct net_if *iface = get_iface(IFACE_TYPE_STA, argc, argv);
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
	struct net_if *iface = get_iface(IFACE_TYPE_STA, argc, argv);
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
	struct net_if *iface = get_iface(IFACE_TYPE_STA, argc, argv);
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
	struct net_if *iface = get_iface(IFACE_TYPE_SAP, argc, argv);
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
	struct net_if *iface = get_iface(IFACE_TYPE_SAP, argc, argv);
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
	struct net_if *iface = get_iface(IFACE_TYPE_SAP, argc, argv);
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
	struct sys_getopt_state *state;
	static const struct sys_getopt_option long_options[] = {
		{"peer", sys_getopt_required_argument, 0, 'p'},
		{"iface", sys_getopt_required_argument, 0, 'i'},
		{0, 0, 0, 0}};
	int ret = 0;
	struct net_if *iface = get_iface(IFACE_TYPE_SAP, argc, argv);
	struct wifi_dpp_params params = {0};

	params.action = WIFI_DPP_AUTH_INIT;

	while ((opt = sys_getopt_long(argc, argv, "p:i:",
				  long_options, &opt_index)) != -1) {
		state = sys_getopt_state_get();
		switch (opt) {
		case 'p':
			params.auth_init.peer = shell_strtol(state->optarg, 10, &ret);
			break;
		case 'i':
			/* Unused, but parsing to avoid unknown option error */
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
	struct net_if *iface = get_iface(IFACE_TYPE_STA, argc, argv);
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

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_P2P
static void print_peer_info(const struct shell *sh, int index,
			    const struct wifi_p2p_device_info *peer)
{
	uint8_t mac_string_buf[sizeof("xx:xx:xx:xx:xx:xx")];
	const char *device_name;
	const char *device_type;
	const char *config_methods;

	device_name = (peer->device_name[0] != '\0') ?
		      peer->device_name : "<Unknown>";
	device_type = (peer->pri_dev_type_str[0] != '\0') ?
		      peer->pri_dev_type_str : "-";
	config_methods = (peer->config_methods_str[0] != '\0') ?
			 peer->config_methods_str : "-";

	PR("%-4d | %-32s | %-17s | %-4d | %-20s | %s\n",
	   index,
	   device_name,
	   net_sprint_ll_addr_buf(peer->mac, WIFI_MAC_ADDR_LEN,
				  mac_string_buf,
				  sizeof(mac_string_buf)),
	   peer->rssi,
	   device_type,
	   config_methods);
}

static int cmd_wifi_p2p_peer(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface = get_iface(IFACE_TYPE_STA, argc, argv);
	struct wifi_p2p_params params = {0};
	uint8_t mac_addr[WIFI_MAC_ADDR_LEN];
	static struct wifi_p2p_device_info peers[WIFI_P2P_MAX_PEERS];
	int ret;
	int max_peers = (argc < 2) ? WIFI_P2P_MAX_PEERS : 1;

	context.sh = sh;

	memset(peers, 0, sizeof(peers));

	params.peers = peers;
	params.oper = WIFI_P2P_PEER;
	params.peer_count = max_peers;

	if (argc >= 2) {
		if (sscanf(argv[1], "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
			   &mac_addr[0], &mac_addr[1], &mac_addr[2],
			   &mac_addr[3], &mac_addr[4], &mac_addr[5]) != WIFI_MAC_ADDR_LEN) {
			PR_ERROR("Invalid MAC address format. Use: XX:XX:XX:XX:XX:XX\n");
			return -EINVAL;
		}
		memcpy(params.peer_addr, mac_addr, WIFI_MAC_ADDR_LEN);
		params.peer_count = 1;
	} else {
		/* Use broadcast MAC to query all peers */
		memset(params.peer_addr, 0xFF, WIFI_MAC_ADDR_LEN);
	}

	ret = net_mgmt(NET_REQUEST_WIFI_P2P_OPER, iface, &params, sizeof(params));
	if (ret) {
		PR_WARNING("P2P peer info request failed\n");
		return -ENOEXEC;
	}

	if (params.peer_count > 0) {
		PR("\n%-4s | %-32s | %-17s | %-4s | %-20s | %s\n",
		   "Num", "Device Name", "MAC Address", "RSSI", "Device Type", "Config Methods");
		for (int i = 0; i < params.peer_count; i++) {
			print_peer_info(sh, i + 1, &peers[i]);
		}
	} else {
		if (argc >= 2) {
			shell_print(sh, "No information available for peer %s", argv[1]);
		} else {
			shell_print(sh, "No P2P peers found");
		}
	}

	return 0;
}


static int cmd_wifi_p2p_find(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface = get_iface(IFACE_TYPE_STA, argc, argv);
	struct wifi_p2p_params params = {0};

	context.sh = sh;

	params.oper = WIFI_P2P_FIND;
	params.discovery_type = WIFI_P2P_FIND_START_WITH_FULL;
	params.timeout = 10; /* Default 10 second timeout */

	if (argc > 1) {
		int opt;
		int opt_index = 0;
		struct sys_getopt_state *state;
		static const struct sys_getopt_option long_options[] = {
			{"timeout", sys_getopt_required_argument, 0, 't'},
			{"type", sys_getopt_required_argument, 0, 'T'},
			{"iface", sys_getopt_required_argument, 0, 'i'},
			{"help", sys_getopt_no_argument, 0, 'h'},
			{0, 0, 0, 0}
		};
		long val;

		while ((opt = sys_getopt_long(argc, argv, "t:T:i:h",
						long_options, &opt_index)) != -1) {
			state = sys_getopt_state_get();
			switch (opt) {
			case 't':
				if (!parse_number(sh, &val, state->optarg, "timeout", 0, 65535)) {
					return -EINVAL;
				}
				params.timeout = (uint16_t)val;
				break;
			case 'T':
				if (!parse_number(sh, &val, state->optarg, "type", 0, 2)) {
					return -EINVAL;
				}
				switch (val) {
				case 0:
					params.discovery_type = WIFI_P2P_FIND_ONLY_SOCIAL;
					break;
				case 1:
					params.discovery_type = WIFI_P2P_FIND_PROGRESSIVE;
					break;
				case 2:
					params.discovery_type = WIFI_P2P_FIND_START_WITH_FULL;
					break;
				default:
					return -EINVAL;
				}
				break;
			case 'i':
				/* Unused, but parsing to avoid unknown option error */
				break;
			case 'h':
				shell_help(sh);
				return -ENOEXEC;
			default:
				PR_ERROR("Invalid option %c\n", state->optopt);
				return -EINVAL;
			}
		}
	}

	if (net_mgmt(NET_REQUEST_WIFI_P2P_OPER, iface, &params, sizeof(params))) {
		PR_WARNING("P2P find request failed\n");
		return -ENOEXEC;
	}
	PR("P2P find started\n");
	return 0;
}

static int cmd_wifi_p2p_stop_find(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface = get_iface(IFACE_TYPE_STA, argc, argv);
	struct wifi_p2p_params params = {0};

	context.sh = sh;

	params.oper = WIFI_P2P_STOP_FIND;

	if (net_mgmt(NET_REQUEST_WIFI_P2P_OPER, iface, &params, sizeof(params))) {
		PR_WARNING("P2P stop find request failed\n");
		return -ENOEXEC;
	}
	PR("P2P find stopped\n");
	return 0;
}

static int cmd_wifi_p2p_connect(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface = get_iface(IFACE_TYPE_STA, argc, argv);
	struct wifi_p2p_params params = {0};
	uint8_t mac_addr[WIFI_MAC_ADDR_LEN];
	const char *method_arg = NULL;
	int opt;
	int opt_index = 0;
	struct sys_getopt_state *state;
	static const struct sys_getopt_option long_options[] = {
		{"go-intent", sys_getopt_required_argument, 0, 'g'},
		{"freq", sys_getopt_required_argument, 0, 'f'},
		{"join", sys_getopt_no_argument, 0, 'j'},
		{"iface", sys_getopt_required_argument, 0, 'i'},
		{"help", sys_getopt_no_argument, 0, 'h'},
		{0, 0, 0, 0}
	};
	long val;

	context.sh = sh;

	if (argc < 3) {
		PR_ERROR("Usage: wifi p2p connect <MAC address> <pbc|pin> [PIN] "
			 "[--go-intent=<0-15>] [--freq=<frequency>] [--join]\n");
		return -EINVAL;
	}

	/* Parse MAC address */
	if (sscanf(argv[1], "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
		   &mac_addr[0], &mac_addr[1], &mac_addr[2],
		   &mac_addr[3], &mac_addr[4], &mac_addr[5]) != WIFI_MAC_ADDR_LEN) {
		PR_ERROR("Invalid MAC address format. Use: XX:XX:XX:XX:XX:XX\n");
		return -EINVAL;
	}
	memcpy(params.peer_addr, mac_addr, WIFI_MAC_ADDR_LEN);

	method_arg = argv[2];
	if (strcmp(method_arg, "pbc") == 0) {
		params.connect.method = WIFI_P2P_METHOD_PBC;
	} else if (strcmp(method_arg, "pin") == 0) {
		if (argc > 3 && argv[3][0] != '-') {
			params.connect.method = WIFI_P2P_METHOD_KEYPAD;
			strncpy(params.connect.pin, argv[3], WIFI_WPS_PIN_MAX_LEN);
			params.connect.pin[WIFI_WPS_PIN_MAX_LEN] = '\0';
		} else {
			params.connect.method = WIFI_P2P_METHOD_DISPLAY;
			params.connect.pin[0] = '\0';
		}
	} else {
		PR_ERROR("Invalid connection method. Use: pbc or pin\n");
		return -EINVAL;
	}

	/* Set default GO intent */
	params.connect.go_intent = 0;
	/* Set default frequency to 2462 MHz (channel 11, 2.4 GHz) */
	params.connect.freq = 2462;
	/* Set default join to false */
	params.connect.join = false;

	while ((opt = sys_getopt_long(argc, argv, "g:f:ji:h", long_options, &opt_index)) != -1) {
		state = sys_getopt_state_get();
		switch (opt) {
		case 'g':
			if (!parse_number(sh, &val, state->optarg, "go-intent", 0, 15)) {
				return -EINVAL;
			}
			params.connect.go_intent = (uint8_t)val;
			break;
		case 'f':
			if (!parse_number(sh, &val, state->optarg, "freq", 0, 6000)) {
				return -EINVAL;
			}
			params.connect.freq = (unsigned int)val;
			break;
		case 'j':
			params.connect.join = true;
			break;
		case 'i':
			/* Unused, but parsing to avoid unknown option error */
			break;
		case 'h':
			shell_help(sh);
			return -ENOEXEC;
		default:
			PR_ERROR("Invalid option %c\n", state->optopt);
			return -EINVAL;
		}
	}

	params.oper = WIFI_P2P_CONNECT;

	if (net_mgmt(NET_REQUEST_WIFI_P2P_OPER, iface, &params, sizeof(params))) {
		PR_WARNING("P2P connect request failed\n");
		return -ENOEXEC;
	}

	/* Display the generated PIN for DISPLAY method */
	if (params.connect.method == WIFI_P2P_METHOD_DISPLAY && params.connect.pin[0] != '\0') {
		PR("%s\n", params.connect.pin);
	} else {
		PR("P2P connection initiated\n");
	}
	return 0;
}

static int cmd_wifi_p2p_group_add(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface = get_iface(IFACE_TYPE_STA, argc, argv);
	struct wifi_p2p_params params = {0};
	int opt;
	int opt_index = 0;
	struct sys_getopt_state *state;
	static const struct sys_getopt_option long_options[] = {
		{"freq", sys_getopt_required_argument, 0, 'f'},
		{"persistent", sys_getopt_required_argument, 0, 'p'},
		{"ht40", sys_getopt_no_argument, 0, 'h'},
		{"vht", sys_getopt_no_argument, 0, 'v'},
		{"he", sys_getopt_no_argument, 0, 'H'},
		{"edmg", sys_getopt_no_argument, 0, 'e'},
		{"go-bssid", sys_getopt_required_argument, 0, 'b'},
		{"iface", sys_getopt_required_argument, 0, 'i'},
		{"help", sys_getopt_no_argument, 0, '?'},
		{0, 0, 0, 0}
	};
	long val;
	uint8_t mac_addr[WIFI_MAC_ADDR_LEN];

	context.sh = sh;

	params.oper = WIFI_P2P_GROUP_ADD;
	params.group_add.freq = 0;
	params.group_add.persistent = -1;
	params.group_add.ht40 = false;
	params.group_add.vht = false;
	params.group_add.he = false;
	params.group_add.edmg = false;
	params.group_add.go_bssid_length = 0;

	while ((opt = sys_getopt_long(argc, argv, "f:p:hvHeb:i:?", long_options,
				      &opt_index)) != -1) {
		state = sys_getopt_state_get();
		switch (opt) {
		case 'f':
			if (!parse_number(sh, &val, state->optarg, "freq", 0, 6000)) {
				return -EINVAL;
			}
			params.group_add.freq = (int)val;
			break;
		case 'p':
			if (!parse_number(sh, &val, state->optarg, "persistent", -1, 255)) {
				return -EINVAL;
			}
			params.group_add.persistent = (int)val;
			break;
		case 'h':
			params.group_add.ht40 = true;
			break;
		case 'v':
			params.group_add.vht = true;
			params.group_add.ht40 = true;
			break;
		case 'H':
			params.group_add.he = true;
			break;
		case 'e':
			params.group_add.edmg = true;
			break;
		case 'b':
			if (sscanf(state->optarg, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
				   &mac_addr[0], &mac_addr[1], &mac_addr[2], &mac_addr[3],
				   &mac_addr[4], &mac_addr[5]) != WIFI_MAC_ADDR_LEN) {
				PR_ERROR("Invalid GO BSSID format. Use: XX:XX:XX:XX:XX:XX\n");
				return -EINVAL;
			}
			memcpy(params.group_add.go_bssid, mac_addr, WIFI_MAC_ADDR_LEN);
			params.group_add.go_bssid_length = WIFI_MAC_ADDR_LEN;
			break;
		case 'i':
			/* Unused, but parsing to avoid unknown option error */
			break;
		case '?':
			shell_help(sh);
			return -ENOEXEC;
		default:
			PR_ERROR("Invalid option %c\n", state->optopt);
			return -EINVAL;
		}
	}

	if (net_mgmt(NET_REQUEST_WIFI_P2P_OPER, iface, &params, sizeof(params))) {
		PR_WARNING("P2P group add request failed\n");
		return -ENOEXEC;
	}
	PR("P2P group add initiated\n");
	return 0;
}

static int cmd_wifi_p2p_group_remove(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface = get_iface(IFACE_TYPE_STA, argc, argv);
	struct wifi_p2p_params params = {0};

	context.sh = sh;

	if (argc < 2) {
		PR_ERROR("Interface name required. Usage: wifi p2p group_remove <ifname>\n");
		return -EINVAL;
	}

	params.oper = WIFI_P2P_GROUP_REMOVE;
	strncpy(params.group_remove.ifname, argv[1],
		CONFIG_NET_INTERFACE_NAME_LEN);
	params.group_remove.ifname[CONFIG_NET_INTERFACE_NAME_LEN] = '\0';

	if (net_mgmt(NET_REQUEST_WIFI_P2P_OPER, iface, &params, sizeof(params))) {
		PR_WARNING("P2P group remove request failed\n");
		return -ENOEXEC;
	}
	PR("P2P group remove initiated\n");
	return 0;
}

static int cmd_wifi_p2p_invite(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface = get_iface(IFACE_TYPE_STA, argc, argv);
	struct wifi_p2p_params params = {0};
	uint8_t mac_addr[WIFI_MAC_ADDR_LEN];
	int opt;
	int opt_index = 0;
	struct sys_getopt_state *state;
	static const struct sys_getopt_option long_options[] = {
		{"persistent", sys_getopt_required_argument, 0, 'p'},
		{"group", sys_getopt_required_argument, 0, 'g'},
		{"peer", sys_getopt_required_argument, 0, 'P'},
		{"freq", sys_getopt_required_argument, 0, 'f'},
		{"go-dev-addr", sys_getopt_required_argument, 0, 'd'},
		{"iface", sys_getopt_required_argument, 0, 'i'},
		{"help", sys_getopt_no_argument, 0, 'h'},
		{0, 0, 0, 0}
	};
	long val;

	context.sh = sh;

	params.oper = WIFI_P2P_INVITE;
	params.invite.type = WIFI_P2P_INVITE_PERSISTENT;
	params.invite.persistent_id = -1;
	params.invite.group_ifname[0] = '\0';
	params.invite.freq = 0;
	params.invite.go_dev_addr_length = 0;
	memset(params.invite.peer_addr, 0, WIFI_MAC_ADDR_LEN);

	if (argc < 2) {
		PR_ERROR("Usage: wifi p2p invite --persistent=<id> <peer MAC> OR "
			 "wifi p2p invite --group=<ifname> --peer=<MAC> [options]\n");
		return -EINVAL;
	}

	while ((opt = sys_getopt_long(argc, argv, "p:g:P:f:d:i:h", long_options,
				      &opt_index)) != -1) {
		state = sys_getopt_state_get();
		switch (opt) {
		case 'p':
			if (!parse_number(sh, &val, state->optarg, "persistent", 0, 255)) {
				return -EINVAL;
			}
			params.invite.type = WIFI_P2P_INVITE_PERSISTENT;
			params.invite.persistent_id = (int)val;
			break;
		case 'g':
			params.invite.type = WIFI_P2P_INVITE_GROUP;
			strncpy(params.invite.group_ifname, state->optarg,
				CONFIG_NET_INTERFACE_NAME_LEN);
			params.invite.group_ifname[CONFIG_NET_INTERFACE_NAME_LEN] = '\0';
			break;
		case 'P':
			if (sscanf(state->optarg, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
				   &mac_addr[0], &mac_addr[1], &mac_addr[2], &mac_addr[3],
				   &mac_addr[4], &mac_addr[5]) != WIFI_MAC_ADDR_LEN) {
				PR_ERROR("Invalid peer MAC address format\n");
				return -EINVAL;
			}
			memcpy(params.invite.peer_addr, mac_addr, WIFI_MAC_ADDR_LEN);
			break;
		case 'f':
			if (!parse_number(sh, &val, state->optarg, "freq", 0, 6000)) {
				return -EINVAL;
			}
			params.invite.freq = (int)val;
			break;
		case 'd':
			if (sscanf(state->optarg, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
				   &mac_addr[0], &mac_addr[1], &mac_addr[2], &mac_addr[3],
				   &mac_addr[4], &mac_addr[5]) != WIFI_MAC_ADDR_LEN) {
				PR_ERROR("Invalid GO device address format\n");
				return -EINVAL;
			}
			memcpy(params.invite.go_dev_addr, mac_addr, WIFI_MAC_ADDR_LEN);
			params.invite.go_dev_addr_length = WIFI_MAC_ADDR_LEN;
			break;
		case 'i':
			/* Unused, but parsing to avoid unknown option error */
			break;
		case 'h':
			shell_help(sh);
			return -ENOEXEC;
		default:
			PR_ERROR("Invalid option %c\n", state->optopt);
			return -EINVAL;
		}
	}

	state = sys_getopt_state_get();

	if (params.invite.type == WIFI_P2P_INVITE_PERSISTENT &&
	    params.invite.persistent_id >= 0 &&
	    params.invite.peer_addr[0] == 0 && params.invite.peer_addr[1] == 0 &&
	    argc > state->optind && argv[state->optind][0] != '-') {
		if (sscanf(argv[state->optind], "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
			   &mac_addr[0], &mac_addr[1], &mac_addr[2], &mac_addr[3],
			   &mac_addr[4], &mac_addr[5]) != WIFI_MAC_ADDR_LEN) {
			PR_ERROR("Invalid peer MAC address format\n");
			return -EINVAL;
		}
		memcpy(params.invite.peer_addr, mac_addr, WIFI_MAC_ADDR_LEN);
	}

	if (params.invite.type == WIFI_P2P_INVITE_PERSISTENT) {
		if (params.invite.persistent_id < 0) {
			PR_ERROR("Persistent group ID required. Use --persistent=<id>\n");
			return -EINVAL;
		}
		if (params.invite.peer_addr[0] == 0 && params.invite.peer_addr[1] == 0) {
			PR_ERROR("Peer MAC address required\n");
			return -EINVAL;
		}
	} else if (params.invite.type == WIFI_P2P_INVITE_GROUP) {
		if (params.invite.group_ifname[0] == '\0') {
			PR_ERROR("Group interface name required. Use --group=<ifname>\n");
			return -EINVAL;
		}
		if (params.invite.peer_addr[0] == 0 && params.invite.peer_addr[1] == 0) {
			PR_ERROR("Peer MAC address required. Use --peer=<MAC>\n");
			return -EINVAL;
		}
	}

	if (net_mgmt(NET_REQUEST_WIFI_P2P_OPER, iface, &params, sizeof(params))) {
		PR_WARNING("P2P invite request failed\n");
		return -ENOEXEC;
	}
	PR("P2P invite initiated\n");
	return 0;
}

static int cmd_wifi_p2p_power_save(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface = get_iface(IFACE_TYPE_STA, argc, argv);
	struct wifi_p2p_params params = {0};
	bool power_save_enable = false;

	context.sh = sh;

	if (argc < 2) {
		PR_ERROR("Usage: wifi p2p power_save <on|off>\n");
		return -EINVAL;
	}

	if (strcmp(argv[1], "on") == 0) {
		power_save_enable = true;
	} else if (strcmp(argv[1], "off") == 0) {
		power_save_enable = false;
	} else {
		PR_ERROR("Invalid argument. Use 'on' or 'off'\n");
		return -EINVAL;
	}

	params.oper = WIFI_P2P_POWER_SAVE;
	params.power_save = power_save_enable;

	if (net_mgmt(NET_REQUEST_WIFI_P2P_OPER, iface, &params, sizeof(params))) {
		PR_WARNING("P2P power save request failed\n");
		return -ENOEXEC;
	}

	PR("P2P power save %s\n", power_save_enable ? "enabled" : "disabled");
	return 0;
}
#endif /* CONFIG_WIFI_NM_WPA_SUPPLICANT_P2P */

static int cmd_wifi_pmksa_flush(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface = get_iface(IFACE_TYPE_STA, argc, argv);

	context.sh = sh;

	if (net_mgmt(NET_REQUEST_WIFI_PMKSA_FLUSH, iface, NULL, 0)) {
		PR_WARNING("Flush PMKSA cache entries failed\n");
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_wifi_set_bss_max_idle_period(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface = get_iface(IFACE_TYPE_STA, argc, argv);
	unsigned short bss_max_idle_period = 0;
	int idx = 1;
	unsigned long val = 0;

	if (!parse_number(sh, &val, argv[idx++], "bss_max_idle_period", 0, USHRT_MAX)) {
		return -EINVAL;
	}

	bss_max_idle_period = (unsigned short)val;

	if (net_mgmt(NET_REQUEST_WIFI_BSS_MAX_IDLE_PERIOD, iface,
		     &bss_max_idle_period, sizeof(bss_max_idle_period))) {
		shell_fprintf(sh, SHELL_WARNING,
			      "Setting BSS maximum idle period failed.\n");
		return -ENOEXEC;
	}

	shell_fprintf(sh, SHELL_NORMAL, "BSS max idle period: %hu\n", bss_max_idle_period);

	return 0;
}

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_BGSCAN
static int wifi_bgscan_args_to_params(const struct shell *sh, size_t argc, char *argv[],
				      struct wifi_bgscan_params *params)
{
	int err;
	int opt;
	int opt_index = 0;
	struct sys_getopt_state *state;
	static const struct sys_getopt_option long_options[] = {
		{"type", sys_getopt_required_argument, 0, 't'},
		{"short-interval", sys_getopt_required_argument, 0, 's'},
		{"rss-threshold", sys_getopt_required_argument, 0, 'r'},
		{"long-interval", sys_getopt_required_argument, 0, 'l'},
		{"btm-queries", sys_getopt_required_argument, 0, 'b'},
		{"iface", sys_getopt_required_argument, 0, 'i'},
		{0, 0, 0, 0}};
	unsigned long uval;
	long val;

	while ((opt = sys_getopt_long(argc, argv, "t:s:r:l:b:i:", long_options,
				      &opt_index)) != -1) {
		state = sys_getopt_state_get();
		switch (opt) {
		case 't':
			if (strcmp("simple", state->optarg) == 0) {
				params->type = WIFI_BGSCAN_SIMPLE;
			} else if (strcmp("learn", state->optarg) == 0) {
				params->type = WIFI_BGSCAN_LEARN;
			} else if (strcmp("none", state->optarg) == 0) {
				params->type = WIFI_BGSCAN_NONE;
			} else {
				PR_ERROR("Invalid type %s\n", state->optarg);
				shell_help(sh);
				return SHELL_CMD_HELP_PRINTED;
			}
			break;
		case 's':
			uval = shell_strtoul(state->optarg, 10, &err);
			if (err < 0) {
				PR_ERROR("Invalid short interval %s\n", state->optarg);
				return err;
			}
			params->short_interval = uval;
			break;
		case 'l':
			uval = shell_strtoul(state->optarg, 10, &err);
			if (err < 0) {
				PR_ERROR("Invalid long interval %s\n", state->optarg);
				return err;
			}
			params->long_interval = uval;
			break;
		case 'b':
			uval = shell_strtoul(state->optarg, 10, &err);
			if (err < 0) {
				PR_ERROR("Invalid BTM queries %s\n", state->optarg);
				return err;
			}
			params->btm_queries = uval;
			break;
		case 'r':
			val = shell_strtol(state->optarg, 10, &err);
			if (err < 0) {
				PR_ERROR("Invalid RSSI threshold %s\n", state->optarg);
				return err;
			}
			params->rssi_threshold = val;
			break;
		case 'i':
			/* Unused, but parsing to avoid unknown option error */
			break;
		default:
			PR_ERROR("Invalid option %c\n", state->optopt);
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
	}

	return 0;
}

static int cmd_wifi_set_bgscan(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface = get_iface(IFACE_TYPE_STA, argc, argv);
	struct wifi_bgscan_params bgscan_params = {
		.type = WIFI_BGSCAN_NONE,
		.short_interval = 30,
		.long_interval = 300,
		.rssi_threshold = 0,
		.btm_queries = 0,
	};
	int ret;

	if (wifi_bgscan_args_to_params(sh, argc, argv, &bgscan_params) != 0) {
		return -ENOEXEC;
	}

	ret = net_mgmt(NET_REQUEST_WIFI_BGSCAN, iface, &bgscan_params,
		       sizeof(struct wifi_bgscan_params));
	if (ret != 0) {
		PR_WARNING("Setting background scanning parameters failed: %s\n", strerror(-ret));
		return -ENOEXEC;
	}

	return 0;
}
#endif

static int wifi_config_args_to_params(const struct shell *sh, size_t argc, char *argv[],
					 struct wifi_config_params *params)
{
	int opt;
	int opt_index = 0;
	struct sys_getopt_state *state;
	static const struct sys_getopt_option long_options[] = {
		{"okc", sys_getopt_required_argument, 0, 'o'},
		{"iface", sys_getopt_required_argument, 0, 'i'},
		{0, 0, 0, 0}};
	long val;

	while ((opt = sys_getopt_long(argc, argv, "o:i:",
				  long_options, &opt_index)) != -1) {
		state = sys_getopt_state_get();
		switch (opt) {
		case 'o':
			if (!parse_number(sh, &val, state->optarg, "okc", 0, 1)) {
				return -EINVAL;
			}
			params->okc = val;
			params->type |= WIFI_CONFIG_PARAM_OKC;
			break;
		case 'i':
			/* Unused, but parsing to avoid unknown option error */
			break;
		default:
			PR_ERROR("Invalid option %c\n", state->optopt);
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
	}

	return 0;
}

static int cmd_wifi_config_params(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface = get_iface(IFACE_TYPE_STA, argc, argv);
	struct wifi_config_params config_params = { 0 };
	int ret = -1;

	context.sh = sh;

	if (wifi_config_args_to_params(sh, argc, argv, &config_params)) {
		return -ENOEXEC;
	}

	ret = net_mgmt(NET_REQUEST_WIFI_CONFIG_PARAM, iface,
		       &config_params, sizeof(struct wifi_config_params));
	if (ret) {
		PR_WARNING("Setting STA parameter failed: %s\n",
			   strerror(-ret));
		return -ENOEXEC;
	}

	return 0;
}

SHELL_SUBCMD_ADD((wifi), 11k, NULL,
		 SHELL_HELP("Configure 11k or get 11k status",
			    "[-i, --iface=<interface index>]\n"
			    "[enable/disable]"),
		 cmd_wifi_11k,
		 1, 3);

SHELL_SUBCMD_ADD((wifi), 11k_neighbor_request, NULL,
		 SHELL_HELP("Send Neighbor Report Request frame",
			    "[-i, --iface=<interface index>]\n"
			    "[ssid <ssid>]"),
		 cmd_wifi_11k_neighbor_request,
		 1, 4);

SHELL_SUBCMD_ADD((wifi), 11v_btm_query, NULL,
		 SHELL_HELP("Setting BTM query reason",
			    "[-i, --iface=<interface index>]\n"
			    "<reason code for BSS transition management query>"),
		 cmd_wifi_btm_query,
		 2, 2);

SHELL_STATIC_SUBCMD_SET_CREATE(
	wifi_cmd_ap,
	SHELL_CMD_ARG(disable, NULL,
		      SHELL_HELP("Disable Access Point mode",
				 "[-i, --iface=<interface index>]"),
		cmd_wifi_ap_disable, 1, 2),
	SHELL_CMD_ARG(enable, NULL,
		      SHELL_HELP("Enable Access Point mode",
				 "[-i, --iface=<interface index>]\n"
				 "-s --ssid=<SSID>\n"
				 "-c --channel=<channel number>\n"
				 "-p --passphrase=<PSK> (valid only for secure SSIDs)\n"
				 "-k --key-mgmt=<Security type> (valid only for secure SSIDs)\n"
				 "0:None, 1:WPA2-PSK, 2:WPA2-PSK-256, 3:SAE-HNP, 4:SAE-H2E, "
				 "5:SAE-AUTO, 6:WAPI, 7:EAP-TLS, 8:WEP, 9: WPA-PSK, "
				 "10:WPA-Auto-Personal, 11:DPP 12:EAP-PEAP-MSCHAPv2, "
				 "13:EAP-PEAP-GTC, 14:EAP-TTLS-MSCHAPv2, 15:EAP-PEAP-TLS, "
				 "20:SAE-EXT-KEY\n"
				 "-w --ieee-80211w=<MFP> (optional: needs security type to "
				 "be specified)\n"
				 "0:Disable, 1:Optional, 2:Required\n"
				 "-b --band=<band> (2 -2.6GHz, 5 - 5Ghz, 6 - 6GHz)\n"
				 "-m --bssid=<BSSID>\n"
				 "-g --ignore-broadcast-ssid=<type>. Hide SSID in AP mode.\n"
				 "0: disabled (default)\n"
				 "1: send empty (length=0) SSID in beacon and ignore probe "
				 "request for broadcast SSID\n"
				 "2: clear SSID (ASCII 0), but keep the original length "
				 "and ignore probe requests for broadcast SSID\n"
				 "[-B, --bandwidth=<bandwidth>]: 1:20MHz, 2:40MHz, 3:80MHz\n"
				 "[-K, --key1-pwd for eap phase1 or --key2-pwd for eap phase2]:\n"
				 "Private key passwd for enterprise mode. Default is no password "
				 "for private key\n"
				 "[-S, --wpa3-enterprise]: WPA3 enterprise mode:\n"
				 "Default is 0: Not WPA3 enterprise mode\n"
				 "1:Suite-b mode, 2:Suite-b-192-bit mode, 3:WPA3-enterprise-only mode\n"
				 "[-V, --eap-version]: 0 or 1. Default 1: eap version 1\n"
				 "[-I, --eap-id1...--eap-id8]: Client Identity. Default no eap identity\n"
				 "[-P, --eap-pwd1...--eap-pwd8]: Client Password\n"
				 "Default no password for eap user"),
		      cmd_wifi_ap_enable, 2, 47),
	SHELL_CMD_ARG(stations, NULL,
		      SHELL_HELP("List stations connected to the AP",
				 "[-i, --iface=<interface index>]"),
		      cmd_wifi_ap_stations, 1, 2),
	SHELL_CMD_ARG(disconnect, NULL,
		      SHELL_HELP("Disconnect a station from the AP",
				 "[-i, --iface=<interface index>]\n"
				 "<MAC address of the station>"),
		      cmd_wifi_ap_sta_disconnect, 2, 3),
	SHELL_CMD_ARG(config, NULL,
		      SHELL_HELP("Configure AP parameters",
				 "[-i, --iface=<interface index>]\n"
				 "-t --max_inactivity=<time duration (in seconds)>\n"
				 "-s --max_num_sta=<maximum number of stations>"
#if defined(CONFIG_WIFI_NM_HOSTAPD_AP)
		      "\nPlease refer to hostapd.conf to set the following options,\n"
		      "============ IEEE 802.11 related configuration ============\n"
		      "-n --ht_capab=<HT capabilities (string)>\n"
		      "(e.g. \"ht_capab=[HT40+]\" is that \"-n [HT40+]\")\n"
		      "-c --vht_capab=<VHT capabilities (string)>\n"
		      "(e.g. \"vht_capab=[SU-BEAMFORMEE][BF-ANTENNA-4]\" is that\n"
		      "\"-c [SU-BEAMFORMEE][BF-ANTENNA-4]\")\n"
		      "==========================================================="
#endif
		      ),
		      cmd_wifi_ap_config_params, 3, 12),
	SHELL_CMD_ARG(wps_pbc, NULL,
		      SHELL_HELP("Start AP WPS PBC session",
				 "[-i, --iface=<interface index>]"),
		      cmd_wifi_ap_wps_pbc, 1, 2),
	SHELL_CMD_ARG(wps_pin, NULL,
		      SHELL_HELP("Get or Set AP WPS PIN",
				 "[-i, --iface=<interface index>]\n"
				 "[pin] Only applicable for set"),
		      cmd_wifi_ap_wps_pin, 1, 3),
	SHELL_CMD_ARG(status, NULL,
		      SHELL_HELP("Status of Wi-Fi SAP",
				 "[-i, --iface=<interface index>]"),
		      cmd_wifi_ap_status, 1, 2),
	SHELL_CMD_ARG(rts_threshold,
		      NULL,
		      SHELL_HELP("Set RTS value or turn it off",
				 "[-i, --iface=<interface index>]\n"
				 "<rts_threshold value> | off"),
		      cmd_wifi_ap_set_rts_threshold,
		      2,
		      2),
	SHELL_SUBCMD_SET_END);

SHELL_SUBCMD_ADD((wifi), ap, &wifi_cmd_ap,
		 "Access Point mode commands.",
		 NULL,
		 0, 0);

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_BGSCAN
SHELL_SUBCMD_ADD((wifi), bgscan, NULL,
		 SHELL_HELP("Configure background scanning",
			    "[-i, --iface=<interface index>]\n"
			    "<-t, --type simple/learn/none> : The scanning type, "
			    "use none to disable\n"
			    "[-s, --short-interval <val>] : Short scan interval (default: 30)\n"
			    "[-l, --long-interval <val>] : Long scan interval (default: 300)\n"
			    "[-r, --rssi-threshold <val>] : Signal strength threshold "
			    "(default: disabled)\n"
			    "[-b, --btm-queries <val>] : BTM queries before scanning "
			    "(default: disabled)"),
		 cmd_wifi_set_bgscan,
		 2, 6);
#endif /* CONFIG_WIFI_NM_WPA_SUPPLICANT_BGSCAN */

SHELL_SUBCMD_ADD((wifi), bss_max_idle_period, NULL,
		 SHELL_HELP("Set BSS max idle period preference timeout",
			    "[-i, --iface=<interface index>]\n"
			    "<timeout(in seconds)>"),
		 cmd_wifi_set_bss_max_idle_period,
		 2, 2);

SHELL_SUBCMD_ADD((wifi), channel, NULL,
		 SHELL_HELP("Wifi channel setting",
			    "This command is used to set the channel when\n"
			    "monitor or TX-Injection mode is enabled\n"
			    "Currently 20 MHz is only supported and no BW parameter is provided\n"
			    "[-i, --iface=<interface index>]\n"
			    "[-c, --channel <chan>] : Set a specific channel number "
			    "to the lower layer\n"
			    "[-g, --get] : Get current set channel number from the lower layer\n"
			    "Examples:\n"
			    "Get operation example for interface index 1\n"
			    "wifi channel -g -i1\n"
			    "Set operation example for interface index 1 (setting channel 5)\n"
			    "wifi -i1 -c5"),
		 cmd_wifi_channel,
		 2, 6);

SHELL_SUBCMD_ADD((wifi), config, NULL,
		 SHELL_HELP("Configure STA parameters",
			    "[-i, --iface=<interface index>]\n"
			    "-o, --okc=<0/1>: Opportunistic Key Caching. 0: disable, 1: enable"),
		 cmd_wifi_config_params,
		 3, 12);

SHELL_SUBCMD_ADD((wifi), connect, NULL,
		 SHELL_HELP("Connect to a Wi-Fi AP",
			    "[-i, --iface=<interface index>]\n"
			    "<-s --ssid \"<SSID>\">: SSID to connect\n"
			    "[-c --channel]: Channel that needs to be scanned for connection. "
			    "Value 0 indicates any channel\n"
			    "[-b, --band] 0: any band (2:2.4GHz, 5:5GHz, 6:6GHz]\n"
			    "[-p, --passphrase]: Passphrase (valid only for secure SSIDs)\n"
			    "[-k, --key-mgmt]: Key Management type (valid only for secure SSIDs)\n"
			    "0:None, 1:WPA2-PSK, 2:WPA2-PSK-256, 3:SAE-HNP, 4:SAE-H2E, "
			    "5:SAE-AUTO, 6:WAPI, "
			    "7:EAP-TLS, 8:WEP, 9:WPA-PSK, 10:WPA-Auto-Personal, 11:DPP, "
			    "12:EAP-PEAP-MSCHAPv2, 13:EAP-PEAP-GTC, 14:EAP-TTLS-MSCHAPv2, "
			    "15:EAP-PEAP-TLS, 20:SAE-EXT-KEY\n"
			    "[-w, --ieee-80211w]: MFP (optional: needs security type to be "
			    "specified): 0:Disable, 1:Optional, 2:Required\n"
			    "[-m, --bssid]: MAC address of the AP (BSSID)\n"
			    "[-t, --timeout]: Timeout for the connection attempt (in seconds)\n"
			    "[-a, --anon-id]: Anonymous identity for enterprise mode\n"
			    "[-K, --key1-pwd for eap phase1 or --key2-pwd for eap phase2]:\n"
			    "Private key passwd for enterprise mode. Default is no password "
			    "for private key\n"
			    "[-S, --wpa3-enterprise]: WPA3 enterprise mode:\n"
			    "Default is 0. 0:No WPA3 enterprise mode, "
			    "1:Suite-b mode, 2:Suite-b-192-bit mode, 3:WPA3-enterprise-only mode\n"
			    "[-T, --TLS-cipher]: 0:TLS-NONE, 1:TLS-ECC-P384, 2:TLS-RSA-3K\n"
			    "[-A, --verify-peer-cert]: apply for EAP-PEAP-MSCHAPv2 and "
			    "EAP-TTLS-MSCHAPv2\n"
			    "Default is 0. 0:do not use CA to verify peer, "
			    "1:use CA to verify peer\n"
			    "[-V, --eap-version]: 0 or 1. Default is 1: use eap version 1\n"
			    "[-I, --eap-id1]: Client Identity. Default is no eap identity\n"
			    "[-P, --eap-pwd1]: Client Password. "
			    "Default is no password for eap user\n"
			    "[-R, --ieee-80211r]: Use IEEE80211R fast BSS transition connect\n"
			    "[-e, --server-cert-domain-exact]: Full domain names for "
			    "server certificate match\n"
			    "[-x, --server-cert-domain-suffix]: Domain name suffixes for "
			    "server certificate match"),
		 cmd_wifi_connect,
		 2, 46);

SHELL_SUBCMD_ADD((wifi), disconnect, NULL,
		 SHELL_HELP("Disconnect from the Wi-Fi AP",
			    "[-i, --iface=<interface index>]"),
		 cmd_wifi_disconnect,
		 1, 2);

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_DPP
SHELL_STATIC_SUBCMD_SET_CREATE(
	wifi_cmd_dpp,
	SHELL_CMD_ARG(configurator_add, NULL,
		      SHELL_HELP("Add DPP configurator",
				 "[-i, --iface=<interface index>]"),
		      cmd_wifi_dpp_configurator_add, 1, 2),
	SHELL_CMD_ARG(auth_init, NULL,
		      SHELL_HELP("DPP start auth request",
				 "[-i, --iface=<interface index>]\n"
				 "-p --peer <peer_bootstrap_id>\n"
				 "[-r --role <1/2>]: DPP role default 1: "
				 "1: configurator, 2: enrollee\n"
				 "[-c --configurator <configurator_id>]\n"
				 "[-m --mode <1/2>]: Peer mode. 1: STA, 2: AP\n"
				 "[-s --ssid <SSID>]: SSID"),
		      cmd_wifi_dpp_auth_init, 3, 10),
	SHELL_CMD_ARG(qr_code, NULL,
		      SHELL_HELP("Input QR code",
				 "[-i, --iface=<interface index>]\n"
				 "<qr_code_string>"),
		      cmd_wifi_dpp_qr_code, 2, 2),
	SHELL_CMD_ARG(chirp, NULL,
		      SHELL_HELP("DPP chirp",
				 "[-i, --iface=<interface index>]\n"
				 "-o --own <own_bootstrap_id>\n"
				 "-f --freq <listen_freq>"),
		      cmd_wifi_dpp_chirp, 5, 2),
	SHELL_CMD_ARG(listen, NULL,
		      SHELL_HELP("DPP listen",
				 "[-i, --iface=<interface index>]\n"
				 "-f --freq <listen_freq>\n"
				 "-r --role <1/2>: DPP role. 1: configurator, 2: enrollee"),
		      cmd_wifi_dpp_listen, 5, 2),
	SHELL_CMD_ARG(btstrap_gen, NULL,
		      SHELL_HELP("DPP bootstrap generate",
				 "[-i, --iface=<interface index>]\n"
				 "[-t --type <1/2/3>]\n"
				 "Bootstrap type. 1: qr_code, 2: pkex, 3: nfc\n"
				 "Currently only support qr_code\n"
				 "[-o --opclass <operating_class>]\n"
				 "[-h --channel <channel>]\n"
				 "[-a --mac <mac_addr>]"),
		      cmd_wifi_dpp_btstrap_gen, 1, 10),
	SHELL_CMD_ARG(btstrap_get_uri, NULL,
		      SHELL_HELP("Get DPP bootstrap uri by id",
				 "[-i, --iface=<interface index>]\n"
				 "<bootstrap_id>"),
		      cmd_wifi_dpp_btstrap_get_uri, 2, 2),
	SHELL_CMD_ARG(configurator_set, NULL,
		      SHELL_HELP("Set DPP configurator parameters",
				 "[-i, --iface=<interface index>]\n"
				 "-c --configurator <configurator_id>\n"
				 "[-m --mode <1/2>]: Peer mode. 1: STA, 2: AP\n"
				 "[-s --ssid <SSID>]: SSID"),
		      cmd_wifi_dpp_configurator_set, 3, 6),
	SHELL_CMD_ARG(resp_timeout_set, NULL,
		      SHELL_HELP("Set DPP RX response wait timeout ms",
				 "[-i, --iface=<interface index>]\n"
				 "<timeout_ms>"),
		      cmd_wifi_dpp_resp_timeout_set, 2, 2),
	SHELL_CMD_ARG(ap_btstrap_gen, NULL,
		      SHELL_HELP("AP DPP bootstrap generate",
				 "[-i, --iface=<interface index>]\n"
				 "[-t --type <1/2/3>]\n"
				 "Bootstrap type. 1: qr_code, 2: pkex, 3: nfc\n"
				 "Currently only support qr_code\n"
				 "[-o --opclass <operating_class>]\n"
				 "[-h --channel <channel>]\n"
				 "[-a --mac <mac_addr>]"),
		      cmd_wifi_dpp_ap_btstrap_gen, 1, 10),
	SHELL_CMD_ARG(ap_btstrap_get_uri, NULL,
		      SHELL_HELP("AP get DPP bootstrap uri by id",
				 "[-i, --iface=<interface index>]\n"
				 "<bootstrap_id>"),
		      cmd_wifi_dpp_ap_btstrap_get_uri, 2, 2),
	SHELL_CMD_ARG(ap_qr_code, NULL,
		      SHELL_HELP("AP Input QR code",
				 "[-i, --iface=<interface index>]\n"
				 "<qr_code_string>"),
		      cmd_wifi_dpp_ap_qr_code, 2, 2),
	SHELL_CMD_ARG(ap_auth_init, NULL,
		      SHELL_HELP("AP DPP start auth request as enrollee",
				 "[-i, --iface=<interface index>]\n"
				 "-p --peer <peer_bootstrap_id>"),
		      cmd_wifi_dpp_ap_auth_init, 3, 2),
	SHELL_CMD_ARG(reconfig, NULL,
		      SHELL_HELP("Reconfig network by id",
				 "[-i, --iface=<interface index>]\n"
				 "<network_id>"),
		      cmd_wifi_dpp_reconfig, 2, 2),
	SHELL_SUBCMD_SET_END
);

SHELL_SUBCMD_ADD((wifi), dpp, &wifi_cmd_dpp,
		 "DPP actions.",
		 NULL,
		 0, 0);
#endif /* CONFIG_WIFI_NM_WPA_SUPPLICANT_DPP */

SHELL_SUBCMD_ADD((wifi), mode, NULL,
		 SHELL_HELP("Mode operational setting",
			    "This command may be used to set the Wi-Fi device into a specific "
			    "mode of operation\n"
			    "[-i, --iface=<interface index>]\n"
			    "[-s, --sta] : Station mode\n"
			    "[-m, --monitor] : Monitor mode\n"
			    "[-a, --ap] : AP mode\n"
			    "[-k, --softap] : Softap mode\n"
			    "[-h, --help] : Help\n"
			    "Examples:\n"
			    "Get operation example for interface index 1\n"
			    "wifi mode -i1\n"
			    "Set operation example for interface index 1 - "
			    "set station+promiscuous\n"
			    "wifi mode -i1 -sp"),
		 cmd_wifi_mode,
		 1, 11);

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_P2P
SHELL_STATIC_SUBCMD_SET_CREATE(
	wifi_cmd_p2p,
	SHELL_CMD_ARG(find, NULL,
		      SHELL_HELP("Start P2P device discovery",
				 "[-i, --iface=<interface index>]\n"
				 "[-t, --timeout=<timeout in seconds>]: Discovery timeout\n"
				 "[-T, --type=<0|1|2>]: Discovery type\n"
				 "0: Social channels only (1, 6, 11)\n"
				 "1: Progressive scan (all channels)\n"
				 "2: Full scan first, then social (default)"),
		      cmd_wifi_p2p_find, 1, 6),
	SHELL_CMD_ARG(stop_find, NULL,
		      SHELL_HELP("Stop P2P device discovery",
				 "[-i, --iface=<interface index>]"),
		      cmd_wifi_p2p_stop_find, 1, 2),
	SHELL_CMD_ARG(peer, NULL,
		      SHELL_HELP("Show information about P2P peers",
				 "[-i, --iface=<interface index>]\n"
				 "<MAC address>: Show detailed info for specific peer "
				 "(format: XX:XX:XX:XX:XX:XX)"),
		      cmd_wifi_p2p_peer, 1, 3),
	SHELL_CMD_ARG(connect, NULL,
		      SHELL_HELP("Connect to a P2P peer",
				 "[-i, --iface=<interface index>]\n"
				 "<MAC address>: Peer device MAC address "
				 "(format: XX:XX:XX:XX:XX:XX)\n"
				 "<pbc>: Use Push Button Configuration\n"
				 "<pin>: Use PIN method\n"
				 "- Without PIN: Device displays generated PIN for peer to enter\n"
				 "- With PIN: Device uses provided PIN to connect\n"
				 "[PIN]: 8-digit PIN (optional, generates if omitted)\n"
				 "[-g, --go-intent=<0-15>]: GO intent "
				 "(0=client, 15=GO, default: 0)\n"
				 "[-f, --freq=<frequency>]: Frequency in MHz (default: 2462)\n"
				 "[-j, --join]: Join an existing group (as a client) "
				 "instead of starting GO negotiation\n"
				 "Examples:\n"
				 "wifi p2p connect 9c:b1:50:e3:81:96 pin -g 0  (displays PIN)\n"
				 "wifi p2p connect 9c:b1:50:e3:81:96 pin 12345670 -g 0  "
				 "(uses PIN)\n"
				 "wifi p2p connect f4:ce:36:01:00:38 pbc --join  "
				 "(join existing group)"),
		      cmd_wifi_p2p_connect, 3, 6),
	SHELL_CMD_ARG(group_add, NULL,
		      SHELL_HELP("Add a P2P group (start as GO)",
				 "[-i, --iface=<interface index>]\n"
				 "[-f, --freq=<MHz>]: Frequency in MHz (0 = auto)\n"
				 "[-p, --persistent=<id>]: Persistent group ID "
				 "(-1 = not persistent)\n"
				 "[-h, --ht40]: Enable HT40\n"
				 "[-v, --vht]: Enable VHT (also enables HT40)\n"
				 "[-H, --he]: Enable HE\n"
				 "[-e, --edmg]: Enable EDMG\n"
				 "[-b, --go-bssid=<MAC>]: GO BSSID (format: XX:XX:XX:XX:XX:XX)"),
		      cmd_wifi_p2p_group_add, 1, 10),
	SHELL_CMD_ARG(group_remove, NULL,
		      SHELL_HELP("Remove a P2P group",
				 "[-i, --iface=<interface index>]\n"
				 "<ifname>: Interface name (e.g., wlan0)"),
		      cmd_wifi_p2p_group_remove, 2, 3),
	SHELL_CMD_ARG(invite, NULL,
		      SHELL_HELP("Invite a peer to a P2P group",
				 "[-i, --iface=<interface index>]\n"
				 "[-p, --persistent=<id>]: Persistent group ID\n"
				 "[-g, --group=<ifname>]: Group interface name\n"
				 "[-P, --peer=<MAC>]: Peer MAC address "
				 "(format: XX:XX:XX:XX:XX:XX)\n"
				 "[-f, --freq=<MHz>]: Frequency in MHz (0 = auto)\n"
				 "[-d, --go-dev-addr=<MAC>]: GO device address (for group type)\n"
				 "Examples:\n"
				 "wifi p2p invite --persistent=<id> <peer MAC>\n"
				 "wifi p2p invite --group=<ifname> --peer=<MAC> [options]"),
		      cmd_wifi_p2p_invite, 2, 8),
	SHELL_CMD_ARG(power_save, NULL,
		      SHELL_HELP("Set P2P power save mode",
				 "[-i, --iface=<interface index>]\n"
				 "<on|off>\n"
				 "<on>: Enable P2P power save\n"
				 "<off>: Disable P2P power save"),
		      cmd_wifi_p2p_power_save, 2, 3),
	SHELL_SUBCMD_SET_END
);

SHELL_SUBCMD_ADD((wifi), p2p, &wifi_cmd_p2p,
		 "Wi-Fi Direct (P2P) commands.",
		 NULL,
		 0, 0);
#endif /* CONFIG_WIFI_NM_WPA_SUPPLICANT_P2P */

SHELL_SUBCMD_ADD((wifi), packet_filter, NULL,
		 SHELL_HELP("Mode filter setting",
			    "This command is used to set packet filter setting when "
			    "monitor, TX-Injection and promiscuous mode is enabled. "
			    "The different packet filter modes are control, management, "
			    "data and enable all filters\n"
			    "[-i, --iface=<interface index>]\n"
			    "[-a, --all] : Enable all packet filter modes\n"
			    "[-m, --mgmt] : Enable management packets to allowed up the stack\n"
			    "[-c, --ctrl] : Enable control packets to be allowed up the stack\n"
			    "[-d, --data] : Enable Data packets to be allowed up the stack\n"
			    "[-g, --get] : Get current filter settings for a specific "
			    "interface index\n"
			    "[-b, --capture-len <len>] : Capture length buffer size for each "
			    "packet to be captured\n"
			    "Examples:\n"
			    "Get operation example for interface index 1\n"
			    "wifi packet_filter -g -i1\n"
			    "Set operation example for interface index 1 - set data+management "
			    "frame filter\n"
			    "wifi packet_filter -i1 -md"),
		 cmd_wifi_packet_filter,
		 2, 10);

SHELL_SUBCMD_ADD((wifi), pmksa_flush, NULL,
		 SHELL_HELP("Flush PMKSA cache entries",
			    "[-i, --iface=<interface index>]"),
		 cmd_wifi_pmksa_flush,
		 1, 2);

SHELL_SUBCMD_ADD((wifi), ps, NULL,
		 SHELL_HELP("Configure or display Wi-Fi power save state",
			    "[-i, --iface=<interface index>]\n"
			    "[on/off]"),
		 cmd_wifi_ps,
		 1, 2);

SHELL_SUBCMD_ADD((wifi), ps_exit_strategy, NULL,
		 SHELL_HELP("Set PS exit strategy",
			    "[-i, --iface=<interface index>]\n"
			    "tim | custom\n"
			    "tim : Set PS exit strategy to Every TIM\n"
			    "custom : Set PS exit strategy to Custom"),
		 cmd_wifi_ps_exit_strategy,
		 2, 2);

SHELL_SUBCMD_ADD((wifi), ps_listen_interval, NULL,
		 SHELL_HELP("Set PS listen interval",
			    "[-i, --iface=<interface index>]\n"
			    "<val> - Listen interval in the range of <0-65535>"),
		 cmd_wifi_listen_interval,
		 2, 2);

SHELL_SUBCMD_ADD((wifi), ps_mode, NULL,
		 SHELL_HELP("Set PS mode",
			    "[-i, --iface=<interface index>]\n"
			    "<mode: legacy/WMM>"),
		 cmd_wifi_ps_mode,
		 2, 2);

SHELL_SUBCMD_ADD((wifi), ps_timeout, NULL,
		 SHELL_HELP("Set PS inactivity timer value",
			    "[-i, --iface=<interface index>]\n"
			    "<val> - PS inactivity timer(in ms)"),
		 cmd_wifi_ps_timeout,
		 2, 2);

SHELL_SUBCMD_ADD((wifi), ps_wakeup_mode, NULL,
		 SHELL_HELP("Set PS wakeup mode interval",
			    "[-i, --iface=<interface index>]\n"
			    "<wakeup_mode: DTIM/Listen Interval>"),
		 cmd_wifi_ps_wakeup_mode,
		 2, 2);

SHELL_SUBCMD_ADD((wifi), reg_domain, NULL,
		 SHELL_HELP("Set or Get Wi-Fi regulatory domain",
			    "[-i, --iface=<interface index>]\n"
			    "[ISO/IEC 3166-1 alpha2]: Regulatory domain\n"
			    "[-f]: Force to use this regulatory hint over any other "
			    "regulatory hints\n"
			    "Note1: The behavior of this command is dependent on the "
			    "Wi-Fi driver/chipset implementation\n"
			    "Note2: This may cause regulatory compliance issues, "
			    "use it at your own risk.\n"
			    "[-v]: Verbose, display the per-channel regulatory information"),
		 cmd_wifi_reg_domain,
		 1, 5);

SHELL_SUBCMD_ADD((wifi), rts_threshold, NULL,
		 SHELL_HELP("Set RTS value or turn it off",
			    "[-i, --iface=<interface index>]\n"
			    "<rts_threshold value> | off"),
		 cmd_wifi_set_rts_threshold,
		 1, 2);

SHELL_SUBCMD_ADD((wifi), scan, NULL,
		 SHELL_HELP("Scan for Wi-Fi APs",
			    "[-i, --iface=<interface index>]\n"
			    "[-t, --type <active/passive>] : Preferred mode of scan. "
			    "The actual mode of scan can depend on factors such as the Wi-Fi chip "
			    "implementation, regulatory domain restrictions. "
			    "Default type is active\n"
			    "[-b, --bands <Comma separated list of band values (2/5/6)>] : "
			    "Bands to be scanned where 2: 2.4 GHz, 5: 5 GHz, 6: 6 GHz\n"
			    "[-a, --dwell_time_active <val_in_ms>] : "
			    "Active scan dwell time (in ms) on a channel. Range 5 ms to 1000 ms\n"
			    "[-p, --dwell_time_passive <val_in_ms>] : "
			    "Passive scan dwell time (in ms) on a channel. "
			    "Range 10 ms to 1000 ms\n"
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
			    "CONFIG_WIFI_MGMT_SCAN_CHAN_MAX_MANUAL"),
		 cmd_wifi_scan,
		 1, 10);

SHELL_SUBCMD_ADD((wifi), statistics, NULL,
		 SHELL_HELP("Show Wi-Fi interface statistics",
			    "[-i, --iface=<interface index>]\n"
			    "[reset] : Reset Wi-Fi interface statistics"),
		 cmd_wifi_stats,
		 1, 3);

SHELL_SUBCMD_ADD((wifi), status, NULL,
		 SHELL_HELP("Status of the Wi-Fi interface",
			    "[-i, --iface=<interface index>]"),
		 cmd_wifi_status,
		 1, 2);

SHELL_STATIC_SUBCMD_SET_CREATE(wifi_twt_ops,
	SHELL_CMD_ARG(quick_setup, NULL,
		      SHELL_HELP("Start a TWT flow with defaults",
				 "[-i, --iface=<interface index>]\n"
				 "<twt_wake_interval: 1-262144us>\n"
				 "<twt_interval: 1us-2^31us>"),
		      cmd_wifi_twt_setup_quick,
		      3, 2),
	SHELL_CMD_ARG(setup, NULL,
		      SHELL_HELP("Start a TWT flow",
				 "[-i, --iface=<interface index>]\n"
				 "<-n --negotiation-type>: 0: Individual, "
				 "1: Broadcast, 2: Wake TBTT\n"
				 "<-c --setup-cmd>: 0: Request, 1: Suggest, 2: Demand\n"
				 "<-t --dialog-token>: 1-255\n"
				 "<-f --flow-id>: 0-7\n"
				 "<-r --responder>: 0/1\n"
				 "<-T --trigger>: 0/1\n"
				 "<-I --implicit>:0/1\n"
				 "<-a --announce>: 0/1\n"
				 "<-w --wake-interval>: 1-262144us\n"
				 "<-p --interval>: 1us-2^31us\n"
				 "<-D --wake-ahead-duration>: 0us-2^31us\n"
				 "<-d --info-disable>: 0/1\n"
				 "<-e --exponent>: 0-31\n"
				 "<-m --mantissa>: 1-2^16"),
		      cmd_wifi_twt_setup,
		      25, 7),
	SHELL_CMD_ARG(btwt_setup, NULL,
		      SHELL_HELP("Start a BTWT flow",
				 "[-i, --iface=<interface index>]\n"
				 "<sta_wait> <offset> <twtli> <session_num>: 2-5\n"
				 "<id0> <mantissa0> <exponent0> <nominal_wake0>: 64-255\n"
				 "<id1> <mantissa1> <exponent1> <nominal_wake1>: 64-255\n"
				 "<idx> <mantissax> <exponentx> <nominal_wakex>: 64-255\n"
				 " The total number of '0, 1, ..., x' is session_num"),
		      cmd_wifi_btwt_setup,
		      13, 12),
	SHELL_CMD_ARG(teardown, NULL,
		      SHELL_HELP("Teardown a TWT flow",
				 "<negotiation_type, 0: Individual, 1: Broadcast, 2: Wake TBTT>\n"
				 "<setup_cmd: 0: Request, 1: Suggest, 2: Demand>\n"
				 "<dialog_token: 1-255> <flow_id: 0-7>"),
		      cmd_wifi_twt_teardown,
		      5, 0),
	SHELL_CMD_ARG(teardown_all, NULL,
		      SHELL_HELP("Teardown all TWT flows", ""),
		      cmd_wifi_twt_teardown_all,
		      1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_SUBCMD_ADD((wifi), twt, &wifi_twt_ops,
		 "Manage TWT flows.",
		 NULL,
		 0, 0);

SHELL_SUBCMD_ADD((wifi), version, NULL,
		 SHELL_HELP("Print Wi-Fi Driver and Firmware versions",
			    "[-i, --iface=<interface index>]"),
		 cmd_wifi_version,
		 1, 2);

SHELL_SUBCMD_ADD((wifi), wps_pbc, NULL,
		 SHELL_HELP("Start a WPS PBC connection",
			    "[-i, --iface=<interface index>]"),
		 cmd_wifi_wps_pbc,
		 1, 2);

SHELL_SUBCMD_ADD((wifi), wps_pin, NULL,
		 SHELL_HELP("Set and get WPS pin",
			    "[-i, --iface=<interface index>]\n"
			    "[pin] Only applicable for set"),
		 cmd_wifi_wps_pin,
		 1, 3);

SHELL_SUBCMD_SET_CREATE(wifi_commands, (wifi));

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


	net_mgmt_init_event_callback(&wifi_shell_scan_cb,
				     wifi_mgmt_scan_event_handler,
				     WIFI_SHELL_SCAN_EVENTS);

	return 0;
}

SYS_INIT(wifi_shell_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
