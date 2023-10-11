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

#define SCAN_TIMEOUT 2

static void scan_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct wifi_context *ctx = CONTAINER_OF(dwork,
						struct wifi_context,
						scan_work);
	union wpa_event_data event = {0};
	struct scan_info *info = &event.scan_info;

	/* Inform wpa_supplicant that scan is done */
	info->aborted = false;
	info->external_scan = 1;
	info->nl_scan_event = 1;

	ctx->supplicant_callbacks.scan_done(ctx->supplicant_drv_ctx, &event);
	ctx->scan_in_progress = false;
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

	ctx->my_scan_cb = my_scan_results;

	/* We could get the results pretty quick and call the scan result
	 * function here but simulate a way that a normal wifi driver
	 * would get the results.
	 */
	k_work_init_delayable(&ctx->scan_work, scan_handler);
	k_work_schedule(&ctx->scan_work, K_SECONDS(SCAN_TIMEOUT));

	/* Starting the host scan */
	ret = host_wifi_drv_scan2(ctx->host_context, (void *)params);

	/* and telling Zephyr supplicant driver that scan has been started */
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

	ctx->my_scan_cb(ctx, results);

	host_wifi_drv_free_scan_results(results);

	return 0;
}

int wifi_drv_deauthenticate(void *if_priv, const char *addr, unsigned short reason_code)
{
	struct wifi_context *ctx = if_priv;

	LOG_DBG("iface %s [%d] dev %s (%p)", ctx->name, ctx->if_index,
		net_if_get_device(ctx->iface)->name, net_if_get_device(ctx->iface));

	return 0;
}

int wifi_drv_authenticate(void *if_priv, struct wpa_driver_auth_params *params,
			  struct wpa_bss *curr_bss)
{
	struct wifi_context *ctx = if_priv;

	LOG_DBG("iface %s [%d] dev %s (%p)", ctx->name, ctx->if_index,
		net_if_get_device(ctx->iface)->name, net_if_get_device(ctx->iface));

	return 0;
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
		     const unsigned char *key, size_t key_len)
{
	struct wifi_context *ctx = if_priv;

	LOG_DBG("iface %s [%d] dev %s (%p)", ctx->name, ctx->if_index,
		net_if_get_device(ctx->iface)->name, net_if_get_device(ctx->iface));

	return 0;
}

int wifi_drv_set_supp_port(void *if_priv, int authorized, char *bssid)
{
	struct wifi_context *ctx = if_priv;

	LOG_DBG("iface %s [%d] dev %s (%p)", ctx->name, ctx->if_index,
		net_if_get_device(ctx->iface)->name, net_if_get_device(ctx->iface));

	return 0;
}

int wifi_drv_signal_poll(void *if_priv, struct wpa_signal_info *si, unsigned char *bssid)
{
	struct wifi_context *ctx = if_priv;

	LOG_DBG("iface %s [%d] dev %s (%p)", ctx->name, ctx->if_index,
		net_if_get_device(ctx->iface)->name, net_if_get_device(ctx->iface));

	return 0;
}

int wifi_drv_send_mlme(void *if_priv, const u8 *data, size_t data_len, int noack, unsigned int freq,
		       int no_cck, int offchanok, unsigned int wait_time, int cookie)
{
	struct wifi_context *ctx = if_priv;

	LOG_DBG("iface %s [%d] dev %s (%p)", ctx->name, ctx->if_index,
		net_if_get_device(ctx->iface)->name, net_if_get_device(ctx->iface));

	return 0;
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

	return 0;
}

void wifi_drv_cb_scan_start(struct zep_drv_if_ctx *if_ctx)
{
	LOG_DBG("ctx %p", if_ctx);
}

void wifi_drv_cb_scan_done(struct zep_drv_if_ctx *if_ctx, union wpa_event_data *event)
{
	LOG_DBG("ctx %p", if_ctx);
}

void wifi_drv_cb_scan_res(struct zep_drv_if_ctx *if_ctx, struct wpa_scan_res *r, bool more_res)
{
	LOG_DBG("ctx %p", if_ctx);
}

void wifi_drv_cb_auth_resp(struct zep_drv_if_ctx *if_ctx, union wpa_event_data *event)
{
	LOG_DBG("ctx %p", if_ctx);
}

void wifi_drv_cb_assoc_resp(struct zep_drv_if_ctx *if_ctx, union wpa_event_data *event,
			    unsigned int status)
{
	LOG_DBG("ctx %p", if_ctx);
}

void wifi_drv_cb_deauth(struct zep_drv_if_ctx *if_ctx, union wpa_event_data *event)
{
	LOG_DBG("ctx %p", if_ctx);
}

void wifi_drv_cb_disassoc(struct zep_drv_if_ctx *if_ctx, union wpa_event_data *event)
{
	LOG_DBG("ctx %p", if_ctx);
}

void wifi_drv_cb_mgmt_tx_status(struct zep_drv_if_ctx *if_ctx, const u8 *frame, size_t len,
				bool ack)
{
	LOG_DBG("ctx %p", if_ctx);
}

void wifi_drv_cb_unprot_deauth(struct zep_drv_if_ctx *if_ctx, union wpa_event_data *event)
{
	LOG_DBG("ctx %p", if_ctx);
}

void wifi_drv_cb_unprot_disassoc(struct zep_drv_if_ctx *if_ctx, union wpa_event_data *event)
{
	LOG_DBG("ctx %p", if_ctx);
}

void wifi_drv_cb_get_wiphy_res(struct zep_drv_if_ctx *if_ctx, void *band)
{
	LOG_DBG("ctx %p", if_ctx);
}

void wifi_drv_cb_mgmt_rx(struct zep_drv_if_ctx *if_ctx, char *frame, int frame_len, int frequency,
			 int rx_signal_dbm)
{
	LOG_DBG("ctx %p", if_ctx);
}

/* Connect and disconnect are not part of the supplicant API but are
 * needed by this driver so that we can do connect to the nl80211 host
 * based driver.
 */
int wifi_drv_connect(void *if_priv, struct wifi_connect_req_params *params)
{
	struct wifi_context *ctx = if_priv;
	void *network;
	int ret;

	LOG_DBG("iface %s [%d] dev %s (%p)", ctx->name, ctx->if_index,
		net_if_get_device(ctx->iface)->name, net_if_get_device(ctx->iface));

	/* Starting the host connect */
	network = host_supplicant_get_network(ctx->host_context);
	if (network == NULL) {
		return -ENOENT;
	}

	ret = host_supplicant_set_config_option(network, "scan_ssid", "1");
	if (ret < 0) {
		return ret;
	}

	ret = host_supplicant_set_config_option(network, "key_mgmt", "NONE");
	if (ret < 0) {
		return ret;
	}

	ret = host_supplicant_set_config_option(network, "ieee80211w", "0");
	if (ret < 0) {
		return ret;
	}

	if (params->security != WIFI_SECURITY_TYPE_NONE) {
		if (params->security == WIFI_SECURITY_TYPE_SAE) {
			if (params->sae_password) {
				ret = host_supplicant_set_config_option(network,
									"sae_password",
									params->sae_password);
			} else {
				ret = host_supplicant_set_config_option(network,
									"sae_password",
									params->psk);
			}

			if (ret < 0) {
				return ret;
			}

			ret = host_supplicant_set_config_option(network, "key_mgmt", "SAE");
			if (ret < 0) {
				return ret;
			}
		} else if (params->security == WIFI_SECURITY_TYPE_PSK_SHA256) {
			ret = host_supplicant_set_config_option(network, "psk", params->psk);
			if (ret < 0) {
				return ret;
			}

			ret = host_supplicant_set_config_option(network, "key_mgmt",
								"WPA-PSK-SHA256");
			if (ret < 0) {
				return ret;
			}
		} else if (params->security == WIFI_SECURITY_TYPE_PSK) {
			ret = host_supplicant_set_config_option(network, "psk", params->psk);
			if (ret < 0) {
				return ret;
			}

			ret = host_supplicant_set_config_option(network, "key_mgmt",
								"WPA-PSK");
			if (ret < 0) {
				return ret;
			}
		} else {
			LOG_DBG("Unsupported security type %d", params->security);
			return -ENOTSUP;
		}

		if (params->mfp) {
			char mfp[16];

			snprintf(mfp, sizeof(mfp), "%d", params->mfp);

			ret = host_supplicant_set_config_option(network, "ieee80211w", mfp);
			if (ret < 0) {
				return ret;
			}
		}
	}

	ret = host_wifi_drv_connect(ctx->host_context, network, params->channel);

	return ret;
}

int wifi_drv_disconnect(void *if_priv)
{
	struct wifi_context *ctx = if_priv;
	int ret;

	LOG_DBG("iface %s [%d] dev %s (%p)", ctx->name, ctx->if_index,
		net_if_get_device(ctx->iface)->name, net_if_get_device(ctx->iface));

	/* Starting the host disconnect */
	//ret = host_wifi_drv_disconnect(ctx->host_context);
	ret = 0;

	return ret;
}
