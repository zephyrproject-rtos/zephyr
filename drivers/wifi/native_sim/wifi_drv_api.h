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
		     size_t key_len);
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

void wifi_drv_cb_scan_start(struct zep_drv_if_ctx *if_ctx);
void wifi_drv_cb_scan_done(struct zep_drv_if_ctx *if_ctx, union wpa_event_data *event);
void wifi_drv_cb_scan_res(struct zep_drv_if_ctx *if_ctx, struct wpa_scan_res *r, bool more_res);
void wifi_drv_cb_auth_resp(struct zep_drv_if_ctx *if_ctx, union wpa_event_data *event);
void wifi_drv_cb_assoc_resp(struct zep_drv_if_ctx *if_ctx, union wpa_event_data *event,
			    unsigned int status);
void wifi_drv_cb_deauth(struct zep_drv_if_ctx *if_ctx, union wpa_event_data *event);
void wifi_drv_cb_disassoc(struct zep_drv_if_ctx *if_ctx, union wpa_event_data *event);
void wifi_drv_cb_mgmt_tx_status(struct zep_drv_if_ctx *if_ctx, const u8 *frame, size_t len,
				bool ack);
void wifi_drv_cb_unprot_deauth(struct zep_drv_if_ctx *if_ctx, union wpa_event_data *event);
void wifi_drv_cb_unprot_disassoc(struct zep_drv_if_ctx *if_ctx, union wpa_event_data *event);
void wifi_drv_cb_get_wiphy_res(struct zep_drv_if_ctx *if_ctx, void *band);
void wifi_drv_cb_mgmt_rx(struct zep_drv_if_ctx *if_ctx, char *frame, int frame_len, int frequency,
			 int rx_signal_dbm);

int wifi_drv_connect(void *if_priv, struct wifi_connect_req_params *params);
int wifi_drv_disconnect(void *if_priv);
