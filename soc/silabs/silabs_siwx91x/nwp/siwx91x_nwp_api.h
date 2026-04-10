/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SIWX91X_NWP_API_H_
#define SIWX91X_NWP_API_H_
#include <stdint.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/wifi.h>
#include "device/silabs/si91x/wireless/inc/sl_wifi_device.h"
#include "protocol/wifi/inc/sl_wifi_constants.h"
#include "protocol/wifi/inc/sl_wifi_types.h"
#include "sli_wifi/inc/sli_wifi_types.h"

struct device;

/* sl_wifi_firmware_version_t is buggy because it lacks of __packed */
struct siwx91x_nwp_version {
	uint8_t chip_id;
	uint8_t rom_id;
	uint8_t major;
	uint8_t minor;
	uint8_t security_version;
	uint8_t patch_num;
	uint8_t customer_id;
	uint16_t build_num;
} __packed;

/* NWP APIs */
void siwx91x_nwp_soft_reset(const struct device *dev);
void siwx91x_nwp_force_assert(const struct device *dev);
void siwx91x_nwp_opermode(const struct device *dev,
			  sl_wifi_system_boot_configuration_t *params);
void siwx91x_nwp_feature(const struct device *dev, bool enable_pll);
void siwx91x_nwp_dynamic_pool(const struct device *dev, int tx, int rx, int global);
void siwx91x_nwp_get_firmware_version(const struct device *dev,
				      struct siwx91x_nwp_version *version);
int siwx91x_nwp_flash_erase(const struct device *dev, uint32_t dest, size_t len);
int siwx91x_nwp_flash_write(const struct device *dev, uint32_t dest, const void *buf, size_t len);
int siwx91x_nwp_fw_upgrade_start(const struct device *dev, const void *hdr);
int siwx91x_nwp_fw_upgrade_write(const struct device *dev, const void *buf, size_t len);

/* Wifi APIs */
int siwx91x_nwp_wifi_init(const struct device *dev);
int siwx91x_nwp_scan(const struct device *dev, uint16_t channel_list, const char *ssid,
		     bool passive, bool internal);
int siwx91x_nwp_join(const struct device *dev, const char *ssid,
		     const uint8_t *bssid, int security_type);
int siwx91x_nwp_disconnect(const struct device *dev);
void siwx91x_nwp_ap_config(const struct device *dev, sli_wifi_ap_config_request *params);
int siwx91x_nwp_ap_start(const struct device *dev, const char *ssid, int security_type);
void siwx91x_nwp_ap_stop(const struct device *dev);
void siwx91x_nwp_sta_disconnect(const struct device *dev,
				const uint8_t remote_addr[WIFI_MAC_ADDR_LEN]);
void siwx91x_nwp_get_bss_info(const struct device *dev, sl_wifi_operational_statistics_t *info);
void siwx91x_nwp_ps_disable(const struct device *dev);
void siwx91x_nwp_ps_enable(const struct device *dev, sli_wifi_power_save_request_t *params);
void siwx91x_nwp_twt_params(const struct device *dev, sl_wifi_twt_request_t *params);
void siwx91x_nwp_set_region_ap(const struct device *dev);
void siwx91x_nwp_set_region_sta(const struct device *dev, sl_wifi_region_code_t region_code);
void siwx91x_nwp_set_psk(const struct device *dev, const char *psk);
void siwx91x_nwp_set_sta_config(const struct device *dev);
void siwx91x_nwp_set_band(const struct device *dev, sl_wifi_band_mode_t band);
void siwx91x_nwp_set_ht_caps(const struct device *dev, bool enabled);
void siwx91x_nwp_set_config(const struct device *dev, uint16_t type, uint16_t value);
void siwx91x_nwp_get_mac_address(const struct device *dev, uint8_t mac[NET_ETH_ADDR_LEN]);

/* Socket APIs */
int siwx91x_nwp_sock_config_ipv4(const struct device *dev, struct net_in_addr *addr,
				 struct net_in_addr *mask, struct net_in_addr *gw);
int siwx91x_nwp_sock_config_ipv6(const struct device *dev, struct net_in6_addr *lua,
				 struct net_in6_addr *gua, int *prefix_len,
				 struct net_in6_addr *gw);
int siwx91x_nwp_sock_bind(const struct device *dev, const struct net_sockaddr *local);
int siwx91x_nwp_sock_connect(const struct device *dev, int sock_type,
			     const struct net_sockaddr *remote);
int siwx91x_nwp_sock_listen(const struct device *dev,
			    const struct net_sockaddr_ptr *local, int backlog);
int siwx91x_nwp_sock_accept(const struct device *dev, int sockfd,
			    const struct net_sockaddr_ptr *local, struct net_sockaddr *remote);
int siwx91x_nwp_sock_close(const struct device *dev, int sockfd);
int siwx91x_nwp_sock_recv(const struct device *dev, int sockfd, struct net_buf **payload);
void siwx91x_nwp_sock_select(const struct device *dev, int select_id,
			     uint32_t read_fds, uint32_t write_fds);

#endif
