/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_wifi_hwsim

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/wifi_nm.h>
#include <zephyr/net/wifi_utils.h>
#include <zephyr/random/random.h>
#include <string.h>

#include "wifi_hwsim.h"

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT
#include <drivers/driver_zephyr.h>
extern const struct zep_wpa_supp_dev_ops hwsim_wpa_supp_ops;
#ifdef CONFIG_WIFI_HWSIM_NAN
extern const struct zep_wpa_supp_dev_ops hwsim_nan_wpa_supp_ops;
#ifdef CONFIG_WIFI_HWSIM_NAN_SHARED_IFACE
extern const struct zep_wpa_supp_dev_ops hwsim_shared_wpa_supp_ops;
#endif
#endif
#endif

LOG_MODULE_REGISTER(wifi_hwsim, CONFIG_WIFI_HWSIM_LOG_LEVEL);

/* Weak stub for mgmt delivery to wpa_supplicant; overridden in wifi_hwsim_supp.c */
__weak void hwsim_supp_mgmt_rx(struct hwsim_radio *radio, const uint8_t *data,
			       unsigned int len, unsigned int freq_mhz, int rssi_dbm)
{
	ARG_UNUSED(radio);
	ARG_UNUSED(data);
	ARG_UNUSED(len);
	ARG_UNUSED(freq_mhz);
	ARG_UNUSED(rssi_dbm);
}

static int hwsim_dev_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

struct net_if *hwsim_get_secondary_iface(const struct device *dev)
{
	const struct hwsim_config *cfg = (const struct hwsim_config *)dev->config;

	return cfg->secondary_iface;
}

static void hwsim_rx_thread(void *p1, void *p2, void *p3);

static void hwsim_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct hwsim_radio *radio = (struct hwsim_radio *)dev->data;
	const struct hwsim_config *cfg = (const struct hwsim_config *)dev->config;
	uint8_t mac[HWSIM_ETH_ALEN];

	/*
	 * Load the MAC from the devicetree (local-mac-address /
	 * zephyr,random-mac-address). When no MAC is configured fall back to a
	 * random locally-administered address so each radio still gets a unique
	 * link address.
	 */
	if (net_eth_mac_load(&cfg->mac, mac) < 0) {
		sys_rand_get(mac, sizeof(mac));
		mac[0] &= ~0x01U; /* unicast */
		mac[0] |= 0x02U;  /* locally administered */
	}

	if (cfg->secondary_iface != NULL && iface == cfg->secondary_iface) {
		/*
		 * Shared-device NAN role: distinct link address from the primary
		 * role, no data-plane resources (mirrors the nan_management
		 * shortcut below). Does not own radio->iface -- the primary role
		 * keeps the rx thread/medium registration/mgmt event target.
		 */
		mac[HWSIM_ETH_ALEN - 1]++;
		net_if_set_link_addr(iface, mac, sizeof(mac), NET_LINK_ETHERNET);
		net_if_carrier_on(iface);
		((struct ethernet_context *)net_if_l2_data(iface))->eth_if_type =
			L2_ETH_IF_TYPE_WIFI;
		ethernet_init(iface);
		return;
	}

	net_if_set_link_addr(iface, mac, sizeof(mac), NET_LINK_ETHERNET);

	radio->iface = iface;
	if (cfg->nan_management) {
		/* Lifecycle-only NAN interfaces do not consume frame-relay resources. */
		radio->idx = cfg->radio_index;
		net_if_carrier_on(iface);
		((struct ethernet_context *)net_if_l2_data(iface))->eth_if_type =
			L2_ETH_IF_TYPE_WIFI;
		ethernet_init(iface);
		return;
	}

	if (hwsim_medium_register(radio, cfg->radio_index) != 0) {
		LOG_ERR("hwsim medium register failed");
		return;
	}

	/* This is a virtual radio always connected to the shared medium. */
	net_if_carrier_on(iface);

	k_thread_create(&radio->rx_thread, radio->rx_stack,
			K_KERNEL_STACK_SIZEOF(radio->rx_stack),
			hwsim_rx_thread, radio, NULL, NULL,
			K_PRIO_PREEMPT(8), 0, K_NO_WAIT);

	((struct ethernet_context *)net_if_l2_data(iface))->eth_if_type =
		L2_ETH_IF_TYPE_WIFI;
	ethernet_init(iface);
}

static int hwsim_send(const struct device *dev, struct net_pkt *pkt)
{
	struct hwsim_radio *radio = (struct hwsim_radio *)dev->data;
	struct hwsim_frame *scratch;
	size_t len;
	int ret;

	len = net_pkt_get_len(pkt);
	if (len > HWSIM_FRAME_MTU) {
		return -EMSGSIZE;
	}
	/* Linearize into a slab entry rather than a large (HWSIM_FRAME_MTU)
	 * stack buffer, since this runs on the TX thread's limited stack.
	 */
	if (k_mem_slab_alloc(&hwsim_frame_slab, (void **)&scratch, K_NO_WAIT) != 0) {
		return -ENOMEM;
	}
	net_pkt_cursor_init(pkt);
	if (net_pkt_read(pkt, scratch->data, len) < 0) {
		k_mem_slab_free(&hwsim_frame_slab, (void *)scratch);
		return -EIO;
	}
	LOG_DBG("TX radio %u len %zu", radio->idx, len);
	ret = hwsim_medium_xmit(radio->idx, scratch->data, (uint16_t)len, false);
	k_mem_slab_free(&hwsim_frame_slab, (void *)scratch);
	return ret;
}

static int hwsim_mgmt_scan(const struct device *dev, struct net_if *iface,
			   struct wifi_scan_params *params, scan_result_cb_t cb)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(params);

	/* No over-the-air scan: report every other radio currently in AP
	 * mode as a visible BSS, regardless of channel.
	 */
	for (size_t i = 0; i < HWSIM_MAX_RADIOS; i++) {
		struct hwsim_radio *r = hwsim_medium_get_radio((uint8_t)i);
		struct wifi_scan_result res = { 0 };

		if (r == NULL || !r->ap_mode) {
			continue;
		}

		memcpy(res.ssid, r->ssid, r->ssid_len);
		res.ssid[r->ssid_len] = '\0';
		res.ssid_length = r->ssid_len;
		res.channel = r->channel;
		res.rssi = r->tx_power_dbm;
		res.security = (enum wifi_security_type)r->security;
		/* The simulated radio is 2.4 GHz by construction, see the
		 * channel to frequency mapping in hwsim_mgmt_ap_enable().
		 */
		res.band = WIFI_FREQ_BAND_2_4_GHZ;
		memcpy(res.mac, r->bssid, 6);
		res.mac_length = 6;

		cb(iface, 0, &res);
	}

	cb(iface, 0, NULL);
	return 0;
}

static int hwsim_mgmt_connect(const struct device *dev, struct net_if *iface,
			      struct wifi_connect_req_params *params)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(iface);
	ARG_UNUSED(params);
	/* No frame generation: STA/AP are driven by wpa_supplicant/hostapd;
	 * this driver only relays frames between ifaces.
	 */
	return -ENOTSUP;
}

static int hwsim_mgmt_disconnect(const struct device *dev, struct net_if *iface)
{
	struct hwsim_radio *radio = (struct hwsim_radio *)dev->data;

	ARG_UNUSED(iface);

	radio->associated = false;
	memset(radio->ap_bssid, 0, sizeof(radio->ap_bssid));
	wifi_mgmt_raise_disconnect_result_event(radio->iface, 0);
	return 0;
}

static int hwsim_mgmt_ap_enable(const struct device *dev, struct net_if *iface,
				struct wifi_connect_req_params *params)
{
	struct hwsim_radio *radio = (struct hwsim_radio *)dev->data;
	struct net_linkaddr *lla = net_if_get_link_addr(iface);

	if (params == NULL || params->ssid == NULL || lla == NULL ||
	    params->ssid_length == 0 || params->ssid_length > WIFI_SSID_MAX_LEN) {
		wifi_mgmt_raise_ap_enable_result_event(radio->iface,
						       WIFI_STATUS_AP_FAIL);
		return -EINVAL;
	}

	memcpy(radio->bssid, lla->addr, HWSIM_ETH_ALEN);
	radio->channel = params->channel;
	radio->freq_mhz = wifi_utils_chan_to_freq(WIFI_FREQ_BAND_2_4_GHZ, params->channel);
	radio->ssid_len = params->ssid_length;
	memcpy(radio->ssid, params->ssid, params->ssid_length);
	radio->security = (uint8_t)params->security;
	radio->ap_mode = true;
	wifi_mgmt_raise_ap_enable_result_event(radio->iface, WIFI_STATUS_AP_SUCCESS);
	return 0;
}

static int hwsim_mgmt_ap_disable(const struct device *dev, struct net_if *iface)
{
	struct hwsim_radio *radio = (struct hwsim_radio *)dev->data;

	ARG_UNUSED(iface);

	radio->ap_mode = false;
	wifi_mgmt_raise_ap_disable_result_event(radio->iface, WIFI_STATUS_AP_SUCCESS);
	return 0;
}

static int hwsim_mgmt_iface_status(const struct device *dev, struct net_if *iface,
				   struct wifi_iface_status *status)
{
	struct hwsim_radio *radio = (struct hwsim_radio *)dev->data;

	ARG_UNUSED(iface);

	memset(status, 0, sizeof(*status));
	status->channel = radio->channel;
	/* Fixed rather than derived from the channel: the simulated radio only
	 * ever operates in the 2.4 GHz band.
	 */
	status->band = WIFI_FREQ_BAND_2_4_GHZ;
	memcpy(status->bssid, radio->ap_mode ? radio->bssid : radio->ap_bssid, 6);
	memcpy(status->ssid, radio->ssid, radio->ssid_len);
	status->ssid_len = radio->ssid_len;
	status->iface_mode = radio->ap_mode ? WIFI_MODE_AP : WIFI_MODE_INFRA;
	if (radio->ap_mode) {
		status->state = WIFI_STATE_COMPLETED; /* AP enabled and beaconing */
	} else {
		status->state = radio->associated ? WIFI_STATE_COMPLETED :
				WIFI_STATE_DISCONNECTED;
	}
	return 0;
}

static void hwsim_rx_thread(void *p1, void *p2, void *p3)
{
	struct hwsim_radio *radio = p1;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		struct hwsim_frame *f = k_fifo_get(&radio->rx_queue, K_FOREVER);
		struct net_pkt *pkt;

		if (f->len == 0) {
			k_mem_slab_free(&hwsim_frame_slab, f);
			continue;
		}

		/*
		 * Management frames (802.11) go to wpa_supplicant; data frames
		 * (802.3) are injected into this iface's network RX path.
		 */
		if (f->is_mgmt) {
			if (radio->supp_ctx != NULL) {
				hwsim_supp_mgmt_rx(radio, f->data, f->len,
						   f->freq_mhz, f->signal_dbm);
			}
			k_mem_slab_free(&hwsim_frame_slab, f);
			continue;
		}

		pkt = net_pkt_rx_alloc_with_buffer(radio->iface, f->len, AF_UNSPEC,
						   0, K_NO_WAIT);
		LOG_DBG("RX radio %u len %u", radio->idx, f->len);
		if (pkt == NULL) {
			LOG_WRN("RX radio %u: pkt alloc failed", radio->idx);
			k_mem_slab_free(&hwsim_frame_slab, f);
			continue;
		}

		net_pkt_set_iface(pkt, radio->iface);
		/*
		 * All radios share a single IP stack, so a peer's source address
		 * is also a local address. Mark the frame as loopback to bypass
		 * the IP input "source address is mine" anti-spoof drop; without
		 * this, inter-radio data frames would be discarded.
		 */
		net_pkt_set_loopback(pkt, true);
		if (net_pkt_write(pkt, f->data, f->len) < 0) {
			LOG_WRN("RX radio %u: pkt write failed", radio->idx);
			net_pkt_unref(pkt);
			k_mem_slab_free(&hwsim_frame_slab, f);
			continue;
		}

		if (net_recv_data(radio->iface, pkt) < 0) {
			LOG_WRN("RX radio %u: net_recv_data failed", radio->idx);
			net_pkt_unref(pkt);
		}
		k_yield();
		k_mem_slab_free(&hwsim_frame_slab, f);
	}
}

static const struct wifi_mgmt_ops hwsim_mgmt_ops = {
	.scan       = hwsim_mgmt_scan,
	.connect    = hwsim_mgmt_connect,
	.disconnect = hwsim_mgmt_disconnect,
	.ap_enable  = hwsim_mgmt_ap_enable,
	.ap_disable = hwsim_mgmt_ap_disable,
	.iface_status = hwsim_mgmt_iface_status,
};

static DEVICE_API(wifi_native, hwsim_api) = {
	.wifi_iface = {
		.iface_api.init = hwsim_iface_init,
		.send = hwsim_send,
	},
	.wifi_mgmt_api = &hwsim_mgmt_ops,
#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT
	.wifi_drv_ops = &hwsim_wpa_supp_ops,
#endif
};

#ifdef CONFIG_WIFI_HWSIM_NAN
static uint32_t hwsim_nan_iface_caps(const struct device *dev, struct net_if *iface)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(iface);

	return BIT(WIFI_TYPE_NAN);
}

static int hwsim_nan_send(const struct device *dev, struct net_pkt *pkt)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pkt);

	return -ENOTSUP;
}

static const struct wifi_mgmt_ops hwsim_nan_mgmt_ops = {
	.get_iface_caps = hwsim_nan_iface_caps,
};

static DEVICE_API(wifi_native, hwsim_nan_api) = {
	.wifi_iface = {
		.iface_api.init = hwsim_iface_init,
		.send = hwsim_nan_send,
	},
	.wifi_mgmt_api = &hwsim_nan_mgmt_ops,
	.wifi_drv_ops = &hwsim_nan_wpa_supp_ops,
};
#endif /* CONFIG_WIFI_HWSIM_NAN */

#ifdef CONFIG_WIFI_HWSIM_NAN_SHARED_IFACE
static uint32_t hwsim_shared_iface_caps(const struct device *dev, struct net_if *iface)
{
	const struct hwsim_config *cfg = (const struct hwsim_config *)dev->config;

	if (cfg->secondary_iface != NULL && iface == cfg->secondary_iface) {
		return BIT(WIFI_TYPE_NAN);
	}
	return BIT(WIFI_TYPE_STA);
}

static int hwsim_shared_send(const struct device *dev, struct net_pkt *pkt)
{
	const struct hwsim_config *cfg = (const struct hwsim_config *)dev->config;

	if (cfg->secondary_iface != NULL && net_pkt_iface(pkt) == cfg->secondary_iface) {
		/* NAN role: no data-plane resources, see hwsim_iface_init(). */
		return -ENOTSUP;
	}
	return hwsim_send(dev, pkt);
}

static const struct wifi_mgmt_ops hwsim_shared_mgmt_ops = {
	.scan           = hwsim_mgmt_scan,
	.connect        = hwsim_mgmt_connect,
	.disconnect     = hwsim_mgmt_disconnect,
	.ap_enable      = hwsim_mgmt_ap_enable,
	.ap_disable     = hwsim_mgmt_ap_disable,
	.iface_status   = hwsim_mgmt_iface_status,
	.get_iface_caps = hwsim_shared_iface_caps,
};

static DEVICE_API(wifi_native, hwsim_shared_api) = {
	.wifi_iface = {
		.iface_api.init = hwsim_iface_init,
		.send = hwsim_shared_send,
	},
	.wifi_mgmt_api = &hwsim_shared_mgmt_ops,
#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT
	.wifi_drv_ops = &hwsim_shared_wpa_supp_ops,
#endif
};
#endif /* CONFIG_WIFI_HWSIM_NAN_SHARED_IFACE */

#define HWSIM_INIT(n)							\
	BUILD_ASSERT(!DT_INST_PROP(n, nan_management) ||			\
		     IS_ENABLED(CONFIG_WIFI_HWSIM_NAN),			\
		     "nan-management requires CONFIG_WIFI_HWSIM_NAN");	\
	static struct hwsim_radio hwsim_radio_##n = { };		\
	IF_ENABLED(CONFIG_WIFI_HWSIM_NAN_SHARED_IFACE, (			\
		static struct hwsim_radio hwsim_shared_radio_##n = {	\
			/* +1 to match the separate-device topology's dedicated NAN node */ \
			.idx = DT_INST_PROP(n, radio_index) + 1,	\
			.is_secondary = true,				\
		};							\
		NET_IF_DT_INST_DECLARE(n, 1);				\
	))								\
	static const struct hwsim_config hwsim_cfg_##n = {		\
		.radio_index = DT_INST_PROP(n, radio_index),		\
		.mac = NET_ETH_MAC_DT_INST_CONFIG_INIT(n),		\
		.nan_management = DT_INST_PROP(n, nan_management),	\
		IF_ENABLED(CONFIG_WIFI_HWSIM_NAN_SHARED_IFACE, (	\
			.secondary_iface = NET_IF_DT_INST_GET(n, 1),	\
			.secondary_radio = &hwsim_shared_radio_##n,	\
		))							\
	};								\
	NET_DEVICE_DT_INST_DEFINE(n, hwsim_dev_init, NULL,		\
				  &hwsim_radio_##n, &hwsim_cfg_##n,	\
				  CONFIG_WIFI_INIT_PRIORITY,		\
				  COND_CODE_1(IS_ENABLED(CONFIG_WIFI_HWSIM_NAN_SHARED_IFACE), \
					(&hwsim_shared_api),		\
					(COND_CODE_1(DT_INST_PROP(n, nan_management), \
						      (&hwsim_nan_api), (&hwsim_api)))), \
				  ETHERNET_L2,				\
				  NET_L2_GET_CTX_TYPE(ETHERNET_L2),	\
				  NET_ETH_MTU);				\
	IF_ENABLED(CONFIG_WIFI_HWSIM_NAN_SHARED_IFACE, (		\
		NET_DEVICE_DT_INST_ADD_IFACE(n, ETHERNET_L2,		\
			NET_L2_GET_CTX_TYPE(ETHERNET_L2), NET_ETH_MTU, 1); \
	))

DT_INST_FOREACH_STATUS_OKAY(HWSIM_INIT)
