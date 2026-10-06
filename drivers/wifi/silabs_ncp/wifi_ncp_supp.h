/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_WIFI_SILABS_NCP_WIFI_NCP_SUPP_H_
#define ZEPHYR_DRIVERS_WIFI_SILABS_NCP_WIFI_NCP_SUPP_H_

#include <drivers/driver_zephyr.h>

/* Operations of the Zephyr supplicant that do not depend on the mode */
void *wifi_ncp_supp_init(void *supp_drv_if_ctx, const char *iface_name,
			 struct zep_wpa_supp_dev_callbk_fns *callbk_fns);
void wifi_ncp_supp_deinit(void *if_priv);
int wifi_ncp_supp_get_capa(void *if_priv, struct wpa_driver_capa *capa);
int wifi_ncp_supp_get_wiphy(void *if_priv);

#endif /* ZEPHYR_DRIVERS_WIFI_SILABS_NCP_WIFI_NCP_SUPP_H_ */
