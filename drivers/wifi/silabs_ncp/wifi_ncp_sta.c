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

	k_mutex_lock(&sta->scan_lock, K_FOREVER);
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
	k_mutex_unlock(&sta->scan_lock);
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

	k_mutex_lock(&sta->scan_lock, K_FOREVER);
	if (!sta->scanning) {
		k_mutex_unlock(&sta->scan_lock);
		LOG_WRN("%s: unexpected scan confirmation (0x%04x)", dev->name, status);
		return;
	}
	sta->scanning = false;
	k_mutex_unlock(&sta->scan_lock);

	LOG_DBG("%s: scan done: 0x%04x", dev->name, status);
	if (data->supp.if_ctx && data->supp.cb.scan_done) {
		memset(&event, 0, sizeof(event));
		data->supp.cb.scan_done(data->supp.if_ctx, &event);
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

/* Confirmations of the deferred commands and indications */
static const struct {
	uint8_t id;
	void (*fn)(const struct device *dev, const uint8_t *hdr, const uint8_t *frame, size_t len);
} wifi_ncp_rx_handlers[] = {
	{SLI_WIFI_IND_ON_AIR_MGMT, wifi_ncp_scan_add},
	{SLI_WIFI_CMD_SCAN, wifi_ncp_scan_done},
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

	k_mutex_lock(&sta->scan_lock, K_FOREVER);
	if (sta->scanning) {
		k_mutex_unlock(&sta->scan_lock);
		return -EBUSY;
	}
	for (size_t i = 0; i < ARRAY_SIZE(sta->scan); i++) {
		sta->scan[i].used = false;
	}
	sta->scanning = true;
	k_mutex_unlock(&sta->scan_lock);

	/* The confirmation comes once the scan is over */
	ret = wifi_ncp_nwp_send(dev, &hdr, &req, sizeof(req), NULL, 0);
	if (ret < 0) {
		k_mutex_lock(&sta->scan_lock, K_FOREVER);
		sta->scanning = false;
		k_mutex_unlock(&sta->scan_lock);
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

	k_mutex_lock(&sta->scan_lock, K_FOREVER);
	for (size_t i = 0; i < ARRAY_SIZE(sta->scan); i++) {
		if (sta->scan[i].used) {
			count++;
		}
	}
	if (!count) {
		k_mutex_unlock(&sta->scan_lock);
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
	k_mutex_unlock(&sta->scan_lock);

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
