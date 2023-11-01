/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/wifi.h>

#include "wifi_drv_api.h"
#include "internal.h"

LOG_MODULE_DECLARE(wifi_native_sim, CONFIG_WIFI_LOG_LEVEL);

void *wifi_drv_init(void *supp_drv_if_ctx, const char *iface_name,
		    struct zep_wpa_supp_dev_callbk_fns *callbacks)
{
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

	LOG_DBG("iface %s [%d] dev %s (%p)", ctx->name, ctx->if_index, dev->name, dev);

	return ctx;
}

void wifi_drv_deinit(void *if_priv)
{
	struct wifi_context *ctx = if_priv;

	LOG_DBG("iface %s [%d] dev %s (%p)", ctx->name, ctx->if_index,
		net_if_get_device(ctx->iface)->name, net_if_get_device(ctx->iface));

	ctx->supplicant_drv_ctx = NULL;
}

int wifi_drv_scan2(void *if_priv, struct wpa_driver_scan_params *params)
{
	struct wifi_context *ctx = if_priv;

	LOG_DBG("iface %s [%d] dev %s (%p)", ctx->name, ctx->if_index,
		net_if_get_device(ctx->iface)->name, net_if_get_device(ctx->iface));

	return 0;
}

int wifi_drv_scan_abort(void *if_priv)
{
	struct wifi_context *ctx = if_priv;

	LOG_DBG("iface %s [%d] dev %s (%p)", ctx->name, ctx->if_index,
		net_if_get_device(ctx->iface)->name, net_if_get_device(ctx->iface));

	return 0;
}

int wifi_drv_get_scan_results2(void *if_priv)
{
	struct wifi_context *ctx = if_priv;

	LOG_DBG("iface %s [%d] dev %s (%p)", ctx->name, ctx->if_index,
		net_if_get_device(ctx->iface)->name, net_if_get_device(ctx->iface));

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

	return 0;
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

int wifi_drv_get_wiphy(void *if_priv)
{
	struct wifi_context *ctx = if_priv;

	LOG_DBG("iface %s [%d] dev %s (%p)", ctx->name, ctx->if_index,
		net_if_get_device(ctx->iface)->name, net_if_get_device(ctx->iface));

	return 0;
}

int wifi_drv_register_frame(void *if_priv, u16 type, const u8 *match, size_t match_len,
			    bool multicast)
{
	struct wifi_context *ctx = if_priv;

	LOG_DBG("iface %s [%d] dev %s (%p)", ctx->name, ctx->if_index,
		net_if_get_device(ctx->iface)->name, net_if_get_device(ctx->iface));

	return 0;
}

int wifi_drv_get_capa(void *if_priv, struct wpa_driver_capa *capa)
{
	struct wifi_context *ctx = if_priv;

	LOG_DBG("iface %s [%d] dev %s (%p)", ctx->name, ctx->if_index,
		net_if_get_device(ctx->iface)->name, net_if_get_device(ctx->iface));

	return 0;
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
}

void wifi_drv_cb_scan_done(struct zep_drv_if_ctx *if_ctx, union wpa_event_data *event)
{
}

void wifi_drv_cb_scan_res(struct zep_drv_if_ctx *if_ctx, struct wpa_scan_res *r, bool more_res)
{
}

void wifi_drv_cb_auth_resp(struct zep_drv_if_ctx *if_ctx, union wpa_event_data *event)
{
}

void wifi_drv_cb_assoc_resp(struct zep_drv_if_ctx *if_ctx, union wpa_event_data *event,
			    unsigned int status)
{
}

void wifi_drv_cb_deauth(struct zep_drv_if_ctx *if_ctx, union wpa_event_data *event)
{
}

void wifi_drv_cb_disassoc(struct zep_drv_if_ctx *if_ctx, union wpa_event_data *event)
{
}

void wifi_drv_cb_mgmt_tx_status(struct zep_drv_if_ctx *if_ctx, const u8 *frame, size_t len,
				bool ack)
{
}

void wifi_drv_cb_unprot_deauth(struct zep_drv_if_ctx *if_ctx, union wpa_event_data *event)
{
}

void wifi_drv_cb_unprot_disassoc(struct zep_drv_if_ctx *if_ctx, union wpa_event_data *event)
{
}

void wifi_drv_cb_get_wiphy_res(struct zep_drv_if_ctx *if_ctx, void *band)
{
}

void wifi_drv_cb_mgmt_rx(struct zep_drv_if_ctx *if_ctx, char *frame, int frame_len, int frequency,
			 int rx_signal_dbm)
{
}
