/*
 * Copyright (c) 2024 Telink Semiconductor (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT         telink_w91_wifi

#include "wifi_w91.h"

#include <stdlib.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(w91_wifi, CONFIG_WIFI_LOG_LEVEL);

enum wifi_w91_state_flag {
	WIFI_W91_STA_STOPPED,
	WIFI_W91_STA_STARTED,
	WIFI_W91_STA_CONNECTING,
	WIFI_W91_STA_CONNECTED,
	WIFI_W91_AP_STARTED,
	WIFI_W91_AP_STOPPED,

};

enum wifi_w91_auth_mode {
	WIFI_W91_SECURITY_OPEN,
	WIFI_W91_SECURITY_WPAPSK,
	WIFI_W91_SECURITY_WPA2PSK,
	WIFI_W91_SECURITY_SAE,
	WIFI_W91_SECURITY_UNKNOWN,
};

enum wifi_w91_event_id {
	WIFI_W91_EVENT_WIFI_READY = 0,
	WIFI_W91_EVENT_SCAN_DONE,
	WIFI_W91_EVENT_STA_START,
	WIFI_W91_EVENT_STA_STOP,
	WIFI_W91_EVENT_STA_CONNECTED,
	WIFI_W91_EVENT_STA_DISCONNECTED,
	WIFI_W91_EVENT_STA_AUTHMODE_CHANGE,
	WIFI_W91_EVENT_STA_GOT_IP,
	WIFI_W91_EVENT_STA_LOST_IP,
	WIFI_W91_EVENT_STA_WPS_ER_SUCCESS,
	WIFI_W91_EVENT_STA_WPS_ER_FAILED,
	WIFI_W91_EVENT_STA_WPS_ER_TIMEOUT,
	WIFI_W91_EVENT_STA_WPS_ER_PIN,
	WIFI_W91_EVENT_STA_STATE_CHANGE,
	WIFI_W91_EVENT_STA_NO_NETWORK,
	WIFI_W91_EVENT_AP_START,
	WIFI_W91_EVENT_AP_STOP,
	WIFI_W91_EVENT_AP_STACONNECTED,
	WIFI_W91_EVENT_AP_STADISCONNECTED,
	WIFI_W91_EVENT_AP_STAIPASSIGNED,
	WIFI_W91_EVENT_AP_PROBEREQRECVED,
	WIFI_W91_EVENT_GOT_IP6,
	WIFI_W91_EVENT_ETH_START,
	WIFI_W91_EVENT_ETH_STOP,
	WIFI_W91_EVENT_ETH_CONNECTED,
	WIFI_W91_EVENT_ETH_DISCONNECTED,
	WIFI_W91_EVENT_ETH_GOT_IP,
	WIFI_W91_EVENT_TLSR_CHANNEL,
	WIFI_W91_EVENT_TLSR_LINK_UP,
	WIFI_W91_EVENT_REKEY,
	WIFI_W91_EVENT_MAX,
};

struct wifi_w91_init_resp {
	int err;
	uint8_t mac[WIFI_MAC_ADDR_LEN];
};

struct wifi_w91_connect_req {
	uint8_t ssid_len;
	const uint8_t *ssid;
	uint8_t channel;
	uint8_t authmode;
	uint8_t password_len;
	const uint8_t *password;
};

struct wifi_w91_event_scan_done_ap_info {
	uint8_t ssid_len;
	uint8_t ssid[WIFI_SSID_MAX_LEN];
	uint8_t channel;
	uint8_t authmode;
	int8_t rssi;
	uint8_t bssid[WIFI_MAC_ADDR_LEN];
};

struct wifi_w91_event_scan_done {
	uint8_t number;
	struct wifi_w91_event_scan_done_ap_info *ap_info;
};

struct wifi_w91_event_ap_sta_status {
	uint8_t mac[WIFI_MAC_ADDR_LEN];
};

struct wifi_w91_event_sta_connect {
	uint8_t id;
	uint8_t ssid_len;
	uint8_t ssid[WIFI_SSID_MAX_LEN];
	uint8_t bssid[WIFI_MAC_ADDR_LEN];
	uint8_t channel;
	uint8_t authmode;
	int8_t rssi;
};

struct wifi_w91_event_ap_start {
	uint8_t id;
	uint8_t ssid_len;
	uint8_t ssid[WIFI_SSID_MAX_LEN];
	uint8_t bssid[WIFI_MAC_ADDR_LEN];
	uint8_t channel;
	uint8_t authmode;
	uint16_t beacon_interval;
	uint8_t dtim_period;
};

union wifi_w91_event_param {
	struct wifi_w91_event_scan_done scan_done;
	struct wifi_w91_event_ap_sta_status ap_sta_info;
	struct wifi_w91_event_sta_connect sta_connect_info;
	struct wifi_w91_event_ap_start ap_start_info;
};

struct wifi_w91_event {
	uint8_t id;
	union wifi_w91_event_param param;
};

struct ip_v4_data {
	uint32_t ip;
	uint32_t mask;
	uint32_t gw;
};

struct ip_v6_data {
	struct {
		uint32_t ip[4];

		__packed enum {
			WIFI_W91_ADDR_TENTATIVE = 0,
			WIFI_W91_ADDR_PREFERRED,
			WIFI_W91_ADDR_DEPRECATED
		} state;
	} address[CONFIG_WIFI_TELINK_W91_IPV6_ADDR_CNT];
};

/* WiFi state utilities */
static void wifi_w91_show_state(const struct wifi_iface_status *if_state)
{
	char ssid_str[sizeof(if_state->ssid) + 1];
	char bssid_str[sizeof(if_state->bssid) * 2 + 1];

	memcpy(ssid_str, if_state->ssid, sizeof(if_state->ssid));
	ssid_str[if_state->ssid_len] = '\0';
	(void) bin2hex(if_state->bssid, sizeof(if_state->bssid),
		bssid_str, sizeof(bssid_str));

	LOG_INF("State    : %s", wifi_state_txt(if_state->state));
	LOG_INF("SSID     : %s", ssid_str);
	LOG_INF("BSSID    : %s", bssid_str);
	LOG_INF("Bands    : %s", wifi_band_txt(if_state->band));
	LOG_INF("Channel  : %u", if_state->channel);
	LOG_INF("WiFi Mode: %s", wifi_mode_txt(if_state->iface_mode));
	LOG_INF("Link Mode: %s", wifi_link_mode_txt(if_state->link_mode));
	LOG_INF("Security : %s", wifi_security_txt(if_state->security));
	LOG_INF("MFP      : %s", wifi_mfp_txt(if_state->mfp));
	LOG_INF("RSSI     : %d", if_state->rssi);
	LOG_INF("DTim     : %u", if_state->dtim_period);
	LOG_INF("BInt     : %u", if_state->beacon_interval);
	LOG_INF("TWT      : %s", if_state->twt_capable ? "True" : "False");
}

static void wifi_w91_reset_state(struct wifi_iface_status *if_state)
{
	*if_state = (struct wifi_iface_status) {
		.state = WIFI_STATE_INACTIVE,
		.band = WIFI_FREQ_BAND_2_4_GHZ,
		.iface_mode = WIFI_MODE_UNKNOWN,
		.link_mode = WIFI_LINK_MODE_UNKNOWN,
		.security = WIFI_SECURITY_TYPE_UNKNOWN,
	};
}

#if CONFIG_NET_IPV4
/* APIs implementation: set ipv4 */
static size_t pack_wifi_w91_set_ipv4(uint8_t inst, void *unpack_data, uint8_t *pack_data)
{
	struct ip_v4_data *p_ip_v4 = unpack_data;
	size_t pack_data_len = sizeof(uint32_t) +
		sizeof(p_ip_v4->ip) + sizeof(p_ip_v4->mask) + sizeof(p_ip_v4->gw);

	if (pack_data != NULL) {
		uint32_t id = IPC_DISPATCHER_MK_ID(IPC_DISPATCHER_WIFI_IPV4_ADDR, inst);

		IPC_DISPATCHER_PACK_FIELD(pack_data, id);
		IPC_DISPATCHER_PACK_FIELD(pack_data, p_ip_v4->ip);
		IPC_DISPATCHER_PACK_FIELD(pack_data, p_ip_v4->mask);
		IPC_DISPATCHER_PACK_FIELD(pack_data, p_ip_v4->gw);
	}

	return pack_data_len;
}

IPC_DISPATCHER_UNPACK_FUNC_ONLY_WITH_ERROR_PARAM(wifi_w91_set_ipv4);

static int wifi_w91_set_ipv4(struct net_if *iface)
{
	struct wifi_w91_data *data = iface->if_dev->dev->data;
	const struct wifi_w91_config *cfg = iface->if_dev->dev->config;
	int err = 0;

	if (data->base.state == WIFI_W91_STA_CONNECTED ||
		data->base.state == WIFI_W91_AP_STARTED) {

		struct ip_v4_data ipv4 = {
			.ip = iface->config.ip.ipv4->unicast[0].ipv4.address.in_addr.s_addr,
			.mask = iface->config.ip.ipv4->unicast[0].netmask.s_addr,
			.gw = iface->config.ip.ipv4->gw.s_addr
		};

		err = -ETIMEDOUT;
		IPC_DISPATCHER_HOST_SEND_DATA(&data->ipc, cfg->instance_id,
			wifi_w91_set_ipv4, &ipv4, &err,
			CONFIG_TELINK_W91_IPC_DISPATCHER_TIMEOUT_MS);
	}

	if (data->base.state == WIFI_W91_STA_CONNECTED) {
		if (err) {
			LOG_INF("set ip v4 error %d", err);
		}
		wifi_mgmt_raise_connect_result_event(data->base.iface, err);
	}

	return 0;
}
#endif /* CONFIG_NET_IPV4 */

#if CONFIG_NET_IPV6
/* APIs implementation: set ipv6 */
static size_t pack_wifi_w91_set_ipv6(uint8_t inst, void *unpack_data, uint8_t *pack_data)
{
	struct ip_v6_data *p_ip_v6 = unpack_data;
	size_t pack_data_len = sizeof(uint32_t);

	for (size_t i = 0; i < CONFIG_WIFI_TELINK_W91_IPV6_ADDR_CNT; i++) {
		pack_data_len += sizeof(p_ip_v6->address[i].ip);
		pack_data_len += sizeof(p_ip_v6->address[i].state);
	}

	if (pack_data != NULL) {
		uint32_t id = IPC_DISPATCHER_MK_ID(IPC_DISPATCHER_WIFI_IPV6_ADDR, inst);

		IPC_DISPATCHER_PACK_FIELD(pack_data, id);
		for (size_t i = 0; i < CONFIG_WIFI_TELINK_W91_IPV6_ADDR_CNT; i++) {
			IPC_DISPATCHER_PACK_ARRAY(pack_data, p_ip_v6->address[i].ip,
				sizeof(p_ip_v6->address[i].ip));
			IPC_DISPATCHER_PACK_FIELD(pack_data, p_ip_v6->address[i].state);
		}
	}

	return pack_data_len;
}

IPC_DISPATCHER_UNPACK_FUNC_ONLY_WITH_ERROR_PARAM(wifi_w91_set_ipv6);

static int wifi_w91_set_ipv6(struct net_if *iface)
{
	const struct wifi_w91_config *cfg = iface->if_dev->dev->config;
	struct wifi_w91_data *data = iface->if_dev->dev->data;
	int err = 0;

	if (data->base.state == WIFI_W91_STA_CONNECTED ||
		data->base.state == WIFI_W91_AP_STARTED) {

		struct ip_v6_data ipv6 = {0};
		struct net_if_addr *if_addr = iface->config.ip.ipv6->unicast;
		size_t idx = 1;

		for (size_t i = 0; i < CONFIG_NET_IF_UNICAST_IPV6_ADDR_COUNT; i++) {
			if (if_addr[i].is_used && if_addr[i].addr_state != NET_ADDR_ANY_STATE) {
				if (if_addr[i].addr_type == NET_ADDR_AUTOCONF &&
					!memcmp(ipv6.address[0].ip, &(uint32_t [4]) {0},
						sizeof(ipv6.address[0].ip))) {
					for (size_t j = 0; j < 4; j++) {
						ipv6.address[0].ip[j] =
							if_addr[i].address.in6_addr.s6_addr32[j];
					}
					switch (if_addr[i].addr_state) {
					case NET_ADDR_TENTATIVE:
						ipv6.address[0].state = WIFI_W91_ADDR_TENTATIVE;
						break;
					case NET_ADDR_PREFERRED:
						ipv6.address[0].state = WIFI_W91_ADDR_PREFERRED;
						break;
					case NET_ADDR_DEPRECATED:
						ipv6.address[0].state = WIFI_W91_ADDR_DEPRECATED;
						break;
					default:
						break;
					}
				} else if (idx < CONFIG_WIFI_TELINK_W91_IPV6_ADDR_CNT) {
					for (size_t j = 0; j < 4; j++) {
						ipv6.address[idx].ip[j] =
							if_addr[i].address.in6_addr.s6_addr32[j];
					}
					switch (if_addr[i].addr_state) {
					case NET_ADDR_TENTATIVE:
						ipv6.address[idx].state = WIFI_W91_ADDR_TENTATIVE;
						break;
					case NET_ADDR_PREFERRED:
						ipv6.address[idx].state = WIFI_W91_ADDR_PREFERRED;
						break;
					case NET_ADDR_DEPRECATED:
						ipv6.address[idx].state = WIFI_W91_ADDR_DEPRECATED;
						break;
					default:
						break;
					}
					idx++;
				}
			}
		}

		err = -ETIMEDOUT;
		IPC_DISPATCHER_HOST_SEND_DATA(&data->ipc, cfg->instance_id,
			wifi_w91_set_ipv6, &ipv6, &err,
			CONFIG_TELINK_W91_IPC_DISPATCHER_TIMEOUT_MS);
	}

	if (data->base.state == WIFI_W91_STA_CONNECTED) {
		if (err) {
			LOG_INF("set ip v6 error %d", err);
		}
#if !CONFIG_NET_IPV4
		wifi_mgmt_raise_connect_result_event(data->base.iface, err);
#endif /* !CONFIG_NET_IPV4 */
	}

	return 0;
}
#endif /* CONFIG_NET_IPV6 */

#if CONFIG_NET_DHCPV4
static void wifi_w91_got_dhcp_ip(struct net_mgmt_event_callback *cb,
	uint32_t mgmt_event, struct net_if *iface)
{
	if (mgmt_event != NET_EVENT_IPV4_ADDR_ADD) {
		return;
	}
	(void) wifi_w91_set_ipv4(iface);
}
#endif /* CONFIG_NET_DHCPV4 */

/* APIs implementation: wifi init */
IPC_DISPATCHER_PACK_FUNC_WITHOUT_PARAM(wifi_w91_init_if, IPC_DISPATCHER_WIFI_INIT);

static void unpack_wifi_w91_init_if(void *unpack_data,
	const uint8_t *pack_data, size_t pack_data_len)
{
	struct wifi_w91_init_resp *p_init_resp = unpack_data;
	size_t expect_len = sizeof(uint32_t) +
		sizeof(p_init_resp->err) + sizeof(p_init_resp->mac);

	if (expect_len != pack_data_len) {
		p_init_resp->err = -EINVAL;
		return;
	}

	pack_data += sizeof(uint32_t);
	IPC_DISPATCHER_UNPACK_FIELD(pack_data, p_init_resp->err);
	IPC_DISPATCHER_UNPACK_ARRAY(pack_data, p_init_resp->mac,
		sizeof(p_init_resp->mac));
}

static void wifi_w91_init_if(struct net_if *iface)
{
	LOG_INF("%s", __func__);

	struct wifi_w91_init_resp init_resp = { .err = -ETIMEDOUT };
	const struct wifi_w91_config *cfg = iface->if_dev->dev->config;
	struct wifi_w91_data *data = iface->if_dev->dev->data;

	data->base.iface = iface;

	net_if_carrier_off(data->base.iface);

	IPC_DISPATCHER_HOST_SEND_DATA(&data->ipc, cfg->instance_id,
		wifi_w91_init_if, NULL, &init_resp,
		CONFIG_TELINK_W91_IPC_DISPATCHER_TIMEOUT_MS);

	if (init_resp.err) {
		LOG_ERR("Failed to start Wi-Fi driver (response status is incorrect)");
		return;
	}

	memcpy(data->base.mac, init_resp.mac, sizeof(data->base.mac));

	char mac_str[sizeof(data->base.mac) * 2 + 1];

	(void) bin2hex(data->base.mac, sizeof(data->base.mac),
		mac_str, sizeof(mac_str));
	LOG_INF("MAC %s", mac_str);

	/* Assign link local address. */
	net_if_set_link_addr(data->base.iface, data->base.mac,
		WIFI_MAC_ADDR_LEN, NET_LINK_ETHERNET);
#if CONFIG_NET_DHCPV4
	net_mgmt_init_event_callback(&data->base.ev_dhcp,
		wifi_w91_got_dhcp_ip, NET_EVENT_IPV4_ADDR_ADD);
	net_mgmt_add_event_callback(&data->base.ev_dhcp);
#endif /* CONFIG_NET_DHCPV4 */
}

/* APIs implementation: wifi scan */
IPC_DISPATCHER_PACK_FUNC_WITHOUT_PARAM(wifi_w91_scan, IPC_DISPATCHER_WIFI_SCAN);

IPC_DISPATCHER_UNPACK_FUNC_ONLY_WITH_ERROR_PARAM(wifi_w91_scan);

static int wifi_w91_scan(const struct device *dev, struct wifi_scan_params *params, scan_result_cb_t cb)
{
	LOG_INF("%s", __func__);
	ARG_UNUSED(params);

	int err = -ETIMEDOUT;
	const struct wifi_w91_config *cfg = dev->config;
	struct wifi_w91_data *data = dev->data;

	if (data->base.if_state.state == WIFI_STATE_SCANNING) {
		LOG_INF("Scan callback in progress");
		return -EINPROGRESS;
	}

	data->base.scan_cb = cb;

	IPC_DISPATCHER_HOST_SEND_DATA(&data->ipc, cfg->instance_id,
		wifi_w91_scan, NULL, &err,
		CONFIG_TELINK_W91_IPC_DISPATCHER_TIMEOUT_MS);
	if (!err) {
		wifi_w91_reset_state(&data->base.if_state);
		data->base.if_state.state = WIFI_STATE_SCANNING;
		data->base.if_state.iface_mode = WIFI_MODE_INFRA;
	}

	return err;
}

/* APIs implementation: wifi connect */
static size_t pack_wifi_w91_connect(uint8_t inst, void *unpack_data, uint8_t *pack_data)
{
	struct wifi_w91_connect_req *p_connect_req = unpack_data;
	size_t pack_data_len = sizeof(uint32_t) +
		sizeof(p_connect_req->ssid_len) + p_connect_req->ssid_len +
		sizeof(p_connect_req->channel) + sizeof(p_connect_req->authmode) +
		sizeof(p_connect_req->password_len) + p_connect_req->password_len;

	if (pack_data != NULL) {
		uint32_t id = IPC_DISPATCHER_MK_ID(IPC_DISPATCHER_WIFI_CONNECT, inst);

		IPC_DISPATCHER_PACK_FIELD(pack_data, id);
		IPC_DISPATCHER_PACK_FIELD(pack_data, p_connect_req->ssid_len);
		IPC_DISPATCHER_PACK_ARRAY(pack_data, p_connect_req->ssid,
			p_connect_req->ssid_len);
		IPC_DISPATCHER_PACK_FIELD(pack_data, p_connect_req->channel);
		IPC_DISPATCHER_PACK_FIELD(pack_data, p_connect_req->authmode);
		IPC_DISPATCHER_PACK_FIELD(pack_data, p_connect_req->password_len);
		IPC_DISPATCHER_PACK_ARRAY(pack_data, p_connect_req->password,
			p_connect_req->password_len);
	}

	return pack_data_len;
}

IPC_DISPATCHER_UNPACK_FUNC_ONLY_WITH_ERROR_PARAM(wifi_w91_connect);

static int wifi_w91_connect(const struct device *dev, struct wifi_connect_req_params *params)
{
	LOG_INF("%s", __func__);

	int err = -ETIMEDOUT;
	const struct wifi_w91_config *cfg = dev->config;
	struct wifi_w91_data *data = dev->data;
	struct wifi_w91_connect_req connect_req = {
		.ssid_len = params->ssid_length,
		.ssid = (uint8_t *)params->ssid,
		.channel = params->channel,
	};

	if (data->base.state == WIFI_W91_STA_CONNECTING ||
			data->base.state == WIFI_W91_STA_CONNECTED) {
		wifi_mgmt_raise_connect_result_event(data->base.iface, -1);
		return -EALREADY;
	}

	if (data->base.state != WIFI_W91_STA_STARTED) {
		LOG_ERR("Wi-Fi is not in station mode");
		wifi_mgmt_raise_connect_result_event(data->base.iface, -1);
		return -EIO;
	}

	data->base.state = WIFI_W91_STA_CONNECTING;

	if (params->security == WIFI_SECURITY_TYPE_PSK) {
		connect_req.authmode = WIFI_W91_SECURITY_WPA2PSK;
		connect_req.password_len = params->psk_length;
		connect_req.password = params->psk;
	} else if (params->security == WIFI_SECURITY_TYPE_NONE) {
		connect_req.authmode = WIFI_W91_SECURITY_OPEN;
		connect_req.password = 0;
	} else {
		LOG_ERR("Authentication method not supported");
		return -EIO;
	}

	IPC_DISPATCHER_HOST_SEND_DATA(&data->ipc, cfg->instance_id,
		wifi_w91_connect, &connect_req, &err,
		CONFIG_TELINK_W91_IPC_DISPATCHER_TIMEOUT_MS);
	if (!err) {
		wifi_w91_reset_state(&data->base.if_state);
		data->base.if_state.state = WIFI_STATE_AUTHENTICATING;
		data->base.if_state.iface_mode = WIFI_MODE_INFRA;
	}

	return err;
}

/* APIs implementation: wifi disconnect */
IPC_DISPATCHER_PACK_FUNC_WITHOUT_PARAM(wifi_w91_disconnect, IPC_DISPATCHER_WIFI_DISCONNECT);

IPC_DISPATCHER_UNPACK_FUNC_ONLY_WITH_ERROR_PARAM(wifi_w91_disconnect);

static int wifi_w91_disconnect(const struct device *dev)
{
	LOG_INF("%s", __func__);

	int err = -ETIMEDOUT;
	const struct wifi_w91_config *cfg = dev->config;
	struct wifi_w91_data *data = dev->data;

	IPC_DISPATCHER_HOST_SEND_DATA(&data->ipc, cfg->instance_id,
		wifi_w91_disconnect, NULL, &err,
		CONFIG_TELINK_W91_IPC_DISPATCHER_TIMEOUT_MS);

	return err;
}

/* APIs implementation: wifi ap enable */
static size_t pack_wifi_w91_ap_enable(uint8_t inst, void *unpack_data, uint8_t *pack_data)
{
	struct wifi_w91_connect_req *p_connect_req = unpack_data;
	size_t pack_data_len = sizeof(uint32_t) +
		sizeof(p_connect_req->ssid_len) + p_connect_req->ssid_len +
		sizeof(p_connect_req->channel) + sizeof(p_connect_req->authmode) +
		sizeof(p_connect_req->password_len) + p_connect_req->password_len;

	if (pack_data != NULL) {
		uint32_t id = IPC_DISPATCHER_MK_ID(IPC_DISPATCHER_WIFI_AP_ENABLE, inst);

		IPC_DISPATCHER_PACK_FIELD(pack_data, id);
		IPC_DISPATCHER_PACK_FIELD(pack_data, p_connect_req->ssid_len);
		IPC_DISPATCHER_PACK_ARRAY(pack_data, p_connect_req->ssid,
			p_connect_req->ssid_len);
		IPC_DISPATCHER_PACK_FIELD(pack_data, p_connect_req->channel);
		IPC_DISPATCHER_PACK_FIELD(pack_data, p_connect_req->authmode);
		IPC_DISPATCHER_PACK_FIELD(pack_data, p_connect_req->password_len);
		IPC_DISPATCHER_PACK_ARRAY(pack_data, p_connect_req->password,
			p_connect_req->password_len);
	}

	return pack_data_len;
}

IPC_DISPATCHER_UNPACK_FUNC_ONLY_WITH_ERROR_PARAM(wifi_w91_ap_enable);

static int wifi_w91_ap_enable(const struct device *dev, struct wifi_connect_req_params *params)
{
	LOG_INF("%s", __func__);

	int err = -ETIMEDOUT;
	const struct wifi_w91_config *cfg = dev->config;
	struct wifi_w91_data *data = dev->data;
	struct wifi_w91_connect_req connect_req = {
		.ssid_len = params->ssid_length,
		.ssid = (uint8_t *)params->ssid,
		.channel = params->channel,
	};

	if (params->psk_length) {
		connect_req.authmode = WIFI_W91_SECURITY_WPA2PSK;
		connect_req.password_len = params->psk_length;
		connect_req.password = params->psk;
	} else if (params->security == WIFI_SECURITY_TYPE_NONE) {
		connect_req.authmode = WIFI_W91_SECURITY_OPEN;
		connect_req.password = 0;
	} else {
		LOG_ERR("Authentication method not supported");
		return -EIO;
	}

	IPC_DISPATCHER_HOST_SEND_DATA(&data->ipc, cfg->instance_id,
		wifi_w91_ap_enable, &connect_req, &err,
		CONFIG_TELINK_W91_IPC_DISPATCHER_TIMEOUT_MS);

	return err;
}

/* APIs implementation: wifi ap disable */
IPC_DISPATCHER_PACK_FUNC_WITHOUT_PARAM(wifi_w91_ap_disable, IPC_DISPATCHER_WIFI_AP_DISABLE);

IPC_DISPATCHER_UNPACK_FUNC_ONLY_WITH_ERROR_PARAM(wifi_w91_ap_disable);

static int wifi_w91_ap_disable(const struct device *dev)
{
	LOG_INF("%s", __func__);

	int err = -ETIMEDOUT;
	const struct wifi_w91_config *cfg = dev->config;
	struct wifi_w91_data *data = dev->data;

	IPC_DISPATCHER_HOST_SEND_DATA(&data->ipc, cfg->instance_id,
		wifi_w91_ap_disable, NULL, &err,
		CONFIG_TELINK_W91_IPC_DISPATCHER_TIMEOUT_MS);

	return err;
}

/* APIs implementation: wifi interface status */
int wifi_w91_iface_status(const struct device *dev, struct wifi_iface_status *status)
{
	struct wifi_w91_data *data = dev->data;

	memcpy(status, &data->base.if_state, sizeof(struct wifi_iface_status));
	return 0;
}

/* APIs implementation: event callback */
static bool unpack_wifi_w91_event_scan_done_cb(void *unpack_data,
	const uint8_t *pack_data, size_t pack_data_len)
{
	struct wifi_w91_event *p_evt = unpack_data;
	struct wifi_w91_event_scan_done_ap_info *ap_info;
	size_t expect_len = sizeof(uint32_t) + sizeof(p_evt->id) +
		sizeof(p_evt->param.scan_done.number);

	pack_data += sizeof(uint32_t);
	IPC_DISPATCHER_UNPACK_FIELD(pack_data, p_evt->id);
	IPC_DISPATCHER_UNPACK_FIELD(pack_data, p_evt->param.scan_done.number);

	if (p_evt->param.scan_done.number) {
		p_evt->param.scan_done.ap_info =
			malloc(sizeof(*ap_info) * p_evt->param.scan_done.number);
		if (p_evt->param.scan_done.ap_info == NULL) {
			LOG_ERR("No memory for wifi event (scan done)");
			return false;
		}

		for (uint8_t i = 0; i < p_evt->param.scan_done.number; i++) {
			ap_info = &p_evt->param.scan_done.ap_info[i];

			IPC_DISPATCHER_UNPACK_FIELD(pack_data, ap_info->ssid_len);
			IPC_DISPATCHER_UNPACK_ARRAY(pack_data, ap_info->ssid, ap_info->ssid_len);
			IPC_DISPATCHER_UNPACK_FIELD(pack_data, ap_info->channel);
			IPC_DISPATCHER_UNPACK_FIELD(pack_data, ap_info->authmode);
			IPC_DISPATCHER_UNPACK_FIELD(pack_data, ap_info->rssi);
			IPC_DISPATCHER_UNPACK_ARRAY(pack_data,
				ap_info->bssid, sizeof(ap_info->bssid));

			expect_len += sizeof(ap_info->ssid_len) + ap_info->ssid_len +
				sizeof(ap_info->channel) + sizeof(ap_info->authmode) +
				sizeof(ap_info->rssi) + sizeof(ap_info->bssid);
		}
	} else {
		p_evt->param.scan_done.ap_info = NULL;
	}

	if (expect_len != pack_data_len) {
		if (p_evt->param.scan_done.ap_info) {
			free(p_evt->param.scan_done.ap_info);
		}
		return false;
	}

	return true;
}

static bool unpack_wifi_w91_event_ap_sta_status_cb(void *unpack_data,
	const uint8_t *pack_data, size_t pack_data_len)
{
	struct wifi_w91_event *p_evt = unpack_data;
	size_t expect_len = sizeof(uint32_t) + sizeof(p_evt->id) +
		sizeof(p_evt->param.ap_sta_info.mac);

	if (expect_len != pack_data_len) {
		return false;
	}

	pack_data += sizeof(uint32_t);
	IPC_DISPATCHER_UNPACK_FIELD(pack_data, p_evt->id);
	IPC_DISPATCHER_UNPACK_ARRAY(pack_data, p_evt->param.ap_sta_info.mac,
		sizeof(p_evt->param.ap_sta_info.mac));

	return true;
}

static bool unpack_wifi_w91_event_sta_connect_cb(void *unpack_data,
	const uint8_t *pack_data, size_t pack_data_len)
{
	struct wifi_w91_event *p_evt = unpack_data;
	size_t expect_len = sizeof(uint32_t) + sizeof(p_evt->id) +
		sizeof(p_evt->param.sta_connect_info.ssid_len) +
		sizeof(p_evt->param.sta_connect_info.ssid) +
		sizeof(p_evt->param.sta_connect_info.bssid) +
		sizeof(p_evt->param.sta_connect_info.channel) +
		sizeof(p_evt->param.sta_connect_info.authmode) +
		sizeof(p_evt->param.sta_connect_info.rssi);

	if (expect_len != pack_data_len) {
		return false;
	}

	pack_data += sizeof(uint32_t);
	IPC_DISPATCHER_UNPACK_FIELD(pack_data, p_evt->id);
	IPC_DISPATCHER_UNPACK_FIELD(pack_data, p_evt->param.sta_connect_info.ssid_len);
	IPC_DISPATCHER_UNPACK_ARRAY(pack_data, p_evt->param.sta_connect_info.ssid,
		sizeof(p_evt->param.sta_connect_info.ssid));
	IPC_DISPATCHER_UNPACK_ARRAY(pack_data, p_evt->param.sta_connect_info.bssid,
		sizeof(p_evt->param.sta_connect_info.bssid));
	IPC_DISPATCHER_UNPACK_FIELD(pack_data, p_evt->param.sta_connect_info.channel);
	IPC_DISPATCHER_UNPACK_FIELD(pack_data, p_evt->param.sta_connect_info.authmode);
	IPC_DISPATCHER_UNPACK_FIELD(pack_data, p_evt->param.sta_connect_info.rssi);

	return true;
}

static bool unpack_wifi_w91_event_ap_start_cb(void *unpack_data,
	const uint8_t *pack_data, size_t pack_data_len)
{
	struct wifi_w91_event *p_evt = unpack_data;
	size_t expect_len = sizeof(uint32_t) + sizeof(p_evt->id) +
		sizeof(p_evt->param.ap_start_info.ssid_len) +
		sizeof(p_evt->param.ap_start_info.ssid) +
		sizeof(p_evt->param.ap_start_info.bssid) +
		sizeof(p_evt->param.ap_start_info.channel) +
		sizeof(p_evt->param.ap_start_info.authmode) +
		sizeof(p_evt->param.ap_start_info.beacon_interval) +
		sizeof(p_evt->param.ap_start_info.dtim_period);

	if (expect_len != pack_data_len) {
		return false;
	}

	pack_data += sizeof(uint32_t);
	IPC_DISPATCHER_UNPACK_FIELD(pack_data, p_evt->id);
	IPC_DISPATCHER_UNPACK_FIELD(pack_data, p_evt->param.ap_start_info.ssid_len);
	IPC_DISPATCHER_UNPACK_ARRAY(pack_data, p_evt->param.ap_start_info.ssid,
		sizeof(p_evt->param.ap_start_info.ssid));
	IPC_DISPATCHER_UNPACK_ARRAY(pack_data, p_evt->param.ap_start_info.bssid,
		sizeof(p_evt->param.ap_start_info.bssid));
	IPC_DISPATCHER_UNPACK_FIELD(pack_data, p_evt->param.ap_start_info.channel);
	IPC_DISPATCHER_UNPACK_FIELD(pack_data, p_evt->param.ap_start_info.authmode);
	IPC_DISPATCHER_UNPACK_FIELD(pack_data, p_evt->param.ap_start_info.beacon_interval);
	IPC_DISPATCHER_UNPACK_FIELD(pack_data, p_evt->param.ap_start_info.dtim_period);

	return true;
}

static bool unpack_wifi_w91_event_cb(void *unpack_data,
	const uint8_t *pack_data, size_t pack_data_len)
{
	struct wifi_w91_event *p_evt = unpack_data;
	size_t min_expect_len = sizeof(uint32_t) + sizeof(p_evt->id);

	if (min_expect_len > pack_data_len) {
		return false;
	}

	pack_data += sizeof(uint32_t);
	IPC_DISPATCHER_UNPACK_FIELD(pack_data, p_evt->id);
	return true;
}

static void wifi_w91_event_cb(const void *data, size_t len, void *param)
{
	struct wifi_w91_event event;
	const struct wifi_w91_config *cfg = ((struct device *)param)->config;

	if (!unpack_wifi_w91_event_cb(&event, data, len)) {
		return;
	}

	switch (event.id) {
	case WIFI_W91_EVENT_SCAN_DONE:
		if (!unpack_wifi_w91_event_scan_done_cb(&event, data, len)) {
			return;
		}
		break;
	case WIFI_W91_EVENT_AP_STACONNECTED:
	case WIFI_W91_EVENT_AP_STADISCONNECTED:
		if (!unpack_wifi_w91_event_ap_sta_status_cb(&event, data, len)) {
			return;
		}
		break;
	case WIFI_W91_EVENT_STA_CONNECTED:
		if (!unpack_wifi_w91_event_sta_connect_cb(&event, data, len)) {
			return;
		}
		break;
	case WIFI_W91_EVENT_AP_START:
		if (!unpack_wifi_w91_event_ap_start_cb(&event, data, len)) {
			return;
		}
		break;
	default:
		break;
	}

	k_msgq_put(cfg->thread_msgq, &event, K_FOREVER);
}

static void wifi_w91_even_scan_done_handler(struct wifi_w91_data *data,
	struct wifi_w91_event_scan_done *event)
{
	struct wifi_w91_event_scan_done_ap_info *ap_info;
	struct wifi_scan_result scan_result;

	do {
		if ((event->number == 0) || (event->ap_info == NULL)) {
			break;
		}

		for (uint8_t i = 0; i < event->number; i++) {
			ap_info = &event->ap_info[i];

			memset(&scan_result, 0, sizeof(scan_result));

			scan_result.ssid_length = ap_info->ssid_len;
			memcpy(scan_result.ssid, ap_info->ssid, ap_info->ssid_len);
			memcpy(scan_result.mac, ap_info->bssid, sizeof(scan_result.mac));
			scan_result.channel = ap_info->channel;
			scan_result.rssi = ap_info->rssi;
			if (ap_info->authmode > WIFI_W91_SECURITY_OPEN) {
				scan_result.security = WIFI_SECURITY_TYPE_PSK;
			} else {
				scan_result.security = WIFI_SECURITY_TYPE_NONE;
			}

			if (data->base.scan_cb) {
				data->base.scan_cb(data->base.iface, 0, &scan_result);
				/* ensure notifications get delivered */
				k_yield();
			} else {
				break;
			}
		}
	} while (0);

	if (event->ap_info) {
		free(event->ap_info);
	}
}

static void wifi_w91_event_thread(void *p1, void *p2, void *p3)
{
	struct wifi_w91_data *data = ((struct device *)p1)->data;
	const struct wifi_w91_config *cfg = ((struct device *)p1)->config;
	struct wifi_w91_event event;

	while (1) {
		k_msgq_get(cfg->thread_msgq, &event, K_FOREVER);

		switch (event.id) {
		case WIFI_W91_EVENT_STA_START:
			data->base.state = WIFI_W91_STA_STARTED;
			net_if_carrier_on(data->base.iface);
			wifi_w91_reset_state(&data->base.if_state);
			data->base.if_state.state = WIFI_STATE_DISCONNECTED;
			data->base.if_state.iface_mode = WIFI_MODE_INFRA;
			break;
		case WIFI_W91_EVENT_STA_STOP:
			data->base.state = WIFI_W91_STA_STOPPED;
			net_if_carrier_off(data->base.iface);
			wifi_w91_reset_state(&data->base.if_state);
			break;
		case WIFI_W91_EVENT_STA_CONNECTED:
			data->base.state = WIFI_W91_STA_CONNECTED;
			LOG_INF("The WiFi STA connected");
#if CONFIG_NET_IPV6
			(void) wifi_w91_set_ipv6(data->base.iface);
#endif /* CONFIG_NET_IPV6 */
#if CONFIG_NET_IPV4
#if CONFIG_NET_DHCPV4
			net_dhcpv4_restart(data->base.iface);
#else
			(void) wifi_w91_set_ipv4(data->base.iface);
#endif /* CONFIG_NET_DHCPV4 */
#endif /* CONFIG_NET_IPV4 */
			data->base.if_state = (struct wifi_iface_status) {
				.state = WIFI_STATE_COMPLETED,
				.ssid_len = event.param.sta_connect_info.ssid_len,
				.band = WIFI_FREQ_BAND_2_4_GHZ,
				.channel = event.param.sta_connect_info.channel,
				.iface_mode = WIFI_MODE_INFRA,
				.link_mode = WIFI_LINK_MODE_UNKNOWN,
				.security = event.param.sta_connect_info.authmode,
				.rssi = event.param.sta_connect_info.rssi,
			};
			memcpy(data->base.if_state.ssid, event.param.sta_connect_info.ssid,
				sizeof(data->base.if_state.ssid));
			memcpy(data->base.if_state.bssid, event.param.sta_connect_info.bssid,
				sizeof(data->base.if_state.bssid));
			wifi_w91_show_state(&data->base.if_state);
			break;
		case WIFI_W91_EVENT_STA_DISCONNECTED:
			LOG_INF("The WiFi STA disconnected");
			if (data->base.state == WIFI_W91_STA_CONNECTED) {
				wifi_mgmt_raise_disconnect_result_event(data->base.iface, 0);
			} else {
				wifi_mgmt_raise_disconnect_result_event(data->base.iface, -1);
			}
			break;
		case WIFI_W91_EVENT_SCAN_DONE:
			wifi_w91_even_scan_done_handler(data, &event.param.scan_done);
			wifi_w91_reset_state(&data->base.if_state);
			data->base.if_state.state = WIFI_STATE_DISCONNECTED;
			data->base.if_state.iface_mode = WIFI_MODE_INFRA;
			if (data->base.scan_cb) {
				/* report end of scan event */
				data->base.scan_cb(data->base.iface, 0, NULL);
			}
			break;
		case WIFI_W91_EVENT_AP_START:
			data->base.state = WIFI_W91_AP_STARTED;
#if CONFIG_NET_IPV4
			(void) wifi_w91_set_ipv4(data->base.iface);
#endif /* CONFIG_NET_IPV4 */
			LOG_INF("The WiFi Access Point is started");
			data->base.if_state = (struct wifi_iface_status) {
				.state = WIFI_STATE_INACTIVE,
				.ssid_len = event.param.ap_start_info.ssid_len,
				.band = WIFI_FREQ_BAND_2_4_GHZ,
				.channel = event.param.ap_start_info.channel,
				.iface_mode = WIFI_MODE_AP,
				.link_mode = WIFI_LINK_MODE_UNKNOWN,
				.security = event.param.ap_start_info.authmode,
				.dtim_period = event.param.ap_start_info.dtim_period,
				.beacon_interval = event.param.ap_start_info.beacon_interval,
			};
			memcpy(data->base.if_state.ssid, event.param.ap_start_info.ssid,
				sizeof(data->base.if_state.ssid));
			memcpy(data->base.if_state.bssid, event.param.ap_start_info.bssid,
				sizeof(data->base.if_state.bssid));
			wifi_w91_show_state(&data->base.if_state);
			break;
		case WIFI_W91_EVENT_AP_STOP:
			data->base.state = WIFI_W91_AP_STOPPED;
			wifi_w91_reset_state(&data->base.if_state);
			LOG_INF("The WiFi Access Point is stopped");
			break;
		case WIFI_W91_EVENT_AP_STACONNECTED:
			LOG_INF("The WiFi Access Point is connected"
				"(mac = 0x%x:0x%x:0x%x:0x%x:0x%x:0x%x)",
				event.param.ap_sta_info.mac[0], event.param.ap_sta_info.mac[1],
				event.param.ap_sta_info.mac[2], event.param.ap_sta_info.mac[3],
				event.param.ap_sta_info.mac[4], event.param.ap_sta_info.mac[5]);
			break;
		case WIFI_W91_EVENT_AP_STADISCONNECTED:
			LOG_INF("The WiFi Access Point is disconnected"
				"(mac = 0x%x:0x%x:0x%x:0x%x:0x%x:0x%x)",
				event.param.ap_sta_info.mac[0], event.param.ap_sta_info.mac[1],
				event.param.ap_sta_info.mac[2], event.param.ap_sta_info.mac[3],
				event.param.ap_sta_info.mac[4], event.param.ap_sta_info.mac[5]);
			break;
		default:
			break;
		}
	}
}

static int wifi_w91_init(const struct device *dev)
{
	struct wifi_w91_data *data = dev->data;
	const struct wifi_w91_config *cfg = dev->config;

	ipc_based_driver_init(&data->ipc);

	k_tid_t tid = k_thread_create(cfg->thread,
		cfg->thread_stack, CONFIG_TELINK_W91_WIFI_EVENT_THREAD_STACK_SIZE,
		wifi_w91_event_thread, (void *)dev, NULL, NULL,
		CONFIG_TELINK_W91_WIFI_EVENT_THREAD_PRIORITY, 0, K_NO_WAIT);

	k_thread_name_set(tid, "W91_WIFI_EV");

	data->base.state = WIFI_W91_STA_STOPPED;

	ipc_dispatcher_add(IPC_DISPATCHER_MK_ID(IPC_DISPATCHER_WIFI_EVENT, cfg->instance_id),
		wifi_w91_event_cb, (void *)dev);

	wifi_w91_reset_state(&data->base.if_state);

	return 0;
}

static const struct net_wifi_mgmt_offload wifi_w91_driver_api = {
	.wifi_iface.iface_api.init  = wifi_w91_init_if,
	.wifi_mgmt_api              = &(const struct wifi_mgmt_ops) {
		.scan         = wifi_w91_scan,
		.connect      = wifi_w91_connect,
		.disconnect   = wifi_w91_disconnect,
		.ap_enable    = wifi_w91_ap_enable,
		.ap_disable   = wifi_w91_ap_disable,
		.iface_status = wifi_w91_iface_status,
	},
};

#define NET_W91_DEFINE(n)                                       \
                                                                \
	static struct k_thread wifi_event_thread_data_##n;          \
	static K_THREAD_STACK_DEFINE(wifi_event_thread_stack_##n,   \
		CONFIG_TELINK_W91_WIFI_EVENT_THREAD_STACK_SIZE);        \
	K_MSGQ_DEFINE(wifi_event_msgq_##n,                          \
		sizeof(struct wifi_w91_event),                          \
		CONFIG_TELINK_W91_WIFI_EVENT_MAX_MSG,                   \
		sizeof(uint32_t));                                      \
                                                                \
	static const struct wifi_w91_config wifi_config_##n = {     \
		.instance_id = n,                                       \
		.thread = &wifi_event_thread_data_##n,                  \
		.thread_stack = wifi_event_thread_stack_##n,            \
		.thread_msgq = &wifi_event_msgq_##n,                    \
	};                                                          \
                                                                \
	static struct wifi_w91_data wifi_data_##n;                  \
                                                                \
	NET_DEVICE_DT_INST_DEFINE(n, wifi_w91_init,                 \
		NULL, &wifi_data_##n, &wifi_config_##n,                 \
		CONFIG_TELINK_W91_IPC_DRIVERS_INIT_PRIORITY,            \
		&wifi_w91_driver_api, W91_WIFI_L2,                      \
		NET_L2_GET_CTX_TYPE(W91_WIFI_L2), NET_ETH_MTU);

DT_INST_FOREACH_STATUS_OKAY(NET_W91_DEFINE)
