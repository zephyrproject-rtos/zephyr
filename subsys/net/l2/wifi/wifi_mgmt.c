/*
 * Copyright (c) 2016 Intel Corporation.
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L /* For strnlen() */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_wifi_mgmt, CONFIG_NET_L2_WIFI_MGMT_LOG_LEVEL);

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#ifdef CONFIG_WIFI_NM
#include <zephyr/net/wifi_nm.h>
#endif /* CONFIG_WIFI_NM */

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_ROAMING
#define MAX_NEIGHBOR_AP_LIMIT 6U
#define MAX_EVENT_STR_LEN 32

struct wifi_rrm_neighbor_ap_t {
	char ssid[WIFI_SSID_MAX_LEN + 1];
	uint8_t bssid[WIFI_SSID_MAX_LEN];
	uint8_t bssid_info[WIFI_SSID_MAX_LEN];
	int op_class;
	int channel;
	int phy_type;
};

struct wifi_rrm_neighbor_report_t {
	struct wifi_rrm_neighbor_ap_t neighbor_ap[MAX_NEIGHBOR_AP_LIMIT];
	int neighbor_cnt;
};

struct wifi_roaming_params {
	bool is_11r_used;
	bool is_11k_enabled;
	struct wifi_rrm_neighbor_report_t neighbor_rep;
};

static struct wifi_roaming_params roaming_params;
#endif

const char *wifi_security_txt(enum wifi_security_type security)
{
	switch (security) {
	case WIFI_SECURITY_TYPE_NONE:
		return "OPEN";
	case WIFI_SECURITY_TYPE_PSK:
		return "WPA2-PSK";
	case WIFI_SECURITY_TYPE_PSK_SHA256:
		return "WPA2-PSK-SHA256";
	case WIFI_SECURITY_TYPE_SAE_HNP:
		return "WPA3-SAE-HNP";
	case WIFI_SECURITY_TYPE_SAE_H2E:
		return "WPA3-SAE-H2E";
	case WIFI_SECURITY_TYPE_SAE_AUTO:
		return "WPA3-SAE-AUTO";
	case WIFI_SECURITY_TYPE_WAPI:
		return "WAPI";
	case WIFI_SECURITY_TYPE_EAP_TLS:
		return "EAP-TLS";
	case WIFI_SECURITY_TYPE_WEP:
		return "WEP";
	case WIFI_SECURITY_TYPE_WPA_PSK:
		return "WPA-PSK";
	case WIFI_SECURITY_TYPE_WPA_AUTO_PERSONAL:
		return "WPA/WPA2/WPA3 PSK";
	case WIFI_SECURITY_TYPE_DPP:
		return "DPP";
	case WIFI_SECURITY_TYPE_EAP_PEAP_MSCHAPV2:
		return "EAP-PEAP-MSCHAPV2";
	case WIFI_SECURITY_TYPE_EAP_PEAP_GTC:
		return "EAP-PEAP-GTC";
	case WIFI_SECURITY_TYPE_EAP_TTLS_MSCHAPV2:
		return "EAP-TTLS-MSCHAPV2";
	case WIFI_SECURITY_TYPE_EAP_PEAP_TLS:
		return "EAP-PEAP-TLS";
	case WIFI_SECURITY_TYPE_EAP_TLS_SHA256:
		return "EAP-TLS-SHA256";
	case WIFI_SECURITY_TYPE_FT_PSK:
		return "FT-PSK";
	case WIFI_SECURITY_TYPE_FT_SAE:
		return "FT-SAE";
	case WIFI_SECURITY_TYPE_FT_EAP:
		return "FT-EAP";
	case WIFI_SECURITY_TYPE_FT_EAP_SHA384:
		return "FT-EAP-SHA384";
	case WIFI_SECURITY_TYPE_UNKNOWN:
	default:
		return "UNKNOWN";
	}
}

const char *wifi_mfp_txt(enum wifi_mfp_options mfp)
{
	switch (mfp) {
	case WIFI_MFP_DISABLE:
		return "Disable";
	case WIFI_MFP_OPTIONAL:
		return "Optional";
	case WIFI_MFP_REQUIRED:
		return "Required";
	case WIFI_MFP_UNKNOWN:
	default:
		return "UNKNOWN";
	}
}

const char *wifi_band_txt(enum wifi_frequency_bands band)
{
	switch (band) {
	case WIFI_FREQ_BAND_2_4_GHZ:
		return "2.4GHz";
	case WIFI_FREQ_BAND_5_GHZ:
		return "5GHz";
	case WIFI_FREQ_BAND_6_GHZ:
		return "6GHz";
	case WIFI_FREQ_BAND_UNKNOWN:
	default:
		return "UNKNOWN";
	}
}

const char *wifi_state_txt(enum wifi_iface_state state)
{
	switch (state) {
	case WIFI_STATE_DISCONNECTED:
		return "DISCONNECTED";
	case WIFI_STATE_INACTIVE:
		return "INACTIVE";
	case WIFI_STATE_INTERFACE_DISABLED:
		return "INTERFACE_DISABLED";
	case WIFI_STATE_SCANNING:
		return "SCANNING";
	case WIFI_STATE_AUTHENTICATING:
		return "AUTHENTICATING";
	case WIFI_STATE_ASSOCIATING:
		return "ASSOCIATING";
	case WIFI_STATE_ASSOCIATED:
		return "ASSOCIATED";
	case WIFI_STATE_4WAY_HANDSHAKE:
		return "4WAY_HANDSHAKE";
	case WIFI_STATE_GROUP_HANDSHAKE:
		return "GROUP_HANDSHAKE";
	case WIFI_STATE_COMPLETED:
		return "COMPLETED";
	case WIFI_STATE_UNKNOWN:
	default:
		return "UNKNOWN";
	}
}

const char *wifi_mode_txt(enum wifi_iface_mode mode)
{
	switch (mode) {
	case WIFI_MODE_INFRA:
		return "STATION";
	case WIFI_MODE_IBSS:
		return "ADHOC";
	case WIFI_MODE_AP:
		return "ACCESS POINT";
	case WIFI_MODE_P2P_GO:
		return "P2P GROUP OWNER";
	case WIFI_MODE_P2P_GROUP_FORMATION:
		return "P2P GROUP FORMATION";
	case WIFI_MODE_MESH:
		return "MESH";
	case WIFI_MODE_UNKNOWN:
	default:
		return "UNKNOWN";
	}
}

const char *wifi_link_mode_txt(enum wifi_link_mode link_mode)
{
	switch (link_mode) {
	case WIFI_0:
		return "WIFI 0 (802.11)";
	case WIFI_1:
		return "WIFI 1 (802.11b)";
	case WIFI_2:
		return "WIFI 2 (802.11a)";
	case WIFI_3:
		return "WIFI 3 (802.11g)";
	case WIFI_4:
		return "WIFI 4 (802.11n/HT)";
	case WIFI_5:
		return "WIFI 5 (802.11ac/VHT)";
	case WIFI_6:
		return "WIFI 6 (802.11ax/HE)";
	case WIFI_6E:
		return "WIFI 6E (802.11ax 6GHz/HE)";
	case WIFI_7:
		return "WIFI 7 (802.11be/EHT)";
	case WIFI_LINK_MODE_UNKNOWN:
	default:
		return "UNKNOWN";
	}
}

const char *wifi_ps_txt(enum wifi_ps ps_name)
{
	switch (ps_name) {
	case WIFI_PS_DISABLED:
		return "Power save disabled";
	case WIFI_PS_ENABLED:
		return "Power save enabled";
	default:
		return "UNKNOWN";
	}
}

const char *wifi_ps_mode_txt(enum wifi_ps_mode ps_mode)
{
	switch (ps_mode) {
	case WIFI_PS_MODE_LEGACY:
		return "Legacy power save";
	case WIFI_PS_MODE_WMM:
		return "WMM power save";
	default:
		return "UNKNOWN";
	}
}

const char *wifi_twt_operation_txt(enum wifi_twt_operation twt_operation)
{
	switch (twt_operation) {
	case WIFI_TWT_SETUP:
		return "TWT setup";
	case WIFI_TWT_TEARDOWN:
		return "TWT teardown";
	default:
		return "UNKNOWN";
	}
}

const char *wifi_twt_negotiation_type_txt(enum wifi_twt_negotiation_type twt_negotiation)
{
	switch (twt_negotiation) {
	case WIFI_TWT_INDIVIDUAL:
		return "TWT individual negotiation";
	case WIFI_TWT_BROADCAST:
		return "TWT broadcast negotiation";
	case WIFI_TWT_WAKE_TBTT:
		return "TWT wake TBTT negotiation";
	default:
		return "UNKNOWN";
	}
}

const char *wifi_twt_setup_cmd_txt(enum wifi_twt_setup_cmd twt_setup)
{
	switch (twt_setup) {
	case WIFI_TWT_SETUP_CMD_REQUEST:
		return "TWT request";
	case WIFI_TWT_SETUP_CMD_SUGGEST:
		return "TWT suggest";
	case WIFI_TWT_SETUP_CMD_DEMAND:
		return "TWT demand";
	case WIFI_TWT_SETUP_CMD_GROUPING:
		return "TWT grouping";
	case WIFI_TWT_SETUP_CMD_ACCEPT:
		return "TWT accept";
	case WIFI_TWT_SETUP_CMD_ALTERNATE:
		return "TWT alternate";
	case WIFI_TWT_SETUP_CMD_DICTATE:
		return "TWT dictate";
	case WIFI_TWT_SETUP_CMD_REJECT:
		return "TWT reject";
	default:
		return "UNKNOWN";
	}
}

const char *wifi_ps_wakeup_mode_txt(enum wifi_ps_wakeup_mode ps_wakeup_mode)
{
	switch (ps_wakeup_mode) {
	case WIFI_PS_WAKEUP_MODE_DTIM:
		return "PS wakeup mode DTIM";
	case WIFI_PS_WAKEUP_MODE_LISTEN_INTERVAL:
		return "PS wakeup mode listen interval";
	default:
		return "UNKNOWN";
	}
}

const char * const wifi_ps_exit_strategy_txt(enum wifi_ps_exit_strategy ps_exit_strategy)
{
	switch (ps_exit_strategy) {
	case WIFI_PS_EXIT_EVERY_TIM:
		return "Every TIM";
	case WIFI_PS_EXIT_CUSTOM_ALGO:
		return "Custom algorithm";
	default:
		return "UNKNOWN";
	}
}

static const struct wifi_mgmt_ops *const get_wifi_api(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct net_wifi_mgmt_offload *off_api =
			(struct net_wifi_mgmt_offload *) dev->api;
#ifdef CONFIG_WIFI_NM
	struct wifi_nm_instance *nm = wifi_nm_get_instance_iface(iface);

	if (nm) {
		return nm->ops;
	}
#endif /* CONFIG_WIFI_NM */
	return off_api ? off_api->wifi_mgmt_api : NULL;
}

static int wifi_connect(uint32_t mgmt_request, struct net_if *iface,
			void *data, size_t len)
{
	struct wifi_connect_req_params *params =
		(struct wifi_connect_req_params *)data;
	const struct device *dev = net_if_get_device(iface);

	const struct wifi_mgmt_ops *const wifi_mgmt_api = get_wifi_api(iface);

	if (wifi_mgmt_api == NULL || wifi_mgmt_api->connect == NULL) {
		return -ENOTSUP;
	}

	LOG_HEXDUMP_DBG(params->ssid, params->ssid_length, "ssid");
	LOG_HEXDUMP_DBG(params->psk, params->psk_length, "psk");
	if (params->sae_password) {
		LOG_HEXDUMP_DBG(params->sae_password, params->sae_password_length, "sae");
	}
	NET_DBG("ch %u sec %u", params->channel, params->security);

	if ((params->security > WIFI_SECURITY_TYPE_MAX) ||
	    (params->ssid_length > WIFI_SSID_MAX_LEN) ||
	    (params->ssid_length == 0U) ||
	    ((params->security == WIFI_SECURITY_TYPE_PSK ||
		  params->security == WIFI_SECURITY_TYPE_WPA_PSK ||
		  params->security == WIFI_SECURITY_TYPE_PSK_SHA256 ||
		  params->security == WIFI_SECURITY_TYPE_WPA_AUTO_PERSONAL) &&
	     ((params->psk_length < 8) || (params->psk_length > 64) ||
	      (params->psk_length == 0U) || !params->psk)) ||
	    ((params->security == WIFI_SECURITY_TYPE_SAE_HNP ||
		  params->security == WIFI_SECURITY_TYPE_SAE_H2E ||
		  params->security == WIFI_SECURITY_TYPE_SAE_AUTO) &&
	      ((params->psk_length == 0U) || !params->psk) &&
		  ((params->sae_password_length == 0U) || !params->sae_password)) ||
	    ((params->channel != WIFI_CHANNEL_ANY) &&
	     (params->channel > WIFI_CHANNEL_MAX)) ||
	    !params->ssid) {
		return -EINVAL;
	}

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_ROAMING
	memset(&roaming_params, 0x0, sizeof(roaming_params));
	roaming_params.is_11r_used = params->ft_used;
#endif

	return wifi_mgmt_api->connect(dev, params);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_CONNECT, wifi_connect);

static void scan_result_cb(struct net_if *iface, int status,
			    struct wifi_scan_result *entry)
{
	if (!iface) {
		return;
	}

	if (!entry) {
		struct wifi_status scan_status = {
			.status = status,
		};

		net_mgmt_event_notify_with_info(NET_EVENT_WIFI_SCAN_DONE,
						iface, &scan_status,
						sizeof(struct wifi_status));
		return;
	}

#ifndef CONFIG_WIFI_MGMT_RAW_SCAN_RESULTS_ONLY
	net_mgmt_event_notify_with_info(NET_EVENT_WIFI_SCAN_RESULT, iface,
					entry, sizeof(struct wifi_scan_result));
#endif /* CONFIG_WIFI_MGMT_RAW_SCAN_RESULTS_ONLY */
}

static int wifi_scan(uint32_t mgmt_request, struct net_if *iface,
		     void *data, size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	struct wifi_scan_params *params = data;
	const struct wifi_mgmt_ops *const wifi_mgmt_api = get_wifi_api(iface);

	if (wifi_mgmt_api == NULL || wifi_mgmt_api->scan == NULL) {
		return -ENOTSUP;
	}

#ifdef CONFIG_WIFI_MGMT_FORCED_PASSIVE_SCAN
	struct wifi_scan_params default_params = {0};

	if (params == NULL) {
		params = &default_params;
	}
	params->scan_type = WIFI_SCAN_TYPE_PASSIVE;
#endif /* CONFIG_WIFI_MGMT_FORCED_PASSIVE_SCAN */

	return wifi_mgmt_api->scan(dev, params, scan_result_cb);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_SCAN, wifi_scan);

static int wifi_disconnect(uint32_t mgmt_request, struct net_if *iface,
			   void *data, size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	const struct wifi_mgmt_ops *const wifi_mgmt_api = get_wifi_api(iface);

	if (wifi_mgmt_api == NULL || wifi_mgmt_api->disconnect == NULL) {
		return -ENOTSUP;
	}

	return wifi_mgmt_api->disconnect(dev);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_DISCONNECT, wifi_disconnect);

void wifi_mgmt_raise_connect_result_event(struct net_if *iface, int status)
{
	struct wifi_status cnx_status = {
		.status = status,
	};

	net_mgmt_event_notify_with_info(NET_EVENT_WIFI_CONNECT_RESULT,
					iface, &cnx_status,
					sizeof(struct wifi_status));
}

void wifi_mgmt_raise_disconnect_result_event(struct net_if *iface, int status)
{
	struct wifi_status cnx_status = {
		.status = status,
	};

	net_mgmt_event_notify_with_info(NET_EVENT_WIFI_DISCONNECT_RESULT,
					iface, &cnx_status,
					sizeof(struct wifi_status));
}

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_ROAMING
static int wifi_start_roaming(uint32_t mgmt_request, struct net_if *iface,
			      void *data, size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	const struct wifi_mgmt_ops *const wifi_mgmt_api = get_wifi_api(iface);

	if (roaming_params.is_11r_used) {
		if (wifi_mgmt_api == NULL ||
		    wifi_mgmt_api->start_11r_roaming == NULL) {
			return -ENOTSUP;
		}

		return wifi_mgmt_api->start_11r_roaming(dev);
	} else if (roaming_params.is_11k_enabled) {
		memset(&roaming_params.neighbor_rep, 0x0, sizeof(roaming_params.neighbor_rep));
		if (wifi_mgmt_api == NULL
		    || wifi_mgmt_api->send_11k_neighbor_request == NULL) {
			return -ENOTSUP;
		}

		return wifi_mgmt_api->send_11k_neighbor_request(dev, NULL);
	} else if (wifi_mgmt_api == NULL || wifi_mgmt_api->btm_query == NULL) {
		return -ENOTSUP;
	}

	return wifi_mgmt_api->btm_query(dev, 0x10);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_START_ROAMING, wifi_start_roaming);

static int wifi_neighbor_rep_complete(uint32_t mgmt_request, struct net_if *iface,
				      void *data, size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	const struct wifi_mgmt_ops *const wifi_mgmt_api = get_wifi_api(iface);
	struct wifi_scan_params params = {0};

	for (int i = 0; i < roaming_params.neighbor_rep.neighbor_cnt; i++) {
		params.band_chan[i].channel = roaming_params.neighbor_rep.neighbor_ap[i].channel;
		if (params.band_chan[i].channel > 14) {
			params.band_chan[i].band = WIFI_FREQ_BAND_5_GHZ;
		} else {
			params.band_chan[i].band = WIFI_FREQ_BAND_2_4_GHZ;
		}
	}
	if (wifi_mgmt_api == NULL || wifi_mgmt_api->candidate_scan == NULL) {
		return -ENOTSUP;
	}

	return wifi_mgmt_api->candidate_scan(dev, &params);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_NEIGHBOR_REP_COMPLETE,
				  wifi_neighbor_rep_complete);

void wifi_mgmt_raise_neighbor_rep_recv_event(struct net_if *iface, char *inbuf, size_t buf_len)
{
	const uint8_t *buf = inbuf;
	char event[MAX_EVENT_STR_LEN + 1] = {0};
	char bssid[WIFI_SSID_MAX_LEN + 1] = {0};
	char bssid_info[WIFI_SSID_MAX_LEN + 1]  = {0};
	int op_class, channel, phy_type;
	int idx = roaming_params.neighbor_rep.neighbor_cnt;

	if (!buf || buf[0] == '\0') {
		return;
	}

	if (sscanf(buf,
		   "%" STRINGIFY(MAX_EVENT_STR_LEN) "s "
		   "bssid=%" STRINGIFY(WIFI_SSID_MAX_LEN) "s "
		   "info=%" STRINGIFY(WIFI_SSID_MAX_LEN) "s "
		   "op_class=%d chan=%d phy_type=%d",
		   event, bssid, bssid_info, &op_class, &channel, &phy_type) == 6) {
		int i;
		int match  = 0;
		size_t len = 0;

		for (i = 0; i < roaming_params.neighbor_rep.neighbor_cnt; i++) {
			if (strncmp((const char *)roaming_params.neighbor_rep.neighbor_ap[i].bssid,
				    bssid, sizeof(bssid)) == 0) {
				match = 1;
				break;
			}

			if (roaming_params.neighbor_rep.neighbor_ap[i].channel == channel) {
				match = 1;
				break;
			}
		}
		if (!match && (roaming_params.neighbor_rep.neighbor_cnt < MAX_NEIGHBOR_AP_LIMIT)) {
			memcpy((char *)roaming_params.neighbor_rep.neighbor_ap[idx].bssid,
			       bssid,
			       sizeof(roaming_params.neighbor_rep.neighbor_ap[idx].bssid));
			len = strnlen(bssid, sizeof(bssid) - 1);
			roaming_params.neighbor_rep.neighbor_ap[idx].bssid[len] = (uint8_t)'\0';

			memcpy((char *)roaming_params.neighbor_rep.neighbor_ap[idx].bssid_info,
			       bssid_info,
			       sizeof(roaming_params.neighbor_rep.neighbor_ap[idx].bssid_info));
			len = strnlen(bssid_info, sizeof(bssid_info) - 1);
			roaming_params.neighbor_rep.neighbor_ap[idx].bssid_info[len] =
				(uint8_t)'\0';

			roaming_params.neighbor_rep.neighbor_ap[idx].channel  = channel;
			roaming_params.neighbor_rep.neighbor_ap[idx].op_class = op_class;
			roaming_params.neighbor_rep.neighbor_ap[idx].phy_type = phy_type;

			roaming_params.neighbor_rep.neighbor_cnt += 1;
		} else if (match) {
			LOG_INF("BSSID already present in neighbor list, Skipping %s ",
				bssid);
		} else {
			LOG_INF("Maximum neighbors added to list, Skipping.");
		}
	} else {
		LOG_INF("Failed to Parse Neighbor Report - Skipping entry\n");
	}
}
#endif

static int wifi_ap_enable(uint32_t mgmt_request, struct net_if *iface,
			  void *data, size_t len)
{
	struct wifi_connect_req_params *params =
		(struct wifi_connect_req_params *)data;
	const struct device *dev = net_if_get_device(iface);
	const struct wifi_mgmt_ops *const wifi_mgmt_api = get_wifi_api(iface);

	if (wifi_mgmt_api == NULL || wifi_mgmt_api->ap_enable == NULL) {
		return -ENOTSUP;
	}

	return wifi_mgmt_api->ap_enable(dev, params);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_AP_ENABLE, wifi_ap_enable);

static int wifi_ap_disable(uint32_t mgmt_request, struct net_if *iface,
			  void *data, size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	const struct wifi_mgmt_ops *const wifi_mgmt_api = get_wifi_api(iface);

	if (wifi_mgmt_api == NULL || wifi_mgmt_api->ap_enable == NULL) {
		return -ENOTSUP;
	}

	return wifi_mgmt_api->ap_disable(dev);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_AP_DISABLE, wifi_ap_disable);

static int wifi_ap_sta_disconnect(uint32_t mgmt_request, struct net_if *iface,
				  void *data, size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	const struct wifi_mgmt_ops *const wifi_mgmt_api = get_wifi_api(iface);
	uint8_t *mac = data;

	if (dev == NULL) {
		return -ENODEV;
	}

	if (wifi_mgmt_api == NULL || wifi_mgmt_api->ap_sta_disconnect == NULL) {
		return -ENOTSUP;
	}

	if (!data || len != sizeof(uint8_t) * WIFI_MAC_ADDR_LEN) {
		return -EINVAL;
	}

	return wifi_mgmt_api->ap_sta_disconnect(dev, mac);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_AP_STA_DISCONNECT, wifi_ap_sta_disconnect);

static int wifi_ap_config_params(uint32_t mgmt_request, struct net_if *iface,
				 void *data, size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	const struct wifi_mgmt_ops *const wifi_mgmt_api = get_wifi_api(iface);
	struct wifi_ap_config_params *params = data;

	if (dev == NULL) {
		return -ENODEV;
	}

	if (wifi_mgmt_api == NULL ||
	    wifi_mgmt_api->ap_config_params == NULL) {
		return -ENOTSUP;
	}

	if (!data || len != sizeof(*params)) {
		return -EINVAL;
	}

	if (params->type & WIFI_AP_CONFIG_PARAM_MAX_NUM_STA) {
		if (params->max_num_sta > CONFIG_WIFI_MGMT_AP_MAX_NUM_STA) {
			LOG_INF("Maximum number of stations(%d) "
				"exceeded default configured value = %d.",
				params->max_num_sta, CONFIG_WIFI_MGMT_AP_MAX_NUM_STA);
			return -EINVAL;
		}
	}

	return wifi_mgmt_api->ap_config_params(dev, params);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_AP_CONFIG_PARAM, wifi_ap_config_params);

static int wifi_iface_status(uint32_t mgmt_request, struct net_if *iface,
			  void *data, size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	const struct wifi_mgmt_ops *const wifi_mgmt_api = get_wifi_api(iface);
	struct wifi_iface_status *status = data;

	if (wifi_mgmt_api == NULL || wifi_mgmt_api->iface_status == NULL) {
		return -ENOTSUP;
	}

	if (!data || len != sizeof(*status)) {
		return -EINVAL;
	}

	return wifi_mgmt_api->iface_status(dev, status);
}
NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_IFACE_STATUS, wifi_iface_status);

void wifi_mgmt_raise_iface_status_event(struct net_if *iface,
		struct wifi_iface_status *iface_status)
{
	net_mgmt_event_notify_with_info(NET_EVENT_WIFI_IFACE_STATUS,
					iface, iface_status,
					sizeof(struct wifi_iface_status));
}

#ifdef CONFIG_NET_STATISTICS_WIFI
static int wifi_iface_stats(uint32_t mgmt_request, struct net_if *iface,
			  void *data, size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	const struct wifi_mgmt_ops *const wifi_mgmt_api = get_wifi_api(iface);
	struct net_stats_wifi *stats = data;

	if (wifi_mgmt_api == NULL || wifi_mgmt_api->get_stats == NULL) {
		return -ENOTSUP;
	}

	if (!data || len != sizeof(*stats)) {
		return -EINVAL;
	}

	return wifi_mgmt_api->get_stats(dev, stats);
}
NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_STATS_GET_WIFI, wifi_iface_stats);

static int wifi_iface_stats_reset(uint32_t mgmt_request, struct net_if *iface,
				  void *data, size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	const struct wifi_mgmt_ops *const wifi_mgmt_api = get_wifi_api(iface);

	if (wifi_mgmt_api == NULL || wifi_mgmt_api->reset_stats == NULL) {
		return -ENOTSUP;
	}

	return wifi_mgmt_api->reset_stats(dev);
}
NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_STATS_RESET_WIFI, wifi_iface_stats_reset);
#endif /* CONFIG_NET_STATISTICS_WIFI */

static int wifi_11k_cfg(uint32_t mgmt_request, struct net_if *iface,
			void *data, size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	const struct wifi_mgmt_ops *const wifi_mgmt_api = get_wifi_api(iface);
	struct wifi_11k_params *params = data;

	if (wifi_mgmt_api == NULL || wifi_mgmt_api->cfg_11k == NULL) {
		return -ENOTSUP;
	}

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_ROAMING
	roaming_params.is_11k_enabled = params->enable_11k;
#endif

	return wifi_mgmt_api->cfg_11k(dev, params);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_11K_CONFIG, wifi_11k_cfg);

static int wifi_11k_neighbor_request(uint32_t mgmt_request, struct net_if *iface,
				     void *data, size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	const struct wifi_mgmt_ops *const wifi_mgmt_api = get_wifi_api(iface);
	struct wifi_11k_params *params = data;

	if (wifi_mgmt_api == NULL || wifi_mgmt_api->send_11k_neighbor_request == NULL) {
		return -ENOTSUP;
	}

	return wifi_mgmt_api->send_11k_neighbor_request(dev, params);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_11K_NEIGHBOR_REQUEST,
				  wifi_11k_neighbor_request);

static int wifi_set_power_save(uint32_t mgmt_request, struct net_if *iface,
			  void *data, size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	const struct wifi_mgmt_ops *const wifi_mgmt_api = get_wifi_api(iface);
	struct wifi_ps_params *ps_params = data;
	struct wifi_iface_status info = { 0 };

	if (wifi_mgmt_api == NULL || wifi_mgmt_api->set_power_save == NULL) {
		return -ENOTSUP;
	}

	switch (ps_params->type) {
	case WIFI_PS_PARAM_LISTEN_INTERVAL:
	case WIFI_PS_PARAM_MODE:
		if (net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, iface, &info,
			     sizeof(struct wifi_iface_status))) {
			ps_params->fail_reason =
				WIFI_PS_PARAM_FAIL_UNABLE_TO_GET_IFACE_STATUS;
			return -EIO;
		}

		if (info.state >= WIFI_STATE_ASSOCIATED) {
			ps_params->fail_reason =
				WIFI_PS_PARAM_FAIL_DEVICE_CONNECTED;
			return -ENOTSUP;
		}
		break;
	case WIFI_PS_PARAM_STATE:
	case WIFI_PS_PARAM_WAKEUP_MODE:
	case WIFI_PS_PARAM_TIMEOUT:
		break;
	case WIFI_PS_PARAM_EXIT_STRATEGY:
		if (ps_params->exit_strategy > WIFI_PS_EXIT_MAX) {
			ps_params->fail_reason =
				WIFI_PS_PARAM_FAIL_INVALID_EXIT_STRATEGY;
			return -EINVAL;
		}
	break;
	default:
		ps_params->fail_reason =
			WIFI_PS_PARAM_FAIL_OPERATION_NOT_SUPPORTED;
		return -ENOTSUP;
	}

	return wifi_mgmt_api->set_power_save(dev, ps_params);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_PS, wifi_set_power_save);

static int wifi_get_power_save_config(uint32_t mgmt_request, struct net_if *iface,
			  void *data, size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	const struct wifi_mgmt_ops *const wifi_mgmt_api = get_wifi_api(iface);
	struct wifi_ps_config *ps_config = data;

	if (wifi_mgmt_api == NULL || wifi_mgmt_api->get_power_save_config == NULL) {
		return -ENOTSUP;
	}

	if (!data || len != sizeof(*ps_config)) {
		return -EINVAL;
	}

	return wifi_mgmt_api->get_power_save_config(dev, ps_config);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_PS_CONFIG, wifi_get_power_save_config);

static int wifi_set_twt(uint32_t mgmt_request, struct net_if *iface,
			  void *data, size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	const struct wifi_mgmt_ops *const wifi_mgmt_api = get_wifi_api(iface);
	struct wifi_twt_params *twt_params = data;
	struct wifi_iface_status info = { 0 };

	if (wifi_mgmt_api == NULL || wifi_mgmt_api->set_twt == NULL) {
		twt_params->fail_reason =
			WIFI_TWT_FAIL_OPERATION_NOT_SUPPORTED;
		return -ENOTSUP;
	}

	if (twt_params->operation == WIFI_TWT_TEARDOWN) {
		return wifi_mgmt_api->set_twt(dev, twt_params);
	}

	if (net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, iface, &info,
			sizeof(struct wifi_iface_status))) {
		twt_params->fail_reason =
			WIFI_TWT_FAIL_UNABLE_TO_GET_IFACE_STATUS;
		goto fail;
	}

	if (info.state != WIFI_STATE_COMPLETED) {
		twt_params->fail_reason =
			WIFI_TWT_FAIL_DEVICE_NOT_CONNECTED;
		goto fail;
	}

#ifdef CONFIG_WIFI_MGMT_TWT_CHECK_IP
	if ((!net_if_ipv4_get_global_addr(iface, NET_ADDR_PREFERRED)) &&
	    (!net_if_ipv6_get_global_addr(NET_ADDR_PREFERRED, &iface))) {
		twt_params->fail_reason =
			WIFI_TWT_FAIL_IP_NOT_ASSIGNED;
		goto fail;
	}
#else
	NET_WARN("Check for valid IP address been disabled. "
		 "Device might be unreachable or might not receive traffic.\n");
#endif /* CONFIG_WIFI_MGMT_TWT_CHECK_IP */

	if (info.link_mode < WIFI_6) {
		twt_params->fail_reason =
			WIFI_TWT_FAIL_PEER_NOT_HE_CAPAB;
		goto fail;
	}

	if (!info.twt_capable) {
		twt_params->fail_reason =
			WIFI_TWT_FAIL_PEER_NOT_TWT_CAPAB;
		goto fail;
	}

	return wifi_mgmt_api->set_twt(dev, twt_params);
fail:
	return -ENOEXEC;

}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_TWT, wifi_set_twt);

void wifi_mgmt_raise_twt_event(struct net_if *iface, struct wifi_twt_params *twt_params)
{
	net_mgmt_event_notify_with_info(NET_EVENT_WIFI_TWT,
					iface, twt_params,
					sizeof(struct wifi_twt_params));
}

static int wifi_reg_domain(uint32_t mgmt_request, struct net_if *iface,
			   void *data, size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	const struct wifi_mgmt_ops *const wifi_mgmt_api = get_wifi_api(iface);
	struct wifi_reg_domain *reg_domain = data;

	if (wifi_mgmt_api == NULL || wifi_mgmt_api->reg_domain == NULL) {
		return -ENOTSUP;
	}

	if (!data || len != sizeof(*reg_domain)) {
		return -EINVAL;
	}

	return wifi_mgmt_api->reg_domain(dev, reg_domain);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_REG_DOMAIN, wifi_reg_domain);

void wifi_mgmt_raise_twt_sleep_state(struct net_if *iface,
				     int twt_sleep_state)
{
	net_mgmt_event_notify_with_info(NET_EVENT_WIFI_TWT_SLEEP_STATE,
					iface, &twt_sleep_state,
					sizeof(twt_sleep_state));
}

static int wifi_mode(uint32_t mgmt_request, struct net_if *iface,
				void *data, size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	const struct wifi_mgmt_ops *const wifi_mgmt_api = get_wifi_api(iface);
	struct wifi_mode_info *mode_info = data;

	if (dev == NULL) {
		return -ENODEV;
	}

	if (wifi_mgmt_api == NULL || wifi_mgmt_api->mode == NULL) {
		return -ENOTSUP;
	}

	return wifi_mgmt_api->mode(dev, mode_info);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_MODE, wifi_mode);

static int wifi_packet_filter(uint32_t mgmt_request, struct net_if *iface,
				void *data, size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	const struct wifi_mgmt_ops *const wifi_mgmt_api = get_wifi_api(iface);
	struct wifi_filter_info *filter_info = data;

	if (dev == NULL) {
		return -ENODEV;
	}

	if (wifi_mgmt_api == NULL || wifi_mgmt_api->filter == NULL) {
		return -ENOTSUP;
	}

	return wifi_mgmt_api->filter(dev, filter_info);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_PACKET_FILTER, wifi_packet_filter);

static int wifi_channel(uint32_t mgmt_request, struct net_if *iface,
				void *data, size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	const struct wifi_mgmt_ops *const wifi_mgmt_api = get_wifi_api(iface);
	struct wifi_channel_info *channel_info = data;

	if (dev == NULL) {
		return -ENODEV;
	}

	if (wifi_mgmt_api == NULL || wifi_mgmt_api->channel == NULL) {
		return -ENOTSUP;
	}

	return wifi_mgmt_api->channel(dev, channel_info);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_CHANNEL, wifi_channel);

static int wifi_get_version(uint32_t mgmt_request, struct net_if *iface,
			   void *data, size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	const struct wifi_mgmt_ops *const wifi_mgmt_api = get_wifi_api(iface);
	struct wifi_version *ver_params = data;

	if (wifi_mgmt_api == NULL || wifi_mgmt_api->get_version == NULL) {
		return -ENOTSUP;
	}

	return wifi_mgmt_api->get_version(dev, ver_params);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_VERSION, wifi_get_version);

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_WNM
static int wifi_btm_query(uint32_t mgmt_request, struct net_if *iface, void *data, size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	const struct wifi_mgmt_ops *const wifi_mgmt_api = get_wifi_api(iface);
	uint8_t query_reason = *((uint8_t *)data);

	if (wifi_mgmt_api == NULL || wifi_mgmt_api->btm_query == NULL) {
		return -ENOTSUP;
	}

	if (query_reason >= WIFI_BTM_QUERY_REASON_UNSPECIFIED &&
	    query_reason <= WIFI_BTM_QUERY_REASON_LEAVING_ESS) {
		return wifi_mgmt_api->btm_query(dev, query_reason);
	}

	return -EINVAL;
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_BTM_QUERY, wifi_btm_query);
#endif

static int wifi_get_connection_params(uint32_t mgmt_request, struct net_if *iface,
			void *data, size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	const struct wifi_mgmt_ops *const wifi_mgmt_api = get_wifi_api(iface);
	struct wifi_connect_req_params *conn_params = data;

	if (wifi_mgmt_api == NULL || wifi_mgmt_api->get_conn_params == NULL) {
		return -ENOTSUP;
	}

	return wifi_mgmt_api->get_conn_params(dev, conn_params);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_CONN_PARAMS, wifi_get_connection_params);

static int wifi_wps_config(uint32_t mgmt_request, struct net_if *iface, void *data, size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	const struct wifi_mgmt_ops *const wifi_mgmt_api = get_wifi_api(iface);
	struct wifi_wps_config_params *params = data;

	if (wifi_mgmt_api == NULL || wifi_mgmt_api->wps_config == NULL) {
		return -ENOTSUP;
	}

	return wifi_mgmt_api->wps_config(dev, params);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_WPS_CONFIG, wifi_wps_config);

static int wifi_set_rts_threshold(uint32_t mgmt_request, struct net_if *iface,
				  void *data, size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	const struct wifi_mgmt_ops *const wifi_mgmt_api = get_wifi_api(iface);
	unsigned int *rts_threshold = data;

	if (wifi_mgmt_api == NULL || wifi_mgmt_api->set_rts_threshold == NULL) {
		return -ENOTSUP;
	}

	if (!data || len != sizeof(*rts_threshold)) {
		return -EINVAL;
	}

	return wifi_mgmt_api->set_rts_threshold(dev, *rts_threshold);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_RTS_THRESHOLD, wifi_set_rts_threshold);

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_DPP
static int wifi_dpp(uint32_t mgmt_request, struct net_if *iface,
		    void *data, size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	const struct wifi_mgmt_ops *const wifi_mgmt_api = get_wifi_api(iface);
	struct wifi_dpp_params *params = data;

	if (wifi_mgmt_api == NULL || wifi_mgmt_api->dpp_dispatch == NULL) {
		return -ENOTSUP;
	}

	return wifi_mgmt_api->dpp_dispatch(dev, params);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_DPP, wifi_dpp);

#endif /* CONFIG_WIFI_NM_WPA_SUPPLICANT_DPP */

static int wifi_pmksa_flush(uint32_t mgmt_request, struct net_if *iface,
					   void *data, size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	const struct wifi_mgmt_ops *const wifi_mgmt_api = get_wifi_api(iface);

	if (wifi_mgmt_api == NULL || wifi_mgmt_api->pmksa_flush == NULL) {
		return -ENOTSUP;
	}

	return wifi_mgmt_api->pmksa_flush(dev);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_PMKSA_FLUSH, wifi_pmksa_flush);

static int wifi_get_rts_threshold(uint32_t mgmt_request, struct net_if *iface,
				  void *data, size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	const struct wifi_mgmt_ops *const wifi_mgmt_api = get_wifi_api(iface);
	unsigned int *rts_threshold = data;

	if (wifi_mgmt_api == NULL || wifi_mgmt_api->get_rts_threshold == NULL) {
		return -ENOTSUP;
	}

	if (!data || len != sizeof(*rts_threshold)) {
		return -EINVAL;
	}

	return wifi_mgmt_api->get_rts_threshold(dev, rts_threshold);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_RTS_THRESHOLD_CONFIG, wifi_get_rts_threshold);

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE
static int wifi_set_enterprise_creds(uint32_t mgmt_request, struct net_if *iface,
					   void *data, size_t len)
{
	const struct device *dev = net_if_get_device(iface);
	const struct wifi_mgmt_ops *const wifi_mgmt_api = get_wifi_api(iface);
	struct wifi_enterprise_creds_params *params = data;

	if (wifi_mgmt_api == NULL || wifi_mgmt_api->enterprise_creds == NULL) {
		return -ENOTSUP;
	}

	return wifi_mgmt_api->enterprise_creds(dev, params);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_ENTERPRISE_CREDS, wifi_set_enterprise_creds);
#endif

#ifdef CONFIG_WIFI_MGMT_RAW_SCAN_RESULTS
void wifi_mgmt_raise_raw_scan_result_event(struct net_if *iface,
					   struct wifi_raw_scan_result *raw_scan_result)
{
	if (raw_scan_result->frame_length > CONFIG_WIFI_MGMT_RAW_SCAN_RESULT_LENGTH) {
		LOG_INF("raw scan result frame length = %d too big,"
			 "saving upto max raw scan length = %d",
			 raw_scan_result->frame_length,
			 CONFIG_WIFI_MGMT_RAW_SCAN_RESULT_LENGTH);
	}

	net_mgmt_event_notify_with_info(NET_EVENT_WIFI_RAW_SCAN_RESULT,
					iface, raw_scan_result,
					sizeof(*raw_scan_result));
}
#endif /* CONFIG_WIFI_MGMT_RAW_SCAN_RESULTS */

void wifi_mgmt_raise_disconnect_complete_event(struct net_if *iface,
					       int status)
{
	struct wifi_status cnx_status = {
		.status = status,
	};

	net_mgmt_event_notify_with_info(NET_EVENT_WIFI_DISCONNECT_COMPLETE,
					iface, &cnx_status,
					sizeof(struct wifi_status));
}

void wifi_mgmt_raise_ap_enable_result_event(struct net_if *iface,
					    enum wifi_ap_status status)
{
	struct wifi_status cnx_status = {
		.status = status,
	};

	net_mgmt_event_notify_with_info(NET_EVENT_WIFI_AP_ENABLE_RESULT,
					iface, &cnx_status,
					sizeof(enum wifi_ap_status));
}

void wifi_mgmt_raise_ap_disable_result_event(struct net_if *iface,
					     enum wifi_ap_status status)
{
	struct wifi_status cnx_status = {
		.status = status,
	};

	net_mgmt_event_notify_with_info(NET_EVENT_WIFI_AP_DISABLE_RESULT,
					iface, &cnx_status,
					sizeof(enum wifi_ap_status));
}

void wifi_mgmt_raise_ap_sta_connected_event(struct net_if *iface,
					    struct wifi_ap_sta_info *sta_info)
{
	net_mgmt_event_notify_with_info(NET_EVENT_WIFI_AP_STA_CONNECTED,
					iface, sta_info,
					sizeof(struct wifi_ap_sta_info));
}

void wifi_mgmt_raise_ap_sta_disconnected_event(struct net_if *iface,
					       struct wifi_ap_sta_info *sta_info)
{
	net_mgmt_event_notify_with_info(NET_EVENT_WIFI_AP_STA_DISCONNECTED,
					iface, sta_info,
					sizeof(struct wifi_ap_sta_info));
}

#ifdef CONFIG_WIFI_CREDENTIALS_CONNECT_STORED

#include <zephyr/net/wifi_credentials.h>

#if defined(CONFIG_WIFI_CREDENTIALS_STATIC)
BUILD_ASSERT(sizeof(CONFIG_WIFI_CREDENTIALS_STATIC_SSID) != 1,
	     "CONFIG_WIFI_CREDENTIALS_STATIC_SSID required");
#endif /* defined(CONFIG_WIFI_CREDENTIALS_STATIC) */

static int __stored_creds_to_params(struct wifi_credentials_personal *creds,
				    struct wifi_connect_req_params *params)
{
	char *ssid = NULL;
	char *psk = NULL;
	int ret;

	/* SSID */
	ssid = (char *)k_malloc(creds->header.ssid_len + 1);
	if (!ssid) {
		LOG_ERR("Failed to allocate memory for SSID\n");
		ret = -ENOMEM;
		goto err_out;
	}

	memset(ssid, 0, creds->header.ssid_len + 1);
	ret = snprintf(ssid, creds->header.ssid_len + 1, "%s", creds->header.ssid);
	if (ret > creds->header.ssid_len) {
		LOG_ERR("SSID string truncated\n");
		ret = -EINVAL;
		goto err_out;
	}

	params->ssid = ssid;
	params->ssid_length = creds->header.ssid_len;

	/* PSK (optional) */
	if (creds->password_len > 0) {
		psk = (char *)k_malloc(creds->password_len + 1);
		if (!psk) {
			LOG_ERR("Failed to allocate memory for PSK\n");
			ret = -ENOMEM;
			goto err_out;
		}

		memset(psk, 0, creds->password_len + 1);
		ret = snprintf(psk, creds->password_len + 1, "%s", creds->password);
		if (ret > creds->password_len) {
			LOG_ERR("PSK string truncated\n");
			ret = -EINVAL;
			goto err_out;
		}

		params->psk = psk;
		params->psk_length = creds->password_len;
	}

	/* Defaults */
	params->security = creds->header.type;

	/* If channel is set to 0 we default to ANY. 0 is not a valid Wi-Fi channel. */
	params->channel = (creds->header.channel != 0) ? creds->header.channel : WIFI_CHANNEL_ANY;
	params->timeout = (creds->header.timeout != 0)
				  ? creds->header.timeout
				  : CONFIG_WIFI_CREDENTIALS_CONNECT_STORED_CONNECTION_TIMEOUT;

	/* Security type (optional) */
	if (creds->header.type > WIFI_SECURITY_TYPE_MAX) {
		params->security = WIFI_SECURITY_TYPE_NONE;
	}

	if (creds->header.flags & WIFI_CREDENTIALS_FLAG_2_4GHz) {
		params->band = WIFI_FREQ_BAND_2_4_GHZ;
	} else if (creds->header.flags & WIFI_CREDENTIALS_FLAG_5GHz) {
		params->band = WIFI_FREQ_BAND_5_GHZ;
	} else {
		params->band = WIFI_FREQ_BAND_UNKNOWN;
	}

	/* MFP setting (default: optional) */
	if (creds->header.flags & WIFI_CREDENTIALS_FLAG_MFP_DISABLED) {
		params->mfp = WIFI_MFP_DISABLE;
	} else if (creds->header.flags & WIFI_CREDENTIALS_FLAG_MFP_REQUIRED) {
		params->mfp = WIFI_MFP_REQUIRED;
	} else {
		params->mfp = WIFI_MFP_OPTIONAL;
	}

	return 0;
err_out:
	if (ssid) {
		k_free(ssid);
		ssid = NULL;
	}

	if (psk) {
		k_free(psk);
		psk = NULL;
	}

	return ret;
}

static inline const char *wpa_supp_security_txt(enum wifi_security_type security)
{
	switch (security) {
	case WIFI_SECURITY_TYPE_NONE:
		return "NONE";
	case WIFI_SECURITY_TYPE_PSK:
		return "WPA-PSK";
	case WIFI_SECURITY_TYPE_PSK_SHA256:
		return "WPA-PSK-SHA256";
	case WIFI_SECURITY_TYPE_SAE:
		return "SAE";
	case WIFI_SECURITY_TYPE_UNKNOWN:
	default:
		return "UNKNOWN";
	}
}

static int add_network_from_credentials_struct_personal(struct wifi_credentials_personal *creds,
							struct net_if *iface)
{
	int ret = 0;
	struct wifi_connect_req_params cnx_params = {0};

	if (__stored_creds_to_params(creds, &cnx_params)) {
		ret = -ENOEXEC;
		goto out;
	}

	if (net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, &cnx_params,
		     sizeof(struct wifi_connect_req_params))) {
		LOG_ERR("Connection request failed\n");

		return -ENOEXEC;
	}

	LOG_INF("Connection requested");

out:
	if (cnx_params.psk) {
		k_free((void *)cnx_params.psk);
	}

	if (cnx_params.ssid) {
		k_free((void *)cnx_params.ssid);
	}

	return ret;
}

static void add_stored_network(void *cb_arg, const char *ssid, size_t ssid_len)
{
	int ret = 0;
	struct wifi_credentials_personal creds;

	/* load stored data */
	ret = wifi_credentials_get_by_ssid_personal_struct(ssid, ssid_len, &creds);

	if (ret) {
		LOG_ERR("Loading WiFi credentials failed for SSID [%.*s], len: %d, err: %d",
			ssid_len, ssid, ssid_len, ret);
		return;
	}

	add_network_from_credentials_struct_personal(&creds, (struct net_if *)cb_arg);
}

static int add_static_network_config(struct net_if *iface)
{
#if defined(CONFIG_WIFI_CREDENTIALS_STATIC)

	struct wifi_credentials_personal creds = {
		.header = {
			.ssid_len = strlen(CONFIG_WIFI_CREDENTIALS_STATIC_SSID),
		},
		.password_len = strlen(CONFIG_WIFI_CREDENTIALS_STATIC_PASSWORD),
	};

	int ret = wifi_credentials_get_by_ssid_personal_struct(
		CONFIG_WIFI_CREDENTIALS_STATIC_SSID, strlen(CONFIG_WIFI_CREDENTIALS_STATIC_SSID),
		&creds);

	if (!ret) {
		LOG_WRN("Statically configured WiFi network was overridden by storage.");
		return 0;
	}

#if defined(CONFIG_WIFI_CREDENTIALS_STATIC_TYPE_OPEN)
	creds.header.type = WIFI_SECURITY_TYPE_NONE;
#elif defined(CONFIG_WIFI_CREDENTIALS_STATIC_TYPE_PSK)
	creds.header.type = WIFI_SECURITY_TYPE_PSK;
#elif defined(CONFIG_WIFI_CREDENTIALS_STATIC_TYPE_PSK_SHA256)
	creds.header.type = WIFI_SECURITY_TYPE_PSK_SHA256;
#elif defined(CONFIG_WIFI_CREDENTIALS_STATIC_TYPE_SAE)
	creds.header.type = WIFI_SECURITY_TYPE_SAE;
#elif defined(CONFIG_WIFI_CREDENTIALS_STATIC_TYPE_WPA_PSK)
	creds.header.type = WIFI_SECURITY_TYPE_WPA_PSK;
#else
#error "invalid CONFIG_WIFI_CREDENTIALS_STATIC_TYPE"
#endif

	memcpy(creds.header.ssid, CONFIG_WIFI_CREDENTIALS_STATIC_SSID,
	       strlen(CONFIG_WIFI_CREDENTIALS_STATIC_SSID));
	memcpy(creds.password, CONFIG_WIFI_CREDENTIALS_STATIC_PASSWORD,
	       strlen(CONFIG_WIFI_CREDENTIALS_STATIC_PASSWORD));

	LOG_DBG("Adding statically configured WiFi network [%s] to internal list.",
		creds.header.ssid);

	return add_network_from_credentials_struct_personal(&creds, iface);
#else
	return 0;
#endif /* defined(CONFIG_WIFI_CREDENTIALS_STATIC) */
}

static int connect_stored_command(uint32_t mgmt_request, struct net_if *iface, void *data,
				  size_t len)
{
	int ret = 0;

	ret = add_static_network_config(iface);
	if (ret) {
		return ret;
	}

	wifi_credentials_for_each_ssid(add_stored_network, iface);

	return ret;
};

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_CONNECT_STORED, connect_stored_command);

#endif /* CONFIG_WIFI_CREDENTIALS_CONNECT_STORED */
