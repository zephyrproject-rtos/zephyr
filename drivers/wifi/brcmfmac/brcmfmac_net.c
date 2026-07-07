/*
 * Copyright (c) 2026 Jonathan Elliot Peace <jep@alphabetiq.com>
 * SPDX-License-Identifier: Apache-2.0
 *
 * net_if + wifi_mgmt glue + RX dispatch hooks for chan=1 (event) and
 * chan=2 (data) frames coming from the brcmfmac RX thread.
 *
 * iface_api.init + iface_api.send, RX -> net_recv_data,
 * wifi_mgmt_ops.scan via the "escan" IOVAR. Connect/disconnect and
 * full event parsing (WLC_E_LINK / WLC_E_AUTH / WLC_E_DISASSOC_IND)
 * land in 4.5b.
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
/* net_if.h must precede dhcpv4.h -- the latter forward-declares
 * struct net_if in parameter-list scope only.
 */
#include <zephyr/net/net_if.h>
#include <zephyr/net/dhcpv4.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/sys/byteorder.h>

#include "brcmfmac_priv.h"

/* Linux brcmfmac fweh.h: flag bit in the event_msg flags field that
 * indicates "link is up" for WLC_E_LINK events.
 */
#define BRCMF_EVENT_MSG_LINK            0x01

LOG_MODULE_DECLARE(brcmfmac, CONFIG_WIFI_LOG_LEVEL);

#define BRCMFMAC_ETH_FRAME_BUF_SIZE   1600

/* === net_if init =========================================================== */

void brcmfmac_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct brcmfmac_data *data = dev->data;
	struct ethernet_context *eth_ctx = net_if_l2_data(iface);

	eth_ctx->eth_if_type = L2_ETH_IF_TYPE_WIFI;
	data->iface = iface;

	if (net_if_set_link_addr(iface, data->chip_mac, 6, NET_LINK_ETHERNET) != 0) {
		LOG_ERR("iface_init: net_if_set_link_addr failed");
	}

	ethernet_init(iface);
	net_if_dormant_on(iface);

	/* Admin-up the iface so dormant_off transitions cleanly to IF_UP
	 * once association lands. Zephyr's auto-admin-up on net stack
	 * init is not reliable for native-L2 Wi-Fi drivers; explicit is
	 * safer.
	 */
	(void)net_if_up(iface);

	LOG_INF("iface up: MAC %02x:%02x:%02x:%02x:%02x:%02x (dormant until assoc)",
		data->chip_mac[0], data->chip_mac[1], data->chip_mac[2],
		data->chip_mac[3], data->chip_mac[4], data->chip_mac[5]);
}

/* === TX path (L2 frame -> BDC + SDPCM -> F2 chan=2) ======================== */

int brcmfmac_iface_send(const struct device *dev, struct net_pkt *pkt)
{
	struct brcmfmac_data *data = dev->data;
	size_t pkt_len = net_pkt_get_len(pkt);

	if (!data->probed || !data->f2_ready) {
		return -ENETDOWN;
	}

	const size_t hdr_len = sizeof(struct sdpcm_frame_hdr)
			     + sizeof(struct sdpcm_sw_hdr)
			     + sizeof(struct bdc_hdr);

	if (hdr_len + pkt_len > BRCMFMAC_TX_SLOT_SIZE) {
		LOG_ERR("iface_send: oversize (%zu+%zu > %u)",
			hdr_len, pkt_len, (unsigned int)BRCMFMAC_TX_SLOT_SIZE);
		return -EMSGSIZE;
	}

	/* Claim the next ring slot. iface_send is the sole producer, so head
	 * is owned by us; tail is owned by the tx-thread. If head+1 == tail
	 * the ring is full -- wait briefly for the consumer to drain.
	 */
	int head = atomic_get(&data->tx_ring_head);
	int next = (head + 1) % BRCMFMAC_TX_RING_SLOTS;

	for (int retry = 0; next == atomic_get(&data->tx_ring_tail); retry++) {
		if (retry >= 20) {
			return -ENOBUFS;
		}
		k_sleep(K_MSEC(1));
	}

	struct brcmfmac_tx_slot *slot = &data->tx_ring[head];

	/* Build SDPCM sw_hdr + BDC hdr in-place. fh->len/notlen + sw->seq
	 * are filled by the tx-thread at flush time when it knows the
	 * batch's seq numbers.
	 */
	struct sdpcm_sw_hdr *sw =
		(struct sdpcm_sw_hdr *)(slot->data + sizeof(struct sdpcm_frame_hdr));
	sw->seq = 0;
	sw->chan = SDPCM_CHAN_DATA;
	sw->nextlen = 0;
	sw->hdrlen = sizeof(struct sdpcm_frame_hdr) + sizeof(struct sdpcm_sw_hdr);
	sw->flow = 0;
	sw->credit = 0;
	sw->reserved[0] = 0;
	sw->reserved[1] = 0;

	struct bdc_hdr *bdc =
		(struct bdc_hdr *)(slot->data + sizeof(struct sdpcm_frame_hdr)
				   + sizeof(struct sdpcm_sw_hdr));
	bdc->flags = BDC_PROTO_VER << BDC_PROTO_VER_SHIFT;
	bdc->priority = 0;
	bdc->flags2 = 0;
	bdc->data_offset = 0;

	if (net_pkt_read(pkt, slot->data + hdr_len, pkt_len) < 0) {
		LOG_ERR("iface_send: net_pkt_read failed");
		return -EIO;
	}
	slot->len = (uint16_t)(hdr_len + pkt_len);

	/* Publish to consumer + wake it. The tx-thread drains the ring
	 * and issues one glommed CMD53 per batch.
	 */
	atomic_set(&data->tx_ring_head, next);
	k_sem_give(&data->tx_pending_sem);
	return 0;
}

/* === RX dispatch helpers (called from bcdc.c RX thread) ==================== */

/* Strip BDC header + data_offset padding; return pointer/length of the L2
 * frame within `body`. Returns NULL on malformed input.
 */
static const uint8_t *bdc_strip(const uint8_t *body, uint16_t body_len,
				uint16_t *l2_len_out)
{
	if (body_len < BDC_HEADER_LEN) {
		return NULL;
	}
	const struct bdc_hdr *bdc = (const void *)body;
	uint16_t skip = BDC_HEADER_LEN + (uint16_t)(bdc->data_offset * 4);

	if (skip > body_len) {
		return NULL;
	}
	*l2_len_out = body_len - skip;
	return body + skip;
}

void brcmfmac_net_rx_data(struct brcmfmac_data *data,
			  const uint8_t *body, uint16_t body_len)
{
	uint16_t l2_len = 0;
	const uint8_t *l2 = bdc_strip(body, body_len, &l2_len);

	if (l2 == NULL) {
		LOG_WRN("rx data: BDC strip failed (body_len=%u)", body_len);
		return;
	}

	LOG_DBG("RX  len=%u  src=%02x:%02x:%02x:%02x:%02x:%02x  type=0x%02x%02x",
		l2_len,
		l2[6], l2[7], l2[8], l2[9], l2[10], l2[11],
		l2[12], l2[13]);

	if (data->iface == NULL || !net_if_flag_is_set(data->iface, NET_IF_UP)) {
		/* Iface not up yet (e.g. pre-assoc). Drop. */
		LOG_WRN("rx data: iface not up, dropping");
		return;
	}

	struct net_pkt *pkt = net_pkt_rx_alloc_with_buffer(data->iface, l2_len,
							    AF_UNSPEC, 0,
							    K_NO_WAIT);
	if (pkt == NULL) {
		LOG_ERR("rx data: net_pkt alloc failed (len=%u)", l2_len);
		return;
	}

	if (net_pkt_write(pkt, l2, l2_len) < 0) {
		LOG_ERR("rx data: net_pkt_write failed");
		net_pkt_unref(pkt);
		return;
	}

	if (net_recv_data(data->iface, pkt) < 0) {
		LOG_WRN("rx data: net_recv_data rejected");
		net_pkt_unref(pkt);
	}
}

/* === Event parsing (chan=1) ================================================
 *
 * Body layout after BDC strip:
 *   ethhdr (14)            -- dst[6] src[6] type[2 BE] (type == 0x886C)
 *   brcm_ethhdr (10)
 *   brcmf_event_msg_be (36 BE)
 *   event-specific payload
 */
#define EVENT_FIXED_HEADER_LEN  (14 + sizeof(struct brcm_ethhdr) + \
				 sizeof(struct brcmf_event_msg_be))

static void brcmfmac_handle_escan_event(struct brcmfmac_data *data,
					uint32_t status, uint32_t datalen,
					const uint8_t *payload)
{
	if (data->scan_cb == NULL || data->iface == NULL) {
		return;
	}

	if (status == BRCMF_E_STATUS_PARTIAL) {
		if (datalen < sizeof(struct brcmf_escan_result_le)) {
			LOG_WRN("escan partial: undersized payload (%u)", datalen);
			return;
		}
		const struct brcmf_escan_result_le *res = (const void *)payload;
		const struct brcmf_bss_info_le *bss = &res->bss_info_le;

		struct wifi_scan_result entry = {0};

		uint8_t ssid_len = bss->SSID_len;

		if (ssid_len > WIFI_SSID_MAX_LEN) {
			ssid_len = WIFI_SSID_MAX_LEN;
		}
		memcpy(entry.ssid, bss->SSID, ssid_len);
		entry.ssid_length = ssid_len;

		memcpy(entry.mac, bss->BSSID, 6);
		entry.mac_length = 6;

		/* BCM43430A1 is 2.4 GHz only -- low byte of chanspec is
		 * the control channel.
		 */
		entry.band = WIFI_FREQ_BAND_2_4_GHZ;
		entry.channel = (uint8_t)(bss->chanspec & 0xFF);

		/* Defer real RSN/WPA-IE parsing to 4.5c. For 4.5a, mark
		 * security UNKNOWN so the shell prints something visible
		 * but no assumption is made.
		 */
		entry.security = WIFI_SECURITY_TYPE_UNKNOWN;
		entry.mfp = WIFI_MFP_DISABLE;

		int16_t rssi = (int16_t)bss->RSSI;

		entry.rssi = (int8_t)rssi;

		data->scan_cb(data->iface, 0, &entry);
	} else if (status == BRCMF_E_STATUS_SUCCESS) {
		LOG_DBG("escan complete");
		data->scan_cb(data->iface, 0, NULL);
		data->scan_cb = NULL;
	} else {
		LOG_WRN("escan event status=%u (treating as failure)", status);
		data->scan_cb(data->iface, -EIO, NULL);
		data->scan_cb = NULL;
	}
}

void brcmfmac_net_rx_event(struct brcmfmac_data *data,
			   const uint8_t *body, uint16_t body_len)
{
	uint16_t l2_len = 0;
	const uint8_t *l2 = bdc_strip(body, body_len, &l2_len);

	if (l2 == NULL || l2_len < EVENT_FIXED_HEADER_LEN) {
		LOG_DBG("rx event: undersized (body_len=%u)", body_len);
		return;
	}

	/* Ethernet type at offset 12 (after src+dst MACs). */
	uint16_t ethertype = sys_get_be16(l2 + 12);

	if (ethertype != BRCM_ETHERTYPE_EVENT) {
		LOG_DBG("rx event: non-Broadcom ethertype 0x%04x", ethertype);
		return;
	}

	const struct brcmf_event_msg_be *msg =
		(const void *)(l2 + 14 + sizeof(struct brcm_ethhdr));

	uint32_t event_type = sys_be32_to_cpu(msg->event_type);
	uint32_t status     = sys_be32_to_cpu(msg->status);
	uint32_t reason     = sys_be32_to_cpu(msg->reason);
	uint16_t ev_flags   = sys_be16_to_cpu(msg->flags);
	uint32_t datalen    = sys_be32_to_cpu(msg->datalen);

	const uint8_t *payload = (const uint8_t *)msg + sizeof(*msg);
	uint16_t payload_avail = (uint16_t)(l2_len - EVENT_FIXED_HEADER_LEN);

	if (datalen > payload_avail) {
		datalen = payload_avail;
	}

	switch (event_type) {
	case WLC_E_ESCAN_RESULT:
		brcmfmac_handle_escan_event(data, status, datalen, payload);
		break;

	case WLC_E_AUTH:
		LOG_DBG("WLC_E_AUTH  status=%u reason=%u", status, reason);
		if (status != BRCMF_E_STATUS_SUCCESS) {
			data->link_state = BRCMFMAC_LINK_DOWN;
			wifi_mgmt_raise_connect_result_event(data->iface, -EIO);
		} else {
			data->link_state = BRCMFMAC_LINK_ASSOCING;
		}
		break;

	case WLC_E_ASSOC:
		LOG_DBG("WLC_E_ASSOC status=%u reason=%u", status, reason);
		if (status != BRCMF_E_STATUS_SUCCESS) {
			data->link_state = BRCMFMAC_LINK_DOWN;
			wifi_mgmt_raise_connect_result_event(data->iface, -EIO);
		}
		break;

	case WLC_E_LINK:
		if (status == BRCMF_E_STATUS_SUCCESS &&
		    (ev_flags & BRCMF_EVENT_MSG_LINK)) {
			LOG_INF("link UP");
			data->link_state = BRCMFMAC_LINK_UP;
			if (data->iface != NULL) {
				net_if_dormant_off(data->iface);
#if defined(CONFIG_NET_DHCPV4)
				net_dhcpv4_restart(data->iface);
#endif
			}
			wifi_mgmt_raise_connect_result_event(data->iface, 0);
		} else {
			/* For WLC_E_LINK the firmware fills `reason` with its
			 * own BRCMF_E_REASON_* code, NOT the 802.11 reason code:
			 *   0=INITIAL_ASSOC 1=LOW_RSSI 2=DEAUTH 3=DISASSOC
			 *   4=BCNS_LOST 9=MINTXRATE
			 * For an AP-initiated deauth the actual 802.11 reason
			 * lands on the WLC_E_DEAUTH_IND event below.
			 */
			LOG_INF("link DOWN  status=%u reason=%u flags=0x%04x",
				status, reason, ev_flags);
			data->link_state = BRCMFMAC_LINK_DOWN;
			if (data->iface != NULL) {
				net_if_dormant_on(data->iface);
			}
		}
		break;

	case WLC_E_DISASSOC_IND:
		LOG_INF("WLC_E_DISASSOC_IND  reason=%u", reason);
		data->link_state = BRCMFMAC_LINK_DOWN;
		if (data->iface != NULL) {
			net_if_dormant_on(data->iface);
		}
		wifi_mgmt_raise_disconnect_result_event(data->iface, 0);
		break;

	case WLC_E_SET_SSID:
		LOG_DBG("WLC_E_SET_SSID  status=%u (join %s)",
			status,
			status == BRCMF_E_STATUS_SUCCESS ? "ok" : "failed");
		if (status != BRCMF_E_STATUS_SUCCESS) {
			data->link_state = BRCMFMAC_LINK_DOWN;
			wifi_mgmt_raise_connect_result_event(data->iface, -EIO);
		}
		break;

	case WLC_E_PSK_SUP:
		/* PSK_SUP uses its own status-code namespace (WLC_SUP_*),
		 * NOT the generic BRCMF_E_STATUS_*. status=6 = WLC_SUP_KEYED
		 * (pairwise key installed -- success).
		 */
		LOG_DBG("WLC_E_PSK_SUP  status=%u reason=%u  (6=KEYED)",
			status, reason);
		break;

	case WLC_E_DEAUTH:
	case WLC_E_DEAUTH_IND:
		/* `reason` here IS the 802.11 deauth reason code (frame body). */
		LOG_INF("WLC_E_DEAUTH%s  reason=%u",
			event_type == WLC_E_DEAUTH_IND ? "_IND" : "", reason);
		break;

	case WLC_E_AUTH_FAIL:
		LOG_INF("WLC_E_AUTH_FAIL  status=%u reason=%u", status, reason);
		break;

	default:
		LOG_DBG("event %u status=%u reason=%u datalen=%u (no handler)",
			event_type, status, reason, datalen);
		break;
	}
}

/* === wifi_mgmt_ops.scan ==================================================== */

int brcmfmac_mgmt_scan(const struct device *dev, struct net_if *iface,
		       struct wifi_scan_params *params, scan_result_cb_t cb)
{
	struct brcmfmac_data *data = dev->data;

	ARG_UNUSED(iface);
	ARG_UNUSED(params);

	if (!data->probed) {
		return -ENETDOWN;
	}
	if (data->scan_cb != NULL) {
		return -EINPROGRESS;
	}

	struct brcmf_escan_params_le esc;

	memset(&esc, 0, sizeof(esc));
	/* All multi-byte fields in this struct are little-endian on the wire;
	 * convert explicitly so the host CPU's endianness doesn't matter.
	 */
	esc.version = sys_cpu_to_le32(BRCMF_SCAN_PARAMS_VERSION);
	esc.action  = sys_cpu_to_le16(WL_ESCAN_ACTION_START);
	esc.sync_id = sys_cpu_to_le16(1);

	esc.params_le.bss_type     = 2;             /* DOT11_BSSTYPE_ANY (int8) */
	esc.params_le.scan_type    = 0;             /* active (uint8) */
	esc.params_le.nprobes      = sys_cpu_to_le32((uint32_t)-1);
	esc.params_le.active_time  = sys_cpu_to_le32((uint32_t)-1);
	esc.params_le.passive_time = sys_cpu_to_le32((uint32_t)-1);
	esc.params_le.home_time    = sys_cpu_to_le32((uint32_t)-1);
	esc.params_le.channel_num  = sys_cpu_to_le32(0); /* all channels */
	memset(esc.params_le.bssid, 0xFF, 6);            /* broadcast */
	esc.params_le.ssid_le.SSID_len = sys_cpu_to_le32(0); /* broadcast SSID */

	/* Park the cb BEFORE firing the IOVAR so an event arriving fast
	 * (which we've observed -- the chip can have queued frames) has
	 * a place to land.
	 */
	data->scan_cb = cb;

	int ret = brcmfmac_bcdc_iovar_set(data, "escan",
					   (const uint8_t *)&esc, sizeof(esc));
	if (ret != 0) {
		LOG_ERR("escan iovar_set failed: %d", ret);
		data->scan_cb = NULL;
		return ret;
	}
	LOG_DBG("escan started -- results streaming via WLC_E_ESCAN_RESULT");
	return 0;
}

/* === wifi_mgmt_ops.connect (WPA2-PSK) ======================================
 *
 * Sequence (each step is plain iovar / WLC cmd; bsscfgidx=0 = STA, which
 * Linux's brcmf_create_bsscfg short-circuits to a plain iovar with no
 * prefix wrapping):
 *
 *   1. iovar "mpc" = 0           disable multi-power-control during assoc
 *   2. iovar "auth" = 0          802.11 OPEN -- WPA2 does its EAPOL after
 *   3. iovar "wsec" = 4          AES_ENABLED
 *   4. iovar "wpa_auth" = 0x84   WPA_PSK | WPA2_PSK (mixed-mode compat)
 *   5. cmd  WLC_SET_WSEC_PMK     passphrase + flags=PASSPHRASE; chip derives PMK
 *   6. cmd  WLC_SET_SSID         fires the join; events stream on chan=1
 */
int brcmfmac_mgmt_connect(const struct device *dev, struct net_if *iface,
			  struct wifi_connect_req_params *params)
{
	struct brcmfmac_data *data = dev->data;

	ARG_UNUSED(iface);
	int ret;

	if (!data->probed) {
		return -ENETDOWN;
	}
	if (params == NULL || params->ssid == NULL || params->ssid_length == 0 ||
	    params->ssid_length > BRCMF_SSID_MAX_LEN) {
		return -EINVAL;
	}
	if (params->security != WIFI_SECURITY_TYPE_PSK &&
	    params->security != WIFI_SECURITY_TYPE_PSK_SHA256) {
		LOG_ERR("connect: only WPA2-PSK supported (got security=%u)",
			params->security);
		return -ENOTSUP;
	}
	if (params->psk == NULL ||
	    params->psk_length < 8 ||
	    params->psk_length > BRCMF_WSEC_MAX_PASSPHRASE_LEN) {
		return -EINVAL;
	}

	LOG_INF("connect: ssid=\"%.*s\" psk=<%u bytes>",
		params->ssid_length, params->ssid, params->psk_length);

	data->link_state = BRCMFMAC_LINK_AUTHING;
	memcpy(data->connected_ssid, params->ssid, params->ssid_length);
	data->connected_ssid_len = params->ssid_length;
	data->connected_security = params->security;

	/* Sequence mirrors a known-working bare-metal join path for the
	 * same chip family. Plain WLC commands for infra/auth/wsec (chip
	 * doesn't accept these as plain iovars on all firmware variants);
	 * bsscfg-prefixed iovars to enable the firmware supplicant; PMK
	 * before wpa_auth so the chip has key material when assoc fires.
	 */
	const uint32_t infra     = 1;
	const uint32_t auth      = 0;                          /* 802.11 OPEN */
	const uint32_t wsec      = 6;                          /* TKIP | AES (mixed AP) */
	const uint32_t wpa_auth  = 0x80;                       /* WPA2_AUTH_PSK */

	ret = brcmfmac_bcdc_set_dcmd(data, BRCMFMAC_WLC_SET_INFRA,
				     (const uint8_t *)&infra, 4);
	if (ret != 0) {
		LOG_ERR("connect: WLC_SET_INFRA=1 failed: %d", ret);
		goto fail;
	}
	ret = brcmfmac_bcdc_set_dcmd(data, BRCMFMAC_WLC_SET_AUTH,
				     (const uint8_t *)&auth, 4);
	if (ret != 0) {
		LOG_ERR("connect: WLC_SET_AUTH=0 failed: %d", ret);
		goto fail;
	}
	ret = brcmfmac_bcdc_set_dcmd(data, BRCMFMAC_WLC_SET_WSEC,
				     (const uint8_t *)&wsec, 4);
	if (ret != 0) {
		LOG_ERR("connect: WLC_SET_WSEC=%u failed: %d", wsec, ret);
		goto fail;
	}

	/* Enable firmware-side WPA supplicant. The "bsscfg:" prefix is
	 * required even for bsscfgidx=0 on this firmware -- the plain
	 * iovar variant returns OK but doesn't actually take effect.
	 */
	ret = brcmfmac_bcdc_bsscfg_iovar_set_int(data, "sup_wpa", 0, 1);
	if (ret != 0) {
		LOG_ERR("connect: bsscfg:sup_wpa=1 failed: %d", ret);
		goto fail;
	}
	ret = brcmfmac_bcdc_bsscfg_iovar_set_int(data, "sup_wpa2_eapver", 0, -1);
	if (ret != 0) {
		LOG_ERR("connect: bsscfg:sup_wpa2_eapver=-1 failed: %d", ret);
		goto fail;
	}
	ret = brcmfmac_bcdc_bsscfg_iovar_set_int(data, "sup_wpa_tmo", 0, 2500);
	if (ret != 0) {
		LOG_ERR("connect: bsscfg:sup_wpa_tmo=2500 failed: %d", ret);
		goto fail;
	}

	struct brcmf_wsec_pmk_le pmk;

	memset(&pmk, 0, sizeof(pmk));
	pmk.key_len = params->psk_length;
	pmk.flags   = BRCMF_WSEC_PASSPHRASE;   /* chip-side PBKDF2 derives PMK */
	memcpy(pmk.key, params->psk, params->psk_length);

	ret = brcmfmac_bcdc_set_dcmd(data, BRCMFMAC_WLC_SET_WSEC_PMK,
				     (const uint8_t *)&pmk, sizeof(pmk));
	if (ret != 0) {
		LOG_ERR("connect: WLC_SET_WSEC_PMK failed: %d", ret);
		goto fail;
	}

	ret = brcmfmac_bcdc_set_dcmd(data, BRCMFMAC_WLC_SET_WPA_AUTH,
				     (const uint8_t *)&wpa_auth, 4);
	if (ret != 0) {
		LOG_ERR("connect: WLC_SET_WPA_AUTH=0x%x failed: %d",
			wpa_auth, ret);
		goto fail;
	}

	struct brcmf_ssid_le ssid_le;

	memset(&ssid_le, 0, sizeof(ssid_le));
	ssid_le.SSID_len = params->ssid_length;
	memcpy(ssid_le.SSID, params->ssid, params->ssid_length);

	ret = brcmfmac_bcdc_set_dcmd(data, BRCMFMAC_WLC_SET_SSID,
				     (const uint8_t *)&ssid_le, sizeof(ssid_le));
	if (ret != 0) {
		LOG_ERR("connect: WLC_SET_SSID failed: %d", ret);
		goto fail;
	}

	LOG_DBG("connect: join fired -- awaiting WLC_E_LINK on chan=1");
	return 0;

fail:
	data->link_state = BRCMFMAC_LINK_DOWN;
	return ret;
}

int brcmfmac_mgmt_disconnect(const struct device *dev, struct net_if *iface)
{
	struct brcmfmac_data *data = dev->data;

	ARG_UNUSED(iface);

	if (!data->probed) {
		return -ENETDOWN;
	}

	int ret = brcmfmac_bcdc_set_dcmd(data, BRCMFMAC_WLC_DISASSOC, NULL, 0);

	if (ret != 0) {
		LOG_ERR("disconnect: WLC_DISASSOC failed: %d", ret);
		return ret;
	}

	data->link_state = BRCMFMAC_LINK_DOWN;
	data->connected_ssid_len = 0;
	if (data->iface != NULL) {
		net_if_dormant_on(data->iface);
	}
	LOG_DBG("disconnect: fired -- awaiting WLC_E_DISASSOC_IND on chan=1");
	return 0;
}

/* === wifi_mgmt_ops.iface_status ============================================
 *
 * state + ssid + security from last connect(), plus rssi via a
 * WLC_GET_RSSI round-trip when the link is up. Channel stubbed to 0.
 */
int brcmfmac_mgmt_iface_status(const struct device *dev, struct net_if *iface,
			       struct wifi_iface_status *status)
{
	struct brcmfmac_data *data = dev->data;

	ARG_UNUSED(iface);

	memset(status, 0, sizeof(*status));
	status->band = WIFI_FREQ_BAND_2_4_GHZ;
	status->iface_mode = WIFI_MODE_INFRA;
	status->link_mode = WIFI_LINK_MODE_UNKNOWN;
	status->security = data->connected_security;
	status->mfp = WIFI_MFP_OPTIONAL;
	status->channel = 0;
	status->rssi = 0;

	switch (data->link_state) {
	case BRCMFMAC_LINK_DOWN:
		status->state = WIFI_STATE_DISCONNECTED;
		break;
	case BRCMFMAC_LINK_AUTHING:
		status->state = WIFI_STATE_AUTHENTICATING;
		break;
	case BRCMFMAC_LINK_ASSOCING:
		status->state = WIFI_STATE_ASSOCIATING;
		break;
	case BRCMFMAC_LINK_UP:
		status->state = WIFI_STATE_COMPLETED;
		break;
	}

	if (data->link_state == BRCMFMAC_LINK_UP) {
		/* WLC_GET_BSSID returns the associated AP's BSSID. */
		uint8_t bssid[6] = { 0 };
		int rc_bssid = brcmfmac_bcdc_query_dcmd(
				data, BRCMFMAC_WLC_GET_BSSID, NULL, 0,
				bssid, sizeof(bssid));
		if (rc_bssid >= (int)sizeof(bssid)) {
			memcpy(status->bssid, bssid, sizeof(status->bssid));
		}

		/* WLC_GET_RSSI fills a scb_val_t { int32 val; u8 ea[6] } and
		 * returns the RSSI in val. The request buffer must be the
		 * full sizeof(scb_val_t) -- 12 bytes: the 10 logical bytes
		 * rounded up to the struct's 4-byte alignment. The firmware
		 * length-checks the request and rejects a 10-byte buffer with
		 * BCME_BADARG; the tail padding is not optional. A zeroed ea
		 * selects the connected AP (matches Linux brcmfmac).
		 */
		uint8_t scb_val[12] = { 0 };
		int rc = brcmfmac_bcdc_query_dcmd(
				data, BRCMFMAC_WLC_GET_RSSI,
				scb_val, sizeof(scb_val),
				scb_val, sizeof(scb_val));
		if (rc >= 4) {
			int32_t rssi;

			memcpy(&rssi, scb_val, sizeof(rssi));
			status->rssi = (int8_t)rssi;
		}
	}

	if (data->connected_ssid_len > 0 &&
	    data->connected_ssid_len <= sizeof(status->ssid)) {
		memcpy(status->ssid, data->connected_ssid,
		       data->connected_ssid_len);
		status->ssid_len = data->connected_ssid_len;
	}

	return 0;
}
