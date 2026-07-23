/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/dhcpv4.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/wifi.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/wifi_nm.h>
#include <zephyr/net/conn_mgr/connectivity_wifi_mgmt.h>

#include <pb_encode.h>
#include <pb_decode.h>
#include <esp_hosted_mcu_rpc.pb.h>

#include <esp_hosted_mcu.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(esp_hosted_mcu, CONFIG_WIFI_LOG_LEVEL);

#define ESP_HOSTED_MCU_MAC_ADDR_LEN (6)
#define ESP_HOSTED_MCU_MAX_SSID_LEN (32)
#define ESP_HOSTED_MCU_MAX_PASS_LEN (64)

/* Coprocessor-side esp_wifi init magic; esp_wifi_init rejects any other value. */
#define ESP_HOSTED_MCU_WIFI_INIT_MAGIC 0x1F2F3F4F

/* Coprocessor-side wifi_mode_t values (esp_wifi), not the Zephyr WIFI_MODE_* enum. */
#define ESP_HOSTED_MCU_WIFI_MODE_STA   1
#define ESP_HOSTED_MCU_WIFI_MODE_AP    2
#define ESP_HOSTED_MCU_WIFI_MODE_APSTA 3

/*
 * Coprocessor-side wifi_interface_t values (esp_wifi): STA=0, AP=1. These are the
 * values the WifiSetConfig/GetConfig RPC iface field expects, distinct from
 * the esp_hosted_mcu_if_type_t frame-routing enum (STA_IF=1, AP_IF=2).
 */
#define ESP_HOSTED_MCU_WIFI_IF_STA 0
#define ESP_HOSTED_MCU_WIFI_IF_AP  1

/* SAE PWE derivation: 0=HUNT_AND_PECK, 1=HASH_TO_ELEMENT, 2=BOTH. */
#define ESP_HOSTED_MCU_SAE_PWE_BOTH 2

/* Coprocessor-side wifi_ps_type_t values (esp_wifi): none, min and max modem. */
#define ESP_HOSTED_MCU_PS_NONE      0
#define ESP_HOSTED_MCU_PS_MIN_MODEM 1
#define ESP_HOSTED_MCU_PS_MAX_MODEM 2

/* Logical network interface index within this driver. */
enum esp_hosted_mcu_iface {
	ESP_HOSTED_MCU_IFACE_STA = 0,
	ESP_HOSTED_MCU_IFACE_AP,
	ESP_HOSTED_MCU_IFACE_COUNT,
};

struct esp_hosted_mcu_data {
	struct net_if *iface[ESP_HOSTED_MCU_IFACE_COUNT];
	uint8_t mac_addr[ESP_HOSTED_MCU_IFACE_COUNT][ESP_HOSTED_MCU_MAC_ADDR_LEN];
	enum wifi_iface_state state[ESP_HOSTED_MCU_IFACE_COUNT];
	struct k_sem scan_sem;
#if defined(CONFIG_NET_STATISTICS_WIFI)
	struct net_stats_wifi stats;
#endif
};

static struct esp_hosted_mcu_data esp_hosted_mcu_data_0;

/*
 * Serialises the shared transmit scratch buffer. The STA and AP interfaces
 * share one static buffer, so their transmits must take turns filling it.
 * Distinct from the core's own transmit lock, which serialises the bus.
 */
static K_MUTEX_DEFINE(esp_hosted_mcu_netif_tx_lock);

/*
 * Per-instance immutable config. Both network devices share one data struct,
 * so the interface a device drives is carried here rather than derived from
 * the device name: a rename would otherwise silently resolve to the AP.
 */
struct esp_hosted_mcu_iface_config {
	enum esp_hosted_mcu_iface index;
};

static const struct esp_hosted_mcu_iface_config esp_hosted_mcu_sta_config = {
	.index = ESP_HOSTED_MCU_IFACE_STA,
};

#if defined(CONFIG_WIFI_ESP_HOSTED_MCU_AP_STA_MODE)
static const struct esp_hosted_mcu_iface_config esp_hosted_mcu_ap_config = {
	.index = ESP_HOSTED_MCU_IFACE_AP,
};
#endif

static inline size_t esp_hosted_mcu_iface_index(const struct device *dev)
{
	const struct esp_hosted_mcu_iface_config *cfg = dev->config;

	return cfg->index;
}

/* Handle an asynchronous RPC event forwarded by the core. */
static void esp_hosted_mcu_handle_event(const Rpc *rpc, void *user_data)
{
	struct esp_hosted_mcu_data *data = user_data;
	struct net_if *sta = data->iface[ESP_HOSTED_MCU_IFACE_STA];

	/*
	 * A station event may arrive before the netif is registered (the
	 * coprocessor can reconnect on its own after a warm reboot). Drop it
	 * rather than dereference a NULL interface; net_dhcpv4_start() and the
	 * mgmt raise helpers do not tolerate a NULL iface.
	 */
	if (sta == NULL && (rpc->msg_id == RpcId_Event_StaConnected ||
			    rpc->msg_id == RpcId_Event_StaDisconnected)) {
		return;
	}

	switch (rpc->msg_id) {
	case RpcId_Event_ESPInit:
		LOG_INF("coprocessor ESPInit event");
		break;
	case RpcId_Event_StaConnected:
		data->state[ESP_HOSTED_MCU_IFACE_STA] = WIFI_STATE_COMPLETED;
		net_if_dormant_off(sta);
		wifi_mgmt_raise_connect_result_event(sta, 0);
		if (IS_ENABLED(CONFIG_WIFI_STA_AUTO_DHCPV4)) {
			net_dhcpv4_start(sta);
		}
		break;
	case RpcId_Event_StaDisconnected:
		LOG_DBG("StaDisconnected reason=%u",
			rpc->payload.event_sta_disconnected.sta_disconnected.reason);
		if (data->state[ESP_HOSTED_MCU_IFACE_STA] == WIFI_STATE_ASSOCIATING) {
			/*
			 * A disconnect while still associating means the connect
			 * attempt failed (for example a wrong passphrase), so
			 * report a connect failure rather than a clean
			 * disconnect. Otherwise the caller waits forever.
			 */
			data->state[ESP_HOSTED_MCU_IFACE_STA] = WIFI_STATE_DISCONNECTED;
			wifi_mgmt_raise_connect_result_event(sta, WIFI_STATUS_CONN_FAIL);
		} else {
			data->state[ESP_HOSTED_MCU_IFACE_STA] = WIFI_STATE_DISCONNECTED;
			if (IS_ENABLED(CONFIG_WIFI_STA_AUTO_DHCPV4)) {
				net_dhcpv4_stop(sta);
			}
			net_if_dormant_on(sta);
			wifi_mgmt_raise_disconnect_result_event(sta, 0);
		}
		break;
	case RpcId_Event_StaScanDone:
		k_sem_give(&data->scan_sem);
		break;
#if defined(CONFIG_WIFI_ESP_HOSTED_MCU_AP_STA_MODE)
	case RpcId_Event_AP_StaConnected: {
		const Rpc_Event_AP_StaConnected *ev = &rpc->payload.event_ap_sta_connected;
		struct net_if *ap = data->iface[ESP_HOSTED_MCU_IFACE_AP];
		struct wifi_ap_sta_info info = {0};

		info.mac_length = MIN(ev->mac.size, WIFI_MAC_ADDR_LEN);
		memcpy(info.mac, ev->mac.bytes, info.mac_length);
		wifi_mgmt_raise_ap_sta_connected_event(ap, &info);
		break;
	}
	case RpcId_Event_AP_StaDisconnected: {
		const Rpc_Event_AP_StaDisconnected *ev = &rpc->payload.event_ap_sta_disconnected;
		struct net_if *ap = data->iface[ESP_HOSTED_MCU_IFACE_AP];
		struct wifi_ap_sta_info info = {0};

		info.mac_length = MIN(ev->mac.size, WIFI_MAC_ADDR_LEN);
		memcpy(info.mac, ev->mac.bytes, info.mac_length);
		wifi_mgmt_raise_ap_sta_disconnected_event(ap, &info);
		break;
	}
#endif /* CONFIG_WIFI_ESP_HOSTED_MCU_AP_STA_MODE */
	default:
		LOG_DBG("unhandled event msg_id %d", rpc->msg_id);
		break;
	}
}

/* Feed an inbound data frame (STA/AP) up into the network stack. */
static void esp_hosted_mcu_rx_data(const uint8_t *payload, uint16_t len, uint8_t pkt_type,
				   void *user_data)
{
	struct net_if *iface = user_data;
#if defined(CONFIG_NET_STATISTICS_WIFI)
	struct esp_hosted_mcu_data *data = &esp_hosted_mcu_data_0;
#endif
	struct net_pkt *pkt;

	ARG_UNUSED(pkt_type);

	if (iface == NULL) {
		return;
	}

	/*
	 * This runs on the shared receive thread that also drives the Bluetooth
	 * path, so never block here: a full pool must drop the frame rather than
	 * stall the other protocol. The upper transports recover the loss.
	 */
	pkt = net_pkt_rx_alloc_with_buffer(iface, len, AF_UNSPEC, 0, K_NO_WAIT);
	if (pkt == NULL) {
#if defined(CONFIG_NET_STATISTICS_WIFI)
		data->stats.errors.rx++;
#endif
		return;
	}

	if (net_pkt_write(pkt, payload, len) < 0 || net_recv_data(iface, pkt) < 0) {
		net_pkt_unref(pkt);
#if defined(CONFIG_NET_STATISTICS_WIFI)
		data->stats.errors.rx++;
#endif
		return;
	}

#if defined(CONFIG_NET_STATISTICS_WIFI)
	data->stats.bytes.received += len;
	data->stats.pkts.rx++;
#endif
}

/*
 * wifi_mgmt operations. Unlike the serial RPC path, the mcu coprocessor exposes the raw
 * esp_wifi API, so each management action expands into a short sequence of RPC
 * calls that drives the coprocessor's esp_wifi state machine.
 */

/* Map an esp_wifi wifi_auth_mode_t to the Zephyr security enum. */
static enum wifi_security_type esp_hosted_mcu_map_security(int authmode)
{
	switch (authmode) {
	case 0: /* OPEN */
		return WIFI_SECURITY_TYPE_NONE;
	case 1: /* WEP */
		return WIFI_SECURITY_TYPE_WEP;
	case 2: /* WPA_PSK */
		return WIFI_SECURITY_TYPE_WPA_PSK;
	case 3: /* WPA2_PSK */
	case 4: /* WPA_WPA2_PSK */
		return WIFI_SECURITY_TYPE_PSK;
	case 6: /* WPA3_PSK */
	case 7: /* WPA2_WPA3_PSK */
		return WIFI_SECURITY_TYPE_SAE;
	default:
		return WIFI_SECURITY_TYPE_UNKNOWN;
	}
}

static int esp_hosted_mcu_scan(const struct device *dev, struct net_if *iface,
			       struct wifi_scan_params *params, scan_result_cb_t cb)
{
	struct esp_hosted_mcu_data *data = dev->data;
	Rpc_Resp_WifiScanGetApRecords *recs;
	Rpc req = Rpc_init_zero;
	Rpc resp = Rpc_init_zero;
	int ret;

	ARG_UNUSED(params);

	k_sem_reset(&data->scan_sem);

	/* Kick off a blocking scan on the coprocessor. */
	req.msg_id = RpcId_Req_WifiScanStart;
	req.which_payload = Rpc_req_wifi_scan_start_tag;
	req.payload.req_wifi_scan_start.block = true;
	req.payload.req_wifi_scan_start.config_set = 0;
	ret = esp_hosted_mcu_rpc_call(&req, &resp, ESP_HOSTED_MCU_RPC_TIMEOUT);
	if (ret) {
		return -EIO;
	}

	/* Wait for the scan-done event, then read how many APs were found. */
	if (k_sem_take(&data->scan_sem, K_MSEC(ESP_HOSTED_MCU_SCAN_TIMEOUT)) != 0) {
		return -ETIMEDOUT;
	}

	req = (Rpc)Rpc_init_zero;
	req.msg_id = RpcId_Req_WifiScanGetApNum;
	req.which_payload = Rpc_req_wifi_scan_get_ap_num_tag;
	ret = esp_hosted_mcu_rpc_call(&req, &resp, ESP_HOSTED_MCU_RPC_TIMEOUT);
	if (ret) {
		return -EIO;
	}

	int num = resp.payload.resp_wifi_scan_get_ap_num.number;

	/* Page the AP records back and report each to the caller. */
	req = (Rpc)Rpc_init_zero;
	req.msg_id = RpcId_Req_WifiScanGetApRecords;
	req.which_payload = Rpc_req_wifi_scan_get_ap_records_tag;
	req.payload.req_wifi_scan_get_ap_records.number = num;
	ret = esp_hosted_mcu_rpc_call(&req, &resp, ESP_HOSTED_MCU_RPC_TIMEOUT);
	if (ret) {
		pb_release(Rpc_fields, &resp);
		return -EIO;
	}

	/* Report each decoded AP record, then signal scan completion. */
	recs = &resp.payload.resp_wifi_scan_get_ap_records;

	for (pb_size_t i = 0; cb && i < recs->ap_records_count; i++) {
		const wifi_ap_record *ap = &recs->ap_records[i];
		struct wifi_scan_result res = {0};

		res.ssid_length = MIN(ap->ssid.size, WIFI_SSID_MAX_LEN);
		memcpy(res.ssid, ap->ssid.bytes, res.ssid_length);
		res.mac_length = MIN(ap->bssid.size, WIFI_MAC_ADDR_LEN);
		memcpy(res.mac, ap->bssid.bytes, res.mac_length);
		res.channel = ap->primary;
		res.rssi = ap->rssi;
		res.band = WIFI_FREQ_BAND_2_4_GHZ;
		res.security = esp_hosted_mcu_map_security(ap->authmode);

		cb(iface, 0, &res);
	}

	if (cb) {
		cb(iface, 0, NULL);
	}

	/* Free the nanopb-allocated ap_records array before returning. */
	pb_release(Rpc_fields, &resp);

	return 0;
}

static int esp_hosted_mcu_connect(const struct device *dev, struct net_if *iface,
				  struct wifi_connect_req_params *params)
{
	struct esp_hosted_mcu_data *data = dev->data;
	Rpc req = Rpc_init_zero;
	Rpc resp = Rpc_init_zero;
	wifi_sta_config *sta;
	int ret;

	ARG_UNUSED(iface);

	/* Push the STA credentials, then issue connect. */
	req.msg_id = RpcId_Req_WifiSetConfig;
	req.which_payload = Rpc_req_wifi_set_config_tag;
	req.payload.req_wifi_set_config.iface = ESP_HOSTED_MCU_WIFI_IF_STA;
	req.payload.req_wifi_set_config.has_cfg = true;
	req.payload.req_wifi_set_config.cfg.which_u = wifi_config_sta_tag;
	sta = &req.payload.req_wifi_set_config.cfg.u.sta;

	sta->ssid.size = MIN(params->ssid_length, ESP_HOSTED_MCU_MAX_SSID_LEN);
	memcpy(sta->ssid.bytes, params->ssid, sta->ssid.size);

	/*
	 * The passphrase arrives in psk for WPA2 and in sae_password for SAE;
	 * the coprocessor takes both in the single password field.
	 */
	if (params->psk_length) {
		sta->password.size = MIN(params->psk_length, ESP_HOSTED_MCU_MAX_PASS_LEN);
		memcpy(sta->password.bytes, params->psk, sta->password.size);
	} else if (params->sae_password_length) {
		sta->password.size = MIN(params->sae_password_length, ESP_HOSTED_MCU_MAX_PASS_LEN);
		memcpy(sta->password.bytes, params->sae_password, sta->password.size);
	}
	if (params->channel != WIFI_CHANNEL_ANY) {
		sta->channel = params->channel;
	}

	/*
	 * For WPA3-SAE, advertise PMF capable (not required, so a WPA2/WPA3
	 * transition AP still matches) and allow both HUNT-and-PECK and H2E
	 * PWE so either flavour of a WPA3 AP is accepted.
	 */
	if (params->security == WIFI_SECURITY_TYPE_SAE ||
	    params->security == WIFI_SECURITY_TYPE_SAE_H2E ||
	    params->security == WIFI_SECURITY_TYPE_SAE_AUTO ||
	    params->security == WIFI_SECURITY_TYPE_FT_SAE ||
	    params->security == WIFI_SECURITY_TYPE_SAE_EXT_KEY) {
		sta->has_pmf_cfg = true;
		sta->pmf_cfg.capable = true;
		sta->sae_pwe_h2e = ESP_HOSTED_MCU_SAE_PWE_BOTH;
	}

	ret = esp_hosted_mcu_rpc_call(&req, &resp, ESP_HOSTED_MCU_RPC_TIMEOUT);
	if (ret) {
		return -EIO;
	}

	/*
	 * Mark the interface associating before issuing connect. Completion is
	 * reported asynchronously via Rpc_Event_StaConnected, and a fast
	 * StaDisconnected can arrive on the event thread before the RPC even
	 * returns; the event handler keys off this state, so it must be set
	 * first or a connect failure is misread as a clean disconnect.
	 */
	data->state[ESP_HOSTED_MCU_IFACE_STA] = WIFI_STATE_ASSOCIATING;

	req = (Rpc)Rpc_init_zero;
	req.msg_id = RpcId_Req_WifiConnect;
	req.which_payload = Rpc_req_wifi_connect_tag;
	ret = esp_hosted_mcu_rpc_call(&req, &resp, ESP_HOSTED_MCU_RPC_TIMEOUT);
	if (ret) {
		data->state[ESP_HOSTED_MCU_IFACE_STA] = WIFI_STATE_DISCONNECTED;
		return -EAGAIN;
	}

	return 0;
}

static int esp_hosted_mcu_disconnect(const struct device *dev, struct net_if *iface)
{
	Rpc req = Rpc_init_zero;
	Rpc resp = Rpc_init_zero;
	int ret;

	ARG_UNUSED(dev);
	ARG_UNUSED(iface);

	req.msg_id = RpcId_Req_WifiDisconnect;
	req.which_payload = Rpc_req_wifi_disconnect_tag;
	ret = esp_hosted_mcu_rpc_call(&req, &resp, ESP_HOSTED_MCU_RPC_TIMEOUT);

	return ret ? -EIO : 0;
}

#if defined(CONFIG_WIFI_ESP_HOSTED_MCU_AP_STA_MODE)
static int esp_hosted_mcu_ap_enable(const struct device *dev, struct net_if *iface,
				    struct wifi_connect_req_params *params)
{
	struct esp_hosted_mcu_data *data = dev->data;
	Rpc req = Rpc_init_zero;
	Rpc resp = Rpc_init_zero;
	wifi_ap_config *ap;
	int ret;

	/*
	 * The coprocessor boots in station mode, so a SetConfig for the AP interface
	 * is rejected until the mode includes AP. Request combined AP-STA mode
	 * so a concurrent station link keeps working.
	 */
	req.msg_id = RpcId_Req_SetWifiMode;
	req.which_payload = Rpc_req_set_wifi_mode_tag;
	req.payload.req_set_wifi_mode.mode = ESP_HOSTED_MCU_WIFI_MODE_APSTA;
	ret = esp_hosted_mcu_rpc_call(&req, &resp, ESP_HOSTED_MCU_RPC_TIMEOUT);
	if (ret) {
		ret = -EIO;
		goto out;
	}

	req = (Rpc)Rpc_init_zero;
	req.msg_id = RpcId_Req_WifiSetConfig;
	req.which_payload = Rpc_req_wifi_set_config_tag;
	req.payload.req_wifi_set_config.iface = ESP_HOSTED_MCU_WIFI_IF_AP;
	req.payload.req_wifi_set_config.has_cfg = true;
	req.payload.req_wifi_set_config.cfg.which_u = wifi_config_ap_tag;
	ap = &req.payload.req_wifi_set_config.cfg.u.ap;

	ap->ssid.size = MIN(params->ssid_length, ESP_HOSTED_MCU_MAX_SSID_LEN);
	memcpy(ap->ssid.bytes, params->ssid, ap->ssid.size);
	ap->ssid_len = ap->ssid.size;
	if (params->psk_length) {
		ap->password.size = MIN(params->psk_length, ESP_HOSTED_MCU_MAX_PASS_LEN);
		memcpy(ap->password.bytes, params->psk, ap->password.size);
	}
	ap->channel = (params->channel == WIFI_CHANNEL_ANY)
			      ? CONFIG_WIFI_ESP_HOSTED_MCU_AP_CHANNEL_DEF
			      : params->channel;
	ap->max_connection = CONFIG_WIFI_MGMT_AP_MAX_NUM_STA;

	ret = esp_hosted_mcu_rpc_call(&req, &resp, ESP_HOSTED_MCU_RPC_TIMEOUT);
	if (ret) {
		ret = -EIO;
		goto out;
	}

	req = (Rpc)Rpc_init_zero;
	req.msg_id = RpcId_Req_WifiStart;
	req.which_payload = Rpc_req_wifi_start_tag;
	ret = esp_hosted_mcu_rpc_call(&req, &resp, ESP_HOSTED_MCU_RPC_TIMEOUT);
	if (ret) {
		ret = -EIO;
		goto out;
	}

	data->state[ESP_HOSTED_MCU_IFACE_AP] = WIFI_STATE_COMPLETED;
	net_if_dormant_off(iface);

out:
	wifi_mgmt_raise_ap_enable_result_event(iface, ret);
	return ret;
}

static int esp_hosted_mcu_ap_disable(const struct device *dev, struct net_if *iface)
{
	struct esp_hosted_mcu_data *data = dev->data;
	Rpc req = Rpc_init_zero;
	Rpc resp = Rpc_init_zero;
	int ret;

	req.msg_id = RpcId_Req_WifiStop;
	req.which_payload = Rpc_req_wifi_stop_tag;
	ret = esp_hosted_mcu_rpc_call(&req, &resp, ESP_HOSTED_MCU_RPC_TIMEOUT);

	data->state[ESP_HOSTED_MCU_IFACE_AP] = WIFI_STATE_DISCONNECTED;
	wifi_mgmt_raise_ap_disable_result_event(iface, ret ? -EIO : 0);
	return ret ? -EIO : 0;
}
#endif /* CONFIG_WIFI_ESP_HOSTED_MCU_AP_STA_MODE */

static int esp_hosted_mcu_mode(const struct device *dev, struct net_if *iface,
			       struct wifi_mode_info *info)
{
	Rpc req = Rpc_init_zero;
	Rpc resp = Rpc_init_zero;
	int ret;

	ARG_UNUSED(dev);
	ARG_UNUSED(iface);

	if (info->oper == WIFI_MGMT_GET) {
		req.msg_id = RpcId_Req_GetWifiMode;
		req.which_payload = Rpc_req_get_wifi_mode_tag;
		ret = esp_hosted_mcu_rpc_call(&req, &resp, ESP_HOSTED_MCU_RPC_TIMEOUT);
		if (ret) {
			return -EIO;
		}
		info->mode = (resp.payload.resp_get_wifi_mode.mode == ESP_HOSTED_MCU_WIFI_MODE_AP)
				     ? WIFI_MODE_AP
				     : WIFI_MODE_INFRA;
		return 0;
	}

	req.msg_id = RpcId_Req_SetWifiMode;
	req.which_payload = Rpc_req_set_wifi_mode_tag;
	req.payload.req_set_wifi_mode.mode = (info->mode == WIFI_MODE_AP)
						     ? ESP_HOSTED_MCU_WIFI_MODE_AP
						     : ESP_HOSTED_MCU_WIFI_MODE_STA;
	ret = esp_hosted_mcu_rpc_call(&req, &resp, ESP_HOSTED_MCU_RPC_TIMEOUT);

	return ret ? -EIO : 0;
}

static int esp_hosted_mcu_channel(const struct device *dev, struct net_if *iface,
				  struct wifi_channel_info *info)
{
	Rpc req = Rpc_init_zero;
	Rpc resp = Rpc_init_zero;
	int ret;

	ARG_UNUSED(dev);
	ARG_UNUSED(iface);

	if (info->oper == WIFI_MGMT_GET) {
		req.msg_id = RpcId_Req_WifiGetChannel;
		req.which_payload = Rpc_req_wifi_get_channel_tag;
		ret = esp_hosted_mcu_rpc_call(&req, &resp, ESP_HOSTED_MCU_RPC_TIMEOUT);
		if (ret) {
			return -EIO;
		}
		info->channel = resp.payload.resp_wifi_get_channel.primary;
		return 0;
	}

	req.msg_id = RpcId_Req_WifiSetChannel;
	req.which_payload = Rpc_req_wifi_set_channel_tag;
	req.payload.req_wifi_set_channel.primary = info->channel;
	req.payload.req_wifi_set_channel.second = 0;
	ret = esp_hosted_mcu_rpc_call(&req, &resp, ESP_HOSTED_MCU_RPC_TIMEOUT);

	return ret ? -EIO : 0;
}

static int esp_hosted_mcu_set_power_save(const struct device *dev, struct net_if *iface,
					 struct wifi_ps_params *params)
{
	Rpc req = Rpc_init_zero;
	Rpc resp = Rpc_init_zero;
	int type;
	int ret;

	ARG_UNUSED(dev);
	ARG_UNUSED(iface);

	/* Only the enable state and the legacy/WMM mode map to the coprocessor. */
	switch (params->type) {
	case WIFI_PS_PARAM_STATE:
	case WIFI_PS_PARAM_MODE:
		break;
	default:
		params->fail_reason = WIFI_PS_PARAM_FAIL_OPERATION_NOT_SUPPORTED;
		return -ENOTSUP;
	}

	if (params->enabled == WIFI_PS_DISABLED) {
		type = ESP_HOSTED_MCU_PS_NONE;
	} else if (params->mode == WIFI_PS_MODE_WMM) {
		type = ESP_HOSTED_MCU_PS_MAX_MODEM;
	} else {
		type = ESP_HOSTED_MCU_PS_MIN_MODEM;
	}

	req.msg_id = RpcId_Req_WifiSetPs;
	req.which_payload = Rpc_req_wifi_set_ps_tag;
	req.payload.req_wifi_set_ps.type = type;
	ret = esp_hosted_mcu_rpc_call(&req, &resp, ESP_HOSTED_MCU_RPC_TIMEOUT);
	if (ret) {
		params->fail_reason = WIFI_PS_PARAM_FAIL_CMD_EXEC_FAIL;
		return -EIO;
	}

	return 0;
}

static int esp_hosted_mcu_get_power_save(const struct device *dev, struct net_if *iface,
					 struct wifi_ps_config *config)
{
	Rpc req = Rpc_init_zero;
	Rpc resp = Rpc_init_zero;
	int ret;

	ARG_UNUSED(dev);
	ARG_UNUSED(iface);

	req.msg_id = RpcId_Req_WifiGetPs;
	req.which_payload = Rpc_req_wifi_get_ps_tag;
	ret = esp_hosted_mcu_rpc_call(&req, &resp, ESP_HOSTED_MCU_RPC_TIMEOUT);
	if (ret) {
		return -EIO;
	}

	switch (resp.payload.resp_wifi_get_ps.type) {
	case ESP_HOSTED_MCU_PS_MIN_MODEM:
		config->ps_params.enabled = WIFI_PS_ENABLED;
		config->ps_params.mode = WIFI_PS_MODE_LEGACY;
		break;
	case ESP_HOSTED_MCU_PS_MAX_MODEM:
		config->ps_params.enabled = WIFI_PS_ENABLED;
		config->ps_params.mode = WIFI_PS_MODE_WMM;
		break;
	default:
		config->ps_params.enabled = WIFI_PS_DISABLED;
		break;
	}

	return 0;
}

#if defined(CONFIG_WIFI_ESP_HOSTED_MCU_AP_STA_MODE)
static int esp_hosted_mcu_ap_sta_disconnect(const struct device *dev, struct net_if *iface,
					    const uint8_t *mac)
{
	Rpc req = Rpc_init_zero;
	Rpc resp = Rpc_init_zero;
	int aid;
	int ret;

	ARG_UNUSED(dev);
	ARG_UNUSED(iface);

	/* Deauthentication is by association id, so resolve the MAC first. */
	req.msg_id = RpcId_Req_WifiApGetStaAid;
	req.which_payload = Rpc_req_wifi_ap_get_sta_aid_tag;
	req.payload.req_wifi_ap_get_sta_aid.mac.size = WIFI_MAC_ADDR_LEN;
	memcpy(req.payload.req_wifi_ap_get_sta_aid.mac.bytes, mac, WIFI_MAC_ADDR_LEN);
	ret = esp_hosted_mcu_rpc_call(&req, &resp, ESP_HOSTED_MCU_RPC_TIMEOUT);
	if (ret) {
		return -EIO;
	}
	aid = resp.payload.resp_wifi_ap_get_sta_aid.aid;

	req = (Rpc)Rpc_init_zero;
	req.msg_id = RpcId_Req_WifiDeauthSta;
	req.which_payload = Rpc_req_wifi_deauth_sta_tag;
	req.payload.req_wifi_deauth_sta.aid = aid;
	ret = esp_hosted_mcu_rpc_call(&req, &resp, ESP_HOSTED_MCU_RPC_TIMEOUT);

	return ret ? -EIO : 0;
}
#endif /* CONFIG_WIFI_ESP_HOSTED_MCU_AP_STA_MODE */

/* Derive the Zephyr link mode from the ap_record phy bitmask. */
static enum wifi_link_mode esp_hosted_mcu_link_mode(uint32_t bitmask, bool he)
{
	if (he) {
		return WIFI_6;
	}
	if (bitmask & BIT(2)) {
		return WIFI_4;
	}
	if (bitmask & BIT(1)) {
		return WIFI_3;
	}
	if (bitmask & BIT(0)) {
		return WIFI_1;
	}
	return WIFI_LINK_MODE_UNKNOWN;
}

static int esp_hosted_mcu_iface_status(const struct device *dev, struct net_if *iface,
				       struct wifi_iface_status *status)
{
	struct esp_hosted_mcu_data *data = dev->data;
	size_t itf = esp_hosted_mcu_iface_index(dev);
	Rpc req = Rpc_init_zero;
	Rpc resp = Rpc_init_zero;
	const wifi_ap_record *ap;
	int ret;

	ARG_UNUSED(iface);

	status->state = data->state[itf];
	status->band = WIFI_FREQ_BAND_2_4_GHZ;
	status->link_mode = WIFI_LINK_MODE_UNKNOWN;
	status->mfp = WIFI_MFP_DISABLE;
	status->iface_mode = (itf == ESP_HOSTED_MCU_IFACE_AP) ? WIFI_MODE_AP : WIFI_MODE_INFRA;

	/* Only a connected station can report the associated AP details. */
	if (itf != ESP_HOSTED_MCU_IFACE_STA || data->state[itf] != WIFI_STATE_COMPLETED) {
		return 0;
	}

	req.msg_id = RpcId_Req_WifiStaGetApInfo;
	req.which_payload = Rpc_req_wifi_sta_get_ap_info_tag;
	ret = esp_hosted_mcu_rpc_call(&req, &resp, ESP_HOSTED_MCU_RPC_TIMEOUT);
	if (ret || !resp.payload.resp_wifi_sta_get_ap_info.has_ap_record) {
		pb_release(Rpc_fields, &resp);
		return 0;
	}

	ap = &resp.payload.resp_wifi_sta_get_ap_info.ap_record;

	status->ssid_len = MIN(ap->ssid.size, WIFI_SSID_MAX_LEN);
	memcpy(status->ssid, ap->ssid.bytes, status->ssid_len);
	memcpy(status->bssid, ap->bssid.bytes, MIN(ap->bssid.size, WIFI_MAC_ADDR_LEN));
	status->channel = ap->primary;
	status->rssi = ap->rssi;
	status->security = esp_hosted_mcu_map_security(ap->authmode);
	status->link_mode = esp_hosted_mcu_link_mode(ap->bitmask, ap->has_he_ap);
	if (status->security == WIFI_SECURITY_TYPE_SAE) {
		status->mfp = WIFI_MFP_REQUIRED;
	}

	/* The ap_record country field is a nanopb pointer field. */
	pb_release(Rpc_fields, &resp);

	return 0;
}

#if defined(CONFIG_NET_STATISTICS_WIFI)
static int esp_hosted_mcu_stats(const struct device *dev, struct net_if *iface,
				struct net_stats_wifi *stats)
{
	struct esp_hosted_mcu_data *data = dev->data;

	ARG_UNUSED(iface);
	memcpy(stats, &data->stats, sizeof(*stats));
	return 0;
}
#endif

static int esp_hosted_mcu_send(const struct device *dev, struct net_pkt *pkt)
{
#if defined(CONFIG_NET_STATISTICS_WIFI)
	struct esp_hosted_mcu_data *data = dev->data;
#endif
	static uint8_t buf[ESP_HOSTED_MCU_MAX_PAYLOAD];
	size_t itf = esp_hosted_mcu_iface_index(dev);
	size_t pkt_len = net_pkt_get_len(pkt);
	esp_hosted_mcu_if_type_t if_type;
	int ret;

	if (pkt_len > ESP_HOSTED_MCU_MAX_PAYLOAD) {
		return -ENOMEM;
	}

	k_mutex_lock(&esp_hosted_mcu_netif_tx_lock, K_FOREVER);

	if (net_pkt_read(pkt, buf, pkt_len) < 0) {
		k_mutex_unlock(&esp_hosted_mcu_netif_tx_lock);
		return -EIO;
	}

	if_type = (itf == ESP_HOSTED_MCU_IFACE_AP) ? ESP_HOSTED_MCU_AP_IF : ESP_HOSTED_MCU_STA_IF;
	ret = esp_hosted_mcu_send_frame(if_type, 0, 0, buf, pkt_len);

	k_mutex_unlock(&esp_hosted_mcu_netif_tx_lock);

#if defined(CONFIG_NET_STATISTICS_WIFI)
	if (ret >= 0) {
		data->stats.bytes.sent += pkt_len;
		data->stats.pkts.tx++;
	} else {
		data->stats.errors.tx++;
	}
#endif

	return ret < 0 ? -EIO : 0;
}

static void esp_hosted_mcu_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct esp_hosted_mcu_data *data = dev->data;
	struct wifi_nm_instance *nm = wifi_nm_get_instance("esp_hosted_mcu_nm");
	size_t itf = esp_hosted_mcu_iface_index(dev);

	data->iface[itf] = iface;
	net_eth_set_if_type_wifi(iface);
	net_if_set_link_addr(iface, data->mac_addr[itf], ESP_HOSTED_MCU_MAC_ADDR_LEN,
			     NET_LINK_ETHERNET);
	ethernet_init(iface);
	net_if_dormant_on(iface);

	wifi_nm_register_mgd_type_iface(
		nm, itf == ESP_HOSTED_MCU_IFACE_STA ? WIFI_TYPE_STA : WIFI_TYPE_SAP, iface);

	/*
	 * Route inbound frames for this interface up through the core demux.
	 * The receive callback runs on the core event thread and pushes into
	 * this net_if.
	 */
	esp_hosted_mcu_register_if(itf == ESP_HOSTED_MCU_IFACE_STA ? ESP_HOSTED_MCU_STA_IF
								   : ESP_HOSTED_MCU_AP_IF,
				   esp_hosted_mcu_rx_data, iface);
}

/*
 * Fill the WifiInit request with the coprocessor's default esp_wifi init config.
 * These mirror WIFI_INIT_CONFIG_DEFAULT() on an HE-capable coprocessor; the magic
 * and non-zero buffer counts are what esp_wifi_init validates.
 *
 * These values are a fixed contract with the coprocessor firmware, not host
 * tunables. The magic word and the buffer counts must match what the flashed
 * coprocessor build expects, so esp_wifi_init on the coprocessor accepts the
 * request. Do not change any field, and do not expose these as configuration,
 * without matching the change to the specific coprocessor firmware in use: a
 * mismatch makes WifiInit fail and the Wi-Fi interface never comes up.
 */
static void esp_hosted_mcu_default_init_config(Rpc_Req_WifiInit *init)
{
	wifi_init_config *cfg = &init->cfg;

	init->has_cfg = true;
	cfg->static_rx_buf_num = 10;
	cfg->dynamic_rx_buf_num = 32;
	cfg->tx_buf_type = 1;
	cfg->static_tx_buf_num = 0;
	cfg->dynamic_tx_buf_num = 32;
	cfg->cache_tx_buf_num = 0;
	cfg->csi_enable = 0;
	cfg->ampdu_rx_enable = 1;
	cfg->ampdu_tx_enable = 1;
	cfg->amsdu_tx_enable = 0;
	cfg->nvs_enable = 1;
	cfg->nano_enable = 0;
	cfg->rx_ba_win = 6;
	cfg->wifi_task_core_id = 0;
	cfg->beacon_max_len = 752;
	cfg->mgmt_sbuf_num = 32;
	cfg->feature_caps = 1;
	cfg->sta_disconnected_pm = 1;
	cfg->espnow_max_encrypt_num = 7;
	cfg->magic = ESP_HOSTED_MCU_WIFI_INIT_MAGIC;
	cfg->rx_mgmt_buf_type = 0;
	cfg->rx_mgmt_buf_num = 5;
	cfg->tx_hetb_queue_num = 3;
	cfg->dump_hesigb_enable = 0;
}

/* Register with the core and run the coprocessor esp_wifi init sequence. */
static int esp_hosted_mcu_wifi_init(const struct device *dev)
{
	struct esp_hosted_mcu_data *data = dev->data;
	Rpc req = Rpc_init_zero;
	Rpc resp = Rpc_init_zero;

	k_sem_init(&data->scan_sem, 0, 1);

	/* Route asynchronous coprocessor events (connect/disconnect/scan) here. */
	esp_hosted_mcu_register_rpc_event(esp_hosted_mcu_handle_event, data);

	/* Read the STA and AP MAC addresses the coprocessor reports. */
	for (size_t i = 0; i < ESP_HOSTED_MCU_IFACE_COUNT; i++) {
		req = (Rpc)Rpc_init_zero;
		req.msg_id = RpcId_Req_IfaceMacAddrSetGet;
		req.which_payload = Rpc_req_iface_mac_addr_set_get_tag;
		req.payload.req_iface_mac_addr_set_get.set = false;
		req.payload.req_iface_mac_addr_set_get.type = (i == ESP_HOSTED_MCU_IFACE_STA)
								      ? ESP_HOSTED_MCU_WIFI_IF_STA
								      : ESP_HOSTED_MCU_WIFI_IF_AP;
		if (esp_hosted_mcu_rpc_call(&req, &resp, ESP_HOSTED_MCU_RPC_TIMEOUT) == 0) {
			memcpy(data->mac_addr[i],
			       resp.payload.resp_iface_mac_addr_set_get.mac.bytes,
			       MIN(resp.payload.resp_iface_mac_addr_set_get.mac.size,
				   ESP_HOSTED_MCU_MAC_ADDR_LEN));
		}
	}

	/* Initialise and start the coprocessor Wi-Fi driver. */
	req = (Rpc)Rpc_init_zero;
	req.msg_id = RpcId_Req_WifiInit;
	req.which_payload = Rpc_req_wifi_init_tag;
	esp_hosted_mcu_default_init_config(&req.payload.req_wifi_init);
	if (esp_hosted_mcu_rpc_call(&req, &resp, ESP_HOSTED_MCU_RPC_TIMEOUT)) {
		LOG_ERR("coprocessor WifiInit failed");
		return -EIO;
	}

	/* Put the coprocessor in station mode before starting; scan needs it. */
	req = (Rpc)Rpc_init_zero;
	req.msg_id = RpcId_Req_SetWifiMode;
	req.which_payload = Rpc_req_set_wifi_mode_tag;
	req.payload.req_set_wifi_mode.mode = ESP_HOSTED_MCU_WIFI_MODE_STA;
	if (esp_hosted_mcu_rpc_call(&req, &resp, ESP_HOSTED_MCU_RPC_TIMEOUT)) {
		LOG_ERR("coprocessor SetMode failed");
		return -EIO;
	}

	req = (Rpc)Rpc_init_zero;
	req.msg_id = RpcId_Req_WifiStart;
	req.which_payload = Rpc_req_wifi_start_tag;
	if (esp_hosted_mcu_rpc_call(&req, &resp, ESP_HOSTED_MCU_RPC_TIMEOUT)) {
		LOG_ERR("coprocessor WifiStart failed");
		return -EIO;
	}

	return 0;
}

static const struct wifi_mgmt_ops esp_hosted_mcu_mgmt = {
	.scan = esp_hosted_mcu_scan,
	.connect = esp_hosted_mcu_connect,
	.disconnect = esp_hosted_mcu_disconnect,
#if defined(CONFIG_WIFI_ESP_HOSTED_MCU_AP_STA_MODE)
	.ap_enable = esp_hosted_mcu_ap_enable,
	.ap_disable = esp_hosted_mcu_ap_disable,
#endif
#if defined(CONFIG_NET_STATISTICS_WIFI)
	.get_stats = esp_hosted_mcu_stats,
#endif
	.iface_status = esp_hosted_mcu_iface_status,
	.mode = esp_hosted_mcu_mode,
	.channel = esp_hosted_mcu_channel,
	.set_power_save = esp_hosted_mcu_set_power_save,
	.get_power_save_config = esp_hosted_mcu_get_power_save,
#if defined(CONFIG_WIFI_ESP_HOSTED_MCU_AP_STA_MODE)
	.ap_sta_disconnect = esp_hosted_mcu_ap_sta_disconnect,
#endif
};

static const struct net_wifi_mgmt_offload esp_hosted_mcu_api = {
	.wifi_iface.iface_api.init = esp_hosted_mcu_iface_init,
	.wifi_iface.send = esp_hosted_mcu_send,
	.wifi_mgmt_api = &esp_hosted_mcu_mgmt,
};

DEFINE_WIFI_NM_INSTANCE(esp_hosted_mcu_nm, &esp_hosted_mcu_mgmt);

NET_DEVICE_INIT_INSTANCE(esp_hosted_mcu_sta, "sta_if", 0, esp_hosted_mcu_wifi_init, NULL,
			 &esp_hosted_mcu_data_0, &esp_hosted_mcu_sta_config,
			 CONFIG_WIFI_ESP_HOSTED_MCU_INIT_PRIORITY, &esp_hosted_mcu_api, ETHERNET_L2,
			 NET_L2_GET_CTX_TYPE(ETHERNET_L2), NET_ETH_MTU);

#if defined(CONFIG_WIFI_ESP_HOSTED_MCU_AP_STA_MODE)
NET_DEVICE_INIT_INSTANCE(esp_hosted_mcu_ap, "sap_if", 1, NULL, NULL, &esp_hosted_mcu_data_0,
			 &esp_hosted_mcu_ap_config, CONFIG_WIFI_ESP_HOSTED_MCU_INIT_PRIORITY,
			 &esp_hosted_mcu_api, ETHERNET_L2, NET_L2_GET_CTX_TYPE(ETHERNET_L2),
			 NET_ETH_MTU);
#endif
