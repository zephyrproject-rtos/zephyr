/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * BL60x WiFi scan: SCANU_START_REQ submission, SCANU_RESULT_IND parsing
 * into a result cache (with raw RSN/WPA IEs for the supplicant), and
 * delivery to the wifi_mgmt scan callback.
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include <lmac_types.h>
#include <bl60x_fw_api.h>
#include <lmac_mac.h>
#include <lmac_msg.h>

#include "bflb_wifi.h"
#include "bflb_wifi_ipc.h"
#include "bflb_wifi_scan.h"

LOG_MODULE_DECLARE(bflb_wifi, CONFIG_WIFI_LOG_LEVEL);

/* IEEE 802.11 IE tags. */
#define IEEE80211_IE_SSID      0U
#define IEEE80211_IE_DS_PARAMS 3U
#define IEEE80211_IE_RSN       48U
#define IEEE80211_IE_VENDOR    221U

/* Standard 802.11 frame layout in scanu_result_ind payload. */
#define BFLB_BEACON_BODY_OFF    36U /* hdr (24) + fixed (12) */
#define BFLB_BEACON_CAP_OFF     34U /* hdr (24) + ts (8) + bi (2) */
#define BFLB_BEACON_BSSID_OFF   16U
#define BFLB_BEACON_CAP_PRIVACY BIT(4)

#define BFLB_TX_POWER_DBM  20U
#define BFLB_SCAN_DWELL_US 300000U /* ~300ms dwell per channel */

#define BFLB_SCAN_CHAN_MAX 14 /* 2G4 only */

#define BFLB_2G4_FREQ_BASE  2407U
#define BFLB_2G4_FREQ_FIRST 2412U
#define BFLB_2G4_FREQ_LAST  2472U
#define BFLB_2G4_FREQ_CH14  2484U
#define BFLB_2G4_FREQ_STEP  5U
#define BFLB_2G4_CH14       14U

/* SCANU_RESULT_IND payload as the FW emits it -- blob ABI layout. */
struct bflb_scanu_result_ind {
	uint16_t length;      /* offset 0 */
	uint16_t framectrl;   /* offset 2 */
	uint16_t center_freq; /* offset 4 */
	uint8_t band;         /* offset 6 */
	uint8_t sta_idx;      /* offset 7 */
	uint8_t inst_nbr;     /* offset 8 */
	uint8_t sa[6];        /* offset 9 -- source MAC */
	uint8_t pad1;         /* offset 15 */
	uint32_t tsflo;       /* offset 16 */
	uint32_t tsfhi;       /* offset 20 */
	int8_t rssi;          /* offset 24 */
	int8_t ppm_abs;       /* offset 25 */
	int8_t ppm_rel;       /* offset 26 */
	uint8_t flags;        /* offset 27 */
	uint8_t data_rate;    /* offset 28 */
	uint8_t pad2[3];      /* align payload to 4-byte */
	uint32_t payload[];   /* offset 32 -- 802.11 MAC frame */
};

/* 2.4 GHz channel freqs. */
static const uint16_t bflb_ch_freq[BFLB_SCAN_CHAN_MAX] = {
	2412, 2417, 2422, 2427, 2432, 2437, 2442, 2447, 2452, 2457, 2462, 2467, 2472, 2484,
};

static struct bflb_scan_ap scan_ap[BFLB_SCAN_AP_MAX];
static uint8_t scan_ap_cnt;

static uint8_t freq_to_ch(uint16_t f);
static const uint8_t *find_ie(const uint8_t *pos, size_t len, uint8_t id);
static const uint8_t *find_wpa_ie(const uint8_t *pos, size_t len);
static struct bflb_scan_ap *scan_find_bssid(const uint8_t *bssid);
static void bflb_init_scan_req(const struct bflb_wifi_dev *d, struct scanu_start_req *req);

static uint8_t freq_to_ch(uint16_t f)
{
	if ((f >= BFLB_2G4_FREQ_FIRST) && (f <= BFLB_2G4_FREQ_LAST)) {
		return (uint8_t)((f - BFLB_2G4_FREQ_BASE) / BFLB_2G4_FREQ_STEP);
	}
	if (f == BFLB_2G4_FREQ_CH14) {
		return BFLB_2G4_CH14;
	}
	return 0;
}

static const uint8_t *find_ie(const uint8_t *pos, size_t len, uint8_t id)
{
	const uint8_t *end = pos + len;

	while ((pos + 2) <= end) {
		if (pos[0] == id) {
			return pos;
		}
		pos += 2 + pos[1];
	}
	return NULL;
}

/* WPA vendor IE: tag 221, OUI 00:50:F2 type 1. */
static const uint8_t *find_wpa_ie(const uint8_t *pos, size_t len)
{
	const uint8_t *end = pos + len;

	while ((pos + 2) <= end) {
		if ((pos[0] == IEEE80211_IE_VENDOR) && (pos[1] >= 4U) && (pos[2] == 0x00U) &&
		    (pos[3] == 0x50U) && (pos[4] == 0xF2U) && (pos[5] == 0x01U)) {
			return pos;
		}
		pos += 2 + pos[1];
	}
	return NULL;
}

static struct bflb_scan_ap *scan_find_bssid(const uint8_t *bssid)
{
	for (uint8_t i = 0; i < scan_ap_cnt; i++) {
		if (memcmp(scan_ap[i].bssid, bssid, BFLB_WIFI_MAC_ADDR_LEN) == 0) {
			return &scan_ap[i];
		}
	}
	return NULL;
}

static void bflb_init_scan_req(const struct bflb_wifi_dev *d, struct scanu_start_req *req)
{
	memset(req, 0, sizeof(*req));
	for (size_t i = 0; i < ARRAY_SIZE(bflb_ch_freq) - 1U; i++) {
		req->chan[i].band = 0;
		req->chan[i].freq = bflb_ch_freq[i];
		req->chan[i].flags = 0;
		req->chan[i].tx_power = BFLB_TX_POWER_DBM;
	}
	req->chan_cnt = ARRAY_SIZE(bflb_ch_freq) - 1U;
	req->ssid_cnt = 1;
	req->ssid[0].length = 0;
	memset(req->bssid.array, 0xFF, BFLB_WIFI_MAC_ADDR_LEN);
	memcpy(req->mac.array, d->mac_addr, BFLB_WIFI_MAC_ADDR_LEN);
	req->vif_idx = d->vif_idx;
	req->no_cck = 1;
	req->duration_scan = BFLB_SCAN_DWELL_US;
}

/* Called from the IPC E2A path for every received probe-resp/beacon. */
void bflb_wifi_scan_handle_result(struct bflb_wifi_dev *d, const void *payload)
{
	const struct bflb_scanu_result_ind *ind = payload;
	const uint8_t *f;
	const uint8_t *bssid;
	const uint8_t *ies;
	const uint8_t *ssid_ie;
	const uint8_t *ds_ie;
	const uint8_t *rsn_ie;
	const uint8_t *wpa_ie;
	struct bflb_scan_ap *a;
	uint16_t cap;
	size_t ies_len;

	ARG_UNUSED(d);

	if (ind->length < BFLB_BEACON_BODY_OFF) {
		return;
	}

	f = (const uint8_t *)ind->payload;
	bssid = f + BFLB_BEACON_BSSID_OFF;
	cap = sys_get_le16(f + BFLB_BEACON_CAP_OFF);
	ies = f + BFLB_BEACON_BODY_OFF;
	ies_len = ind->length - BFLB_BEACON_BODY_OFF;

	a = scan_find_bssid(bssid);
	if (a != NULL) {
		if (ind->rssi > a->rssi) {
			a->rssi = ind->rssi;
		}
		return;
	}

	if (scan_ap_cnt >= BFLB_SCAN_AP_MAX) {
		return;
	}

	a = &scan_ap[scan_ap_cnt];
	memcpy(a->bssid, bssid, BFLB_WIFI_MAC_ADDR_LEN);
	a->center_freq = ind->center_freq;
	a->rssi = ind->rssi;
	a->caps = cap;

	ds_ie = find_ie(ies, ies_len, IEEE80211_IE_DS_PARAMS);
	if ((ds_ie != NULL) && (ds_ie[1] == 1U)) {
		a->channel = ds_ie[2];
	} else {
		a->channel = freq_to_ch(ind->center_freq);
	}

	ssid_ie = find_ie(ies, ies_len, IEEE80211_IE_SSID);
	if ((ssid_ie != NULL) && (ssid_ie[1] <= BFLB_WIFI_SSID_MAX_LEN)) {
		a->ssid_len = ssid_ie[1];
		memcpy(a->ssid, ssid_ie + 2, a->ssid_len);
	} else {
		a->ssid_len = 0;
	}

	rsn_ie = find_ie(ies, ies_len, IEEE80211_IE_RSN);
	if ((rsn_ie != NULL) && ((size_t)(rsn_ie[1] + 2U) <= BFLB_SCAN_RSN_IE_MAX)) {
		a->rsn_ie_len = rsn_ie[1] + 2U;
		memcpy(a->rsn_ie, rsn_ie, a->rsn_ie_len);
	} else {
		a->rsn_ie_len = 0;
	}

	wpa_ie = find_wpa_ie(ies, ies_len);
	if ((wpa_ie != NULL) && ((size_t)(wpa_ie[1] + 2U) <= BFLB_SCAN_WPA_IE_MAX)) {
		a->wpa_ie_len = wpa_ie[1] + 2U;
		memcpy(a->wpa_ie, wpa_ie, a->wpa_ie_len);
	} else {
		a->wpa_ie_len = 0;
	}

	if (rsn_ie != NULL) {
		a->security = WIFI_SECURITY_TYPE_PSK;
	} else if (wpa_ie != NULL) {
		a->security = WIFI_SECURITY_TYPE_WPA_PSK;
	} else if ((cap & BFLB_BEACON_CAP_PRIVACY) != 0U) {
		a->security = WIFI_SECURITY_TYPE_WEP;
	} else {
		a->security = WIFI_SECURITY_TYPE_NONE;
	}

	scan_ap_cnt++;
}

uint8_t bflb_wifi_scan_count(void)
{
	return scan_ap_cnt;
}

const struct bflb_scan_ap *bflb_wifi_scan_entry(uint8_t idx)
{
	return (idx < scan_ap_cnt) ? &scan_ap[idx] : NULL;
}

const struct bflb_scan_ap *bflb_wifi_scan_find_ssid(const uint8_t *ssid, uint8_t ssid_len)
{
	for (uint8_t i = 0; i < scan_ap_cnt; i++) {
		if ((scan_ap[i].ssid_len == ssid_len) &&
		    (memcmp(scan_ap[i].ssid, ssid, ssid_len) == 0)) {
			return &scan_ap[i];
		}
	}
	return NULL;
}

/* Active wildcard scan across channels 1-13.  Returns immediately;
 * results arrive via SCANU_RESULT_IND and completion via SCANU_START_CFM.
 */
int bflb_wifi_scan_start(struct bflb_wifi_dev *d)
{
	struct scanu_start_req req;
	int ret;

	if (!d->fw_ready) {
		LOG_WRN("scan requested before fw ready");
		return -EAGAIN;
	}

	ret = bflb_wifi_mac_init(d);
	if (ret != 0) {
		LOG_ERR("scan: mac init failed %d", ret);
		return ret;
	}

	scan_ap_cnt = 0;

	bflb_init_scan_req(d, &req);

	ret = bflb_wifi_ipc_send_cmd(d, SCANU_START_REQ, BFLB_WIFI_PENDING_CFM_NONE, &req,
				     sizeof(req), NULL, 0);
	if (ret != 0) {
		LOG_ERR("scan send failed: %d", ret);
		return ret;
	}

	return 0;
}

void bflb_wifi_deliver_scan_results(struct bflb_wifi_dev *d)
{
	scan_result_cb_t cb;

	k_mutex_lock(&d->lock, K_FOREVER);
	cb = d->scan_cb;
	k_mutex_unlock(&d->lock);

	if (cb == NULL) {
		return;
	}

	for (uint8_t i = 0; i < scan_ap_cnt; i++) {
		struct wifi_scan_result e = {0};

		memcpy(e.ssid, scan_ap[i].ssid, scan_ap[i].ssid_len);
		e.ssid_length = scan_ap[i].ssid_len;
		memcpy(e.mac, scan_ap[i].bssid, BFLB_WIFI_MAC_ADDR_LEN);
		e.mac_length = BFLB_WIFI_MAC_ADDR_LEN;
		e.channel = scan_ap[i].channel;
		e.rssi = scan_ap[i].rssi;
		e.security = scan_ap[i].security;
		e.band = WIFI_FREQ_BAND_2_4_GHZ;
		cb(d->iface, 0, &e);
	}
	cb(d->iface, 0, NULL); /* end-of-scan sentinel */

	k_mutex_lock(&d->lock, K_FOREVER);
	d->scan_cb = NULL;
	k_mutex_unlock(&d->lock);
}

int bflb_wifi_scan(const struct device *dev, struct net_if *iface, struct wifi_scan_params *params,
		   scan_result_cb_t cb)
{
	struct bflb_wifi_dev *d = dev->data;
	int ret;

	ARG_UNUSED(iface);
	ARG_UNUSED(params);

	k_mutex_lock(&d->lock, K_FOREVER);
	if (d->scan_cb != NULL) {
		k_mutex_unlock(&d->lock);
		return -EBUSY;
	}
	d->scan_cb = cb;
	k_mutex_unlock(&d->lock);

	ret = bflb_wifi_scan_start(d);
	if (ret != 0) {
		k_mutex_lock(&d->lock, K_FOREVER);
		d->scan_cb = NULL;
		k_mutex_unlock(&d->lock);
	}
	return ret;
}
