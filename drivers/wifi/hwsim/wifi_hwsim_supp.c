/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * hwsim wpa_supplicant driver ops. Builds only when CONFIG_WIFI_NM_WPA_SUPPLICANT.
 * Frames are relayed via the medium; this layer implements the Zephyr wpa_supp
 * driver API so the NM (wpa_supplicant) can drive STA/AP.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/net/net_if.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

#include "wifi_hwsim.h"
#include <drivers/driver_zephyr.h>
#include <string.h>

LOG_MODULE_DECLARE(wifi_hwsim, CONFIG_WIFI_HWSIM_LOG_LEVEL);

#define SSID_IE_ID   0
#define HWSIM_2G_BASE_FREQ_MHZ 2407

/*
 * Beacon frame layout: 24-byte 802.11 MAC header + fixed fields
 * (timestamp 8, beacon interval 2, capability info 2) followed by the
 * variable IEs. Offsets into the captured beacon (start_ap) used to recover
 * the capability field and the variable IE region for scan results.
 */
#define HWSIM_BEACON_CAPS_OFF  (24 + 8 + 2)
#define HWSIM_BEACON_FIXED_LEN (24 + 8 + 2 + 2)

/* Dedicated work queue for scan_done so the wpa_supp callback chain
 * (get_scan_results2, etc.) has enough stack; sysworkq is too small.
 */
#define HWSIM_SUPP_SCAN_WQ_STACK_SIZE 8192
static K_THREAD_STACK_DEFINE(hwsim_supp_scan_wq_stack, HWSIM_SUPP_SCAN_WQ_STACK_SIZE);
static struct k_work_q hwsim_supp_scan_wq;
static atomic_t hwsim_supp_scan_wq_started;

struct hwsim_supp_ctx {
	struct zep_drv_if_ctx *if_ctx;
	struct zep_wpa_supp_dev_callbk_fns callbk_fns;
	struct k_work scan_done_work;
	atomic_t scan_aborted;
};

static struct hwsim_radio *hwsim_dev_to_radio(const struct device *dev)
{
	return (struct hwsim_radio *)dev->data;
}

static struct hwsim_radio *hwsim_iface_to_radio(struct net_if *iface)
{
	return hwsim_dev_to_radio(net_if_get_device(iface));
}

static void hwsim_scan_done_work_fn(struct k_work *work);

/* Called from main RX path for 802.11 mgmt frames when supp_ctx is set. */
void hwsim_supp_mgmt_rx(struct hwsim_radio *radio, const uint8_t *data,
			unsigned int len, unsigned int freq_mhz, int rssi_dbm)
{
	struct hwsim_supp_ctx *ctx = (struct hwsim_supp_ctx *)radio->supp_ctx;
	const struct ieee80211_mgmt *m = (const struct ieee80211_mgmt *)data;
	const size_t auth_hdr_len =
		offsetof(struct ieee80211_mgmt, u.auth.variable);
	union wpa_event_data ev;
	uint16_t fc;

	if (ctx == NULL) {
		return;
	}

	/*
	 * On the AP, hand received management frames (a STA's SAE commit/confirm
	 * Authentication frames) straight to the hostapd / supplicant-AP MLME.
	 */
	if (radio->ap_mode) {
		if (ctx->callbk_fns.mgmt_rx != NULL) {
			ctx->callbk_fns.mgmt_rx(ctx->if_ctx, (char *)data, (int)len,
						(int)freq_mhz, rssi_dbm);
		}
		return;
	}

	/*
	 * On the STA, the SME expects authentication results as EVENT_AUTH (it
	 * drives auth via the authenticate op), not raw frames. The only
	 * management frame the AP relays back is the SAE Authentication reply,
	 * so translate it into an auth_resp event.
	 */
	if (len < auth_hdr_len || ctx->callbk_fns.auth_resp == NULL) {
		return;
	}
	fc = le_to_host16(m->frame_control);
	if (WLAN_FC_GET_TYPE(fc) != WLAN_FC_TYPE_MGMT ||
	    WLAN_FC_GET_STYPE(fc) != WLAN_FC_STYPE_AUTH) {
		return;
	}

	memset(&ev, 0, sizeof(ev));
	memcpy(ev.auth.peer, m->sa, ETH_ALEN);
	ev.auth.auth_type = le_to_host16(m->u.auth.auth_alg);
	ev.auth.auth_transaction = le_to_host16(m->u.auth.auth_transaction);
	ev.auth.status_code = le_to_host16(m->u.auth.status_code);
	if (len > auth_hdr_len) {
		ev.auth.ies = m->u.auth.variable;
		ev.auth.ies_len = len - auth_hdr_len;
	}
	ctx->callbk_fns.auth_resp(ctx->if_ctx, &ev);
}

static void *hwsim_supp_init(void *supp_drv_if_ctx, const char *iface_name,
			     struct zep_wpa_supp_dev_callbk_fns *callbk_fns)
{
	struct zep_drv_if_ctx *if_ctx = (struct zep_drv_if_ctx *)supp_drv_if_ctx;
	struct net_if *iface = if_ctx->iface;
	struct hwsim_radio *radio;
	struct hwsim_supp_ctx *ctx;

	ARG_UNUSED(iface_name);

	radio = hwsim_iface_to_radio(iface);
	if (radio == NULL) {
		return NULL;
	}

	ctx = k_malloc(sizeof(*ctx));
	if (ctx == NULL) {
		return NULL;
	}

	ctx->if_ctx = if_ctx;
	ctx->callbk_fns = *callbk_fns;
	atomic_set(&ctx->scan_aborted, 0);
	k_work_init(&ctx->scan_done_work, hwsim_scan_done_work_fn);
	radio->supp_ctx = ctx;
	if_ctx->dev_priv = radio;
	if_ctx->dev_ctx = net_if_get_device(iface);

	/* Start the shared scan work queue exactly once, even if interfaces are
	 * initialized concurrently.
	 */
	if (atomic_cas(&hwsim_supp_scan_wq_started, 0, 1)) {
		k_work_queue_init(&hwsim_supp_scan_wq);
		k_work_queue_start(&hwsim_supp_scan_wq, hwsim_supp_scan_wq_stack,
				   K_THREAD_STACK_SIZEOF(hwsim_supp_scan_wq_stack),
				   K_PRIO_PREEMPT(10), NULL);
	}

	return radio;
}

static void hwsim_supp_deinit(void *if_priv)
{
	struct hwsim_radio *radio = (struct hwsim_radio *)if_priv;
	struct hwsim_supp_ctx *ctx;
	struct k_work_sync sync;

	if (radio == NULL || radio->supp_ctx == NULL) {
		return;
	}
	ctx = (struct hwsim_supp_ctx *)radio->supp_ctx;
	(void)k_work_cancel_sync(&ctx->scan_done_work, &sync);
	k_free(ctx);
	radio->supp_ctx = NULL;
}

static void hwsim_scan_done_work_fn(struct k_work *work)
{
	struct hwsim_supp_ctx *ctx = CONTAINER_OF(work, struct hwsim_supp_ctx,
						 scan_done_work);
	union wpa_event_data event;

	if (ctx->callbk_fns.scan_done == NULL) {
		return;
	}
	memset(&event, 0, sizeof(event));
	event.scan_info.aborted = (atomic_get(&ctx->scan_aborted) != 0);
	ctx->callbk_fns.scan_done(ctx->if_ctx, &event);
}

/*
 * Scan has no over-the-air step; results come from other hwsim radios in AP
 * mode. Defer scan_done to the dedicated work queue so the supplicant's
 * get_scan_results2 chain runs on a thread with enough stack (avoids stuck
 * scan / sem wait when done synchronously from the caller's thread).
 */
static int hwsim_supp_scan2(void *if_priv, struct wpa_driver_scan_params *params)
{
	struct hwsim_radio *radio = (struct hwsim_radio *)if_priv;
	struct hwsim_supp_ctx *ctx = (struct hwsim_supp_ctx *)radio->supp_ctx;

	ARG_UNUSED(params);

	if (ctx == NULL) {
		return -EINVAL;
	}
	atomic_set(&ctx->scan_aborted, 0);
	k_work_submit_to_queue(&hwsim_supp_scan_wq, &ctx->scan_done_work);
	return 0;
}

static int hwsim_supp_scan_abort(void *if_priv)
{
	struct hwsim_radio *radio = (struct hwsim_radio *)if_priv;
	struct hwsim_supp_ctx *ctx = (struct hwsim_supp_ctx *)radio->supp_ctx;

	if (ctx != NULL) {
		atomic_set(&ctx->scan_aborted, 1);
		k_work_submit_to_queue(&hwsim_supp_scan_wq, &ctx->scan_done_work);
	}
	return 0;
}

static int hwsim_supp_get_scan_results2(void *if_priv)
{
	struct hwsim_radio *radio = (struct hwsim_radio *)if_priv;
	struct hwsim_supp_ctx *ctx = (struct hwsim_supp_ctx *)radio->supp_ctx;
	struct zep_drv_if_ctx *if_ctx;
	struct wpa_scan_results *scan_res;
	struct wpa_scan_res **res_ptrs;
	size_t n = 0;
	size_t i, j;

	if (ctx == NULL) {
		return -1;
	}
	if_ctx = ctx->if_ctx;
	scan_res = if_ctx->scan_res2;
	if (scan_res == NULL) {
		if_ctx->scan_res2_get_in_prog = false;
		k_sem_give(&if_ctx->drv_resp_sem);
		return -1;
	}

	/* Count other radios in AP mode (beacon source = visible BSS). */
	for (i = 0; i < HWSIM_MAX_RADIOS; i++) {
		struct hwsim_radio *r = hwsim_medium_get_radio((uint8_t)i);

		if (r != NULL && r->ap_mode && r->idx != radio->idx) {
			n++;
		}
	}

	scan_res->num = n;
	scan_res->res = NULL;
	os_get_reltime(&scan_res->fetch_time);

	if (n == 0) {
		if_ctx->scan_res2_get_in_prog = false;
		k_sem_give(&if_ctx->drv_resp_sem);
		return 0;
	}

	res_ptrs = os_zalloc(n * sizeof(struct wpa_scan_res *));
	if (res_ptrs == NULL) {
		if_ctx->scan_res2_get_in_prog = false;
		k_sem_give(&if_ctx->drv_resp_sem);
		return -1;
	}
	scan_res->res = res_ptrs;

	j = 0;
	for (i = 0; i < HWSIM_MAX_RADIOS && j < n; i++) {
		struct hwsim_radio *r = hwsim_medium_get_radio((uint8_t)i);
		struct wpa_scan_res *entry;
		uint8_t *ie_ptr;

		if (r == NULL || !r->ap_mode || r->idx == radio->idx) {
			continue;
		}

		/*
		 * A beacon frame is: 24-byte 802.11 header + 12 bytes of fixed
		 * fields (timestamp, beacon interval, capability info) followed
		 * by the variable IEs (SSID, rates, DSSS, RSN, RSNXE, ...). The
		 * AP's real beacon (captured in start_ap) carries the security
		 * IEs, so surface those verbatim as the scan-result IEs; without
		 * the RSN IE the STA treats the AP as open and a WPA2/WPA3
		 * connect fails to match. Fall back to a synthetic open beacon
		 * if the AP has not published a beacon yet.
		 */
		{
			const uint8_t *bcn_ies = NULL;
			size_t bcn_ies_len = 0;
			uint16_t caps = IEEE80211_CAP_ESS;

			if (r->beacon_ie_len > HWSIM_BEACON_FIXED_LEN) {
				bcn_ies = r->beacon_ie + HWSIM_BEACON_FIXED_LEN;
				bcn_ies_len = r->beacon_ie_len - HWSIM_BEACON_FIXED_LEN;
				caps = sys_get_le16(&r->beacon_ie[HWSIM_BEACON_CAPS_OFF]);
			}

			if (bcn_ies != NULL) {
				entry = os_zalloc(sizeof(*entry) + bcn_ies_len);
				if (entry == NULL) {
					goto err_free;
				}
				memcpy(entry + 1, bcn_ies, bcn_ies_len);
				entry->ie_len = bcn_ies_len;
			} else {
				size_t ie_len = 2 + r->ssid_len; /* SSID IE */

				ie_len += 6; /* Supported Rates: 1,2,5.5,11 Mbps */
				ie_len += 3; /* DSSS Parameter Set: channel */

				entry = os_zalloc(sizeof(*entry) + ie_len);
				if (entry == NULL) {
					goto err_free;
				}

				ie_ptr = (uint8_t *)(entry + 1);

				/* SSID IE */
				ie_ptr[0] = 0x00;
				ie_ptr[1] = (uint8_t)r->ssid_len;
				memcpy(&ie_ptr[2], r->ssid, r->ssid_len);

				/* Supported Rates IE: 1,2,5.5,11 Mbps */
				ie_ptr[2 + r->ssid_len] = 0x01;
				ie_ptr[2 + r->ssid_len + 1] = 4;
				ie_ptr[2 + r->ssid_len + 2] = 0x82;  /* 1(B) Mbps */
				ie_ptr[2 + r->ssid_len + 3] = 0x84;  /* 2(B) Mbps */
				ie_ptr[2 + r->ssid_len + 4] = 0x8b;  /* 5.5(B) Mbps */
				ie_ptr[2 + r->ssid_len + 5] = 0x96;  /* 11(B) Mbps */

				/* DSSS Parameter Set IE: current channel */
				ie_ptr[2 + r->ssid_len + 6] = 0x03;
				ie_ptr[2 + r->ssid_len + 7] = 1;
				ie_ptr[2 + r->ssid_len + 8] = (uint8_t)r->channel;

				entry->ie_len = ie_len;
			}
			entry->beacon_ie_len = 0;
			entry->caps = caps;
		}

		entry->flags = 0;
		memcpy(entry->bssid, r->bssid, ETH_ALEN);
		entry->freq = HWSIM_2G_BASE_FREQ_MHZ + (r->channel * 5);
		entry->max_cw = CHAN_WIDTH_20_NOHT;
		entry->beacon_int = 100;
		entry->qual = 0;
		entry->noise = -95;
		entry->level = r->tx_power_dbm;
		entry->tsf = 0;
		entry->age = 0;
		entry->est_throughput = 0;
		entry->snr = entry->level - entry->noise;

		res_ptrs[j++] = entry;
	}

	if_ctx->scan_res2_get_in_prog = false;
	k_sem_give(&if_ctx->drv_resp_sem);
	return 0;

err_free:
	for (i = 0; i < j; i++) {
		os_free(res_ptrs[i]);
	}
	os_free(res_ptrs);
	scan_res->res = NULL;
	scan_res->num = 0;
	if_ctx->scan_res2_get_in_prog = false;
	k_sem_give(&if_ctx->drv_resp_sem);
	return -1;
}

static int hwsim_supp_deauthenticate(void *if_priv, const char *addr,
				     unsigned short reason_code)
{
	ARG_UNUSED(if_priv);
	ARG_UNUSED(addr);
	ARG_UNUSED(reason_code);
	return 0;
}

static int hwsim_supp_authenticate(void *if_priv,
				   struct wpa_driver_auth_params *params,
				   struct wpa_bss *curr_bss)
{
	struct hwsim_radio *radio = (struct hwsim_radio *)if_priv;
	struct hwsim_supp_ctx *ctx = (struct hwsim_supp_ctx *)radio->supp_ctx;
	union wpa_event_data ev;

	ARG_UNUSED(curr_bss);

	if (ctx == NULL || params == NULL || params->bssid == NULL) {
		return -EINVAL;
	}

	/* Retune this STA radio to the target AP's channel so the medium will
	 * relay frames between them (the medium only bridges radios on the same
	 * frequency).
	 */
	if (params->freq > 0) {
		radio->freq_mhz = (uint32_t)params->freq;
		radio->channel = (uint8_t)((params->freq - HWSIM_2G_BASE_FREQ_MHZ) / 5);
	}

	/*
	 * WPA3-SAE performs a real cryptographic commit/confirm exchange that
	 * the AP validates in its frame-based MLME (handle_auth), with no event
	 * shortcut. Build the 802.11 Authentication frame from the SME-provided
	 * auth_data (which begins at the transaction-sequence field) and relay
	 * it over the medium; the AP's reply returns through the RX path and is
	 * turned into an EVENT_AUTH there. Open-system auth carries no crypto,
	 * so report success immediately.
	 */
	if (params->auth_alg & WPA_AUTH_ALG_SAE) {
		struct net_linkaddr *lla = net_if_get_link_addr(radio->iface);
		const size_t hdr_len =
			offsetof(struct ieee80211_mgmt, u.auth.auth_transaction);
		struct ieee80211_mgmt *m;
		void *buf;
		size_t len;
		int ret;

		if (lla == NULL || params->auth_data == NULL) {
			return -EINVAL;
		}
		/* 24-byte header + auth algorithm (2) + auth_data. */
		len = hdr_len + params->auth_data_len;
		if (len > HWSIM_FRAME_MTU) {
			return -EMSGSIZE;
		}
		/* Build in a slab entry rather than a large (HWSIM_FRAME_MTU)
		 * stack buffer; this runs on the supplicant thread's stack.
		 */
		if (k_mem_slab_alloc(&hwsim_frame_slab, &buf, K_NO_WAIT) != 0) {
			return -ENOMEM;
		}
		m = (struct ieee80211_mgmt *)buf;
		memset(m, 0, hdr_len);
		m->frame_control = IEEE80211_FC(WLAN_FC_TYPE_MGMT, WLAN_FC_STYPE_AUTH);
		memcpy(m->da, params->bssid, ETH_ALEN);
		memcpy(m->sa, lla->addr, ETH_ALEN);
		memcpy(m->bssid, params->bssid, ETH_ALEN);
		m->u.auth.auth_alg = host_to_le16(WLAN_AUTH_SAE);
		memcpy((uint8_t *)buf + hdr_len, params->auth_data,
		       params->auth_data_len);
		ret = hwsim_medium_xmit(radio->idx, buf, (uint16_t)len, true);
		k_mem_slab_free(&hwsim_frame_slab, buf);
		return ret;
	}

	if (ctx->callbk_fns.auth_resp == NULL) {
		return 0;
	}

	/* Open-system authentication: no over-the-air crypto, report success. */
	os_memset(&ev, 0, sizeof(ev));
	os_memcpy(ev.auth.peer, params->bssid, ETH_ALEN);
	ev.auth.auth_type = WLAN_AUTH_OPEN;
	ev.auth.status_code = WLAN_STATUS_SUCCESS;
	ev.auth.ies = NULL;
	ev.auth.ies_len = 0;
	ctx->callbk_fns.auth_resp(ctx->if_ctx, &ev);
	return 0;
}

static int hwsim_supp_init_ap(void *if_priv,
			      struct wpa_driver_associate_params *params)
{
	struct hwsim_radio *radio = (struct hwsim_radio *)if_priv;
	struct net_linkaddr *lla;

	if (params == NULL || params->ssid == NULL) {
		return -EINVAL;
	}
	if (params->ssid_len > WIFI_SSID_MAX_LEN) {
		return -EINVAL;
	}
	lla = net_if_get_link_addr(radio->iface);
	if (lla == NULL || lla->len < ETH_ALEN) {
		return -EINVAL;
	}
	memcpy(radio->bssid, lla->addr, ETH_ALEN);
	radio->ssid_len = (uint8_t)params->ssid_len;
	memcpy(radio->ssid, params->ssid, params->ssid_len);
	if (params->freq.channel > 0) {
		radio->channel = (uint8_t)params->freq.channel;
	} else if (params->freq.freq > 0) {
		radio->channel = (uint8_t)((params->freq.freq - HWSIM_2G_BASE_FREQ_MHZ)
					   / 5);
	} else {
		radio->channel = 1;
	}
	radio->freq_mhz = HWSIM_2G_BASE_FREQ_MHZ + (radio->channel * 5);
	radio->ap_mode = true;
	return 0;
}

static int hwsim_supp_deinit_ap(void *if_priv)
{
	struct hwsim_radio *radio = (struct hwsim_radio *)if_priv;

	radio->ap_mode = false;
	radio->beacon_ie_len = 0;
	return 0;
}

/* Store beacon IEs from wpa_supp for use in get_scan_results2. */
static int hwsim_supp_start_ap(void *if_priv, struct wpa_driver_ap_params *params)
{
	struct hwsim_radio *radio = (struct hwsim_radio *)if_priv;
	size_t total;

	/* AP is ready to serve clients. */
	net_if_carrier_on(radio->iface);

	if (params == NULL) {
		return -EINVAL;
	}
	radio->beacon_ie_len = 0;
	total = (params->head_len + params->tail_len);
	if (total > HWSIM_BEACON_IE_MAX) {
		total = HWSIM_BEACON_IE_MAX;
	}
	if (total > 0) {
		if (params->head != NULL && params->head_len > 0) {
			size_t copy = params->head_len;

			if (copy > HWSIM_BEACON_IE_MAX) {
				copy = HWSIM_BEACON_IE_MAX;
			}
			memcpy(radio->beacon_ie, params->head, copy);
			radio->beacon_ie_len = copy;
		}
		if (params->tail != NULL && params->tail_len > 0) {
			size_t tail_copy = params->tail_len;
			size_t remain = HWSIM_BEACON_IE_MAX - radio->beacon_ie_len;

			if (tail_copy > remain) {
				tail_copy = remain;
			}
			if (tail_copy > 0) {
				memcpy(radio->beacon_ie + radio->beacon_ie_len,
				       params->tail, tail_copy);
				radio->beacon_ie_len += tail_copy;
			}
		}
	}
	return 0;
}

/*
 * Find the AP-mode radio whose BSSID matches @bssid. Used to relay a STA's
 * association to the peer AP's supplicant/hostapd context.
 */
static struct hwsim_radio *hwsim_find_ap_radio(const uint8_t *bssid)
{
	for (size_t i = 0; i < HWSIM_MAX_RADIOS; i++) {
		struct hwsim_radio *r = hwsim_medium_get_radio((uint8_t)i);

		if (r != NULL && r->ap_mode &&
		    memcmp(r->bssid, bssid, ETH_ALEN) == 0) {
			return r;
		}
	}
	return NULL;
}

/*
 * Relay an association to the peer AP by injecting an EVENT_ASSOC into its
 * supplicant context, mimicking what firmware would report on a real driver.
 * The AP's (Re)Assoc-Request IEs are the STA's own RSN/WPA IE (params->wpa_ie),
 * from which hostapd_notif_assoc() creates the station and, for a secured
 * network, starts the authenticator (4-way handshake, msg 1/4). No 802.11
 * (Re)Association frame is built: params cross the medium directly.
 */
static void hwsim_relay_assoc_to_ap(struct hwsim_radio *ap_radio,
				    const uint8_t *sta_mac,
				    const uint8_t *req_ies, size_t req_ies_len)
{
	struct hwsim_supp_ctx *ap_ctx = (struct hwsim_supp_ctx *)ap_radio->supp_ctx;
	union wpa_event_data ev;

	if (ap_ctx == NULL || ap_ctx->callbk_fns.assoc_resp == NULL) {
		return;
	}
	memset(&ev, 0, sizeof(ev));
	ev.assoc_info.addr = sta_mac;
	ev.assoc_info.req_ies = req_ies;
	ev.assoc_info.req_ies_len = req_ies_len;
	ap_ctx->callbk_fns.assoc_resp(ap_ctx->if_ctx, &ev, WLAN_STATUS_SUCCESS);
}

static int hwsim_supp_associate(void *if_priv,
			       struct wpa_driver_associate_params *params)
{
	struct hwsim_radio *radio = (struct hwsim_radio *)if_priv;
	struct hwsim_supp_ctx *ctx = (struct hwsim_supp_ctx *)radio->supp_ctx;
	struct net_linkaddr *lla = net_if_get_link_addr(radio->iface);
	bool secured;
	union wpa_event_data ev;

	if (ctx == NULL || params == NULL || params->bssid == NULL) {
		return -EINVAL;
	}
	if (ctx->callbk_fns.assoc_resp == NULL) {
		return 0;
	}

	/* A non-empty RSN/WPA IE means the connection is secured (WPA2-PSK /
	 * WPA3-SAE), so the 4-way handshake must run before the port opens.
	 */
	secured = (params->wpa_ie != NULL && params->wpa_ie_len > 0);

	radio->associated = true;
	memcpy(radio->ap_bssid, params->bssid, ETH_ALEN);

	/* Kick the Zephyr stack to recognize data-link ready. */
	net_if_carrier_on(radio->iface);

	/*
	 * Report this STA's own association up first (so its EAPOL port is
	 * enabled) before telling the AP, so EAPOL msg 1/4 does not arrive
	 * before the supplicant is ready for it.
	 */
	memset(&ev, 0, sizeof(ev));
	ev.assoc_info.addr = params->bssid;
	ev.assoc_info.req_ies = params->wpa_ie;
	ev.assoc_info.req_ies_len = params->wpa_ie_len;
	ev.assoc_info.freq = (unsigned int)params->freq.freq; /* Operating freq so
							       * wpa_s->assoc_freq (and
							       * status channel) is correct.
							       */
	/*
	 * Open network: no EAPOL, so mark the port authorized here to reach
	 * WPA_COMPLETED directly. Secured network: leave it 0 so the host
	 * 4-way handshake runs and opens the port on success.
	 */
	ev.assoc_info.authorized = secured ? 0 : 1;
	ctx->callbk_fns.assoc_resp(ctx->if_ctx, &ev, WLAN_STATUS_SUCCESS);

	/*
	 * For a secured link, relay the association to the peer AP so it starts
	 * the authenticator. (Open links complete on the STA alone; leave the
	 * existing behavior untouched.)
	 */
	if (secured && lla != NULL) {
		struct hwsim_radio *ap_radio = hwsim_find_ap_radio(params->bssid);

		if (ap_radio != NULL) {
			hwsim_relay_assoc_to_ap(ap_radio, lla->addr,
						params->wpa_ie, params->wpa_ie_len);
		} else {
			LOG_WRN("associate: no AP radio for BSSID");
		}
	}
	return 0;
}

static int hwsim_supp_set_supp_port(void *if_priv, int authorized, char *bssid)
{
	ARG_UNUSED(if_priv);
	ARG_UNUSED(authorized);
	ARG_UNUSED(bssid);
	return 0;
}

/*
 * AP-side station management. The medium relays data frames purely by
 * destination MAC, so there is no per-station driver state to program; these
 * ops just acknowledge success so hostapd/wpa_supplicant-AP can drive the
 * authenticator and authorize the station after the 4-way handshake.
 */
static int hwsim_supp_sta_add(void *if_priv, struct hostapd_sta_add_params *params)
{
	ARG_UNUSED(if_priv);
	ARG_UNUSED(params);
	return 0;
}

static int hwsim_supp_sta_remove(void *if_priv, const u8 *addr)
{
	ARG_UNUSED(if_priv);
	ARG_UNUSED(addr);
	return 0;
}

static int hwsim_supp_sta_set_flags(void *if_priv, const u8 *addr,
				    unsigned int total_flags,
				    unsigned int flags_or, unsigned int flags_and)
{
	ARG_UNUSED(if_priv);
	ARG_UNUSED(addr);
	ARG_UNUSED(total_flags);
	ARG_UNUSED(flags_or);
	ARG_UNUSED(flags_and);
	return 0;
}

static int hwsim_supp_set_key(void *if_priv, const unsigned char *ifname,
			      enum wpa_alg alg, const unsigned char *addr,
			      int key_idx, int set_tx, const unsigned char *seq,
			      size_t seq_len, const unsigned char *key,
			      size_t key_len, enum key_flag key_flag)
{
	ARG_UNUSED(if_priv);
	ARG_UNUSED(ifname);
	ARG_UNUSED(alg);
	ARG_UNUSED(addr);
	ARG_UNUSED(key_idx);
	ARG_UNUSED(set_tx);
	ARG_UNUSED(seq);
	ARG_UNUSED(seq_len);
	ARG_UNUSED(key);
	ARG_UNUSED(key_len);
	ARG_UNUSED(key_flag);
	return 0;
}

static int hwsim_supp_signal_poll(void *if_priv, struct wpa_signal_info *si,
				  unsigned char *bssid)
{
	struct hwsim_radio *radio = (struct hwsim_radio *)if_priv;

	memset(si, 0, sizeof(*si));
	si->frequency = HWSIM_2G_BASE_FREQ_MHZ + (radio->channel * 5);
	si->current_noise = -95;
	si->data.last_ack_rssi = (s8)radio->tx_power_dbm;
	if (radio->associated && bssid != NULL) {
		memcpy(bssid, radio->ap_bssid, ETH_ALEN);
	}
	return 0;
}

static int hwsim_supp_send_mlme(void *if_priv, const u8 *data, size_t data_len,
				int noack, unsigned int freq, int no_cck,
				int offchanok, unsigned int wait_time, int cookie)
{
	struct hwsim_radio *radio = (struct hwsim_radio *)if_priv;
	struct hwsim_supp_ctx *ctx = (struct hwsim_supp_ctx *)radio->supp_ctx;
	int ret;

	ARG_UNUSED(noack);
	ARG_UNUSED(freq);
	ARG_UNUSED(no_cck);
	ARG_UNUSED(offchanok);
	ARG_UNUSED(wait_time);
	ARG_UNUSED(cookie);

	if (data_len > HWSIM_FRAME_MTU) {
		return -1;
	}
	ret = hwsim_medium_xmit(radio->idx, data, (uint16_t)data_len, true);

	/*
	 * The transmitting MLME (e.g. the AP's SAE handle_auth) advances on the
	 * transmit status of the frames it sends. The medium delivers reliably,
	 * so report a successful ACK back so the state machine proceeds.
	 */
	if (ctx != NULL && ctx->callbk_fns.mgmt_tx_status != NULL) {
		ctx->callbk_fns.mgmt_tx_status(ctx->if_ctx, data, data_len, true);
	}
	return ret;
}

static int hwsim_supp_get_wiphy(void *if_priv)
{
	struct hwsim_radio *radio = (struct hwsim_radio *)if_priv;
	struct hwsim_supp_ctx *ctx = (struct hwsim_supp_ctx *)radio->supp_ctx;
	struct wpa_supp_event_supported_band band;

	if (ctx == NULL || ctx->callbk_fns.get_wiphy_res == NULL) {
		return -1;
	}

	memset(&band, 0, sizeof(band));
	band.band = 0; /* 2.4 GHz (WIFI_FREQ_BAND_2_4_GHZ) */
	band.wpa_supp_n_channels = 14;
	band.wpa_supp_n_bitrates = 8;
	for (int i = 0; i < 14; i++) {
		band.channels[i].ch_valid = 1;
		band.channels[i].center_frequency = 2407 + (i + 1) * 5;
		band.channels[i].wpa_supp_max_power = 20;
	}
	/* 11b + 11g rates (100 kbps); need at least one > 200 for IEEE80211G mode */
	band.bitrates[0].wpa_supp_bitrate = 10;
	band.bitrates[1].wpa_supp_bitrate = 20;
	band.bitrates[2].wpa_supp_bitrate = 55;
	band.bitrates[3].wpa_supp_bitrate = 110;
	band.bitrates[4].wpa_supp_bitrate = 120;
	band.bitrates[5].wpa_supp_bitrate = 240;
	band.bitrates[6].wpa_supp_bitrate = 360;
	band.bitrates[7].wpa_supp_bitrate = 540;

	ctx->callbk_fns.get_wiphy_res(ctx->if_ctx, &band);
	k_sem_give(&ctx->if_ctx->drv_resp_sem);
	return 0;
}

static int hwsim_supp_register_frame(void *if_priv, u16 type, const u8 *match,
				    size_t match_len, bool multicast)
{
	ARG_UNUSED(if_priv);
	ARG_UNUSED(type);
	ARG_UNUSED(match);
	ARG_UNUSED(match_len);
	ARG_UNUSED(multicast);
	return 0;
}

static int hwsim_supp_register_mgmt_frame(void *if_priv, u16 frame_type,
					  size_t match_len, const u8 *match)
{
	ARG_UNUSED(if_priv);
	ARG_UNUSED(frame_type);
	ARG_UNUSED(match_len);
	ARG_UNUSED(match);
	return 0;
}

static int hwsim_supp_get_capa(void *if_priv, struct wpa_driver_capa *capa)
{
	ARG_UNUSED(if_priv);
	memset(capa, 0, sizeof(*capa));
	capa->key_mgmt = WPA_DRIVER_CAPA_KEY_MGMT_WPA2_PSK |
			 WPA_DRIVER_CAPA_KEY_MGMT_WPA_PSK |
			 WPA_DRIVER_CAPA_KEY_MGMT_SAE;
	capa->enc = WPA_DRIVER_CAPA_ENC_CCMP | WPA_DRIVER_CAPA_ENC_TKIP;
	/*
	 * WPA_DRIVER_FLAGS_SAE tells wpa_supplicant the (user-space SME) driver
	 * can run SAE; without it the supplicant strips the SAE AKM when
	 * selecting suites. SAE commit/confirm are relayed as Authentication
	 * frames over the medium (see hwsim_supp_authenticate / _mgmt_rx).
	 */
	capa->flags = WPA_DRIVER_FLAGS_SME | WPA_DRIVER_FLAGS_AP |
		      WPA_DRIVER_FLAGS_SAE;
	return 0;
}

static int hwsim_supp_get_conn_info(void *if_priv, struct wpa_conn_info *info)
{
	ARG_UNUSED(if_priv);
	memset(info, 0, sizeof(*info));
	info->beacon_interval = 100;
	return 0;
}

static int hwsim_supp_set_country(void *priv, const char *alpha2)
{
	ARG_UNUSED(priv);
	ARG_UNUSED(alpha2);
	return 0;
}

static int hwsim_supp_get_country(void *priv, char *alpha2)
{
	if (alpha2 != NULL) {
		alpha2[0] = '0';
		alpha2[1] = '0';
	}
	return 0;
}

static int hwsim_supp_remain_on_channel(void *priv, unsigned int freq,
					unsigned int duration, u64 host_cookie)
{
	ARG_UNUSED(priv);
	ARG_UNUSED(freq);
	ARG_UNUSED(duration);
	ARG_UNUSED(host_cookie);
	return -1;
}

static int hwsim_supp_cancel_remain_on_channel(void *priv, u64 rpu_cookie)
{
	ARG_UNUSED(priv);
	ARG_UNUSED(rpu_cookie);
	return -1;
}

static int hwsim_supp_set_p2p_powersave(void *if_priv, int legacy_ps,
				       int opp_ps, int ctwindow)
{
	ARG_UNUSED(if_priv);
	ARG_UNUSED(legacy_ps);
	ARG_UNUSED(opp_ps);
	ARG_UNUSED(ctwindow);
	return 0;
}

const struct zep_wpa_supp_dev_ops hwsim_wpa_supp_ops = {
	.init = hwsim_supp_init,
	.deinit = hwsim_supp_deinit,
	.scan2 = hwsim_supp_scan2,
	.scan_abort = hwsim_supp_scan_abort,
	.get_scan_results2 = hwsim_supp_get_scan_results2,
	.deauthenticate = hwsim_supp_deauthenticate,
	.authenticate = hwsim_supp_authenticate,
	.associate = hwsim_supp_associate,
	.init_ap = hwsim_supp_init_ap,
	.deinit_ap = hwsim_supp_deinit_ap,
	.start_ap = hwsim_supp_start_ap,
	.set_supp_port = hwsim_supp_set_supp_port,
	.sta_add = hwsim_supp_sta_add,
	.sta_remove = hwsim_supp_sta_remove,
	.sta_set_flags = hwsim_supp_sta_set_flags,
	.set_key = hwsim_supp_set_key,
	.signal_poll = hwsim_supp_signal_poll,
	.send_mlme = hwsim_supp_send_mlme,
	.get_wiphy = hwsim_supp_get_wiphy,
	.register_frame = hwsim_supp_register_frame,
	.register_mgmt_frame = hwsim_supp_register_mgmt_frame,
	.get_capa = hwsim_supp_get_capa,
	.get_conn_info = hwsim_supp_get_conn_info,
	.set_country = hwsim_supp_set_country,
	.get_country = hwsim_supp_get_country,
	.remain_on_channel = hwsim_supp_remain_on_channel,
	.cancel_remain_on_channel = hwsim_supp_cancel_remain_on_channel,
	.set_p2p_powersave = hwsim_supp_set_p2p_powersave,
};
