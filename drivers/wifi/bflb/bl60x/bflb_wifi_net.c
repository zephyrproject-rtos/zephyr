/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * BL60x WiFi net_if integration: device registration, Ethernet TX/RX
 * data path and event dispatch.
 */

#define DT_DRV_COMPAT bflb_bl60x_wifi

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/sys/byteorder.h>

#include "bflb_wifi.h"
#include "bflb_wifi_ipc.h"
#include "bflb_wifi_scan.h"
#include "bflb_wifi_mgmt.h"
#include "bflb_wifi_wpa_supp.h"

LOG_MODULE_REGISTER(bflb_wifi, CONFIG_WIFI_LOG_LEVEL);

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "Only one bflb,bl60x-wifi instance supported");

#define BFLB_WIFI_FW_READY_TIMEOUT_MS 5000
#define BFLB_TX_BUF_MAX               1518U
#define BFLB_OUI_LEN                  3U
#define BFLB_HWID_BUF_LEN             8U

/* EAPOL-Key frame fields used to spot handshake message 4. */
#define EAPOL_TYPE_KEY     3
#define EAPOL_KEY_INFO_OFF 5
#define EAPOL_MIN_KEY_LEN  (BFLB_WIFI_ETH_HDR_LEN + EAPOL_KEY_INFO_OFF + 2)
#define EAPOL_KI_MIC       BIT(8)
#define EAPOL_KI_SECURE    BIT(9)

struct bflb_wifi_dev bflb_wifi;

struct bflb_wifi_event {
	int code;
	int value;
};

static struct k_work event_work;
K_MSGQ_DEFINE(wifi_event_msgq, sizeof(struct bflb_wifi_event), 16, 4);
static K_SEM_DEFINE(eapol_msg4_sem, 0, 1);

/* RX ring: tcpip_stack_input runs in firmware context; frames are staged
 * here and drained into net_pkts from the system work queue.
 */
#define BFLB_RX_RING_CNT 8U
#define BFLB_RX_RING_SZ  1600U

struct bflb_rx_slot {
	uint8_t data[BFLB_RX_RING_SZ];
	uint16_t len;
	bool used;
};

static struct bflb_rx_slot bflb_rx_ring[BFLB_RX_RING_CNT];
static uint8_t bflb_rx_wr;
static struct k_work bflb_rx_work;

int bflb_wifi_wait_eapol_tx_done(k_timeout_t timeout)
{
	return k_sem_take(&eapol_msg4_sem, timeout);
}

void bflb_wifi_post_event(int code, int value)
{
	struct bflb_wifi_event evt = {.code = code, .value = value};

	if (k_msgq_put(&wifi_event_msgq, &evt, K_NO_WAIT) != 0) {
		LOG_WRN("WiFi event queue full, event %d dropped", code);
	}
	k_work_submit(&event_work);
}

/* 802.11 assoc complete; bring L2 up so the supplicant can run the 4-way
 * handshake over the data path, and tell it association succeeded.
 */
static void handle_associated_event(struct bflb_wifi_dev *d)
{
	d->connected = true;
	net_if_carrier_on(d->iface);
	net_if_dormant_off(d->iface);
	bflb_wpa_supp_assoc_event();
}

static void handle_connected_event(struct bflb_wifi_dev *d, int status)
{
	if (status == 0) {
		d->connected = true;
		net_if_carrier_on(d->iface);
		net_if_dormant_off(d->iface);
	} else {
		d->connected = false;
	}
	bflb_wpa_supp_connect_result((uint16_t)status);
}

static void handle_disconnected_event(struct bflb_wifi_dev *d, int reason)
{
	bool was_connected = d->connected;

	d->connected = false;

	if (was_connected) {
		net_if_dormant_on(d->iface);
		net_if_carrier_off(d->iface);
		wifi_mgmt_raise_disconnect_result_event(d->iface, reason);
		/* Only after a reported association -- hostap's deauth proc
		 * dereferences the (absent) mgmt frame on the !associated
		 * path.  Failed connects are reported via assoc-reject.
		 */
		bflb_wpa_supp_disconnected_event((uint16_t)reason);
	}
}

static void event_work_handler(struct k_work *work)
{
	struct bflb_wifi_dev *d = &bflb_wifi;
	struct bflb_wifi_event evt;

	ARG_UNUSED(work);

	while (k_msgq_get(&wifi_event_msgq, &evt, K_NO_WAIT) == 0) {
		if (d->iface == NULL) {
			continue;
		}

		switch (evt.code) {
		case BFLB_WIFI_EVT_SCAN_DONE:
			bflb_wifi_deliver_scan_results(d);
			bflb_wpa_supp_scan_done_event();
			break;

		case BFLB_WIFI_EVT_ASSOCIATED:
			handle_associated_event(d);
			break;

		case BFLB_WIFI_EVT_CONNECTED:
			handle_connected_event(d, evt.value);
			break;

		case BFLB_WIFI_EVT_DISCONNECTED:
			handle_disconnected_event(d, evt.value);
			break;

		default:
			LOG_DBG("Unhandled event %d", evt.code);
			break;
		}
	}
}

/* RX path.
 *
 * tcpip_stack_input is called by the blob's rxu_swdesc_upload_evt when the
 * MAC HW delivers a received data frame.  pkt->pkt[i] are up to 4 fragment
 * payload pointers; the ethernet frame starts at pkt[0] + msdu_offset.
 * EAPOL frames don't reach here (the FW routes them to wpa_sta_rx_eapol).
 */
struct bflb_wifi_pkt {
	uint32_t pkt[4];
	void *pbuf[4];
	uint16_t len[4];
};

#define BFLB_RX_STAT_FORWARD BIT(0)
#define BFLB_RX_FRAG_MAX     4

static void bflb_rx_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	for (uint8_t n = 0; n < BFLB_RX_RING_CNT; n++) {
		uint8_t idx = (bflb_rx_wr - BFLB_RX_RING_CNT + n) % BFLB_RX_RING_CNT;
		struct bflb_rx_slot *s = &bflb_rx_ring[idx];
		struct net_pkt *pkt;

		if (!s->used) {
			continue;
		}
		s->used = false;

		if (bflb_wifi.iface == NULL) {
			continue;
		}

		pkt = net_pkt_rx_alloc_with_buffer(bflb_wifi.iface, s->len, AF_UNSPEC, 0,
						   K_NO_WAIT);
		if (pkt == NULL) {
			LOG_WRN("rx: alloc failed len=%u", s->len);
			continue;
		}

		if ((net_pkt_write(pkt, s->data, s->len) != 0) ||
		    (net_recv_data(bflb_wifi.iface, pkt) < 0)) {
			net_pkt_unref(pkt);
		}
	}
}

int tcpip_stack_input(void *swdesc, uint8_t status, void *hwhdr, unsigned int msdu_offset,
		      void *pkt_v, uint8_t extra_status)
{
	struct bflb_wifi_pkt *pkt = pkt_v;
	struct bflb_rx_slot *s;
	size_t total, off;
	uint8_t i;

	ARG_UNUSED(swdesc);
	ARG_UNUSED(hwhdr);
	ARG_UNUSED(extra_status);

	if (((status & BFLB_RX_STAT_FORWARD) == 0U) || (pkt == NULL)) {
		return -EINVAL;
	}
	if ((pkt->pkt[0] == 0U) || (pkt->len[0] <= msdu_offset)) {
		return -EINVAL;
	}

	total = (size_t)pkt->len[0] - msdu_offset;
	for (i = 1; i < BFLB_RX_FRAG_MAX; i++) {
		if (pkt->len[i] == 0U) {
			break;
		}
		total += pkt->len[i];
	}

	s = &bflb_rx_ring[bflb_rx_wr];
	if (s->used || (total > BFLB_RX_RING_SZ)) {
		LOG_WRN("rx-drop: slot=%u used=%d total=%u", (unsigned int)bflb_rx_wr, s->used,
			(unsigned int)total);
		return -EINVAL;
	}

	off = pkt->len[0] - msdu_offset;
	memcpy(s->data, (const uint8_t *)(uintptr_t)pkt->pkt[0] + msdu_offset, off);
	for (i = 1; i < BFLB_RX_FRAG_MAX; i++) {
		if (pkt->len[i] == 0U) {
			break;
		}
		memcpy(s->data + off, (const void *)(uintptr_t)pkt->pkt[i], pkt->len[i]);
		off += pkt->len[i];
	}
	s->len = (uint16_t)total;
	s->used = true;
	bflb_rx_wr = (bflb_rx_wr + 1U) % BFLB_RX_RING_CNT;

	k_work_submit(&bflb_rx_work);
	return -EINVAL;
}

/* RX control-path passthrough (diagnostics hook point). */
extern int __real_rxu_cntrl_frame_handle(void *frame);

int __wrap_rxu_cntrl_frame_handle(void *frame)
{
	return __real_rxu_cntrl_frame_handle(frame);
}

/* TX path. */

static int bflb_wifi_eapol_msg_num(const uint8_t *buf, size_t len)
{
	uint16_t ki;

	if (len < EAPOL_MIN_KEY_LEN || buf[BFLB_WIFI_ETH_HDR_LEN + 1] != EAPOL_TYPE_KEY) {
		return 0;
	}

	ki = sys_get_be16(&buf[BFLB_WIFI_ETH_HDR_LEN + EAPOL_KEY_INFO_OFF]);

	if ((ki & EAPOL_KI_MIC) == 0) {
		return 0;
	}

	return (ki & EAPOL_KI_SECURE) != 0 ? 4 : 2;
}

static uint8_t bflb_tx_stage[BFLB_TX_BUF_MAX];

static int bflb_wifi_send(const struct device *dev, struct net_pkt *pkt)
{
	uint8_t *buf = bflb_tx_stage;
	struct bflb_wifi_dev *d = dev->data;
	size_t total;
	bool is_eapol;
	int ret;

	if (!d->connected) {
		return -ENETDOWN;
	}

	total = net_pkt_get_len(pkt);
	if ((total < BFLB_WIFI_ETH_HDR_LEN) || (total > BFLB_TX_BUF_MAX)) {
		return -EMSGSIZE;
	}

	if (net_pkt_read(pkt, buf, total) != 0) {
		return -EIO;
	}

	is_eapol = sys_get_be16(&buf[BFLB_WIFI_ETH_TYPE_OFF]) == BFLB_WIFI_ETH_TYPE_EAPOL;
	if (is_eapol) {
		LOG_INF("EAPOL TX len=%zu msg=%d", total, bflb_wifi_eapol_msg_num(buf, total));
	}
	if (is_eapol && (bflb_wifi_eapol_msg_num(buf, total) == 4)) {
		k_sem_reset(&eapol_msg4_sem);
		/* bflb_wifi_tx_eth polls the TX-done status before
		 * returning, so the sem can be given on success -- set_key
		 * waits on it before installing the PTK.
		 */
		ret = bflb_wifi_tx_eth(buf, (uint16_t)total, d->vif_idx, d->sta_idx);
		if (ret == 0) {
			k_sem_give(&eapol_msg4_sem);
		}
		return ret;
	}

	ret = bflb_wifi_tx_eth(buf, (uint16_t)total, d->vif_idx, d->sta_idx);
	if (ret != 0) {
		LOG_DBG("tx ret=%d len=%zu", ret, total);
	}
	return ret;
}

static void bflb_wifi_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct bflb_wifi_dev *d = dev->data;
	struct ethernet_context *eth_ctx = net_if_l2_data(iface);
	static const uint8_t bouffalo_oui[BFLB_OUI_LEN] = {0x18, 0xB9, 0x05};
	uint8_t mac[BFLB_WIFI_MAC_ADDR_LEN] = {0};

	d->iface = iface;

	/* hwinfo returns the factory MAC from efuse (OUI + NIC, network
	 * order).  Fall back to a locally-administered address derived from
	 * the device ID if the efuse MAC is blank/invalid.
	 */
	if (hwinfo_get_device_id(mac, sizeof(mac)) == (ssize_t)sizeof(mac) &&
	    (mac[0] | mac[1] | mac[2]) != 0U) {
		memcpy(d->mac_addr, mac, BFLB_WIFI_MAC_ADDR_LEN);
	} else {
		uint8_t hwid[BFLB_HWID_BUF_LEN] = {0};

		(void)hwinfo_get_device_id(hwid, sizeof(hwid));
		memcpy(d->mac_addr, bouffalo_oui, BFLB_OUI_LEN);
		d->mac_addr[BFLB_OUI_LEN] = hwid[0] ^ hwid[3];
		d->mac_addr[BFLB_OUI_LEN + 1U] = hwid[1] ^ hwid[4];
		d->mac_addr[BFLB_OUI_LEN + 2U] = hwid[2] ^ hwid[5];
		d->mac_addr[0] |= 0x02U; /* locally administered */
	}

	if (net_if_set_link_addr(iface, d->mac_addr, BFLB_WIFI_MAC_ADDR_LEN, NET_LINK_ETHERNET) !=
	    0) {
		LOG_WRN("failed to set interface MAC address");
	}

	eth_ctx->eth_if_type = L2_ETH_IF_TYPE_WIFI;
	net_if_carrier_off(iface);
	ethernet_init(iface);
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
	struct bflb_wifi_dev *d = dev->data;
	int ret;

	k_mutex_init(&d->lock);
	k_mutex_init(&d->cmd_mutex);
	k_work_init(&event_work, event_work_handler);
	k_work_init(&bflb_rx_work, bflb_rx_work_handler);

	ret = bflb_wifi_hw_init();
	if (ret != 0) {
		LOG_ERR("hw init failed: %d", ret);
		return ret;
	}

	ret = bflb_wifi_ipc_init(d);
	if (ret != 0) {
		return ret;
	}

	wifi_task_create();
	ret = wifi_task_wait_ready(K_MSEC(BFLB_WIFI_FW_READY_TIMEOUT_MS));
	if (ret != 0) {
		LOG_ERR("firmware task failed to start");
		return -ETIMEDOUT;
	}
	d->fw_ready = true;

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

NET_DEVICE_DT_INST_DEFINE(0, bflb_wifi_init, NULL, &bflb_wifi, NULL, CONFIG_WIFI_INIT_PRIORITY,
			  &bflb_wifi_api, ETHERNET_L2, NET_L2_GET_CTX_TYPE(ETHERNET_L2),
			  NET_ETH_MTU);
