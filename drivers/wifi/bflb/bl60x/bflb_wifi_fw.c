/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * BL60x WiFi firmware control: MAC bring-up (MM/ME task configuration),
 * connect/disconnect requests, E2A event dispatch and the wpa_funcs
 * bridge the firmware calls during scan/connect.
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/ethernet.h>
#include <mbedtls/platform_util.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/byteorder.h>

#include <lmac_types.h>
#include <bl60x_fw_api.h>
#include <lmac_mac.h>
#include <lmac_msg.h>
#include <supplicant_api.h>

#include "bflb_wifi.h"
#include "bflb_wifi_ipc.h"
#include "bflb_wifi_scan.h"
#include "bflb_wifi_sae.h"
#include "bflb_wifi_wpa_supp.h"

LOG_MODULE_DECLARE(bflb_wifi, CONFIG_WIFI_LOG_LEVEL);

#define IFTYPE_STA 0U /* MM_STA */

/* WIFI_CIPHER_TYPE_* values the FW exposes via wifi_wpa_ie_t. */
#define BFLB_CIPHER_NONE     0U
#define BFLB_CIPHER_WEP40    1U
#define BFLB_CIPHER_WEP104   2U
#define BFLB_CIPHER_TKIP     3U
#define BFLB_CIPHER_CCMP     4U
#define BFLB_CIPHER_AES_CMAC 6U
#define BFLB_CIPHER_UNKNOWN  8U

/* sec_proto_t values (1 byte under blob's -fshort-enums). */
#define BFLB_SEC_PROTO_WPA2 3U

/* Vendor key_mgmt bit positions (matches supplicant_api.h). */
#define BFLB_KEY_MGMT_IEEE8021X BIT(0)
#define BFLB_KEY_MGMT_PSK       BIT(1)
#define BFLB_KEY_MGMT_FT_PSK    BIT(8)
#define BFLB_KEY_MGMT_SAE       BIT(10)

/* WPA protocol IDs (wpa_parse_wpa_ie output). */
#define BFLB_WPA_PROTO_WPA 1U
#define BFLB_WPA_PROTO_RSN 2U

/* RSN/WPA suite selector values (last byte of OUI suite). */
#define BFLB_RSN_SUITE_LEN      4U
#define BFLB_RSN_SUITE_WEP40    1U
#define BFLB_RSN_SUITE_TKIP     2U
#define BFLB_RSN_SUITE_CCMP     4U
#define BFLB_RSN_SUITE_WEP104   5U
#define BFLB_RSN_SUITE_AES_CMAC 6U
#define BFLB_RSN_AKM_8021X      1U
#define BFLB_RSN_AKM_PSK        2U
#define BFLB_RSN_AKM_FT_PSK     6U
#define BFLB_RSN_AKM_SAE        8U

/* IEEE 802.11 IE tags. */
#define IEEE80211_IE_RSN    48U
#define IEEE80211_IE_VENDOR 221U

/* MM/ME init parameters (vendor defaults). */
#define BFLB_TX_LIFETIME_MS         2000U
#define BFLB_MM_PHY_PARAM_BL602     0x1U
#define BFLB_MM_UAPSD_TIMEOUT_US    3000U
#define BFLB_MM_LP_CLK_ACCURACY_PPM 20U
#define BFLB_TX_POWER_DBM           20U
#define BFLB_RESET_SETTLE_MS        5U

/* SM_CONNECT params. */
#define BFLB_AUTH_TYPE_AUTO           0U
#define BFLB_AUTH_TYPE_SAE            3U
#define BFLB_ETHERTYPE_EAPOL_LE16     0x8E88U /* 0x888E byte-swapped */
#define BFLB_LISTEN_INTERVAL          1U
#define BFLB_DISCONNECT_REASON_UNSPEC 1U

/* SM connect flags -- must match firmware's sm_connect_flags enum. */
#define BFLB_SM_FLAG_CONTROL_PORT_HOST   BIT(0)
#define BFLB_SM_FLAG_CONTROL_PORT_NO_ENC BIT(1)
#define BFLB_SM_FLAG_WPA_WPA2_IN_USE     BIT(3)

#define BFLB_SCAN_CHAN_NUM 13U
#define BFLB_CHAN_FREQ_ANY UINT16_MAX

struct mm_version_cfm_raw {
	uint32_t version_lmac;
	uint32_t version_machw_1;
	uint32_t version_machw_2;
	uint32_t version_phy_1;
	uint32_t version_phy_2;
	uint32_t features;
};

static const uint16_t bflb_ch_freq[14] = {
	2412, 2417, 2422, 2427, 2432, 2437, 2442, 2447, 2452, 2457, 2462, 2467, 2472, 2484,
};

static bool bflb_mac_init_done;

extern int bl_wifi_register_wpa_cb_internal(const struct wpa_funcs *cb);
extern int bl_wifi_set_appie_internal(uint8_t vif_idx, wifi_appie_t type, uint8_t *ie, uint16_t len,
				      bool sta);

/* Firmware-workaround timers (see bflb_rx_batch_reset_handler). */
extern uint8_t rxl_cntrl_env[];
extern uint8_t vif_info_tab[];
#define BFLB_RXL_CNTRL_BATCH_OFF 20U
#define BFLB_RX_BATCH_RESET_MS   20
#define BFLB_VIF_TBTT_CNT_OFF    0x74U
#define BFLB_VIF_MAX             2U

static bool bssid_is_specific(const uint8_t *bssid)
{
	struct net_eth_addr a;

	memcpy(a.addr, bssid, sizeof(a.addr));
	return !net_eth_is_addr_unspecified(&a) && !net_eth_is_addr_broadcast(&a);
}

/* RX-batch counter unstick: rxu_swdesc_upload_evt drops new RX descs once
 * `rxl_cntrl_env + 20` reaches 5.  On a busy channel the counter pegs and
 * unicast RX (e.g. DHCP OFFER) gets dropped.  Periodically reset it.
 * Also zero the per-VIF TBTT counter: once it exceeds 100 the FW sends a
 * null-data probe through a TX path the SW-CCMP wrap can't service, the
 * probe fails and the FW tears the link down.
 */
static void bflb_rx_batch_reset_handler(struct k_timer *t)
{
	ARG_UNUSED(t);
	*(volatile uint32_t *)&rxl_cntrl_env[BFLB_RXL_CNTRL_BATCH_OFF] = 0U;

	for (uint8_t i = 0; i < BFLB_VIF_MAX; i++) {
		uint8_t *vif = &vif_info_tab[i * BFLB_WIFI_VIF_INFO_STRIDE];

		vif[BFLB_VIF_TBTT_CNT_OFF] = 0U;
	}
}

static K_TIMER_DEFINE(bflb_rx_batch_timer, bflb_rx_batch_reset_handler, NULL);

/* wpa_funcs bridge.
 *
 * The firmware has an internal supplicant interface (wpa_funcs) it calls
 * during scan/connect.  These bridge callbacks route the events to the
 * Zephyr hostap supplicant instead:
 *   wpa_sta_config        -> fix up cipher defaults, install the assoc IEs
 *   wpa_sta_connect       -> report assoc success to hostap
 *   wpa_sta_disconnected  -> notify hostap of deauth
 *   wpa_sta_rx_eapol      -> inject EAPOL into the Zephyr net stack
 *   wpa_parse_wpa_ie      -> parse RSN/WPA IE (scanu calls this for every
 *                           AP; a NULL pointer crashes the FW)
 */

static bool bridge_sta_init(void)
{
	return true;
}

static bool bridge_sta_deinit(void)
{
	return true;
}

static void bridge_sta_config(wifi_connect_parm_t *p)
{
	if (p == NULL) {
		return;
	}

	LOG_INF("sta_config proto=%u key_mgmt=0x%x pairwise=%u group=%u vif=%u", p->proto,
		p->key_mgmt, p->pairwise_cipher, p->group_cipher, p->vif_idx);

	g_supp_ctx.sta_idx = p->sta_idx;
	bflb_wifi.vif_idx = p->vif_idx;
	bflb_wifi.sta_idx = p->sta_idx;

	if ((p->proto < BFLB_SEC_PROTO_WPA2) &&
	    ((p->key_mgmt & (BFLB_KEY_MGMT_PSK | BFLB_KEY_MGMT_FT_PSK | BFLB_KEY_MGMT_SAE)) ==
	     0U)) {
		return;
	}

	/* The FW leaves the ciphers at NONE; default to CCMP. */
	if (p->pairwise_cipher == BFLB_CIPHER_NONE) {
		p->pairwise_cipher = BFLB_CIPHER_CCMP;
	}
	if (p->group_cipher == BFLB_CIPHER_NONE) {
		p->group_cipher = BFLB_CIPHER_CCMP;
	}

	/* The supplicant-provided RSN IE governs what actually goes in the
	 * Association Request; these fields only steer the FW's own checks.
	 */
	p->key_mgmt = g_supp_ctx.conn_sae ? BFLB_KEY_MGMT_SAE : BFLB_KEY_MGMT_PSK;
	p->pmf_required = false;

	/* Register the supplicant's association IEs so the FW includes
	 * them in the Association Request.  Without this the AP refuses
	 * to associate.
	 */
	if (g_supp_ctx.assoc_ie_len > 0U) {
		int ret = bl_wifi_set_appie_internal(p->vif_idx, 0, g_supp_ctx.assoc_ie,
						     g_supp_ctx.assoc_ie_len, true);
		if (ret != 0) {
			LOG_WRN("set_appie failed: %d", ret);
		}
	}
}

/* Fires right after 802.11 association, before the 4-way handshake.
 * The FW won't send SM_CONNECT_IND until the handshake is done, and the
 * supplicant won't start the handshake until it hears assoc success --
 * so the assoc must be reported to hostap from here.
 */
static void bridge_sta_connect(wifi_connect_parm_t *p)
{
	if (p == NULL) {
		return;
	}

	bflb_wifi.vif_idx = p->vif_idx;
	bflb_wifi.sta_idx = p->sta_idx;
	g_supp_ctx.sta_idx = p->sta_idx;

	LOG_INF("sta_connect vif=%u sta=%u ssid=%.*s", p->vif_idx, p->sta_idx, p->ssid.len,
		p->ssid.ssid);

	bflb_wifi_post_event(BFLB_WIFI_EVT_ASSOCIATED, 0);
}

static void bridge_sta_disconnected(uint8_t reason)
{
	bflb_wifi_post_event(BFLB_WIFI_EVT_DISCONNECTED, reason);
}

static int bridge_sta_rx_eapol(uint8_t *src, uint8_t *buf, uint32_t len)
{
	if ((src == NULL) || (buf == NULL)) {
		return -EINVAL;
	}
	LOG_INF("EAPOL RX len=%u", len);
	/* Cache the AP's BSSID for status reporting. */
	memcpy(bflb_wifi.connected_bssid, src, BFLB_WIFI_MAC_ADDR_LEN);
	bflb_wifi_eapol_input(src, buf, len);
	return 0;
}

static uint8_t bflb_rsn_suite_to_cipher(const uint8_t *oui_suite)
{
	switch (oui_suite[3]) {
	case BFLB_RSN_SUITE_WEP40:
		return BFLB_CIPHER_WEP40;
	case BFLB_RSN_SUITE_TKIP:
		return BFLB_CIPHER_TKIP;
	case BFLB_RSN_SUITE_CCMP:
		return BFLB_CIPHER_CCMP;
	case BFLB_RSN_SUITE_WEP104:
		return BFLB_CIPHER_WEP104;
	case BFLB_RSN_SUITE_AES_CMAC:
		return BFLB_CIPHER_AES_CMAC;
	default:
		return BFLB_CIPHER_UNKNOWN;
	}
}

/* Parse RSN (tag 48) or WPA (tag 221) IE into wifi_wpa_ie_t.  The FW uses
 * the parsed proto/cipher/key_mgmt to validate the AP's security during
 * the join scan; an all-zeros result makes the join fail with status 14.
 */
static int bridge_parse_wpa_ie(const uint8_t *ie, size_t len, wifi_wpa_ie_t *out)
{
	const uint8_t *pos;
	const uint8_t *end;
	uint16_t count;
	bool is_rsn;

	if ((out == NULL) || (ie == NULL) || (len < 2U)) {
		return -EINVAL;
	}
	memset(out, 0, sizeof(*out));

	if (ie[0] == IEEE80211_IE_RSN) {
		is_rsn = true;
		pos = ie + 2;
		end = ie + 2 + ie[1];
	} else if ((ie[0] == IEEE80211_IE_VENDOR) && (len >= 6U) && (ie[2] == 0x00U) &&
		   (ie[3] == 0x50U) && (ie[4] == 0xF2U) && (ie[5] == 0x01U)) {
		is_rsn = false;
		pos = ie + 6;
		end = ie + 2 + ie[1];
	} else {
		return -EINVAL;
	}

	out->proto = is_rsn ? BFLB_WPA_PROTO_RSN : BFLB_WPA_PROTO_WPA;

	if ((pos + 2) > end) {
		return -EINVAL;
	}
	pos += 2; /* version */

	if ((pos + BFLB_RSN_SUITE_LEN) > end) {
		return 0;
	}
	out->group_cipher = bflb_rsn_suite_to_cipher(pos);
	pos += BFLB_RSN_SUITE_LEN;

	if ((pos + 2) > end) {
		return 0;
	}
	count = sys_get_le16(pos);
	pos += 2;
	if ((count > 0U) && ((pos + BFLB_RSN_SUITE_LEN) <= end)) {
		out->pairwise_cipher = bflb_rsn_suite_to_cipher(pos);
	}
	pos += (size_t)count * BFLB_RSN_SUITE_LEN;

	if ((pos + 2) > end) {
		return 0;
	}
	count = sys_get_le16(pos);
	pos += 2;
	if ((count > 0U) && ((pos + BFLB_RSN_SUITE_LEN) <= end)) {
		switch (pos[3]) {
		case BFLB_RSN_AKM_8021X:
			out->key_mgmt = BFLB_KEY_MGMT_IEEE8021X;
			break;
		case BFLB_RSN_AKM_PSK:
			out->key_mgmt = BFLB_KEY_MGMT_PSK;
			break;
		case BFLB_RSN_AKM_FT_PSK:
			out->key_mgmt = is_rsn ? BFLB_KEY_MGMT_FT_PSK : BFLB_KEY_MGMT_PSK;
			break;
		case BFLB_RSN_AKM_SAE:
			out->key_mgmt = BFLB_KEY_MGMT_SAE;
			break;
		default:
			out->key_mgmt = BFLB_KEY_MGMT_PSK;
			break;
		}
	}
	pos += (size_t)count * BFLB_RSN_SUITE_LEN;

	if ((pos + 2) <= end) {
		out->capabilities = sys_get_le16(pos);
	}
	return 0;
}

/* Stubs for table slots the host doesn't service -- the FW calls several
 * of them unconditionally (e.g. wpa_reg_diag_tlv_cb from
 * sm_rsp_timeout_ind_handler); a NULL slot is a jump to address 0.
 */
static bool bridge_stub_false(void *a)
{
	ARG_UNUSED(a);
	return false;
}

static void bridge_stub_void(void *a)
{
	ARG_UNUSED(a);
}

static bool bridge_stub_ap_join(void **sm, uint8_t *mac, uint8_t *ie, uint8_t ie_len)
{
	ARG_UNUSED(sm);
	ARG_UNUSED(mac);
	ARG_UNUSED(ie);
	ARG_UNUSED(ie_len);
	return false;
}

static void bridge_stub_ap_sta_associated(void *sm, uint8_t sta_idx)
{
	ARG_UNUSED(sm);
	ARG_UNUSED(sta_idx);
}

static bool bridge_stub_ap_rx_eapol(void *hapd, void *sm, uint8_t *data, size_t len)
{
	ARG_UNUSED(hapd);
	ARG_UNUSED(sm);
	ARG_UNUSED(data);
	ARG_UNUSED(len);
	return false;
}

static const struct wpa_funcs bridge_wpa_cb = {
	.wpa_sta_init = bridge_sta_init,
	.wpa_sta_deinit = bridge_sta_deinit,
	.wpa_sta_config = bridge_sta_config,
	.wpa_sta_connect = bridge_sta_connect,
	.wpa_sta_disconnected_cb = bridge_sta_disconnected,
	.wpa_sta_rx_eapol = bridge_sta_rx_eapol,
	.wpa_ap_init = NULL,
	.wpa_ap_deinit = bridge_stub_false,
	.wpa_ap_join = bridge_stub_ap_join,
	.wpa_ap_sta_associated = bridge_stub_ap_sta_associated,
	.wpa_ap_remove = bridge_stub_false,
	.wpa_ap_rx_eapol = bridge_stub_ap_rx_eapol,
	.wpa_parse_wpa_ie = bridge_parse_wpa_ie,
	.wpa_reg_diag_tlv_cb = bridge_stub_void,
	.wpa3_build_sae_msg = bflb_wifi_sae_build,
	.wpa3_parse_sae_msg = bflb_wifi_sae_parse,
	.wpa3_clear_sae = bflb_wifi_sae_clear,
};

/* Write a 6-byte MAC address into a pair of NXMAC lo/hi registers. */
static void bflb_nxmac_write_addr(const uint8_t addr[BFLB_WIFI_MAC_ADDR_LEN], uint32_t reg_lo,
				  uint32_t reg_hi)
{
	uint32_t lo = (uint32_t)addr[0] | ((uint32_t)addr[1] << 8) | ((uint32_t)addr[2] << 16) |
		      ((uint32_t)addr[3] << 24);
	uint32_t hi = (uint32_t)addr[4] | ((uint32_t)addr[5] << 8);

	sys_write32(lo, reg_lo);
	sys_write32(hi, reg_hi);
}

/* Send an MM/ME/SM cmd that returns a single uint32 status. */
static int bflb_send_status_cmd(struct bflb_wifi_dev *d, const char *name, uint16_t req_id,
				uint16_t cfm_id, const void *params, uint32_t params_len,
				uint32_t *cfm_status)
{
	int ret;

	*cfm_status = 0;
	ret = bflb_wifi_ipc_send_cmd(d, req_id, cfm_id, params, params_len, cfm_status,
				     sizeof(*cfm_status));
	if (ret != 0) {
		LOG_ERR("%s failed: %d", name, ret);
		return ret;
	}
	LOG_DBG("%s -> status=%u", name, (unsigned int)*cfm_status);
	return 0;
}

static void bflb_init_me_config(struct me_config_req *m)
{
	memset(m, 0, sizeof(*m));
	/* Force legacy 802.11g -- with HT advertised the AP enables A-MPDU/
	 * BlockAck for unicast TX and the BL602 MAC HW BA reassembly path
	 * doesn't reliably surface unicast frames afterwards.
	 */
	m->ht_supp = 0;
	m->vht_supp = 0;
	m->ps_on = 0;
	m->tx_lft = BFLB_TX_LIFETIME_MS;
}

static void bflb_init_chan_config(struct me_chan_config_req *c)
{
	memset(c, 0, sizeof(*c));
	for (size_t i = 0; i < BFLB_SCAN_CHAN_NUM; i++) {
		c->chan2G4[i].freq = bflb_ch_freq[i];
		c->chan2G4[i].band = 0;
		c->chan2G4[i].flags = 0;
		c->chan2G4[i].tx_power = BFLB_TX_POWER_DBM;
	}
	c->chan2G4_cnt = BFLB_SCAN_CHAN_NUM;
}

int bflb_wifi_mac_init(struct bflb_wifi_dev *d)
{
	struct mm_version_cfm_raw ver = {0};
	struct me_config_req mecfg;
	struct me_chan_config_req chancfg;
	struct mm_start_req start_req = {0};
	struct mm_add_if_req addif = {0};
	struct mm_add_if_cfm addif_cfm = {0};
	uint32_t cfm_status;
	int ret;

	if (bflb_mac_init_done) {
		return 0;
	}

	/* Register the wpa_funcs table first: scanu_frame_handler calls
	 * wpa_parse_wpa_ie for every received frame; if NULL the FW jumps
	 * to PC=0 on the first one.
	 */
	(void)bl_wifi_register_wpa_cb_internal(&bridge_wpa_cb);

	ret = bflb_send_status_cmd(d, "MM_RESET", MM_RESET_REQ, MM_RESET_CFM, NULL, 0, &cfm_status);
	if (ret != 0) {
		return ret;
	}

	k_msleep(BFLB_RESET_SETTLE_MS);

	ret = bflb_wifi_ipc_send_cmd(d, MM_VERSION_REQ, MM_VERSION_CFM, NULL, 0, &ver, sizeof(ver));
	if (ret != 0) {
		LOG_ERR("MM_VERSION failed: %d", ret);
		return ret;
	}
	LOG_DBG("FW lmac=%08x machw=%08x phy=%08x feat=%08x", (unsigned int)ver.version_lmac,
		(unsigned int)ver.version_machw_1, (unsigned int)ver.version_phy_1,
		(unsigned int)ver.features);

	bflb_init_me_config(&mecfg);
	(void)bflb_send_status_cmd(d, "ME_CONFIG", ME_CONFIG_REQ, ME_CONFIG_CFM, &mecfg,
				   sizeof(mecfg), &cfm_status);

	bflb_init_chan_config(&chancfg);
	(void)bflb_send_status_cmd(d, "ME_CHAN_CONFIG", ME_CHAN_CONFIG_REQ, ME_CHAN_CONFIG_CFM,
				   &chancfg, sizeof(chancfg), &cfm_status);

	/* phy_cfg.parameters[0] selects the only band-config the BL602 PHY
	 * supports; uapsd/lp_clk values mirror the vendor module defaults.
	 */
	start_req.phy_cfg.parameters[0] = BFLB_MM_PHY_PARAM_BL602;
	start_req.uapsd_timeout = BFLB_MM_UAPSD_TIMEOUT_US;
	start_req.lp_clk_accuracy = BFLB_MM_LP_CLK_ACCURACY_PPM;
	ret = bflb_send_status_cmd(d, "MM_START", MM_START_REQ, MM_START_CFM, &start_req,
				   sizeof(start_req), &cfm_status);
	if (ret != 0) {
		return ret;
	}

	addif.type = IFTYPE_STA;
	memcpy(addif.addr.array, d->mac_addr, BFLB_WIFI_MAC_ADDR_LEN);
	addif.p2p = 0;
	ret = bflb_wifi_ipc_send_cmd(d, MM_ADD_IF_REQ, MM_ADD_IF_CFM, &addif, sizeof(addif),
				     &addif_cfm, sizeof(addif_cfm));
	if (ret != 0) {
		LOG_ERR("MM_ADD_IF failed: %d", ret);
		return ret;
	}
	if (addif_cfm.status != 0U) {
		LOG_ERR("MM_ADD_IF status=%u", addif_cfm.status);
		return -EIO;
	}
	LOG_DBG("STA vif added: idx=%u", addif_cfm.inst_nbr);

	d->vif_idx = addif_cfm.inst_nbr;

	/* The FW's hal_machw_init leaves the NXMAC MAC ADDR registers at
	 * zero (no efuse read path).  The ACCEPT_MY_UNICAST filter checks
	 * against them, so without this unicast RX is silently dropped.
	 */
	bflb_nxmac_write_addr(d->mac_addr, NXMAC_MAC_ADDR_LOW_REG, NXMAC_MAC_ADDR_HIGH_REG);

	bflb_mac_init_done = true;
	k_timer_start(&bflb_rx_batch_timer, K_MSEC(BFLB_RX_BATCH_RESET_MS),
		      K_MSEC(BFLB_RX_BATCH_RESET_MS));
	return 0;
}

/* Wildcard BSSID triggers a status-14 join failure on this blob; resolve
 * from the scan cache when the caller didn't pin one.
 */
static void bflb_connect_fill_bssid(struct bflb_wifi_dev *d, struct sm_connect_req *req,
				    const struct bflb_wifi_connect_params *p)
{
	const struct bflb_scan_ap *ap;

	if ((p->bssid != NULL) && bssid_is_specific(p->bssid)) {
		memcpy(req->bssid.array, p->bssid, BFLB_WIFI_MAC_ADDR_LEN);
		memcpy(d->connected_bssid, p->bssid, BFLB_WIFI_MAC_ADDR_LEN);
		return;
	}

	ap = bflb_wifi_scan_find_ssid(p->ssid, p->ssid_len);
	if (ap != NULL) {
		memcpy(req->bssid.array, ap->bssid, BFLB_WIFI_MAC_ADDR_LEN);
		memcpy(d->connected_bssid, ap->bssid, BFLB_WIFI_MAC_ADDR_LEN);
		d->connected_rssi = ap->rssi;
	} else {
		memset(req->bssid.array, 0xFF, BFLB_WIFI_MAC_ADDR_LEN);
	}
}

int bflb_wifi_connect_req(struct bflb_wifi_dev *d, const struct bflb_wifi_connect_params *p)
{
	struct sm_connect_req req = {0};
	struct sm_connect_cfm cfm = {0};
	struct mm_remove_if_req rmif = {0};
	struct mm_add_if_req addif = {0};
	struct mm_add_if_cfm addif_cfm = {0};
	int ret;

	if (!d->fw_ready) {
		return -EAGAIN;
	}

	ret = bflb_wifi_mac_init(d);
	if (ret != 0) {
		return ret;
	}

	if ((p->ssid_len == 0U) || (p->ssid_len > BFLB_WIFI_SSID_MAX_LEN)) {
		return -EINVAL;
	}

	/* Remove any pre-existing VIF before adding a fresh one -- leaving
	 * the scan VIF alive while connecting on a new one makes the FW
	 * route unicast data frames to the idle VIF, silently dropping them.
	 */
	rmif.inst_nbr = d->vif_idx;
	(void)bflb_wifi_ipc_send_cmd(d, MM_REMOVE_IF_REQ, MM_REMOVE_IF_CFM, &rmif, sizeof(rmif),
				     NULL, 0);

	addif.type = IFTYPE_STA;
	memcpy(addif.addr.array, d->mac_addr, BFLB_WIFI_MAC_ADDR_LEN);
	ret = bflb_wifi_ipc_send_cmd(d, MM_ADD_IF_REQ, MM_ADD_IF_CFM, &addif, sizeof(addif),
				     &addif_cfm, sizeof(addif_cfm));
	if ((ret != 0) || (addif_cfm.status != 0U)) {
		LOG_ERR("MM_ADD_IF failed ret=%d status=%u", ret, addif_cfm.status);
		return (ret != 0) ? ret : -EIO;
	}
	d->vif_idx = addif_cfm.inst_nbr;

	memcpy(req.ssid.array, p->ssid, p->ssid_len);
	req.ssid.length = p->ssid_len;

	bflb_connect_fill_bssid(d, &req, p);

	req.chan.band = 0;
	if ((p->channel >= 1U) && (p->channel <= ARRAY_SIZE(bflb_ch_freq))) {
		req.chan.freq = bflb_ch_freq[p->channel - 1U];
	} else {
		req.chan.freq = BFLB_CHAN_FREQ_ANY;
	}
	req.chan.tx_power = BFLB_TX_POWER_DBM;

	req.ctrl_port_ethertype = BFLB_ETHERTYPE_EAPOL_LE16;
	req.listen_interval = BFLB_LISTEN_INTERVAL;
	req.dont_wait_bcmc = 0;
	/* auth_type 3 routes the firmware SM through the SAE auth exchange
	 * (wpa3_build/parse_sae_msg callbacks).
	 */
	req.auth_type = p->sae ? BFLB_AUTH_TYPE_SAE : BFLB_AUTH_TYPE_AUTO;
	req.uapsd_queues = 0;
	req.vif_idx = d->vif_idx;
	/* The FW gates all connect work on this flag (=0 returns status 12
	 * before any auth/assoc reaches the air).  The "embedded supplicant"
	 * work it enables is routed through the wpa_funcs bridge above, so
	 * the Zephyr hostap supplicant stays in control of the handshake.
	 */
	req.is_supplicant_enabled = 1;
	if (p->secured) {
		req.flags = BFLB_SM_FLAG_CONTROL_PORT_HOST | BFLB_SM_FLAG_CONTROL_PORT_NO_ENC |
			    BFLB_SM_FLAG_WPA_WPA2_IN_USE;
	} else {
		req.flags = 0;
	}

	/* The FW needs the passphrase too -- without it SM_CONNECT_IND
	 * returns status 11/12 (assoc/auth timeout).
	 */
	if (p->phrase != NULL) {
		size_t plen = strlen(p->phrase);

		if (plen > BFLB_WIFI_PSK_MAX_LEN) {
			plen = BFLB_WIFI_PSK_MAX_LEN;
		}
		memcpy(req.phrase, p->phrase, plen);
	}

	memcpy(d->connected_ssid, p->ssid, p->ssid_len);
	d->connected_ssid_len = p->ssid_len;
	d->connected_security = p->secured ? WIFI_SECURITY_TYPE_PSK : WIFI_SECURITY_TYPE_NONE;
	d->connected_channel = (p->channel >= 1U) ? p->channel : 0U;

	LOG_INF("SM_CONNECT_REQ ssid='%.*s' chan=%u", p->ssid_len, p->ssid, p->channel);

	ret = bflb_wifi_ipc_send_cmd(d, SM_CONNECT_REQ, SM_CONNECT_CFM, &req, sizeof(req), &cfm,
				     sizeof(cfm));
	mbedtls_platform_zeroize(&req, sizeof(req));
	if (ret != 0) {
		LOG_ERR("SM_CONNECT_REQ failed: %d", ret);
		return ret;
	}
	if (cfm.status != 0U) {
		LOG_ERR("SM_CONNECT_CFM status=%u", cfm.status);
		return -EIO;
	}

	/* Connection completes asynchronously via SM_CONNECT_IND. */
	return 0;
}

struct bflb_sm_disconnect_req {
	uint16_t reason_code;
	uint8_t vif_idx;
};

struct bflb_sm_disconnect_ind {
	uint16_t status_code;
	uint16_t reason_code;
	uint8_t vif_idx;
};

int bflb_wifi_disconnect_req(struct bflb_wifi_dev *d)
{
	struct bflb_sm_disconnect_req req = {
		.reason_code = BFLB_DISCONNECT_REASON_UNSPEC,
		.vif_idx = d->vif_idx,
	};
	struct mm_remove_if_req rmif = {.inst_nbr = d->vif_idx};
	uint8_t cfm_status = 0;
	int ret;

	if ((!d->fw_ready) || (!bflb_mac_init_done)) {
		return -EAGAIN;
	}

	ret = bflb_wifi_ipc_send_cmd(d, SM_DISCONNECT_REQ, SM_DISCONNECT_CFM, &req, sizeof(req),
				     &cfm_status, sizeof(cfm_status));
	LOG_DBG("disconnect ret=%d cfm=%u", ret, cfm_status);

	ret = bflb_wifi_ipc_send_cmd(d, MM_REMOVE_IF_REQ, MM_REMOVE_IF_CFM, &rmif, sizeof(rmif),
				     NULL, 0);
	LOG_DBG("MM_REMOVE_IF vif=%u ret=%d", d->vif_idx, ret);
	return 0;
}

int bflb_wifi_set_ps_mode_off(struct bflb_wifi_dev *d)
{
	struct mm_set_ps_mode_req ps = {.new_state = MM_PS_MODE_OFF};
	uint32_t cfm = 0;

	return bflb_wifi_ipc_send_cmd(d, MM_SET_PS_MODE_REQ, MM_SET_PS_MODE_CFM, &ps, sizeof(ps),
				      &cfm, sizeof(cfm));
}

static void bflb_handle_sm_connect_ind(struct bflb_wifi_dev *d, const struct sm_connect_ind *ind)
{
	LOG_INF("connect status=%u reason=%u vif=%u ap=%u qos=%u freq=%u", ind->status_code,
		ind->reason_code, ind->vif_idx, ind->ap_idx, ind->qos, ind->center_freq);

	if (ind->status_code == 0U) {
		d->vif_idx = ind->vif_idx;
		d->sta_idx = ind->ap_idx;
		g_supp_ctx.sta_idx = ind->ap_idx;
		if (bssid_is_specific(ind->bssid.array)) {
			memcpy(d->connected_bssid, ind->bssid.array, BFLB_WIFI_MAC_ADDR_LEN);
		}
		d->connected_freq = ind->center_freq;
		/* BSS address feeds the MAC HW RX filter. */
		bflb_nxmac_write_addr(d->connected_bssid, NXMAC_BSS_ADDR_LOW_REG,
				      NXMAC_BSS_ADDR_HIGH_REG);
	}

	bflb_wifi_post_event(BFLB_WIFI_EVT_CONNECTED, ind->status_code);
}

void bflb_wifi_handle_e2a_msg(struct bflb_wifi_dev *d, uint16_t id, const void *payload,
			      uint32_t len)
{
	switch (id) {
	case SCANU_RESULT_IND:
		bflb_wifi_scan_handle_result(d, payload);
		break;

	case SCANU_START_CFM:
	case SCAN_DONE_IND:
		LOG_DBG("scan done, %u APs", bflb_wifi_scan_count());
		bflb_wifi_post_event(BFLB_WIFI_EVT_SCAN_DONE, 0);
		break;

	case SM_CONNECT_IND:
		bflb_handle_sm_connect_ind(d, payload);
		break;

	case SM_STA_ADD_IND: {
		const struct sm_sta_add_ind *ind = payload;

		d->vif_idx = ind->vif_idx;
		d->sta_idx = ind->ap_idx;
		g_supp_ctx.sta_idx = ind->ap_idx;
		LOG_DBG("STA_ADD vif=%u ap_idx=%u qos=%u", ind->vif_idx, ind->ap_idx, ind->qos);
		break;
	}

	case SM_DISCONNECT_IND: {
		const struct bflb_sm_disconnect_ind *ind = payload;

		LOG_INF("SM_DISCONNECT_IND status=%u reason=%u vif=%u", ind->status_code,
			ind->reason_code, ind->vif_idx);
		bflb_wifi_post_event(BFLB_WIFI_EVT_DISCONNECTED, ind->reason_code);
		break;
	}

	case MM_RSSI_STATUS_IND: {
		/* Periodic; cache the latest RSSI for iface_status. */
		const struct mm_rssi_status_ind *ind = payload;

		d->connected_rssi = ind->rssi;
		break;
	}

	case MM_CONNECTION_LOSS_IND:
		LOG_DBG("MM_CONNECTION_LOSS_IND");
		bflb_wifi_post_event(BFLB_WIFI_EVT_DISCONNECTED, 0);
		break;

	case MM_CHANNEL_SWITCH_IND:
	case MM_CHANNEL_PRE_SWITCH_IND:
	case MM_PRIMARY_TBTT_IND:
	case MM_SECONDARY_TBTT_IND:
		/* High-frequency MAC events; not interesting for the host. */
		break;

	default:
		LOG_DBG("e2a id=0x%04x len=%u (unhandled)", id, (unsigned int)len);
		break;
	}
}
