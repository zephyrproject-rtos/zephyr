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
	uint8_t *ssid;
	uint8_t channel;
	uint8_t authmode;
	uint8_t password_len;
	uint8_t *password;
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

union wifi_w91_event_param {
	struct wifi_w91_event_scan_done scan_done;
	struct wifi_w91_event_ap_sta_status ap_sta_info;
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
	int err = 0;

	if (data->base.state == WIFI_W91_STA_CONNECTED ||
		data->base.state == WIFI_W91_AP_STARTED) {

		struct ip_v4_data ipv4 = {
			.ip = iface->config.ip.ipv4->unicast[0].address.in_addr.s_addr,
			.mask = iface->config.ip.ipv4->netmask.s_addr,
			.gw = iface->config.ip.ipv4->gw.s_addr
		};

		err = -ETIMEDOUT;
		IPC_DISPATCHER_HOST_SEND_DATA(&data->ipc, 0,
			wifi_w91_set_ipv4, &ipv4, &err,
			CONFIG_WIFI_TELINK_W91_IPC_RESPONSE_TIMEOUT_MS);
	}

	if (data->base.state == WIFI_W91_STA_CONNECTED) {
		if (err) {
			LOG_INF("set ip error %d", err);
		}
		wifi_mgmt_raise_connect_result_event(data->base.iface, err);
	}

	return 0;
}

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

static struct k_thread wifi_event_thread_data;
K_THREAD_STACK_DEFINE(wifi_event_thread_stack, CONFIG_TELINK_W91_WIFI_EVENT_THREAD_STACK_SIZE);
K_MSGQ_DEFINE(wifi_event_msgq, sizeof(struct wifi_w91_event),
	CONFIG_TELINK_W91_WIFI_EVENT_MAX_MSG, sizeof(uint32_t));

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
	IPC_DISPATCHER_UNPACK_FIELD(pack_data, p_init_resp->mac);
	IPC_DISPATCHER_UNPACK_ARRAY(pack_data, p_init_resp->mac,
		sizeof(p_init_resp->mac));
}

static void wifi_w91_init_if(struct net_if *iface)
{
	LOG_INF("%s", __func__);

	struct wifi_w91_init_resp init_resp;
	struct wifi_w91_data *data = iface->if_dev->dev->data;

	data->base.iface = iface;

	net_if_carrier_off(data->base.iface);

	IPC_DISPATCHER_HOST_SEND_DATA(&data->ipc, 0,
		wifi_w91_init_if, NULL, &init_resp,
		CONFIG_WIFI_TELINK_W91_IPC_RESPONSE_TIMEOUT_MS);

	if (init_resp.err) {
		LOG_ERR("Failed to start Wi-Fi driver (response status is incorrect)");
		return;
	}

	memcpy(data->base.mac, init_resp.mac, sizeof(data->base.mac));

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

static int wifi_w91_scan(const struct device *dev, scan_result_cb_t cb)
{
	LOG_INF("%s", __func__);

	int err;
	struct wifi_w91_data *data = dev->data;

	if (data->base.scan_cb != NULL) {
		LOG_INF("Scan callback in progress");
		return -EINPROGRESS;
	}

	data->base.scan_cb = cb;

	IPC_DISPATCHER_HOST_SEND_DATA(&data->ipc, 0,
		wifi_w91_scan, NULL, &err,
		CONFIG_WIFI_TELINK_W91_IPC_RESPONSE_TIMEOUT_MS);

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

	int err;
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

	IPC_DISPATCHER_HOST_SEND_DATA(&data->ipc, 0,
		wifi_w91_connect, &connect_req, &err,
		CONFIG_WIFI_TELINK_W91_IPC_RESPONSE_TIMEOUT_MS);

	return err;
}

/* APIs implementation: wifi disconnect */
IPC_DISPATCHER_PACK_FUNC_WITHOUT_PARAM(wifi_w91_disconnect, IPC_DISPATCHER_WIFI_DISCONNECT);

IPC_DISPATCHER_UNPACK_FUNC_ONLY_WITH_ERROR_PARAM(wifi_w91_disconnect);

static int wifi_w91_disconnect(const struct device *dev)
{
	LOG_INF("%s", __func__);

	int err;
	struct wifi_w91_data *data = dev->data;

	IPC_DISPATCHER_HOST_SEND_DATA(&data->ipc, 0,
		wifi_w91_disconnect, NULL, &err,
		CONFIG_WIFI_TELINK_W91_IPC_RESPONSE_TIMEOUT_MS);

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

	int err;
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

	IPC_DISPATCHER_HOST_SEND_DATA(&data->ipc, 0,
		wifi_w91_ap_enable, &connect_req, &err,
		CONFIG_WIFI_TELINK_W91_IPC_RESPONSE_TIMEOUT_MS);

	return err;
}

/* APIs implementation: wifi ap disable */
IPC_DISPATCHER_PACK_FUNC_WITHOUT_PARAM(wifi_w91_ap_disable, IPC_DISPATCHER_WIFI_AP_DISABLE);

IPC_DISPATCHER_UNPACK_FUNC_ONLY_WITH_ERROR_PARAM(wifi_w91_ap_disable);

static int wifi_w91_ap_disable(const struct device *dev)
{
	LOG_INF("%s", __func__);

	int err;
	struct wifi_w91_data *data = dev->data;

	IPC_DISPATCHER_HOST_SEND_DATA(&data->ipc, 0,
		wifi_w91_ap_disable, NULL, &err,
		CONFIG_WIFI_TELINK_W91_IPC_RESPONSE_TIMEOUT_MS);

	return err;
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

	p_evt->param.scan_done.ap_info = malloc(sizeof(*ap_info) * p_evt->param.scan_done.number);
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
		IPC_DISPATCHER_UNPACK_ARRAY(pack_data, ap_info->bssid, sizeof(ap_info->bssid));

		expect_len += sizeof(ap_info->ssid_len) + ap_info->ssid_len +
			sizeof(ap_info->channel) + sizeof(ap_info->authmode) +
			sizeof(ap_info->rssi) + sizeof(ap_info->bssid);
	}

	if (expect_len != pack_data_len) {
		free(p_evt->param.scan_done.ap_info);
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
	default:
		break;
	}

	k_msgq_put(&wifi_event_msgq, &event, K_FOREVER);
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

	if (data->base.scan_cb) {
		/* report end of scan event */
		data->base.scan_cb(data->base.iface, 0, NULL);
		data->base.scan_cb = NULL;
	}
}

static void wifi_w91_event_thread(void *p1, void *p2, void *p3)
{
	struct wifi_w91_data *data = ((struct device *)p1)->data;
	struct wifi_w91_event event;

	while (1) {
		k_msgq_get(&wifi_event_msgq, &event, K_FOREVER);

		switch (event.id) {
		case WIFI_W91_EVENT_STA_START:
			data->base.state = WIFI_W91_STA_STARTED;
			net_if_carrier_on(data->base.iface);
			break;
		case WIFI_W91_EVENT_STA_STOP:
			data->base.state = WIFI_W91_STA_STOPPED;
			net_if_carrier_off(data->base.iface);
			break;
		case WIFI_W91_EVENT_STA_CONNECTED:
			data->base.state = WIFI_W91_STA_CONNECTED;
			LOG_INF("The WiFi STA connected");
#if !CONFIG_NET_DHCPV4
			(void) wifi_w91_set_ipv4(data->base.iface);
#endif /* !CONFIG_NET_DHCPV4 */
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
			break;
		case WIFI_W91_EVENT_AP_START:
			data->base.state = WIFI_W91_AP_STARTED;
			(void) wifi_w91_set_ipv4(data->base.iface);
			LOG_INF("The WiFi Access Point is started");
			break;
		case WIFI_W91_EVENT_AP_STOP:
			data->base.state = WIFI_W91_AP_STOPPED;
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

	ipc_based_driver_init(&data->ipc);

	k_thread_create(&wifi_event_thread_data,
		wifi_event_thread_stack, K_THREAD_STACK_SIZEOF(wifi_event_thread_stack),
		wifi_w91_event_thread, (void *)dev, NULL, NULL,
		CONFIG_TELINK_W91_WIFI_EVENT_THREAD_PRIORITY, 0, K_NO_WAIT);

	data->base.state = WIFI_W91_STA_STOPPED;

	ipc_dispatcher_add(IPC_DISPATCHER_MK_ID(IPC_DISPATCHER_WIFI_EVENT, 0),
		wifi_w91_event_cb, NULL);

	return 0;
}

static const struct net_wifi_mgmt_offload wifi_w91_driver_api = {
	.wifi_iface.iface_api.init  = wifi_w91_init_if,
	.scan                       = wifi_w91_scan,
	.connect                    = wifi_w91_connect,
	.disconnect                 = wifi_w91_disconnect,
	.ap_enable                  = wifi_w91_ap_enable,
	.ap_disable                 = wifi_w91_ap_disable,
};

#define NET_W91_DEFINE(n)                                       \
                                                                \
	static const struct wifi_w91_config wifi_config_##n = {     \
		.instance_id = n,                                       \
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
