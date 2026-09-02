/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BFLB_WIFI_AP_H_
#define BFLB_WIFI_AP_H_

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "supplicant.h"

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_AP

struct wpa_driver_associate_params;
struct wpa_driver_ap_params;
struct hostapd_sta_add_params;
struct zep_hostapd_dev_callbk_fns;

uint8_t bflb_ap_find_sta(const uint8_t *mac);

void *bflb_ap_bridge_init(wifi_ap_parm_t *parm);
bool bflb_ap_bridge_deinit(void *data);
bool bflb_ap_bridge_join(void **sm, uint8_t *mac, uint8_t *wpa_ie, uint8_t wpa_ie_len);
void bflb_ap_bridge_sta_associated(void *wpa_sm, uint8_t sta_idx);
bool bflb_ap_bridge_remove(void *sm);
bool bflb_ap_bridge_rx_eapol(void *hapd_data, void *sm, uint8_t *data, size_t data_len);

void *bflb_hapd_init(void *hapd_drv_if_ctx, const char *iface_name,
		     struct zep_hostapd_dev_callbk_fns *callbk_fns);
void bflb_hapd_deinit(void *priv);
int bflb_set_ap(void *priv, int beacon_set, struct wpa_driver_ap_params *params);
int bflb_sta_set_flags(void *if_priv, const u8 *addr, unsigned int total_flags,
		       unsigned int flags_or, unsigned int flags_and);
int bflb_sta_add(void *if_priv, struct hostapd_sta_add_params *params);
int bflb_sta_remove(void *if_priv, const u8 *addr);
int bflb_get_inact_sec(void *if_priv, const u8 *addr);
int bflb_wpa_supp_init_ap(void *if_priv, struct wpa_driver_associate_params *params);
int bflb_wpa_supp_deinit_ap(void *if_priv);
int bflb_wpa_supp_register_mgmt_frame(void *if_priv, u16 frame_type, size_t match_len,
				      const u8 *match);
int bflb_wpa_supp_start_ap(void *if_priv, struct wpa_driver_ap_params *params);
int bflb_wpa_supp_stop_ap(void *if_priv);
int bflb_wpa_supp_change_beacon(void *if_priv, struct wpa_driver_ap_params *params);

#endif /* CONFIG_WIFI_NM_WPA_SUPPLICANT_AP */

#endif /* BFLB_WIFI_AP_H_ */
