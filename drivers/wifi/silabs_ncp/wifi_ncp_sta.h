/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_WIFI_SILABS_NCP_WIFI_NCP_STA_H_
#define ZEPHYR_DRIVERS_WIFI_SILABS_NCP_WIFI_NCP_STA_H_

#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>

#include <drivers/driver_zephyr.h>

#include "wifi_ncp.h"

/* Send a command and wait for its confirmation. Returns the status of the
 * confirmation, or a negative error. resp_size is the capacity of resp on
 * entry and the length of the returned payload on success.
 */
int wifi_ncp_nwp_cmd(const struct device *dev, const struct wifi_ncp_nwp_hdr *hdr, const void *head,
		     size_t head_len, const void *body, size_t body_len, void *resp,
		     size_t *resp_size);

int wifi_ncp_sta_init(const struct device *dev);

/* Handle a frame of the management endpoint, on the work queue */
void wifi_ncp_sta_rx(const struct device *dev, const uint8_t *frame, size_t len);

/* Supplicant operations */
int wifi_ncp_sta_scan(void *if_priv, struct wpa_driver_scan_params *params);

int wifi_ncp_sta_scan_abort(void *if_priv);

int wifi_ncp_sta_get_scan_results(void *if_priv);

int wifi_ncp_sta_associate(void *if_priv, struct wpa_driver_associate_params *params);

int wifi_ncp_sta_deauthenticate(void *if_priv, const char *addr, unsigned short reason_code);

int wifi_ncp_sta_set_key(void *if_priv, const unsigned char *ifname, enum wpa_alg alg,
			 const unsigned char *addr, int key_idx, int set_tx,
			 const unsigned char *seq, size_t seq_len, const unsigned char *key,
			 size_t key_len, enum key_flag key_flag);

int wifi_ncp_sta_set_supp_port(void *if_priv, int authorized, char *bssid);

int wifi_ncp_sta_tx_control_port(void *if_priv, const unsigned char *dest, unsigned short proto,
				 const unsigned char *buf, size_t len, int no_encrypt);

int wifi_ncp_sta_signal_poll(void *if_priv, struct wpa_signal_info *si, unsigned char *bssid);

int wifi_ncp_sta_get_conn_info(void *if_priv, struct wpa_conn_info *info);

int wifi_ncp_sta_send_mlme(void *if_priv, const u8 *data, size_t data_len, int noack,
			   unsigned int freq, int no_cck, int offchanok, unsigned int wait_time,
			   int cookie);

int wifi_ncp_sta_send_external_auth_status(void *if_priv, struct external_auth *params);

#endif /* ZEPHYR_DRIVERS_WIFI_SILABS_NCP_WIFI_NCP_STA_H_ */
