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

#endif /* ZEPHYR_DRIVERS_WIFI_SILABS_NCP_WIFI_NCP_STA_H_ */
