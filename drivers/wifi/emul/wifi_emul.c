/*
 * SPDX-FileCopyrightText: 2026 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_wifi_emul

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wifi_emul, CONFIG_WIFI_LOG_LEVEL);

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/wifi/wifi_emul.h>
#include <zephyr/kernel.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/wifi.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/sys/util.h>

enum wifi_emul_state {
	WIFI_EMUL_IDLE,
	WIFI_EMUL_CONNECTING,
	WIFI_EMUL_CONNECTED,
};

struct wifi_emul_config {
	const struct device *eth_dev;
	struct net_eth_mac_config mac_config;
};

struct wifi_emul_data {
	struct net_if *iface;
	struct net_if *eth_iface;
	enum wifi_emul_state state;
	struct k_mutex lock;

	struct wifi_emul_ap aps[CONFIG_WIFI_EMUL_AP_COUNT_MAX];
	bool ap_used[CONFIG_WIFI_EMUL_AP_COUNT_MAX];
	int current_ap;

	struct k_work_delayable scan_work;
	struct k_work_delayable connect_work;
	struct k_work_delayable disconnect_work;

	scan_result_cb_t scan_cb;

	uint8_t req_ssid[WIFI_SSID_MAX_LEN + 1];
	uint8_t req_ssid_length;
	uint8_t req_bssid[WIFI_MAC_ADDR_LEN];
	enum wifi_security_type req_security;
	char req_psk[WIFI_PSK_MAX_LEN + 1];
	uint8_t req_psk_length;

	k_timeout_t scan_delay;
	k_timeout_t connect_delay;
	int forced_conn_status;
	enum wifi_disconn_reason disconn_reason;

	wifi_emul_tx_cb_t tx_cb;
	void *tx_user_data;
};

static struct net_if *wifi_emul_recv_redirect(struct net_if *iface, struct net_pkt *pkt);

/* Bridge the data plane to an Ethernet interface: install the RX redirect so
 * its received frames reach the Wi-Fi interface.
 */
static int wifi_emul_attach_eth_iface(const struct device *dev, struct net_if *eth_iface)
{
	struct wifi_emul_data *data = dev->data;
	int ret;

	if (eth_iface == data->iface ||
	    net_if_l2(eth_iface) != &NET_L2_GET_NAME(ETHERNET) ||
	    net_eth_type_is_wifi(eth_iface)) {
		return -EINVAL;
	}

	ret = net_eth_recv_redirect_set(eth_iface, wifi_emul_recv_redirect);
	if (ret < 0) {
		LOG_ERR("Failed to install RX redirect (%d)", ret);
		return ret;
	}

	data->eth_iface = eth_iface;

	LOG_DBG("Bound to iface %d", net_if_get_by_iface(eth_iface));

	return 0;
}

/* Resolve the data plane from the devicetree "ethernet" phandle, if any.
 * Best effort and retried, as the underlying interface may not be initialized
 * yet when this driver initializes. Returns -ENODEV in self-contained mode
 * (no phandle), which is not a failure.
 */
static int wifi_emul_bind(const struct device *dev)
{
	const struct wifi_emul_config *config = dev->config;
	struct wifi_emul_data *data = dev->data;
	struct net_if *eth_iface;

	if (data->eth_iface != NULL) {
		return 0;
	}

	if (config->eth_dev == NULL) {
		return -ENODEV;
	}

	if (!device_is_ready(config->eth_dev)) {
		return -ENODEV;
	}

	eth_iface = net_if_lookup_by_dev(config->eth_dev);
	if (eth_iface == NULL) {
		return -ENODEV;
	}

	return wifi_emul_attach_eth_iface(dev, eth_iface);
}

static int wifi_emul_find_ap(struct wifi_emul_data *data, const uint8_t *bssid)
{
	ARRAY_FOR_EACH(data->aps, i) {
		if (data->ap_used[i] &&
		    memcmp(data->aps[i].bssid, bssid, WIFI_MAC_ADDR_LEN) == 0) {
			return i;
		}
	}

	return -ENOENT;
}

static void wifi_emul_scan_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct wifi_emul_data *data = CONTAINER_OF(dwork, struct wifi_emul_data, scan_work);
	struct wifi_scan_result entry;
	scan_result_cb_t cb;

	k_mutex_lock(&data->lock, K_FOREVER);

	ARRAY_FOR_EACH(data->aps, i) {
		const struct wifi_emul_ap *ap = &data->aps[i];

		if (!data->ap_used[i] || ap->hidden) {
			continue;
		}

		entry = (struct wifi_scan_result){
			.ssid_length = ap->ssid_length,
			.band = ap->band,
			.channel = ap->channel,
			.security = ap->security,
			.mfp = WIFI_MFP_DISABLE,
			.rssi = ap->rssi,
			.mac_length = WIFI_MAC_ADDR_LEN,
		};
		memcpy(entry.ssid, ap->ssid, ap->ssid_length);
		memcpy(entry.mac, ap->bssid, WIFI_MAC_ADDR_LEN);

		k_mutex_unlock(&data->lock);
		data->scan_cb(data->iface, 0, &entry);
		k_mutex_lock(&data->lock, K_FOREVER);
	}

	cb = data->scan_cb;
	data->scan_cb = NULL;

	k_mutex_unlock(&data->lock);

	/* Report end of scan */
	cb(data->iface, 0, NULL);
}

static void wifi_emul_connect_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct wifi_emul_data *data = CONTAINER_OF(dwork, struct wifi_emul_data, connect_work);
	const struct device *dev = net_if_get_device(data->iface);
	const struct wifi_emul_config *config = dev->config;
	int status = WIFI_STATUS_CONN_AP_NOT_FOUND;
	static const uint8_t no_bssid[WIFI_MAC_ADDR_LEN];
	int ap_idx = -1;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (data->state != WIFI_EMUL_CONNECTING) {
		/* The connection request got canceled */
		k_mutex_unlock(&data->lock);
		return;
	}

	/* Only failure statuses are forced; a forced success would have no
	 * matching access point, so fall through to normal evaluation.
	 */
	if (data->forced_conn_status > WIFI_STATUS_CONN_SUCCESS) {
		status = data->forced_conn_status;
		goto done;
	}

	ARRAY_FOR_EACH(data->aps, i) {
		const struct wifi_emul_ap *ap = &data->aps[i];

		if (!data->ap_used[i] || ap->ssid_length != data->req_ssid_length ||
		    memcmp(ap->ssid, data->req_ssid, ap->ssid_length) != 0) {
			continue;
		}

		if (memcmp(data->req_bssid, no_bssid, WIFI_MAC_ADDR_LEN) != 0 &&
		    memcmp(data->req_bssid, ap->bssid, WIFI_MAC_ADDR_LEN) != 0) {
			continue;
		}

		if (ap->security != data->req_security ||
		    (ap->security != WIFI_SECURITY_TYPE_NONE &&
		     (strlen(ap->psk) != data->req_psk_length ||
		      memcmp(ap->psk, data->req_psk, data->req_psk_length) != 0))) {
			status = WIFI_STATUS_CONN_WRONG_PASSWORD;
			continue;
		}

		status = WIFI_STATUS_CONN_SUCCESS;
		ap_idx = i;
		break;
	}

	/* When a devicetree data plane is configured it must be available;
	 * otherwise the driver operates self-contained.
	 */
	if (status == WIFI_STATUS_CONN_SUCCESS && config->eth_dev != NULL &&
	    wifi_emul_bind(dev) < 0) {
		LOG_ERR("Configured Ethernet data plane unavailable");
		status = WIFI_STATUS_CONN_FAIL;
	}

done:
	if (status == WIFI_STATUS_CONN_SUCCESS) {
		data->state = WIFI_EMUL_CONNECTED;
		data->current_ap = ap_idx;
		net_if_dormant_off(data->iface);
	} else {
		data->state = WIFI_EMUL_IDLE;
	}

	k_mutex_unlock(&data->lock);

	wifi_mgmt_raise_connect_result_event(data->iface, status);
}

static void wifi_emul_disconnect_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct wifi_emul_data *data = CONTAINER_OF(dwork, struct wifi_emul_data, disconnect_work);
	struct wifi_status status;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (data->state != WIFI_EMUL_CONNECTED) {
		k_mutex_unlock(&data->lock);
		return;
	}

	data->state = WIFI_EMUL_IDLE;
	data->current_ap = -1;
	status.disconn_reason = data->disconn_reason;
	net_if_dormant_on(data->iface);

	k_mutex_unlock(&data->lock);

	wifi_mgmt_raise_disconnect_result_event(data->iface, status.status);
}

static int wifi_emul_scan(const struct device *dev, struct net_if *iface,
			  struct wifi_scan_params *params, scan_result_cb_t cb)
{
	struct wifi_emul_data *data = dev->data;
	int ret = 0;

	ARG_UNUSED(iface);
	ARG_UNUSED(params);

	k_mutex_lock(&data->lock, K_FOREVER);

	if (data->scan_cb != NULL) {
		ret = -EBUSY;
	} else {
		(void)wifi_emul_bind(dev);
		data->scan_cb = cb;
		k_work_schedule(&data->scan_work, data->scan_delay);
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

static int wifi_emul_connect(const struct device *dev, struct net_if *iface,
			     struct wifi_connect_req_params *params)
{
	struct wifi_emul_data *data = dev->data;
	const uint8_t *psk = params->psk;
	uint8_t psk_length = params->psk_length;
	int ret = 0;

	ARG_UNUSED(iface);

	if (params->ssid == NULL || params->ssid_length == 0 ||
	    params->ssid_length > WIFI_SSID_MAX_LEN) {
		return -EINVAL;
	}

	if (psk == NULL && params->sae_password != NULL) {
		psk = params->sae_password;
		psk_length = params->sae_password_length;
	}

	if (psk_length > WIFI_PSK_MAX_LEN) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	switch (data->state) {
	case WIFI_EMUL_CONNECTING:
		ret = -EINPROGRESS;
		break;
	case WIFI_EMUL_CONNECTED:
		ret = -EALREADY;
		break;
	case WIFI_EMUL_IDLE:
		data->state = WIFI_EMUL_CONNECTING;
		data->req_ssid_length = params->ssid_length;
		memcpy(data->req_ssid, params->ssid, params->ssid_length);
		memcpy(data->req_bssid, params->bssid, WIFI_MAC_ADDR_LEN);
		data->req_security = params->security;
		data->req_psk_length = psk_length;
		if (psk_length > 0) {
			memcpy(data->req_psk, psk, psk_length);
		}
		k_work_schedule(&data->connect_work, data->connect_delay);
		break;
	default:
		CODE_UNREACHABLE;
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

static int wifi_emul_disconnect(const struct device *dev, struct net_if *iface)
{
	struct wifi_emul_data *data = dev->data;
	int ret = 0;

	ARG_UNUSED(iface);

	k_mutex_lock(&data->lock, K_FOREVER);

	switch (data->state) {
	case WIFI_EMUL_IDLE:
		ret = -EALREADY;
		break;
	case WIFI_EMUL_CONNECTING:
		k_work_cancel_delayable(&data->connect_work);
		data->state = WIFI_EMUL_IDLE;
		k_mutex_unlock(&data->lock);
		wifi_mgmt_raise_connect_result_event(data->iface, WIFI_STATUS_CONN_FAIL);
		return 0;
	case WIFI_EMUL_CONNECTED:
		data->disconn_reason = WIFI_REASON_DISCONN_USER_REQUEST;
		k_work_schedule(&data->disconnect_work, K_NO_WAIT);
		break;
	default:
		CODE_UNREACHABLE;
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

static int wifi_emul_iface_status(const struct device *dev, struct net_if *iface,
				  struct wifi_iface_status *status)
{
	struct wifi_emul_data *data = dev->data;

	ARG_UNUSED(iface);

	k_mutex_lock(&data->lock, K_FOREVER);

	memset(status, 0, sizeof(*status));
	status->iface_mode = WIFI_MODE_INFRA;

	switch (data->state) {
	case WIFI_EMUL_IDLE:
		status->state = data->scan_cb != NULL ? WIFI_STATE_SCANNING
						      : WIFI_STATE_DISCONNECTED;
		break;
	case WIFI_EMUL_CONNECTING:
		status->state = WIFI_STATE_ASSOCIATING;
		break;
	case WIFI_EMUL_CONNECTED: {
		const struct wifi_emul_ap *ap = &data->aps[data->current_ap];

		status->state = WIFI_STATE_COMPLETED;
		status->ssid_len = ap->ssid_length;
		memcpy(status->ssid, ap->ssid, ap->ssid_length);
		memcpy(status->bssid, ap->bssid, WIFI_MAC_ADDR_LEN);
		status->band = ap->band;
		status->channel = ap->channel;
		status->link_mode = WIFI_4;
		status->security = ap->security;
		status->mfp = WIFI_MFP_DISABLE;
		status->rssi = ap->rssi;
		status->dtim_period = 1;
		status->beacon_interval = 100;
		break;
	}
	default:
		CODE_UNREACHABLE;
	}

	k_mutex_unlock(&data->lock);

	return 0;
}

static int wifi_emul_send(const struct device *dev, struct net_pkt *pkt)
{
	struct wifi_emul_data *data = dev->data;

	if (data->state != WIFI_EMUL_CONNECTED) {
		return -ENETDOWN;
	}

	if (data->eth_iface != NULL) {
		const struct device *eth_dev = net_if_get_device(data->eth_iface);
		const struct ethernet_api *api = eth_dev->api;

		return api->send(eth_dev, pkt);
	}

	/* Self-contained: hand the frame to the transmit callback, if any */
	if (data->tx_cb != NULL) {
		data->tx_cb(dev, pkt, data->tx_user_data);
	}

	return 0;
}

static void wifi_emul_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	const struct wifi_emul_config *config = dev->config;
	struct wifi_emul_data *data = dev->data;
	uint8_t mac_addr[NET_ETH_ADDR_LEN] = {0};

	data->iface = iface;

	net_eth_set_if_type_wifi(iface);

	/* Use the device tree MAC address configuration when provided,
	 * otherwise generate a random one. The underlying Ethernet MAC is
	 * not used: it is not guaranteed to be available at this point, and
	 * the data path works with an independent MAC.
	 */
	if (net_eth_mac_load(&config->mac_config, mac_addr) != 0) {
		const struct net_eth_mac_config rnd = {.type = NET_ETH_MAC_RANDOM};

		(void)net_eth_mac_load(&rnd, mac_addr);
	}

	net_if_set_link_addr(iface, mac_addr, sizeof(mac_addr), NET_LINK_ETHERNET);

	ethernet_init(iface);
	net_if_dormant_on(iface);

	/* Best effort, retried on scan/connect requests */
	(void)wifi_emul_bind(dev);
}

int wifi_emul_ap_add(const struct device *dev, const struct wifi_emul_ap *ap)
{
	struct wifi_emul_data *data = dev->data;
	int ret = -ENOMEM;

	if (ap->ssid_length == 0 || ap->ssid_length > WIFI_SSID_MAX_LEN) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	if (wifi_emul_find_ap(data, ap->bssid) >= 0) {
		ret = -EEXIST;
		goto out;
	}

	ARRAY_FOR_EACH(data->aps, i) {
		if (data->ap_used[i]) {
			continue;
		}

		data->aps[i] = *ap;
		/* Guarantee NUL-termination so the connect path can use
		 * strlen() on the stored PSK safely.
		 */
		data->aps[i].psk[sizeof(data->aps[i].psk) - 1] = '\0';
		data->ap_used[i] = true;
		ret = 0;
		break;
	}

out:
	k_mutex_unlock(&data->lock);

	return ret;
}

int wifi_emul_ap_remove(const struct device *dev, const uint8_t bssid[WIFI_MAC_ADDR_LEN])
{
	struct wifi_emul_data *data = dev->data;
	int idx;

	k_mutex_lock(&data->lock, K_FOREVER);

	idx = wifi_emul_find_ap(data, bssid);
	if (idx >= 0) {
		data->ap_used[idx] = false;
	}

	k_mutex_unlock(&data->lock);

	return MIN(idx, 0);
}

void wifi_emul_ap_clear(const struct device *dev)
{
	struct wifi_emul_data *data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);
	memset(data->ap_used, 0, sizeof(data->ap_used));
	k_mutex_unlock(&data->lock);
}

int wifi_emul_ap_update_rssi(const struct device *dev, const uint8_t bssid[WIFI_MAC_ADDR_LEN],
			     int8_t rssi)
{
	struct wifi_emul_data *data = dev->data;
	int idx;

	k_mutex_lock(&data->lock, K_FOREVER);

	idx = wifi_emul_find_ap(data, bssid);
	if (idx >= 0) {
		data->aps[idx].rssi = rssi;
	}

	k_mutex_unlock(&data->lock);

	return MIN(idx, 0);
}

void wifi_emul_ap_foreach(const struct device *dev, wifi_emul_ap_cb_t cb, void *user_data)
{
	struct wifi_emul_data *data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);

	ARRAY_FOR_EACH(data->aps, i) {
		if (data->ap_used[i]) {
			cb(&data->aps[i], user_data);
		}
	}

	k_mutex_unlock(&data->lock);
}

void wifi_emul_set_scan_delay(const struct device *dev, k_timeout_t delay)
{
	struct wifi_emul_data *data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);
	data->scan_delay = delay;
	k_mutex_unlock(&data->lock);
}

void wifi_emul_set_connect_delay(const struct device *dev, k_timeout_t delay)
{
	struct wifi_emul_data *data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);
	data->connect_delay = delay;
	k_mutex_unlock(&data->lock);
}

void wifi_emul_force_connect_status(const struct device *dev, int conn_status)
{
	struct wifi_emul_data *data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);
	data->forced_conn_status = conn_status;
	k_mutex_unlock(&data->lock);
}

int wifi_emul_trigger_disconnect(const struct device *dev, int reason)
{
	struct wifi_emul_data *data = dev->data;
	int ret = 0;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (data->state != WIFI_EMUL_CONNECTED) {
		ret = -ENOTCONN;
	} else {
		data->disconn_reason = reason;
		k_work_schedule(&data->disconnect_work, K_NO_WAIT);
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

bool wifi_emul_is_connected(const struct device *dev)
{
	struct wifi_emul_data *data = dev->data;

	return data->state == WIFI_EMUL_CONNECTED;
}

int wifi_emul_get_current_ap(const struct device *dev, struct wifi_emul_ap *ap)
{
	struct wifi_emul_data *data = dev->data;
	int ret = 0;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (data->state != WIFI_EMUL_CONNECTED) {
		ret = -ENOTCONN;
	} else {
		*ap = data->aps[data->current_ap];
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

void wifi_emul_set_tx_cb(const struct device *dev, wifi_emul_tx_cb_t cb, void *user_data)
{
	struct wifi_emul_data *data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);
	data->tx_cb = cb;
	data->tx_user_data = user_data;
	k_mutex_unlock(&data->lock);
}

int wifi_emul_rx_inject(const struct device *dev, const uint8_t *frame, size_t len)
{
	struct wifi_emul_data *data = dev->data;
	struct net_pkt *pkt;
	int ret;

	if (data->state != WIFI_EMUL_CONNECTED) {
		return -ENETDOWN;
	}

	pkt = net_pkt_rx_alloc_with_buffer(data->iface, len, NET_AF_UNSPEC, 0, K_NO_WAIT);
	if (pkt == NULL) {
		return -ENOMEM;
	}

	ret = net_pkt_write(pkt, frame, len);
	if (ret < 0) {
		net_pkt_unref(pkt);
		return ret;
	}

	ret = net_recv_data(data->iface, pkt);
	if (ret < 0) {
		net_pkt_unref(pkt);
	}

	return ret;
}

int wifi_emul_set_eth_iface(const struct device *dev, struct net_if *eth_iface)
{
	struct wifi_emul_data *data = dev->data;
	int ret = 0;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (eth_iface == NULL) {
		/* Detach the data plane and revert to self-contained mode */
		if (data->eth_iface != NULL) {
			(void)net_eth_recv_redirect_set(data->eth_iface, NULL);
			data->eth_iface = NULL;
		}
	} else {
		ret = wifi_emul_attach_eth_iface(dev, eth_iface);
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

static int wifi_emul_dev_init(const struct device *dev)
{
	struct wifi_emul_data *data = dev->data;

	k_mutex_init(&data->lock);
	k_work_init_delayable(&data->scan_work, wifi_emul_scan_work_handler);
	k_work_init_delayable(&data->connect_work, wifi_emul_connect_work_handler);
	k_work_init_delayable(&data->disconnect_work, wifi_emul_disconnect_work_handler);

	data->current_ap = -1;
	data->forced_conn_status = -1;
	data->scan_delay = K_MSEC(CONFIG_WIFI_EMUL_SCAN_DELAY_MS);
	data->connect_delay = K_MSEC(CONFIG_WIFI_EMUL_CONNECT_DELAY_MS);

	return 0;
}

static const struct wifi_mgmt_ops wifi_emul_mgmt_ops = {
	.scan = wifi_emul_scan,
	.connect = wifi_emul_connect,
	.disconnect = wifi_emul_disconnect,
	.iface_status = wifi_emul_iface_status,
};

static const struct net_wifi_mgmt_offload wifi_emul_api = {
	.wifi_iface.iface_api.init = wifi_emul_iface_init,
	.wifi_iface.send = wifi_emul_send,
	.wifi_mgmt_api = &wifi_emul_mgmt_ops,
};

#define WIFI_EMUL_DEFINE(inst)                                                                     \
	static const struct wifi_emul_config wifi_emul_config_##inst = {                           \
		.eth_dev = DEVICE_DT_GET_OR_NULL(DT_INST_PHANDLE(inst, ethernet)),                 \
		.mac_config = NET_ETH_MAC_DT_INST_CONFIG_INIT(inst),                               \
	};                                                                                         \
                                                                                                   \
	static struct wifi_emul_data wifi_emul_data_##inst;                                        \
                                                                                                   \
	NET_DEVICE_DT_INST_DEFINE(inst, wifi_emul_dev_init, NULL, &wifi_emul_data_##inst,          \
				  &wifi_emul_config_##inst, CONFIG_WIFI_INIT_PRIORITY,             \
				  &wifi_emul_api, ETHERNET_L2, NET_L2_GET_CTX_TYPE(ETHERNET_L2),   \
				  NET_ETH_MTU);

DT_INST_FOREACH_STATUS_OKAY(WIFI_EMUL_DEFINE)

#define WIFI_EMUL_DATA_REF(inst) &wifi_emul_data_##inst,

static struct wifi_emul_data *const wifi_emul_instances[] = {
	DT_INST_FOREACH_STATUS_OKAY(WIFI_EMUL_DATA_REF)};

static struct net_if *wifi_emul_recv_redirect(struct net_if *iface, struct net_pkt *pkt)
{
	ARG_UNUSED(pkt);

	ARRAY_FOR_EACH(wifi_emul_instances, i) {
		struct wifi_emul_data *data = wifi_emul_instances[i];

		if (data->eth_iface != iface) {
			continue;
		}

		if (data->state != WIFI_EMUL_CONNECTED) {
			/* Drop, the station is not connected */
			return NULL;
		}

		return data->iface;
	}

	return iface;
}
