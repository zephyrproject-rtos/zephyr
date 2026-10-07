/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Station mode: commands sent to the network processor on the management
 * endpoint and handling of its confirmations and indications.
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_utils.h>
#include <zephyr/sys/byteorder.h>

#include "wifi_ncp.h"
#include "wifi_ncp_sta.h"

LOG_MODULE_DECLARE(wifi_ncp, CONFIG_WIFI_LOG_LEVEL);

/* Fixed fields of a beacon or probe response after the MAC header */
#define WIFI_NCP_BEACON_FIXED_LEN 12

BUILD_ASSERT(offsetof(sli_wifi_sta_join_ap_info_t, scan_result) == 0);

int wifi_ncp_nwp_cmd(const struct device *dev, const struct wifi_ncp_nwp_hdr *hdr, const void *head,
		     size_t head_len, const void *body, size_t body_len, void *resp,
		     size_t *resp_size)
{
	struct wifi_ncp_data *data = dev->data;
	struct wifi_ncp_cmd *cmd = &data->cmd;
	int ret;

	k_mutex_lock(&cmd->lock, K_FOREVER);

	cmd->id = hdr->command_id;
	cmd->status = 0;
	cmd->resp = resp;
	cmd->resp_size = resp ? *resp_size : 0;
	k_sem_reset(&cmd->done);
	cmd->pending = true;

	ret = wifi_ncp_nwp_send(dev, hdr, head, head_len, body, body_len);
	if (!ret && k_sem_take(&cmd->done, K_MSEC(CONFIG_WIFI_SILABS_NCP_CMD_TIMEOUT_MS))) {
		LOG_ERR("%s: command 0x%02x: no confirmation", dev->name, hdr->command_id);
		ret = -ETIMEDOUT;
	}

	/* A confirmation arriving after the timeout would be matched against
	 * a command that is over; the co-processor is not expected to be that
	 * late.
	 */
	cmd->pending = false;

	if (!ret) {
		ret = cmd->status;
		if (resp_size) {
			*resp_size = cmd->resp_size;
		}
		LOG_DBG("%s: command 0x%02x: status 0x%04x", dev->name, hdr->command_id, ret);
	}

	k_mutex_unlock(&cmd->lock);

	return ret;
}

/* Deliver a confirmation to the command waiting for it, if any */
static bool wifi_ncp_cmd_confirm(const struct device *dev, const struct wifi_ncp_nwp_hdr *hdr,
				 const uint8_t *payload, size_t len)
{
	struct wifi_ncp_data *data = dev->data;
	struct wifi_ncp_cmd *cmd = &data->cmd;

	if (!cmd->pending || cmd->id != hdr->command_id) {
		LOG_ERR("%s: unexpected confirmation %02x", dev->name, hdr->command_id);
		return false;
	}

	cmd->status = wifi_ncp_nwp_status(hdr);
	cmd->resp_size = MIN(len, cmd->resp_size);
	memcpy(cmd->resp, payload, cmd->resp_size);
	cmd->pending = false;
	k_sem_give(&cmd->done);

	return true;
}

/* Keep a beacon or probe response for the supplicant. The information
 * elements are truncated to whole elements when they do not fit.
 */
static void wifi_ncp_scan_add(const struct device *dev, const uint8_t *hdr, const uint8_t *frame,
			      size_t len)
{
	struct wifi_ncp_data *data = dev->data;
	struct wifi_ncp_sta *sta = &data->sta;
	const struct ieee80211_mgmt *mgmt = (const struct ieee80211_mgmt *)frame;
	struct wifi_ncp_scan_entry *entry = NULL;
	const uint8_t *ie;
	size_t ie_len, kept = 0;
	uint16_t fc, stype;
	uint8_t channel = hdr[WIFI_NCP_NWP_DESC_CHANNEL];

	if (len < IEEE80211_HDRLEN + WIFI_NCP_BEACON_FIXED_LEN) {
		return;
	}
	fc = sys_le16_to_cpu(mgmt->frame_control);
	stype = WLAN_FC_GET_STYPE(fc);
	if (WLAN_FC_GET_TYPE(fc) != WLAN_FC_TYPE_MGMT ||
	    (stype != WLAN_FC_STYPE_BEACON && stype != WLAN_FC_STYPE_PROBE_RESP)) {
		return;
	}

	ie = mgmt->u.beacon.variable;
	ie_len = len - IEEE80211_HDRLEN - WIFI_NCP_BEACON_FIXED_LEN;
	while (kept + 2 <= ie_len && kept + 2 + ie[kept + 1] <= ie_len &&
	       kept + 2 + ie[kept + 1] <= CONFIG_WIFI_SILABS_NCP_SCAN_IE_MAX) {
		kept += 2 + ie[kept + 1];
	}

	k_mutex_lock(&sta->lock, K_FOREVER);
	for (size_t i = 0; i < ARRAY_SIZE(sta->scan); i++) {
		if (sta->scan[i].used &&
		    !memcmp(sta->scan[i].bssid, mgmt->bssid, NET_ETH_ADDR_LEN)) {
			entry = &sta->scan[i];
			break;
		}
		if (!entry && !sta->scan[i].used) {
			entry = &sta->scan[i];
		}
	}
	if (!entry) {
		LOG_WRN("%s: scan results full, dropping a BSS", dev->name);
	} else {
		entry->used = true;
		memcpy(entry->bssid, mgmt->bssid, NET_ETH_ADDR_LEN);
		entry->channel = channel;
		entry->beacon_int = sys_le16_to_cpu(mgmt->u.beacon.beacon_int);
		entry->caps = sys_le16_to_cpu(mgmt->u.beacon.capab_info);
		entry->ie_len = kept;
		memcpy(entry->ie, ie, kept);
	}
	k_mutex_unlock(&sta->lock);
}

static void wifi_ncp_scan_done(const struct device *dev, const uint8_t *hdr, const uint8_t *frame,
			       size_t len)
{
	struct wifi_ncp_data *data = dev->data;
	struct wifi_ncp_sta *sta = &data->sta;
	union wpa_event_data event;
	uint16_t status = wifi_ncp_nwp_status((const struct wifi_ncp_nwp_hdr *)hdr);

	ARG_UNUSED(frame);
	ARG_UNUSED(len);

	k_mutex_lock(&sta->lock, K_FOREVER);
	if (!sta->scanning) {
		k_mutex_unlock(&sta->lock);
		LOG_WRN("%s: unexpected scan confirmation (0x%04x)", dev->name, status);
		return;
	}
	sta->scanning = false;
	k_mutex_unlock(&sta->lock);

	LOG_DBG("%s: scan done: 0x%04x", dev->name, status);
	if (data->supp.if_ctx && data->supp.cb.scan_done) {
		memset(&event, 0, sizeof(event));
		data->supp.cb.scan_done(data->supp.if_ctx, &event);
	}
}

/* Deferred confirmation of the join. The result is reported to the
 * supplicant, which then runs the key exchange.
 */
static void wifi_ncp_join_status(const struct device *dev, const uint8_t *hdr,
				 const uint8_t *payload, size_t len)
{
	struct wifi_ncp_data *data = dev->data;
	struct wifi_ncp_sta *sta = &data->sta;
	union wpa_event_data event;
	enum wifi_ncp_sta_state state;
	enum wifi_frequency_bands band = wifi_utils_chan_to_band(sta->channel);
	uint16_t status = wifi_ncp_nwp_status((const struct wifi_ncp_nwp_hdr *)hdr);

	k_mutex_lock(&sta->lock, K_FOREVER);
	state = sta->state;
	if (state == WIFI_NCP_STA_JOINING && status != SLI_STATUS_JOIN_SAE_TRIGGER) {
		sta->state = !status ? WIFI_NCP_STA_ASSOCIATED : WIFI_NCP_STA_IDLE;
	}
	k_mutex_unlock(&sta->lock);

	if (!data->supp.if_ctx || !data->supp.cb.assoc_resp) {
		return;
	}

	memset(&event, 0, sizeof(event));
	if (state != WIFI_NCP_STA_JOINING) {
		LOG_WRN("%s: unexpected join status 0x%04x", dev->name, status);
	} else if (status == SLI_STATUS_JOIN_SAE_TRIGGER) {
		/* The co-processor waits for the supplicant to authenticate;
		 * the join goes on once SLI_MGMTIF_CMD_SAE_CONFIRM_SUCCESS is
		 * sent and is confirmed later.
		 */
		LOG_INF("%s: SAE authentication requested", dev->name);
		if (!data->supp.cb.external_auth) {
			LOG_ERR("%s: the supplicant cannot authenticate", dev->name);
			return;
		}
		event.external_auth.action = EXT_AUTH_START;
		event.external_auth.bssid = sta->bssid;
		event.external_auth.ssid = sta->ssid;
		event.external_auth.ssid_len = sta->ssid_len;
		event.external_auth.key_mgmt_suite = RSN_AUTH_KEY_MGMT_SAE;
		data->supp.cb.external_auth(data->supp.if_ctx, &event);
	} else if (!status) {
		LOG_INF("%s: associated", dev->name);
		event.assoc_info.addr = sta->bssid;
		event.assoc_info.req_ies = sta->wpa_ie;
		event.assoc_info.req_ies_len = sta->wpa_ie_len;
		event.assoc_info.freq = wifi_utils_chan_to_freq(band, sta->channel);
		/* No key exchange on an open network */
		event.assoc_info.authorized = (sta->key_mgmt == WPA_KEY_MGMT_NONE) ? 1 : 0;
		data->supp.cb.assoc_resp(data->supp.if_ctx, &event, WLAN_STATUS_SUCCESS);
	} else {
		LOG_WRN("%s: join failed: 0x%04x", dev->name, status);
		event.assoc_reject.bssid = sta->bssid;
		event.assoc_reject.status_code = WLAN_STATUS_UNSPECIFIED_FAILURE;
		data->supp.cb.assoc_resp(data->supp.if_ctx, &event,
					 WLAN_STATUS_UNSPECIFIED_FAILURE);
	}
}

/* The layout of DW3 is not known yet, trace it */
static void wifi_ncp_tx_status(const struct device *dev, const uint8_t *hdr, const uint8_t *frame,
			       size_t len)
{
	ARG_UNUSED(frame);
	ARG_UNUSED(len);

	LOG_INF("%s: TX status: %02x %02x %02x %02x", dev->name, hdr[12], hdr[13], hdr[14],
		hdr[15]);
}

/* Authentication frame of the SAE exchange, for the supplicant */
static void wifi_ncp_auth_rx(const struct device *dev, const uint8_t *hdr, const uint8_t *frame,
			     size_t len)
{
	struct wifi_ncp_data *data = dev->data;
	enum wifi_frequency_bands band = wifi_utils_chan_to_band(data->sta.channel);

	ARG_UNUSED(hdr);

	if (data->supp.if_ctx && data->supp.cb.mgmt_rx) {
		data->supp.cb.mgmt_rx(data->supp.if_ctx, (char *)frame, len,
				      wifi_utils_chan_to_freq(band, data->sta.channel), 0);
	}
}

/* Confirmations of the deferred commands and indications */
static const struct {
	uint8_t id;
	void (*fn)(const struct device *dev, const uint8_t *hdr, const uint8_t *frame, size_t len);
} wifi_ncp_rx_handlers[] = {
	{SLI_WIFI_IND_ON_AIR_MGMT, wifi_ncp_scan_add}, {SLI_WIFI_CMD_SCAN, wifi_ncp_scan_done},
	{SLI_WIFI_CMD_JOIN, wifi_ncp_join_status},     {SLI_NWP_IND_ON_AIR_MGMT, wifi_ncp_auth_rx},
	{SLI_NWP_IND_TX_STATUS, wifi_ncp_tx_status},
};

void wifi_ncp_sta_rx(const struct device *dev, const uint8_t *frame, size_t len)
{
	const struct wifi_ncp_nwp_hdr *hdr = (const struct wifi_ncp_nwp_hdr *)frame;
	const uint8_t *payload = frame + sizeof(struct wifi_ncp_nwp_hdr);
	size_t payload_len = len - sizeof(struct wifi_ncp_nwp_hdr);

	if (len < sizeof(struct wifi_ncp_nwp_hdr)) {
		LOG_WRN("%s: short management frame (%u bytes)", dev->name, len);
		return;
	}
	/* No extended descriptor is expected between the descriptor and the
	 * payload
	 */
	if (frame[WIFI_NCP_NWP_DESC_EXT_SIZE]) {
		LOG_WRN("%s: frame 0x%02x with an extended descriptor of %u bytes", dev->name,
			hdr->command_id, frame[WIFI_NCP_NWP_DESC_EXT_SIZE]);
	}

	if (wifi_ncp_cmd_confirm(dev, hdr, payload, payload_len)) {
		return;
	}

	for (size_t i = 0; i < ARRAY_SIZE(wifi_ncp_rx_handlers); i++) {
		if (wifi_ncp_rx_handlers[i].id == hdr->command_id) {
			wifi_ncp_rx_handlers[i].fn(dev, frame, payload, payload_len);
			return;
		}
	}
	LOG_WRN("%s: unhandled frame 0x%02x, status 0x%04x, %u bytes", dev->name, hdr->command_id,
		wifi_ncp_nwp_status(hdr), payload_len);
}

int wifi_ncp_sta_scan(void *if_priv, struct wpa_driver_scan_params *params)
{
	const struct device *dev = if_priv;
	struct wifi_ncp_data *data = dev->data;
	struct wifi_ncp_sta *sta = &data->sta;
	struct wifi_ncp_nwp_hdr hdr = {.command_id = SLI_WIFI_CMD_SCAN};
	sli_mgmtif_request_scan_t req;
	int ret;

	memset(&req, 0, sizeof(req));
	/* A single frequency restricts the scan, anything else scans all */
	if (params->freqs && params->freqs[0] && !params->freqs[1]) {
		req.scan_req.channel[0] = wifi_utils_freq_to_chan(params->freqs[0]);
	}
	if (params->num_ssids > 0 && params->ssids[0].ssid_len > 0 &&
	    params->ssids[0].ssid_len < SLI_WIFI_SSID_LEN) {
		memcpy(req.scan_req.ssid, params->ssids[0].ssid, params->ssids[0].ssid_len);
	}
	req.scan_req.power_level = SLI_WIFI_TX_POWER_DECIDBM_MAX;

	k_mutex_lock(&sta->lock, K_FOREVER);
	if (sta->scanning) {
		k_mutex_unlock(&sta->lock);
		return -EBUSY;
	}
	for (size_t i = 0; i < ARRAY_SIZE(sta->scan); i++) {
		sta->scan[i].used = false;
	}
	sta->scanning = true;
	k_mutex_unlock(&sta->lock);

	/* The confirmation comes once the scan is over */
	ret = wifi_ncp_nwp_send(dev, &hdr, &req, sizeof(req), NULL, 0);
	if (ret < 0) {
		k_mutex_lock(&sta->lock, K_FOREVER);
		sta->scanning = false;
		k_mutex_unlock(&sta->lock);
	}

	return ret;
}

int wifi_ncp_sta_scan_abort(void *if_priv)
{
	const struct device *dev = if_priv;

	/* The co-processor completes the scan on its own */
	LOG_DBG("%s: scan abort ignored", dev->name);

	return 0;
}

/* Replay the kept results to the supplicant. The results have to be reported
 * here and not during the scan: driver_zephyr.c drops them until this call.
 */
int wifi_ncp_sta_get_scan_results(void *if_priv)
{
	const struct device *dev = if_priv;
	struct wifi_ncp_data *data = dev->data;
	struct wifi_ncp_sta *sta = &data->sta;
	struct wpa_scan_res *res = (struct wpa_scan_res *)sta->scratch;
	size_t count = 0, sent = 0;

	if (!data->supp.if_ctx || !data->supp.cb.scan_res) {
		return -EINVAL;
	}

	k_mutex_lock(&sta->lock, K_FOREVER);
	for (size_t i = 0; i < ARRAY_SIZE(sta->scan); i++) {
		if (sta->scan[i].used) {
			count++;
		}
	}
	if (!count) {
		k_mutex_unlock(&sta->lock);
		data->supp.if_ctx->scan_res2_get_in_prog = false;
		k_sem_give(&data->supp.if_ctx->drv_resp_sem);
		return 0;
	}

	for (size_t i = 0; i < ARRAY_SIZE(sta->scan); i++) {
		const struct wifi_ncp_scan_entry *entry = &sta->scan[i];

		if (!entry->used) {
			continue;
		}
		memset(res, 0, sizeof(*res));
		memcpy(res->bssid, entry->bssid, NET_ETH_ADDR_LEN);
		res->freq = wifi_utils_chan_to_freq(wifi_utils_chan_to_band(entry->channel),
						    entry->channel);
		res->beacon_int = entry->beacon_int;
		res->caps = entry->caps;
		/* The signal level is not reported by the co-processor */
		res->level = 0;
		res->noise = -95;
		res->ie_len = entry->ie_len;
		memcpy(res + 1, entry->ie, entry->ie_len);
		sent++;
		data->supp.cb.scan_res(data->supp.if_ctx, res, sent < count);
	}
	k_mutex_unlock(&sta->lock);

	return 0;
}

static const uint8_t *wifi_ncp_ie_find(const uint8_t *ies, size_t len, uint8_t eid)
{
	size_t off = 0;

	while (off + 2 <= len && off + 2 + ies[off + 1] <= len) {
		if (ies[off] == eid) {
			return &ies[off];
		}
		off += 2 + ies[off + 1];
	}

	return NULL;
}

static const uint8_t *wifi_ncp_ie_find_ext(const uint8_t *ies, size_t len, uint8_t ext_eid)
{
	size_t off = 0;

	while (off + 2 <= len && off + 2 + ies[off + 1] <= len) {
		if (ies[off] == WLAN_EID_EXTENSION && ies[off + 1] >= 1 &&
		    ies[off + 2] == ext_eid) {
			return &ies[off];
		}
		off += 2 + ies[off + 1];
	}

	return NULL;
}

static const uint8_t *wifi_ncp_ie_find_vendor(const uint8_t *ies, size_t len, const uint8_t *oui,
					      uint8_t type)
{
	size_t off = 0;

	while (off + 2 <= len && off + 2 + ies[off + 1] <= len) {
		if (ies[off] == WLAN_EID_VENDOR_SPECIFIC && ies[off + 1] >= 4 &&
		    !memcmp(&ies[off + 2], oui, 3) && ies[off + 5] == type) {
			return &ies[off];
		}
		off += 2 + ies[off + 1];
	}

	return NULL;
}

/* Describe the selected BSS to the co-processor, from the beacon kept during
 * the scan. This is what the WiseConnect management interface appends to the
 * join request.
 */
static void wifi_ncp_join_ap_info(sli_wifi_sta_join_ap_info_t *info,
				  const struct wifi_ncp_scan_entry *entry, uint8_t sec_mode)
{
	static const uint8_t wmm_oui[] = {0x00, 0x50, 0xf2};
	/* First member of the packed structure, hence aligned */
	sli_wifi_scan_result_t *res = (sli_wifi_scan_result_t *)info;
	const uint8_t *ie;
	struct wpa_ie_data rsn;

	memset(info, 0, sizeof(*info));
	res->channel_num = entry->channel;
	res->sec_mode = sec_mode;
	memcpy(res->bssid, entry->bssid, NET_ETH_ADDR_LEN);
	res->beacon_interval = entry->beacon_int;
	res->capability_info = entry->caps;
	info->ap_rrm_capable = (entry->caps & WLAN_CAPABILITY_RADIO_MEASUREMENT) ? 1 : 0;

	ie = wifi_ncp_ie_find(entry->ie, entry->ie_len, WLAN_EID_SSID);
	if (ie && ie[1] < sizeof(res->ssid)) {
		res->ssid_len = ie[1];
		memcpy(res->ssid, &ie[2], ie[1]);
	}
	ie = wifi_ncp_ie_find(entry->ie, entry->ie_len, WLAN_EID_SUPP_RATES);
	if (ie) {
		res->supported_rates.rates_len = MIN(ie[1], SLI_WIFI_RATES_LEN);
		res->supported_rates.rates_eid = ie[0];
		memcpy(res->supported_rates.rates, &ie[2], res->supported_rates.rates_len);
	}
	ie = wifi_ncp_ie_find(entry->ie, entry->ie_len, WLAN_EID_EXT_SUPP_RATES);
	if (ie) {
		res->supported_rates.ext_rates_len = MIN(ie[1], SLI_WIFI_RATES_LEN);
		memcpy(res->supported_rates.ext_rates, &ie[2], res->supported_rates.ext_rates_len);
	}
	ie = wifi_ncp_ie_find(entry->ie, entry->ie_len, WLAN_EID_HT_CAP);
	if (ie && 2 + ie[1] <= sizeof(res->ht_cap_ie)) {
		memcpy(res->ht_cap_ie, ie, 2 + ie[1]);
		res->ht_cap_present = 1;
		res->capable_11n = 1;
	}
	ie = wifi_ncp_ie_find(entry->ie, entry->ie_len, WLAN_EID_HT_OPERATION);
	if (ie && 2 + ie[1] <= sizeof(res->ht_oper_ie)) {
		memcpy(res->ht_oper_ie, ie, 2 + ie[1]);
		res->ht_oper_present = 1;
	}
	ie = wifi_ncp_ie_find_vendor(entry->ie, entry->ie_len, wmm_oui, WMM_OUI_TYPE);
	if (ie && ie[1] >= 6) {
		res->vendor_ie.eid = ie[0];
		res->vendor_ie.eid_len = ie[1];
		memcpy(res->vendor_ie.oui, &ie[2], sizeof(res->vendor_ie.oui));
		res->vendor_ie.oui_type = ie[5];
		memcpy(&res->vendor_ie.wmm, &ie[6], MIN(ie[1] - 4, sizeof(res->vendor_ie.wmm)));
		res->qos_enable = 1;
	}
	ie = wifi_ncp_ie_find(entry->ie, entry->ie_len, WLAN_EID_RSN);
	if (ie && !wpa_parse_wpa_ie_rsn(ie, 2 + ie[1], &rsn)) {
		/* Cipher suite selector of the group cipher */
		if (rsn.group_cipher & WPA_CIPHER_CCMP) {
			res->group_cipher = RSN_CIPHER_SUITE_CCMP & 0xff;
		} else if (rsn.group_cipher & WPA_CIPHER_TKIP) {
			res->group_cipher = RSN_CIPHER_SUITE_TKIP & 0xff;
		}
		res->mfpc = (rsn.capabilities & WPA_CAPABILITY_MFPC) ? 1 : 0;
	}
	res->vht_capable = wifi_ncp_ie_find(entry->ie, entry->ie_len, WLAN_EID_VHT_CAP) ? 1 : 0;
	res->he_capable =
		wifi_ncp_ie_find_ext(entry->ie, entry->ie_len, WLAN_EID_EXT_HE_CAPABILITIES) ? 1
											     : 0;
	ie = wifi_ncp_ie_find(entry->ie, entry->ie_len, WLAN_EID_MOBILITY_DOMAIN);
	if (ie && 2 + ie[1] == sizeof(info->mdie)) {
		memcpy(&info->mdie, ie, sizeof(info->mdie));
	}
}

static uint8_t wifi_ncp_security_type(unsigned int key_mgmt)
{
	if (key_mgmt & WPA_KEY_MGMT_SAE) {
		return SL_WIFI_WPA3;
	}
	if ((key_mgmt & (WPA_KEY_MGMT_PSK | WPA_KEY_MGMT_PSK_SHA256)) != 0) {
		return SL_WIFI_WPA2;
	}
	return SL_WIFI_OPEN;
}

/* Load the RSN element of the association request, then join. The join is
 * confirmed later, once the co-processor has authenticated and associated.
 */
int wifi_ncp_sta_associate(void *if_priv, struct wpa_driver_associate_params *params)
{
	const struct device *dev = if_priv;
	struct wifi_ncp_data *data = dev->data;
	struct wifi_ncp_sta *sta = &data->sta;
	struct wifi_ncp_nwp_hdr load_hdr = {.command_id = SLI_MGMTIF_CMD_LOAD_IE_INFO};
	struct wifi_ncp_nwp_hdr join_hdr = {.command_id = SLI_WIFI_CMD_JOIN};
	sli_wifi_sta_join_ap_info_t *ap_info = (sli_wifi_sta_join_ap_info_t *)sta->scratch;
	const struct wifi_ncp_scan_entry *entry = NULL;
	sli_mgmtif_load_ie_t load = { };
	sli_mgmtif_join_request_t join = { };
	int ret;

	if (!params->bssid || !params->ssid || params->ssid_len > WIFI_SSID_MAX_LEN ||
	    params->wpa_ie_len > WIFI_NCP_WPA_IE_MAX) {
		return -EINVAL;
	}

	k_mutex_lock(&sta->lock, K_FOREVER);
	if (sta->state != WIFI_NCP_STA_IDLE) {
		k_mutex_unlock(&sta->lock);
		return -EBUSY;
	}
	for (size_t i = 0; i < ARRAY_SIZE(sta->scan); i++) {
		if (sta->scan[i].used &&
		    !memcmp(sta->scan[i].bssid, params->bssid, NET_ETH_ADDR_LEN)) {
			entry = &sta->scan[i];
			break;
		}
	}
	if (!entry) {
		k_mutex_unlock(&sta->lock);
		LOG_ERR("%s: BSS not found in the scan results", dev->name);
		return -ENOENT;
	}

	memcpy(sta->bssid, params->bssid, NET_ETH_ADDR_LEN);
	memcpy(sta->ssid, params->ssid, params->ssid_len);
	sta->ssid_len = params->ssid_len;
	sta->channel = entry->channel;
	sta->beacon_int = entry->beacon_int;
	sta->key_mgmt = params->key_mgmt_suite;
	sta->wpa_ie_len = params->wpa_ie_len;
	if (params->wpa_ie_len > 0) {
		memcpy(sta->wpa_ie, params->wpa_ie, params->wpa_ie_len);
	}

	load.frame_type = SLI_WIFI_IE_ASSOC_REQ;
	load.ie_length = MIN(params->wpa_ie_len, SLI_WIFI_MAX_IE_SIZE);
	memcpy(load.ie_data, params->wpa_ie, load.ie_length);

	join.join_req.security_type = wifi_ncp_security_type(params->key_mgmt_suite);
	join.join_req.power_level = SLI_WIFI_TX_POWER_DECIDBM_MAX;
	memcpy(join.join_req.ssid, params->ssid, params->ssid_len);
	join.join_req.ssid_len = params->ssid_len;
	join.join_req.listen_interval_multiplier = SLI_WIFI_LISTEN_INTERVAL_MULTIPLIER_DEFAULT;
	join.join_req.vap_id = SLI_WIFI_STA_MODE;
	memcpy(join.join_req.join_bssid, params->bssid, NET_ETH_ADDR_LEN);
	join.join_ext_info.vif_type = SLI_NWP_OP_STATION;
	memcpy(join.join_ext_info.bssid, params->bssid, NET_ETH_ADDR_LEN);
	join.join_ext_info.wps_session = SLI_WIFI_WPS_SESSION_DISABLE;
	join.join_ext_info.sae_pmksa_caching = SLI_WIFI_SAE_PMKSA_CACHING_DISABLE;
	wifi_ncp_join_ap_info(ap_info, entry, join.join_req.security_type);
	sta->state = WIFI_NCP_STA_JOINING;
	k_mutex_unlock(&sta->lock);

	ret = wifi_ncp_nwp_cmd(dev, &load_hdr, &load, sizeof(load), NULL, 0, NULL, NULL);
	if (!ret) {
		ret = wifi_ncp_nwp_send(dev, &join_hdr, &join, sizeof(join), ap_info,
					sizeof(*ap_info));
	} else if (ret > 0) {
		LOG_ERR("%s: load IE failed: 0x%04x", dev->name, ret);
		ret = -EIO;
	}
	if (ret < 0) {
		k_mutex_lock(&sta->lock, K_FOREVER);
		sta->state = WIFI_NCP_STA_IDLE;
		k_mutex_unlock(&sta->lock);
	}

	return ret;
}

int wifi_ncp_sta_deauthenticate(void *if_priv, const char *addr, unsigned short reason_code)
{
	const struct device *dev = if_priv;
	struct wifi_ncp_data *data = dev->data;
	struct wifi_ncp_sta *sta = &data->sta;
	struct wifi_ncp_nwp_hdr hdr = {.command_id = SLI_WIFI_CMD_DISCONNECT};
	sli_mgmtif_disconnect_req_t req;
	int ret;

	ARG_UNUSED(addr);
	ARG_UNUSED(reason_code);

	k_mutex_lock(&sta->lock, K_FOREVER);
	sta->state = WIFI_NCP_STA_IDLE;
	k_mutex_unlock(&sta->lock);
	net_if_dormant_on(data->iface);

	memset(&req, 0, sizeof(req));
	req.disconn_req.mode_flag = SLI_WIFI_STA_MODE;
	req.host_initiated = SLI_WIFI_DISCONN_FROM_HOST;
	ret = wifi_ncp_nwp_cmd(dev, &hdr, &req, sizeof(req), NULL, 0, NULL, NULL);
	if (ret > 0) {
		/* Not connected, for instance */
		LOG_DBG("%s: disconnect: 0x%04x", dev->name, ret);
		ret = 0;
	}

	return ret;
}

int wifi_ncp_sta_set_key(void *if_priv, const unsigned char *ifname, enum wpa_alg alg,
			 const unsigned char *addr, int key_idx, int set_tx,
			 const unsigned char *seq, size_t seq_len, const unsigned char *key,
			 size_t key_len, enum key_flag key_flag)
{
	const struct device *dev = if_priv;
	struct wifi_ncp_nwp_hdr hdr = {.command_id = SLI_MGMTIF_CMD_SET_KEYS};
	sli_mgmtif_set_keys_t keys;
	int ret;

	ARG_UNUSED(ifname);
	ARG_UNUSED(addr);
	ARG_UNUSED(set_tx);
	ARG_UNUSED(seq);
	ARG_UNUSED(seq_len);

	/* Key removal is implied by the disconnection */
	if (alg == WPA_ALG_NONE) {
		return 0;
	}
	if (!key || key_len > SLI_WIFI_MAX_KEY_LEN || alg > 0xf) {
		return -EINVAL;
	}

	memset(&keys, 0, sizeof(keys));
	/* The high nibble is the algorithm, aligned with enum wpa_alg */
	keys.type = (alg << 4) | ((key_flag & KEY_FLAG_PAIRWISE) ? SLI_WIFI_KEY_TYPE_PTK
								 : SLI_WIFI_KEY_TYPE_GTK);
	keys.key_id = key_idx & 0x3;
	memcpy(keys.key, key, key_len);
	keys.key_len = key_len;
	/* DW1.B3: station interface */
	hdr.reserved2[WIFI_NCP_NWP_DESC_IFACE - 3] = SLI_WIFI_STA_MODE;

	ret = wifi_ncp_nwp_cmd(dev, &hdr, &keys, sizeof(keys), NULL, 0, NULL, NULL);
	if (ret > 0) {
		LOG_ERR("%s: set key failed: 0x%04x", dev->name, ret);
		return -EIO;
	}

	return ret;
}

/* The supplicant opens the port once the key exchange is over */
int wifi_ncp_sta_set_supp_port(void *if_priv, int authorized, char *bssid)
{
	const struct device *dev = if_priv;
	struct wifi_ncp_data *data = dev->data;

	ARG_UNUSED(bssid);

	if (authorized) {
		LOG_INF("%s: connected", dev->name);
		net_if_dormant_off(data->iface);
	} else {
		net_if_dormant_on(data->iface);
	}

	return 0;
}

/* EAPOL frames go to the co-processor as 802.11 data frames. The
 * confirmation means the frame was acknowledged.
 */
int wifi_ncp_sta_tx_control_port(void *if_priv, const unsigned char *dest, unsigned short proto,
				 const unsigned char *buf, size_t len, int no_encrypt)
{
	const struct device *dev = if_priv;
	struct wifi_ncp_data *data = dev->data;
	struct wifi_ncp_sta *sta = &data->sta;
	struct wifi_ncp_nwp_hdr hdr = {.command_id = SLI_WIFI_TX_DOT11_MGMT_FRAME};
	struct net_linkaddr *own = net_if_get_link_addr(data->iface);
	static const uint8_t llc_snap[] = { 0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00 };
	uint8_t head[IEEE80211_HDRLEN + sizeof(llc_snap) + sizeof(uint16_t)];
	struct ieee80211_hdr *mac = (struct ieee80211_hdr *)head;
	uint8_t *llc = &head[IEEE80211_HDRLEN];
	int ret;

	ARG_UNUSED(no_encrypt);

	if (!own || own->len != NET_ETH_ADDR_LEN) {
		return -EINVAL;
	}

	memset(head, 0, sizeof(head));
	mac->frame_control =
		sys_cpu_to_le16(IEEE80211_FC(WLAN_FC_TYPE_DATA, WLAN_FC_STYPE_DATA) | WLAN_FC_TODS);
	memcpy(mac->addr1, sta->bssid, NET_ETH_ADDR_LEN);
	memcpy(mac->addr2, own->addr, NET_ETH_ADDR_LEN);
	memcpy(mac->addr3, dest, NET_ETH_ADDR_LEN);
	memcpy(llc, llc_snap, sizeof(llc_snap));
	sys_put_be16(proto, llc + sizeof(llc_snap));

	ret = wifi_ncp_nwp_cmd(dev, &hdr, head, sizeof(head), buf, len, NULL, NULL);
	if (ret > 0) {
		LOG_ERR("%s: EAPOL transmission failed: 0x%04x", dev->name, ret);
		return -EIO;
	}

	return ret;
}

/* The signal level is not reported by the co-processor */
int wifi_ncp_sta_signal_poll(void *if_priv, struct wpa_signal_info *si, unsigned char *bssid)
{
	const struct device *dev = if_priv;
	struct wifi_ncp_data *data = dev->data;
	struct wifi_ncp_sta *sta = &data->sta;

	memset(si, 0, sizeof(*si));
	si->frequency = wifi_utils_chan_to_freq(wifi_utils_chan_to_band(sta->channel),
						sta->channel);
	si->current_noise = -95;
	si->data.signal = 0;
	if (bssid) {
		memcpy(bssid, sta->bssid, NET_ETH_ADDR_LEN);
	}

	return 0;
}

int wifi_ncp_sta_get_conn_info(void *if_priv, struct wpa_conn_info *info)
{
	const struct device *dev = if_priv;
	struct wifi_ncp_data *data = dev->data;

	memset(info, 0, sizeof(*info));
	info->beacon_interval = data->sta.beacon_int;

	return 0;
}

/* Authentication frames built by the supplicant for the SAE exchange. The
 * co-processor is told when the commit is sent, and the confirmation of the
 * transmission is reported as an acknowledgment.
 */
int wifi_ncp_sta_send_mlme(void *if_priv, const u8 *data_buf, size_t data_len, int noack,
			   unsigned int freq, int no_cck, int offchanok, unsigned int wait_time,
			   int cookie)
{
	const struct device *dev = if_priv;
	struct wifi_ncp_data *data = dev->data;
	struct wifi_ncp_nwp_hdr tx_hdr = {.command_id = SLI_WIFI_TX_DOT11_MGMT_FRAME};
	struct wifi_ncp_nwp_hdr commit_hdr = {.command_id = SLI_MGMTIF_CMD_SAE_COMMIT_STATUS};
	const struct ieee80211_mgmt *mgmt = (const struct ieee80211_mgmt *)data_buf;
	sli_mgmtif_sae_commit_t commit = SLI_WIFI_SAE_COMMIT_STATE_TRUE;
	uint16_t fc;
	int ret;

	ARG_UNUSED(noack);
	ARG_UNUSED(freq);
	ARG_UNUSED(no_cck);
	ARG_UNUSED(offchanok);
	ARG_UNUSED(wait_time);
	ARG_UNUSED(cookie);

	if (data_len < IEEE80211_HDRLEN + sizeof(mgmt->u.auth)) {
		return -EINVAL;
	}
	fc = sys_le16_to_cpu(mgmt->frame_control);
	if (WLAN_FC_GET_TYPE(fc) != WLAN_FC_TYPE_MGMT ||
	    WLAN_FC_GET_STYPE(fc) != WLAN_FC_STYPE_AUTH) {
		LOG_WRN("%s: only authentication frames can be sent", dev->name);
		return -ENOTSUP;
	}

	ret = wifi_ncp_nwp_cmd(dev, &tx_hdr, data_buf, data_len, NULL, 0, NULL, NULL);
	if (ret) {
		LOG_ERR("%s: authentication frame transmission failed: %d", dev->name, ret);
		return -EIO;
	}

	if (sys_le16_to_cpu(mgmt->u.auth.auth_transaction) == 1) {
		ret = wifi_ncp_nwp_cmd(dev, &commit_hdr, &commit, sizeof(commit), NULL, 0, NULL,
				       NULL);
		if (ret) {
			LOG_ERR("%s: SAE commit status failed: %d", dev->name, ret);
			return -EIO;
		}
	}

	if (data->supp.if_ctx && data->supp.cb.mgmt_tx_status) {
		data->supp.cb.mgmt_tx_status(data->supp.if_ctx, data_buf, data_len, true);
	}

	return 0;
}

/* The SAE exchange is over: on success the co-processor associates, which
 * confirms the join.
 */
int wifi_ncp_sta_send_external_auth_status(void *if_priv, struct external_auth *params)
{
	const struct device *dev = if_priv;
	struct wifi_ncp_nwp_hdr hdr = {.command_id = SLI_MGMTIF_CMD_SAE_CONFIRM_SUCCESS};
	int ret;

	if (params->status != WLAN_STATUS_SUCCESS) {
		LOG_WRN("%s: SAE authentication failed: %u", dev->name, params->status);
		return 0;
	}

	ret = wifi_ncp_nwp_cmd(dev, &hdr, NULL, 0, NULL, 0, NULL, NULL);
	if (ret) {
		LOG_ERR("%s: SAE confirm success failed: %d", dev->name, ret);
		return -EIO;
	}

	return 0;
}

/* The co-processor is reachable: initialize the station and learn its MAC
 * address.
 */
int wifi_ncp_sta_init(const struct device *dev)
{
	struct wifi_ncp_data *data = dev->data;
	struct wifi_ncp_nwp_hdr hdr = {.command_id = SLI_WIFI_CMD_INIT};
	uint8_t resp[2 * NET_ETH_ADDR_LEN];
	size_t resp_size = sizeof(resp);
	int ret;

	ret = wifi_ncp_nwp_cmd(dev, &hdr, NULL, 0, NULL, 0, resp, &resp_size);
	if (ret < 0) {
		return ret;
	}
	if (ret) {
		LOG_WRN("%s: init failed: 0x%04x", dev->name, ret);
		return 0;
	}
	if (resp_size < NET_ETH_ADDR_LEN) {
		LOG_WRN("%s: init returned no MAC address", dev->name);
		return 0;
	}
	memcpy(data->mac, resp, NET_ETH_ADDR_LEN);
	LOG_INF("%s: MAC %02x:%02x:%02x:%02x:%02x:%02x", dev->name, data->mac[0], data->mac[1],
		data->mac[2], data->mac[3], data->mac[4], data->mac[5]);

	return 0;
}
