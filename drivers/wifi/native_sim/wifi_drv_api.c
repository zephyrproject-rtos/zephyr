/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/wifi.h>
#include <zephyr/net/wifi_mgmt.h>

#include "wifi_drv_api.h"
#include "internal.h"
#include "wifi_drv_priv.h"

#include "common/ieee802_11_common.h"
#include "drivers/driver_zephyr.h"
#include "drivers/driver.h"
#include "wpa_supplicant/driver_i.h"
#include "wpa_supplicant/scan.h"
#include "rsn_supp/wpa.h"

LOG_MODULE_DECLARE(wifi_native_sim, CONFIG_WIFI_LOG_LEVEL);

void *wifi_drv_init(void *supp_drv_if_ctx, const char *iface_name,
		    struct zep_wpa_supp_dev_callbk_fns *callbacks)
{
	static bool started;
	struct wifi_context *ctx;
	const struct device *dev;
	struct net_if *iface;

	iface = net_if_get_by_index(net_if_get_by_name(iface_name));
	if (iface == NULL) {
		LOG_ERR("Network interface %s not found", iface_name);
		return NULL;
	}

	dev = net_if_get_device(iface);
	if (dev == NULL) {
		LOG_ERR("Device for %s not found", iface_name);
		return NULL;
	}

	ctx = dev->data;
	ctx->supplicant_drv_ctx = supp_drv_if_ctx;
	memcpy(&ctx->supplicant_callbacks, callbacks, sizeof(ctx->supplicant_callbacks));

	if (!started) {
		ctx->host_context = host_wifi_drv_init(ctx->supplicant_drv_ctx,
						       ctx->if_name_host, 0,
						       CONFIG_WIFI_NATIVE_SIM_SUPPLICANT_CONF_FILE,
						       CONFIG_WIFI_NATIVE_SIM_SUPPLICANT_LOG_FILE,
						       CONFIG_WIFI_NM_WPA_SUPPLICANT_DEBUG_LEVEL);
		if (ctx->host_context == NULL) {
			LOG_ERR("Cannot start host handler thread!");
			return NULL;
		}

		/* Read end of the host event pipe, polled by the RX thread. */
		ctx->event_fd = host_wifi_drv_get_event_fd(ctx->host_context);

		started = true;
	}


	LOG_DBG("iface %s [%d] dev %s (%p)", ctx->name, ctx->if_index, dev->name, dev);

	return ctx;
}

void wifi_drv_deinit(void *if_priv)
{
	struct wifi_context *ctx = if_priv;

	LOG_DBG("iface %s [%d] dev %s (%p)", ctx->name, ctx->if_index,
		net_if_get_device(ctx->iface)->name, net_if_get_device(ctx->iface));

	host_wifi_drv_deinit(ctx->host_context);

	ctx->supplicant_drv_ctx = NULL;
}

/* Called by the Zephyr RX thread for each event datagram received from the
 * host nl80211 driver over the event pipe. Runs in a Zephyr thread context so
 * it may invoke the supplicant callbacks (which post to the supplicant thread).
 */
void wifi_drv_event_dispatch(struct wifi_context *ctx, void *buf, size_t len)
{
	struct host_wifi_event hdr;
	const uint8_t *p = buf;

	if (len < sizeof(hdr)) {
		return;
	}

	memcpy(&hdr, p, sizeof(hdr));
	p += sizeof(hdr);
	len -= sizeof(hdr);

	switch (hdr.event) {
	case EVENT_SCAN_RESULTS: {
		union wpa_event_data event = {0};
		struct scan_info *info = &event.scan_info;

		/* Scans triggered by the supplicant itself (e.g. as part of a
		 * connect) must be reported as non-external, otherwise the
		 * supplicant refuses to use the results for network selection.
		 * Only scans started via the Wi-Fi mgmt API ("wifi scan") are
		 * external to the supplicant.
		 */
		info->aborted = false;
		info->external_scan = ctx->external_scan ? 1 : 0;
		info->nl_scan_event = 1;

		ctx->supplicant_callbacks.scan_done(ctx->supplicant_drv_ctx, &event);
		ctx->scan_in_progress = false;
		break;
	}
	case EVENT_AUTH: {
		struct host_event_auth a;
		union wpa_event_data event = {0};

		if (len < sizeof(a)) {
			break;
		}

		memcpy(&a, p, sizeof(a));
		p += sizeof(a);
		len -= sizeof(a);

		memcpy(event.auth.peer, a.peer, sizeof(event.auth.peer));
		memcpy(event.auth.bssid, a.peer, sizeof(event.auth.bssid));
		event.auth.auth_type = a.auth_type;
		event.auth.auth_transaction = a.auth_transaction;
		event.auth.status_code = a.status_code;
		if (a.ies_len && a.ies_len <= len) {
			event.auth.ies = p;
			event.auth.ies_len = a.ies_len;
		}

		ctx->supplicant_callbacks.auth_resp(ctx->supplicant_drv_ctx, &event);
		break;
	}
	case EVENT_ASSOC: {
		struct host_event_assoc a;
		union wpa_event_data event = {0};

		if (len < sizeof(a)) {
			break;
		}

		memcpy(&a, p, sizeof(a));
		p += sizeof(a);
		len -= sizeof(a);

		event.assoc_info.addr = a.addr;
		event.assoc_info.freq = a.freq;

		if (a.req_ies_len && a.req_ies_len <= len) {
			event.assoc_info.req_ies = p;
			event.assoc_info.req_ies_len = a.req_ies_len;
			p += a.req_ies_len;
			len -= a.req_ies_len;
		}

		if (a.resp_ies_len && a.resp_ies_len <= len) {
			event.assoc_info.resp_ies = p;
			event.assoc_info.resp_ies_len = a.resp_ies_len;
		}

		ctx->supplicant_callbacks.assoc_resp(ctx->supplicant_drv_ctx, &event,
						     WLAN_STATUS_SUCCESS);

		/* Association succeeded: bring the interface out of dormant so
		 * the data path is usable. The 4-way handshake EAPOL frames are
		 * sent over l2_packet, which requires the interface to be up.
		 */
		net_if_dormant_off(ctx->iface);
		break;
	}
	case EVENT_ASSOC_REJECT: {
		struct host_event_assoc_reject a;
		union wpa_event_data event = {0};

		if (len < sizeof(a)) {
			break;
		}

		memcpy(&a, p, sizeof(a));
		p += sizeof(a);
		len -= sizeof(a);

		event.assoc_reject.bssid = a.bssid;
		event.assoc_reject.status_code = a.status_code;
		if (a.resp_ies_len && a.resp_ies_len <= len) {
			event.assoc_reject.resp_ies = p;
			event.assoc_reject.resp_ies_len = a.resp_ies_len;
		}

		ctx->supplicant_callbacks.assoc_resp(ctx->supplicant_drv_ctx, &event,
						     a.status_code);
		break;
	}
	case EVENT_DEAUTH: {
		struct host_event_deauth d;
		union wpa_event_data event = {0};
		struct ieee80211_mgmt mgmt = {0};

		if (len < sizeof(d)) {
			break;
		}

		memcpy(&d, p, sizeof(d));
		p += sizeof(d);
		len -= sizeof(d);

		event.deauth_info.addr = d.addr;
		event.deauth_info.reason_code = d.reason_code;
		event.deauth_info.locally_generated = d.locally_generated;
		if (d.ie_len && d.ie_len <= len) {
			event.deauth_info.ie = p;
			event.deauth_info.ie_len = d.ie_len;
		}

		/* The deauth consumer reads mgmt->bssid; the nl80211 disconnect
		 * path has no frame, so synthesize one carrying the AP address.
		 */
		memcpy(mgmt.bssid, d.addr, sizeof(mgmt.bssid));

		ctx->supplicant_callbacks.deauth(ctx->supplicant_drv_ctx, &event, &mgmt);

		/* Link down: mark the interface dormant again. */
		net_if_dormant_on(ctx->iface);
		break;
	}
	case EVENT_DISASSOC: {
		struct host_event_deauth d;
		union wpa_event_data event = {0};

		if (len < sizeof(d)) {
			break;
		}

		memcpy(&d, p, sizeof(d));
		p += sizeof(d);
		len -= sizeof(d);

		event.disassoc_info.addr = d.addr;
		event.disassoc_info.reason_code = d.reason_code;
		event.disassoc_info.locally_generated = d.locally_generated;
		if (d.ie_len && d.ie_len <= len) {
			event.disassoc_info.ie = p;
			event.disassoc_info.ie_len = d.ie_len;
		}

		ctx->supplicant_callbacks.disassoc(ctx->supplicant_drv_ctx, &event);

		/* Link down: mark the interface dormant again. */
		net_if_dormant_on(ctx->iface);
		break;
	}
	case EVENT_RX_MGMT: {
		struct host_event_rx_mgmt m;

		if (len < sizeof(m)) {
			break;
		}

		memcpy(&m, p, sizeof(m));
		p += sizeof(m);
		len -= sizeof(m);

		if (m.frame_len && m.frame_len <= len) {
			ctx->supplicant_callbacks.mgmt_rx(ctx->supplicant_drv_ctx,
							  (char *)p, m.frame_len,
							  m.freq, m.signal);
		}
		break;
	}
	default:
		LOG_DBG("Unhandled host event %u", hdr.event);
		break;
	}
}

static enum wifi_mfp_options get_mfp(struct wpa_scan_res *res)
{
	struct wpa_ie_data data;
	const uint8_t *ie;
	int ret;

	ie = wpa_scan_get_ie(res, WLAN_EID_RSN);
	if (ie == NULL) {
		return WIFI_MFP_DISABLE;
	}

	ret = wpa_parse_wpa_ie(ie, 2 + ie[1], &data);
	if (ret < 0) {
		return WIFI_MFP_UNKNOWN;
	}

	if (data.capabilities & WPA_CAPABILITY_MFPR) {
		return WIFI_MFP_REQUIRED;
	}

	if (data.capabilities & WPA_CAPABILITY_MFPC) {
		return WIFI_MFP_OPTIONAL;
	}

	return WIFI_MFP_DISABLE;
}

static int get_key_mgmt(struct wpa_scan_res *res)
{
	struct wpa_ie_data data;
	const uint8_t *ie;
	int ret;

	ie = wpa_scan_get_ie(res, WLAN_EID_RSN);
	if (ie == NULL) {
		return WPA_KEY_MGMT_NONE;
	}

	ret = wpa_parse_wpa_ie(ie, 2 + ie[1], &data);
	if (ret < 0) {
		return -EINVAL;
	}

	return data.key_mgmt;
}

static enum wifi_security_type get_security_type(struct wpa_scan_res *res)
{
	int key_mgmt;

	key_mgmt = get_key_mgmt(res);
	if (key_mgmt < 0) {
		LOG_DBG("Cannot get key mgmt (%d)", key_mgmt);
		return WIFI_SECURITY_TYPE_UNKNOWN;
	}

	if (wpa_key_mgmt_wpa_ieee8021x(key_mgmt)) {
		return WIFI_SECURITY_TYPE_EAP;

	} else if (key_mgmt & WPA_KEY_MGMT_PSK) {
		return WIFI_SECURITY_TYPE_PSK;

	} else if (key_mgmt & WPA_KEY_MGMT_NONE) {
		return WIFI_SECURITY_TYPE_NONE;

	} else if (key_mgmt & WPA_KEY_MGMT_IEEE8021X_NO_WPA) {
		return WIFI_SECURITY_TYPE_UNKNOWN;

	} else if (key_mgmt & WPA_KEY_MGMT_WPA_NONE) {
		return WIFI_SECURITY_TYPE_UNKNOWN;

	} else if (key_mgmt & WPA_KEY_MGMT_FT_PSK) {
		return WIFI_SECURITY_TYPE_UNKNOWN;

	} else if (key_mgmt & WPA_KEY_MGMT_PSK_SHA256) {
		return WIFI_SECURITY_TYPE_PSK_SHA256;

	} else if (key_mgmt & WPA_KEY_MGMT_WPS) {
		return WIFI_SECURITY_TYPE_UNKNOWN;

	} else if (key_mgmt & WPA_KEY_MGMT_SAE) {
		return WIFI_SECURITY_TYPE_SAE;

	} else if (key_mgmt & WPA_KEY_MGMT_FT_SAE) {
		return WIFI_SECURITY_TYPE_UNKNOWN;

	} else if (key_mgmt & WPA_KEY_MGMT_WAPI_PSK) {
		return WIFI_SECURITY_TYPE_WAPI;

	} else if (key_mgmt & WPA_KEY_MGMT_WAPI_CERT) {
		return WIFI_SECURITY_TYPE_WAPI;

	} else if (key_mgmt & WPA_KEY_MGMT_OWE) {
		return WIFI_SECURITY_TYPE_UNKNOWN;

	} else if (key_mgmt & WPA_KEY_MGMT_DPP) {
		return WIFI_SECURITY_TYPE_UNKNOWN;

	} else if (key_mgmt & WPA_KEY_MGMT_PASN) {
		return WIFI_SECURITY_TYPE_UNKNOWN;
	}

	return WIFI_SECURITY_TYPE_UNKNOWN;
}

static void my_scan_results(struct wifi_context *ctx,
			    struct wpa_scan_results *results)
{
	struct wifi_scan_result result;
	struct wpa_scan_res *res;
	int i;

	LOG_DBG("%d results", results->num);

	for (i = 0; ctx->scan_cb != NULL && i < results->num; i++) {
		const uint8_t *ssid;

		memset(&result, 0, sizeof(result));

		res = results->res[i];
		ssid = wpa_scan_get_ie(res, WLAN_EID_SSID);
		if (ssid == NULL) {
			continue;
		}

		result.ssid_length = MIN(sizeof(result.ssid), ssid[1]);
		memcpy(result.ssid, ssid + 2, result.ssid_length);

		memcpy(result.mac, res->bssid, sizeof(result.mac));
		result.mac_length = WIFI_MAC_ADDR_LEN;

		(void)ieee80211_freq_to_chan(res->freq, &result.channel);

		result.security = get_security_type(res);
		result.rssi = res->level;
		result.mfp = get_mfp(res);

		ctx->scan_cb(ctx->iface, 0, &result);
	}

	/* Mark the scan done */
	if (ctx->scan_cb != NULL) {
		ctx->scan_cb(ctx->iface, 0, NULL);
	} else {
		struct wifi_status scan_status = { 0 };

		net_mgmt_event_notify_with_info(NET_EVENT_WIFI_SCAN_DONE,
						ctx->iface, &scan_status,
						sizeof(struct wifi_status));
	}
}

int wifi_drv_scan2(void *if_priv, struct wpa_driver_scan_params *params)
{
	struct wifi_context *ctx = if_priv;
	int ret;

	LOG_DBG("iface %s [%d] dev %s (%p)", ctx->name, ctx->if_index,
		net_if_get_device(ctx->iface)->name, net_if_get_device(ctx->iface));

	/* Assume the scan was initiated by the supplicant. The Wi-Fi mgmt
	 * path (wifi_scan()) overrides this after calling us.
	 */
	ctx->external_scan = false;

	/* Start the host scan. Completion arrives asynchronously as a real
	 * EVENT_SCAN_RESULTS from the nl80211 driver (see wifi_drv_event_dispatch).
	 */
	ret = host_wifi_drv_scan2(ctx->host_context, (void *)params);

	/* Tell the Zephyr supplicant driver that the scan has been started. */
	ctx->supplicant_callbacks.scan_start(ctx->supplicant_drv_ctx);

	return ret;
}

int wifi_drv_scan_abort(void *if_priv)
{
	struct wifi_context *ctx = if_priv;

	LOG_DBG("iface %s [%d] dev %s (%p)", ctx->name, ctx->if_index,
		net_if_get_device(ctx->iface)->name, net_if_get_device(ctx->iface));

	return host_wifi_drv_scan_abort(ctx->host_context);
}

int wifi_drv_get_scan_results2(void *if_priv)
{
	struct wifi_context *ctx = if_priv;
	struct wpa_scan_results *results;
	int i;

	LOG_DBG("iface %s [%d] dev %s (%p)", ctx->name, ctx->if_index,
		net_if_get_device(ctx->iface)->name, net_if_get_device(ctx->iface));

	/* Get the scan results from nl80211 driver */
	results = (struct wpa_scan_results *)host_wifi_drv_get_scan_results(ctx->host_context);
	if (results == NULL) {
		LOG_DBG("No scan results.");
		return -ENOENT;
	}

	/* Convert the scan result from nl80211 driver to how zephyr uses them */
	for (i = 0; i < results->num; i++) {
		ctx->supplicant_callbacks.scan_res(ctx->supplicant_drv_ctx,
						   results->res[i], false);
	}

	/* Deliver the results to any registered Wi-Fi mgmt scan callback. This
	 * may run for a scan not started through our scan2() (e.g. a boot-time
	 * or externally triggered scan), so call the handler directly rather
	 * than through a stored pointer that may not be set.
	 */
	my_scan_results(ctx, results);

	host_wifi_drv_free_scan_results(results);

	return 0;
}

int wifi_drv_deauthenticate(void *if_priv, const char *addr, unsigned short reason_code)
{
	struct wifi_context *ctx = if_priv;

	LOG_DBG("iface %s [%d] dev %s (%p)", ctx->name, ctx->if_index,
		net_if_get_device(ctx->iface)->name, net_if_get_device(ctx->iface));

	return host_wifi_drv_deauthenticate(ctx->host_context, addr, reason_code);
}

int wifi_drv_authenticate(void *if_priv, struct wpa_driver_auth_params *params,
			  struct wpa_bss *curr_bss)
{
	struct wifi_context *ctx = if_priv;

	LOG_DBG("iface %s [%d] dev %s (%p)", ctx->name, ctx->if_index,
		net_if_get_device(ctx->iface)->name, net_if_get_device(ctx->iface));

	return host_wifi_drv_authenticate(ctx->host_context, params, curr_bss);
}

int wifi_drv_associate(void *if_priv, struct wpa_driver_associate_params *params)
{
	struct wifi_context *ctx = if_priv;

	LOG_DBG("iface %s [%d] dev %s (%p)", ctx->name, ctx->if_index,
		net_if_get_device(ctx->iface)->name, net_if_get_device(ctx->iface));

	return host_wifi_drv_associate(ctx->host_context, params);
}

int wifi_drv_set_key(void *if_priv, const unsigned char *ifname, enum wpa_alg alg,
		     const unsigned char *addr, int key_idx, int set_tx,
		     const unsigned char *seq, size_t seq_len,
		     const unsigned char *key, size_t key_len, enum key_flag key_flag)
{
	struct wifi_context *ctx = if_priv;

	LOG_DBG("iface %s [%d] dev %s (%p)", ctx->name, ctx->if_index,
		net_if_get_device(ctx->iface)->name, net_if_get_device(ctx->iface));

	return host_wifi_drv_set_key(ctx->host_context, ifname, alg, addr, key_idx, set_tx,
				     seq, seq_len, key, key_len, key_flag);
}

int wifi_drv_set_supp_port(void *if_priv, int authorized, char *bssid)
{
	struct wifi_context *ctx = if_priv;

	LOG_DBG("iface %s [%d] dev %s (%p) authorized %d", ctx->name, ctx->if_index,
		net_if_get_device(ctx->iface)->name, net_if_get_device(ctx->iface),
		authorized);

	return host_wifi_drv_set_supp_port(ctx->host_context, authorized, bssid);
}

int wifi_drv_signal_poll(void *if_priv, struct wpa_signal_info *si, unsigned char *bssid)
{
	struct wifi_context *ctx = if_priv;

	LOG_DBG("iface %s [%d] dev %s (%p)", ctx->name, ctx->if_index,
		net_if_get_device(ctx->iface)->name, net_if_get_device(ctx->iface));

	return host_wifi_drv_signal_poll(ctx->host_context, si, bssid);
}

int wifi_drv_send_mlme(void *if_priv, const u8 *data, size_t data_len, int noack, unsigned int freq,
		       int no_cck, int offchanok, unsigned int wait_time, int cookie)
{
	struct wifi_context *ctx = if_priv;

	LOG_DBG("iface %s [%d] dev %s (%p)", ctx->name, ctx->if_index,
		net_if_get_device(ctx->iface)->name, net_if_get_device(ctx->iface));

	return host_wifi_drv_send_mlme(ctx->host_context, data, data_len, noack, freq,
				       no_cck, offchanok, wait_time, cookie);
}

int wifi_drv_get_wiphy(void *priv)
{
	struct wpa_supp_event_supported_band *band = NULL;
	struct wifi_context *ctx = priv;
	int ret, i, count;

	LOG_DBG("iface %s [%d] dev %s (%p)", ctx->name, ctx->if_index,
		net_if_get_device(ctx->iface)->name, net_if_get_device(ctx->iface));

	ret = host_wifi_drv_get_wiphy(ctx->host_context, (void **)&band, &count);
	if (ret < 0) {
		return ret;
	}

	if (band == NULL) {
		return -ENOENT;
	}

	for (i = 0; i < count; i++) {
		ctx->supplicant_callbacks.get_wiphy_res(ctx->supplicant_drv_ctx, &band[i]);
	}

	ctx->supplicant_callbacks.get_wiphy_res(ctx->supplicant_drv_ctx, NULL);

	host_wifi_drv_free_bands(band);

	return 0;
}

int wifi_drv_register_frame(void *priv, u16 type, const u8 *match, size_t match_len,
			    bool multicast)
{
	struct wifi_context *ctx = priv;

	LOG_DBG("iface %s [%d] dev %s (%p)", ctx->name, ctx->if_index,
		net_if_get_device(ctx->iface)->name, net_if_get_device(ctx->iface));

	return host_wifi_drv_register_frame(ctx->host_context, type, match, match_len, multicast);
}

int wifi_drv_get_capa(void *if_priv, struct wpa_driver_capa *capa)
{
	struct wifi_context *ctx = if_priv;

	LOG_DBG("iface %s [%d] dev %s (%p)", ctx->name, ctx->if_index,
		net_if_get_device(ctx->iface)->name, net_if_get_device(ctx->iface));

	return host_wifi_drv_get_capa(ctx->host_context, capa);
}

int wifi_drv_get_conn_info(void *if_priv, struct wpa_conn_info *info)
{
	struct wifi_context *ctx = if_priv;

	LOG_DBG("iface %s [%d] dev %s (%p)", ctx->name, ctx->if_index,
		net_if_get_device(ctx->iface)->name, net_if_get_device(ctx->iface));

	return host_wifi_drv_get_conn_info(ctx->host_context, info);
}

/* In the driver-only host model the Zephyr supplicant owns the connection
 * state machine and drives it through the authenticate/associate ops. The
 * connect/disconnect routing is reworked in a later stage; keep these as
 * no-ops for now so the build is complete.
 */
int wifi_drv_connect(void *if_priv, struct wifi_connect_req_params *params)
{
	struct wifi_context *ctx = if_priv;

	ARG_UNUSED(params);

	LOG_DBG("iface %s [%d] dev %s (%p)", ctx->name, ctx->if_index,
		net_if_get_device(ctx->iface)->name, net_if_get_device(ctx->iface));

	return 0;
}

int wifi_drv_disconnect(void *if_priv)
{
	struct wifi_context *ctx = if_priv;

	LOG_DBG("iface %s [%d] dev %s (%p)", ctx->name, ctx->if_index,
		net_if_get_device(ctx->iface)->name, net_if_get_device(ctx->iface));

	return host_wifi_drv_disconnect(ctx->host_context);
}
