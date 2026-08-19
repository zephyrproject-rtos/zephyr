/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BFLB_WIFI_BLOB_API_H_
#define BFLB_WIFI_BLOB_API_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "supplicant.h"
#include "wl80211.h"

#define ETH_ADDR_LOCAL_ADMIN_BIT 0x02

/* Blob-provided symbols used by the driver. Declared here so the
 * compiler can verify signatures across all translation units.
 */

extern void wifi_task_create(void);
extern int bflb_rf_init(void);
extern int wifi_task_wait_ready(k_timeout_t timeout);

extern const struct wpa_funcs *wpa_cbs;
extern int wl80211_supplicant_register_wpa_cb_internal(const struct wpa_funcs *cb);
extern int wl80211_supplicant_set_assoc_ie(uint8_t *ie, uint16_t ie_len);
extern void wpa_supplicant_event_wrapper(void *ctx, enum wpa_event_type event,
					 union wpa_event_data *data);
extern void wl80211_supplicant_set_hostap_private(void *priv);

int mac_cipher_suite_value(uint32_t cipher_suite);

/* Driver callback invoked from blob shim (wl80211_port.c) */
void bflb_wifi_eapol_input(uint8_t vif_type, uint8_t *payload, size_t len);

struct wpa_supplicant;
extern void sme_external_auth_mgmt_rx(struct wpa_supplicant *wpa_s, const uint8_t *auth_frame,
				      size_t len);

#endif /* BFLB_WIFI_BLOB_API_H_ */
