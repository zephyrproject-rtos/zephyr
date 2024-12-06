/**
 * Copyright 2023-2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __HAPD_API_H_
#define __HAPD_API_H_

#ifdef CONFIG_WIFI_NM_HOSTAPD_CRYPTO_ENTERPRISE
int hostapd_add_enterprise_creds(const struct device *dev,
			struct wifi_enterprise_creds_params *creds);
#endif

/**
 * @brief Wi-Fi AP configuration parameter.
 *
 * @param dev Wi-Fi device
 * @param params AP parameters
 * @return 0 for OK; -1 for ERROR
 */
int hostapd_ap_config_params(const struct device *dev, struct wifi_ap_config_params *params);

/**
 * @brief Set Wi-Fi AP region domain
 *
 * @param reg_domain region domain parameters
 * @return true for OK; false for ERROR
 */
bool hostapd_ap_reg_domain(struct wifi_reg_domain *reg_domain);

#ifdef CONFIG_WIFI_NM_HOSTAPD_WPS
/** Start AP WPS PBC/PIN
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param params wps operarion parameters
 *
 * @return 0 if ok, < 0 if error
 */
int hostapd_ap_wps_config(const struct device *dev, struct wifi_wps_config_params *params);
#endif

/**
 * @brief Get Wi-Fi SAP status
 *
 * @param dev Wi-Fi device
 * @param status SAP status
 * @return 0 for OK; -1 for ERROR
 */
int hostapd_ap_status(const struct device *dev, struct wifi_iface_status *status);

/**
 * @brief Set Wi-Fi AP configuration
 *
 * @param dev Wi-Fi interface name to use
 * @param params AP configuration parameters to set
 * @return 0 for OK; -1 for ERROR
 */
int hostapd_ap_enable(const struct device *dev,
		      struct wifi_connect_req_params *params);

/**
 * @brief Disable Wi-Fi AP
 * @param dev Wi-Fi interface name to use
 * @return 0 for OK; -1 for ERROR
 */
int hostapd_ap_disable(const struct device *dev);

/**
 * @brief Set Wi-Fi AP STA disconnect
 * @param dev Wi-Fi interface name to use
 * @param mac_addr MAC address of the station to disconnect
 * @return 0 for OK; -1 for ERROR
 */
int hostapd_ap_sta_disconnect(const struct device *dev,
			      const uint8_t *mac_addr);

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_DPP
#ifdef CONFIG_WIFI_NM_HOSTAPD_AP
/**
 * @brief Dispatch DPP operations for AP
 *
 * @param dev Wi-Fi interface name to use
 * @param dpp_params DPP action enum and params in string
 * @return 0 for OK; -1 for ERROR
 */
int hostapd_dpp_dispatch(const struct device *dev, struct wifi_dpp_params *params);
#endif /* CONFIG_WIFI_NM_HOSTAPD_AP */
#endif /* CONFIG_WIFI_NM_WPA_SUPPLICANT_DPP */

#endif /* __HAPD_API_H_ */
