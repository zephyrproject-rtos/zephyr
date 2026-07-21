/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bflb_bl61x_wifi

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/queue.h>

#include <zephyr/toolchain.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_stats.h>
#include <zephyr/net/wifi.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#define IOVEC_DEFINED
#include <assert.h>
#include <wl_api.h>
#include "timeout.h"
#include "macsw_config.h"
#include "macsw_plat.h"
#include "mac_types.h"
#include "mac_frame.h"
#include "ieee80211.h"
#include "phy.h"
#include "macsw.h"
#include "wl80211.h"
#include "wl80211_mac.h"
#include "wl80211_platform.h"
#include <zephyr/net/wifi_nm.h>

#include "bflb_wifi.h"
#include "bflb_wifi_scan.h"
#include "bflb_wifi_mgmt.h"
#include "bflb_wifi_wpa_supp.h"

LOG_MODULE_REGISTER(bflb_wifi, CONFIG_WIFI_LOG_LEVEL);

#define BFLB_WIFI_MACSW_INIT_TIMEOUT_MS 2000
#define BFLB_WIFI_TX_TIMEOUT_MS         1000
#define BFLB_WIFI_EVT_CODE_MASK         0xFFFFU

#define BFLB_STA_KEEPALIVE_PERIOD_S 60
/* Local Experimental Ethertype 1 (IEEE Std 802) - private to this link. */
#define BFLB_KEEPALIVE_ETHERTYPE    0x88B5
#define BFLB_KEEPALIVE_PAYLOAD_LEN  6

#define BFLB_WIFI_TX_ALIGN sizeof(uint32_t)
#define TX_CTX_PAD         ROUND_UP(sizeof(struct tx_ctx), BFLB_WIFI_TX_ALIGN)

#define ETH_HDR_LEN        14
#define ETH_BROADCAST_BYTE 0xff
#define EAPOL_TYPE_KEY     3
#define EAPOL_KEY_INFO_OFF 5
#define EAPOL_MIN_KEY_LEN  (ETH_HDR_LEN + EAPOL_KEY_INFO_OFF + 2)
#define EAPOL_KI_INSTALL   BIT(8)
#define EAPOL_KI_SECURE    BIT(9)

#include "bflb_wifi_blob_api.h"

struct bflb_wifi_event {
	int code;
	int value;
};

struct tx_ctx {
	void (*user_cb)(void *arg);
	void *user_arg;
	struct net_pkt *pkt;
};

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "Only one bflb,bl61x-wifi instance supported");

struct bflb_wifi_dev wl80211_dev;
static struct k_work event_work;
static struct k_work_delayable keepalive_work;
K_MSGQ_DEFINE(wifi_event_msgq, sizeof(struct bflb_wifi_event), 16, 4);
static K_SEM_DEFINE(eapol_msg4_sem, 0, 1);
static K_SEM_DEFINE(tx_pending_sem, CONFIG_BFLB_WIFI_BL61X_TX_MAX_PENDING,
		    CONFIG_BFLB_WIFI_BL61X_TX_MAX_PENDING);

/* picolibc memcpy uses byte loads on WRAM -- 23 cyc/byte; word-aligned copy: 6 cyc/byte */
static void __noinline wram_copy(void *dst, const void *src, size_t len)
{
	const volatile uint32_t *s = (const volatile uint32_t *)src;
	uint32_t *d = (uint32_t *)dst;
	size_t words = len / sizeof(uint32_t);
	size_t tail = len % sizeof(uint32_t);

	while (words >= 4) {
		uint32_t w0 = s[0], w1 = s[1], w2 = s[2], w3 = s[3];

		d[0] = w0;
		d[1] = w1;
		d[2] = w2;
		d[3] = w3;
		s += 4;
		d += 4;
		words -= 4;
	}
	while (words--) {
		*d++ = *s++;
	}
	if (tail != 0) {
		uint8_t *dst_byte = (uint8_t *)d;
		const volatile uint8_t *src_byte = (const volatile uint8_t *)s;

		while (tail--) {
			*dst_byte++ = *src_byte++;
		}
	}
}

/* Statistics helpers (conditional) */

#if defined(CONFIG_NET_STATISTICS_WIFI)
static void stats_count_rx(struct bflb_wifi_dev *wdev, const uint8_t *eth, uint32_t len)
{
	wdev->stats.pkts.rx++;
	wdev->stats.bytes.received += len;
	if ((eth[0] & 0x01) == 0) {
		wdev->stats.unicast.rx++;
	} else if (eth[0] == ETH_BROADCAST_BYTE && eth[1] == ETH_BROADCAST_BYTE &&
		   eth[2] == ETH_BROADCAST_BYTE && eth[3] == ETH_BROADCAST_BYTE &&
		   eth[4] == ETH_BROADCAST_BYTE && eth[5] == ETH_BROADCAST_BYTE) {
		wdev->stats.broadcast.rx++;
	} else {
		wdev->stats.multicast.rx++;
	}
}

static void stats_count_tx(struct bflb_wifi_dev *wdev, const uint8_t *eth, uint32_t len)
{
	wdev->stats.pkts.tx++;
	wdev->stats.bytes.sent += len;
	if ((eth[0] & 0x01) == 0) {
		wdev->stats.unicast.tx++;
	} else if (eth[0] == ETH_BROADCAST_BYTE && eth[1] == ETH_BROADCAST_BYTE &&
		   eth[2] == ETH_BROADCAST_BYTE && eth[3] == ETH_BROADCAST_BYTE &&
		   eth[4] == ETH_BROADCAST_BYTE && eth[5] == ETH_BROADCAST_BYTE) {
		wdev->stats.broadcast.tx++;
	} else {
		wdev->stats.multicast.tx++;
	}
}

#define STATS_RX(wdev, eth, len) stats_count_rx((wdev), (eth), (len))
#define STATS_TX(wdev, eth, len) stats_count_tx((wdev), (eth), (len))
#define STATS_RX_ERR(wdev)       ((wdev)->stats.errors.rx++)
#define STATS_TX_ERR(wdev)       ((wdev)->stats.errors.tx++)
#define STATS_OVERRUN(wdev)      ((wdev)->stats.overrun_count++)
#else
#define STATS_RX(wdev, eth, len) ((void)0)
#define STATS_TX(wdev, eth, len) ((void)0)
#define STATS_RX_ERR(wdev)       ((void)0)
#define STATS_TX_ERR(wdev)       ((void)0)
#define STATS_OVERRUN(wdev)      ((void)0)
#endif

static void eapol_msg4_tx_done(void *arg)
{
	ARG_UNUSED(arg);
	k_sem_give(&eapol_msg4_sem);
}

/* Called by firmware from wifi task when 802.11 TX completes; frees WRAM and releases sem slot */
static void tx_done_cb(void *opaque)
{
	struct tx_ctx *ctx = opaque;
	struct net_pkt *pkt;
	void (*user_cb)(void *arg);
	void *user_arg;

	if (ctx == NULL) {
		return;
	}

	pkt = ctx->pkt;
	user_cb = ctx->user_cb;
	user_arg = ctx->user_arg;

	wl80211_platform_free_wram(opaque);

	if (pkt != NULL) {
		k_sem_give(&tx_pending_sem);
	}
	if (user_cb != NULL) {
		user_cb(user_arg);
	}
}

static void keepalive_work_handler(struct k_work *work)
{
	struct bflb_wifi_dev *wdev = &wl80211_dev;
	uint8_t frame[ETH_HDR_LEN + BFLB_KEEPALIVE_PAYLOAD_LEN];
	struct net_linkaddr *la;
	int ret;

	ARG_UNUSED(work);

	if (wdev->iface == NULL || !net_if_is_carrier_ok(wdev->iface)) {
		return;
	}

	if (wl80211_sta_get_bssid(frame) != 0) {
		goto resched;
	}
	la = net_if_get_link_addr(wdev->iface);
	memcpy(&frame[6], la->addr, WIFI_MAC_ADDR_LEN);
	sys_put_be16(BFLB_KEEPALIVE_ETHERTYPE, &frame[12]);
	memset(&frame[ETH_HDR_LEN], 0, BFLB_KEEPALIVE_PAYLOAD_LEN);

	ret = wl80211_output_raw(WL80211_VIF_STA, frame, sizeof(frame), 0, NULL, NULL);
	if (ret != 0) {
		LOG_DBG("keepalive TX failed: %d", ret);
	}

resched:
	k_work_reschedule(&keepalive_work, K_SECONDS(BFLB_STA_KEEPALIVE_PERIOD_S));
}

static void handle_disconnect_event(struct bflb_wifi_dev *wdev, int value)
{
	uint16_t reason_code = value & BFLB_WIFI_EVT_CODE_MASK;

	LOG_DBG("Disconnected (reason=%u)", reason_code);

	k_work_cancel_delayable(&keepalive_work);

	if (!net_if_is_carrier_ok(wdev->iface)) {
		return;
	}

	if (wl80211_glb.connecting != 0 || wl80211_glb.authenticating != 0) {
		return;
	}

	net_if_carrier_off(wdev->iface);
	wifi_mgmt_raise_disconnect_result_event(wdev->iface, reason_code);
}

static void event_work_handler(struct k_work *work)
{
	struct bflb_wifi_dev *wdev = &wl80211_dev;
	struct bflb_wifi_event evt;

	ARG_UNUSED(work);

	while (k_msgq_get(&wifi_event_msgq, &evt, K_NO_WAIT) == 0) {
		if (wdev->iface == NULL) {
			continue;
		}

		switch (evt.code) {
		case WL80211_EVT_SCAN_DONE:
			LOG_DBG("Scan done");
			bflb_wifi_deliver_scan_results(wdev);
			bflb_wpa_supp_scan_done_event();
			break;

		case WL80211_EVT_STA_CONNECTED:
			LOG_DBG("Connected");
			wl80211_mac_set_ps_mode(0);
			k_work_reschedule(&keepalive_work, K_SECONDS(BFLB_STA_KEEPALIVE_PERIOD_S));
			net_if_carrier_on(wdev->iface);
			bflb_wpa_supp_sta_connected_event();
			break;

		case WL80211_EVT_STA_DISCONNECTED:
			handle_disconnect_event(wdev, evt.value);
			break;

		case WL80211_EVT_AP_STARTED:
			LOG_DBG("AP started");
			wdev->ap_enabled = true;
			net_if_carrier_on(wdev->iface);
			if (wdev->ap_security == WIFI_SECURITY_TYPE_NONE) {
				int i;

				for (i = 0; i < ARRAY_SIZE(wl80211_glb.aid_list); i++) {
					wl80211_mac_ctrl_port(i, 1);
				}
			}
			break;

		case WL80211_EVT_AP_STOPPED:
			LOG_DBG("AP stopped");
			wdev->ap_enabled = false;
			net_if_carrier_off(wdev->iface);
			break;

		case WL80211_EVT_AP_STA_ADDED:
			break;

		case WL80211_EVT_AP_STA_DEL: {
			uint8_t idx = evt.value;
			struct wifi_ap_sta_info sta_info = {0};
			bool valid_sta = false;

			if (idx < ARRAY_SIZE(wl80211_glb.aid_list) &&
			    wl80211_glb.aid_list[idx].used) {
				memcpy(sta_info.mac, wl80211_glb.aid_list[idx].mac,
				       WIFI_MAC_ADDR_LEN);
				sta_info.mac_length = WIFI_MAC_ADDR_LEN;
				valid_sta = !net_eth_is_addr_unspecified(
					(struct net_eth_addr *)sta_info.mac);
			}
			if (valid_sta) {
				wifi_mgmt_raise_ap_sta_disconnected_event(wdev->iface, &sta_info);
			}
			break;
		}

		default:
			LOG_DBG("Unhandled event %d", evt.code);
			break;
		}
	}
}

static int bflb_wifi_eapol_msg_num(const uint8_t *buf, size_t len)
{
	uint16_t ki;

	if (len < EAPOL_MIN_KEY_LEN || buf[ETH_HDR_LEN + 1] != EAPOL_TYPE_KEY) {
		return 0;
	}

	ki = sys_get_be16(&buf[ETH_HDR_LEN + EAPOL_KEY_INFO_OFF]);

	if ((ki & EAPOL_KI_INSTALL) == 0) {
		return 0;
	}

	return (ki & EAPOL_KI_SECURE) != 0 ? 4 : 2;
}

static int bflb_wifi_send_eapol(struct net_pkt *pkt, size_t len)
{
	uint8_t *buf;
	void (*tx_cb)(void *) = NULL;
	int eapol_msg;
	int ret;

	if (len < ETH_HDR_LEN || len > NET_ETH_MTU + ETH_HDR_LEN) {
		return -EINVAL;
	}

	buf = k_malloc(len);
	if (buf == NULL) {
		return -ENOMEM;
	}

	if (net_pkt_read(pkt, buf, len) != 0) {
		k_free(buf);
		return -EIO;
	}

	if (sys_get_be16(&buf[12]) != NET_ETH_PTYPE_EAPOL) {
		k_free(buf);
		return 0;
	}

	eapol_msg = bflb_wifi_eapol_msg_num(buf, len);
	if (eapol_msg == 4) {
		k_sem_reset(&eapol_msg4_sem);
		tx_cb = eapol_msg4_tx_done;
	}

	ret = wl80211_output_raw(WL80211_VIF_STA, buf, len, 0, tx_cb, NULL);
	LOG_DBG("EAPOL TX: len=%zu msg=%d ret=%d", len, eapol_msg, ret);
	k_free(buf);
	if (ret != 0) {
		return -EIO;
	}

	return 0;
}

/*
 * TX: sem limits in-flight frames -> alloc WRAM buf [tx_ctx | mac_hdr | payload] -> copy pkt
 * payload into WRAM -> submit to firmware. net_pkt is NOT retained -- ethernet_send frees it
 * immediately after we return. WRAM buf lives until firmware calls tx_done_cb.
 */
static int bflb_wifi_send(const struct device *dev, struct net_pkt *pkt)
{
	struct bflb_wifi_dev *wdev = dev->data;
	const size_t len = net_pkt_get_len(pkt);
	const size_t hdr_size = sizeof(struct wl80211_mac_tx_desc);
	const size_t total = TX_CTX_PAD + hdr_size + len;
	struct iovec txseg[1];
	struct wl80211_tx_header *txhdr;
	struct tx_ctx *ctx;
	uint8_t *payload;
	void *txbuf;
	uint8_t vif;
	int rc;

	if (wdev == NULL) {
		return -ENETDOWN;
	}

	if (wl80211_sta_is_connected() == 0 && !wdev->ap_enabled) {
		return bflb_wifi_send_eapol(pkt, len);
	}

	if (k_sem_take(&tx_pending_sem, K_MSEC(BFLB_WIFI_TX_TIMEOUT_MS)) != 0) {
		STATS_TX_ERR(wdev);
		return -EAGAIN;
	}

	txbuf = wl80211_platform_malloc_wram(total);
	if (txbuf == NULL) {
		STATS_TX_ERR(wdev);
		k_sem_give(&tx_pending_sem);
		return -ENOMEM;
	}

	memset(txbuf, 0, TX_CTX_PAD + hdr_size);

	ctx = txbuf;
	ctx->pkt = pkt;

	txhdr = (void *)((uint8_t *)txbuf + TX_CTX_PAD);
	payload = (uint8_t *)txhdr + hdr_size;

	if (pkt->frags != NULL && pkt->frags->frags == NULL && pkt->frags->len >= len) {
		memcpy(payload, pkt->frags->data, len);
	} else if (net_pkt_read(pkt, payload, len) != 0) {
		STATS_TX_ERR(wdev);
		wl80211_platform_free_wram(txbuf);
		k_sem_give(&tx_pending_sem);
		return -EIO;
	}

	if (wdev->ap_enabled && len >= 14 && sys_get_be16(&payload[12]) == NET_ETH_PTYPE_EAPOL) {
		uint8_t *eapol_buf = k_malloc(len);

		if (eapol_buf == NULL) {
			wl80211_platform_free_wram(txbuf);
			k_sem_give(&tx_pending_sem);
			return -ENOMEM;
		}
		memcpy(eapol_buf, payload, len);
		wl80211_platform_free_wram(txbuf);
		k_sem_give(&tx_pending_sem);
		rc = wl80211_output_raw(WL80211_VIF_AP, eapol_buf, len, 0, NULL, NULL);
		k_free(eapol_buf);
		return rc != 0 ? -EIO : 0;
	}

	txseg[0].iov_base = payload;
	txseg[0].iov_len = len;

	vif = wdev->ap_enabled ? WL80211_VIF_AP : WL80211_VIF_STA;
	rc = wl80211_mac_tx(vif, txhdr, 0, txseg, 1, tx_done_cb, txbuf);

	if (rc != 0) {
		STATS_TX_ERR(wdev);
		wl80211_platform_free_wram(txbuf);
		k_sem_give(&tx_pending_sem);
		return -EIO;
	}

	STATS_TX(wdev, payload, len);
	return 0;
}

static void bflb_wifi_iface_init(struct net_if *iface)
{
	struct bflb_wifi_dev *wdev = net_if_get_device(iface)->data;
	struct ethernet_context *eth_ctx = net_if_l2_data(iface);
	uint8_t mac[WIFI_MAC_ADDR_LEN] = {0};
#if defined(CONFIG_WIFI_NM_WPA_SUPPLICANT_AP)
	struct wifi_nm_instance *nm;
	int i;
#endif

	wdev->iface = iface;

	if (platform_get_mac(WL80211_VIF_STA, mac) != 0) {
		LOG_ERR("Failed to read MAC address");
		return;
	}
	net_if_set_link_addr(iface, mac, WIFI_MAC_ADDR_LEN, NET_LINK_ETHERNET);

	eth_ctx->eth_if_type = L2_ETH_IF_TYPE_WIFI;
	net_if_carrier_off(iface);
	ethernet_init(iface);

#if defined(CONFIG_WIFI_NM_WPA_SUPPLICANT_AP)
	nm = wifi_nm_get_instance("wifi_supplicant");

	if (nm != NULL) {
		wifi_nm_register_mgd_iface(nm, iface);
		for (i = 0; i < CONFIG_WIFI_NM_MAX_MANAGED_INTERFACES; i++) {
			if (nm->mgd_ifaces[i].iface == iface) {
				nm->mgd_ifaces[i].type = BIT(WIFI_TYPE_STA) | BIT(WIFI_TYPE_SAP);
				break;
			}
		}
	}
#endif
}

static enum ethernet_hw_caps bflb_wifi_get_capabilities(const struct device *dev,
							struct net_if *iface)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(iface);
	return (enum ethernet_hw_caps)0;
}

static int bflb_wifi_init(const struct device *dev)
{
	struct bflb_wifi_dev *wdev = dev->data;
	int ret;

	k_work_init(&event_work, event_work_handler);
	k_work_init_delayable(&keepalive_work, keepalive_work_handler);
	k_mutex_init(&wdev->lock);

	ret = bflb_rf_init();
	if (ret != 0) {
		return ret;
	}

	wifi_task_create();
	ret = wifi_task_wait_ready(K_MSEC(BFLB_WIFI_MACSW_INIT_TIMEOUT_MS));
	if (ret != 0) {
		LOG_ERR("MAC SW task failed to start");
		return -ETIMEDOUT;
	}

	wl80211_init();

	ret = wl80211_set_country_code(CONFIG_BFLB_WIFI_BL61X_DEFAULT_COUNTRY);
	if (ret != 0) {
		LOG_WRN("Country code '%s' not found, using blob default",
			CONFIG_BFLB_WIFI_BL61X_DEFAULT_COUNTRY);
	}

	bflb_wpa_supp_register_wpa_cbs();

	wdev->initialized = true;
	LOG_DBG("WiFi driver initialized");

	return 0;
}

static const struct net_wifi_mgmt_offload bflb_wifi_api = {
	.wifi_iface.iface_api.init = bflb_wifi_iface_init,
	.wifi_iface.get_capabilities = bflb_wifi_get_capabilities,
	.wifi_iface.send = bflb_wifi_send,
	.wifi_mgmt_api = &bflb_wifi_mgmt_ops,
	.wifi_drv_ops = &bflb_wpa_supp_ops,
};

/* Public functions */

int bflb_wifi_wait_eapol_tx_done(k_timeout_t timeout)
{
	return k_sem_take(&eapol_msg4_sem, timeout);
}

void wl80211_post_event(int code1, int code2)
{
	struct bflb_wifi_event evt = {.code = code1, .value = code2};

	if (k_msgq_put(&wifi_event_msgq, &evt, K_NO_WAIT) != 0) {
		LOG_WRN("WiFi event queue full, event %d dropped", code1);
	}
	k_work_submit(&event_work);
}

/*
 * RX: firmware calls this from wifi task with frame in WRAM. We alloc a net_pkt, wram_copy
 * the Ethernet frame out of WRAM, then release the firmware buffer. net_pkt ownership passes
 * to the net stack via net_recv_data.
 */
void wl80211_tcpip_input(uint8_t vif_type, void *rxhdr, void *buf, uint32_t frm_len,
			 uint32_t status)
{
	struct bflb_wifi_dev *wdev = &wl80211_dev;
	const uint8_t *src = (const uint8_t *)buf;
	size_t remaining = frm_len;
	struct net_buf *frag;
	struct net_pkt *pkt;
	size_t chunk;

	ARG_UNUSED(vif_type);
	ARG_UNUSED(status);

	if (wdev->iface == NULL || frm_len < ETH_HDR_LEN || frm_len > NET_ETH_MTU + ETH_HDR_LEN) {
		wl80211_mac_rx_free(rxhdr);
		return;
	}

	if (sys_get_be16((const uint8_t *)buf + 12) == NET_ETH_PTYPE_EAPOL) {
		LOG_DBG("tcpip_input EAPOL: vif=%u len=%u", vif_type, frm_len);
	}

	pkt = net_pkt_rx_alloc_with_buffer(wdev->iface, frm_len, AF_UNSPEC, 0, K_NO_WAIT);
	if (pkt == NULL) {
		wl80211_mac_rx_free(rxhdr);
		return;
	}

	for (frag = pkt->frags; frag != NULL && remaining > 0; frag = frag->frags) {
		chunk = MIN(remaining, net_buf_tailroom(frag));

		wram_copy(frag->data, src, chunk);
		net_buf_add(frag, chunk);
		src += chunk;
		remaining -= chunk;
	}
	wl80211_mac_rx_free(rxhdr);

	STATS_RX(wdev, (const uint8_t *)pkt->frags->data, MIN(frm_len, pkt->frags->len));

	if (net_recv_data(wdev->iface, pkt) != 0) {
		net_pkt_unref(pkt);
	}
}

/* TX for control frames (EAPOL). Bypasses sem/backpressure -- uses nolimit WRAM alloc. */
int wl80211_output_raw(uint8_t vif_type, void *buffer, uint16_t len, unsigned int flags,
		       void (*cb)(void *), void *opaque)
{
	const size_t hdr_size = sizeof(struct wl80211_mac_tx_desc);
	const size_t total = TX_CTX_PAD + hdr_size + len;
	struct iovec txseg[1];
	struct wl80211_tx_header *txhdr;
	struct tx_ctx *ctx;
	uint8_t *payload;
	void *txbuf;

	txbuf = wl80211_platform_malloc_wram_nolimit(total);
	if (txbuf == NULL) {
		return -ENOMEM;
	}

	memset(txbuf, 0, TX_CTX_PAD + hdr_size);

	ctx = txbuf;
	ctx->user_cb = cb;
	ctx->user_arg = opaque;
	ctx->pkt = NULL;

	txhdr = (void *)((uint8_t *)txbuf + TX_CTX_PAD);
	payload = (uint8_t *)txhdr + hdr_size;

	memcpy(payload, buffer, len);

	txseg[0].iov_base = payload;
	txseg[0].iov_len = len;

	if (wl80211_mac_tx(vif_type, txhdr, flags, txseg, 1, tx_done_cb, txbuf) != 0) {
		wl80211_platform_free_wram_nolimit(txbuf);
		return -EIO;
	}

	return 0;
}

NET_DEVICE_DT_INST_DEFINE(0, bflb_wifi_init, NULL, &wl80211_dev, NULL, CONFIG_WIFI_INIT_PRIORITY,
			  &bflb_wifi_api, ETHERNET_L2, NET_L2_GET_CTX_TYPE(ETHERNET_L2),
			  NET_ETH_MTU);
