/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "internal.h"

void *wifi_drv_init(void *supp_drv_if_ctx, const char *iface_name,
		    struct zep_wpa_supp_dev_callbk_fns *callbk_fns);
void wifi_drv_deinit(void *if_priv);
int wifi_drv_scan2(void *if_priv, struct wpa_driver_scan_params *params);
int wifi_drv_scan_abort(void *if_priv);
int wifi_drv_get_scan_results2(void *if_priv);
int wifi_drv_deauthenticate(void *if_priv, const char *addr, unsigned short reason_code);
int wifi_drv_authenticate(void *if_priv, struct wpa_driver_auth_params *params,
			  struct wpa_bss *curr_bss);
int wifi_drv_associate(void *if_priv, struct wpa_driver_associate_params *params);
int wifi_drv_set_key(void *if_priv, const unsigned char *ifname, enum wpa_alg alg,
		     const unsigned char *addr, int key_idx, int set_tx,
		     const unsigned char *seq, size_t seq_len, const unsigned char *key,
		     size_t key_len, enum key_flag key_flag);
int wifi_drv_set_supp_port(void *if_priv, int authorized, char *bssid);
int wifi_drv_signal_poll(void *if_priv, struct wpa_signal_info *si, unsigned char *bssid);
int wifi_drv_send_mlme(void *if_priv, const u8 *data, size_t data_len, int noack,
		       unsigned int freq, int no_cck, int offchanok, unsigned int wait_time,
		       int cookie);
int wifi_drv_get_wiphy(void *if_priv);
int wifi_drv_register_frame(void *if_priv, u16 type, const u8 *match, size_t match_len,
			    bool multicast);
int wifi_drv_get_capa(void *if_priv, struct wpa_driver_capa *capa);
int wifi_drv_get_conn_info(void *if_priv, struct wpa_conn_info *info);

int wifi_drv_connect(void *if_priv, struct wifi_connect_req_params *params);
int wifi_drv_disconnect(void *if_priv);

/* Dispatch one event datagram (struct host_wifi_event + payload) received from
 * the host nl80211 driver to the Zephyr supplicant callbacks. Called from the
 * RX thread.
 */
void wifi_drv_event_dispatch(struct wifi_context *ctx, void *buf, size_t len);
