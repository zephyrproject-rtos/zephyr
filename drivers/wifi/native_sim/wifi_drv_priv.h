/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Private functions for native_sim wifi driver.
 */

#ifndef ZEPHYR_DRIVERS_WIFI_NATIVE_SIM_PRIV_H_
#define ZEPHYR_DRIVERS_WIFI_NATIVE_SIM_PRIV_H_

int eth_iface_create(const char *if_name);
int eth_iface_remove(int fd);
int eth_wait_data(int fd);
ssize_t eth_read_data(int fd, void *buf, size_t buf_len);
ssize_t eth_write_data(int fd, void *buf, size_t buf_len);
int eth_promisc_mode(const char *if_name, bool enable);

/* Interface to wpa_supplicant side that is compiled to host */
void *host_wifi_drv_init(void *ctx, const char *iface_name, size_t stack_size,
			 const char *config_file, const char *debug_file,
			 int wpa_debug_level);
void host_wifi_drv_deinit(void *priv);
int host_wifi_drv_get_wiphy(void *priv, void **bands, int *count);
void host_wifi_drv_free_bands(void *bands);
int host_wifi_drv_scan2(void *priv, void *params);
int host_wifi_drv_register_frame(void *priv, uint16_t type, const uint8_t *match,
				 size_t match_len, bool multicast);
int host_wifi_drv_get_capa(void *priv, void *capa);
void *host_wifi_drv_get_scan_results(void *priv);
void host_wifi_drv_free_scan_results(void *res);
int host_wifi_drv_scan_abort(void *priv);
int host_wifi_drv_associate(void *priv, void *param);
int host_wifi_drv_connect(void *priv, void *network, int channel);
void *host_supplicant_get_network(void *priv);
int host_supplicant_set_config_option(void *network, char *opt, const char *value);

#endif /* ZEPHYR_DRIVERS_WIFI_NATIVE_SIM_PRIV_H_ */
