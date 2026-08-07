/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Header containing Coexistence APIs.
 */

#ifndef __COEX_H__
#define __COEX_H__

#include <stdbool.h>

/* Indicates WLAN frequency band of operation */
enum nrf_wifi_pta_wlan_op_band {
	NRF_WIFI_PTA_WLAN_OP_BAND_2_4_GHZ = 0,
	NRF_WIFI_PTA_WLAN_OP_BAND_5_GHZ,
	NRF_WIFI_PTA_WLAN_OP_BAND_NONE = 0xFF
};

/**
 * @function   nrf_wifi_coex_config_pta(enum nrf_wifi_pta_wlan_op_band wlan_band,
 *             bool separate_antennas, bool is_sr_protocol_ble)
 *
 * @brief      Function used to configure PTA tables of coexistence hardware.
 *
 * @param[in]  enum nrf_wifi_pta_wlan_op_band wlan_band
 * @param[in]  separate_antennas
 *             Indicates whether separate antenans are used or not.
 * @param[in]  is_sr_protocol_ble
 *             Indicates if SR protocol is Bluetooth LE or not.
 * @return     Returns status of configuration.
 *             Returns zero upon successful configuration.
 *             Returns non-zero upon unsuccessful configuration.
 */
int nrf_wifi_coex_config_pta(enum nrf_wifi_pta_wlan_op_band wlan_band, bool separate_antennas,
	bool is_sr_protocol_ble);

#if defined(CONFIG_NRF70_SR_COEX_RF_SWITCH) || defined(__DOXYGEN__)
/**
 * @function   nrf_wifi_config_sr_switch(bool separate_antennas)
 *
 * @brief      Function used to configure SR side switch (nRF5340 side switch in nRF7002 DK).
 *
 * @param[in]  separate_antennas
 *               Indicates whether separate antenans are used or not.
 *
 * @return     Returns status of configuration.
 *             Returns zero upon successful configuration.
 *             Returns non-zero upon unsuccessful configuration.
 */
int nrf_wifi_config_sr_switch(bool separate_antennas);
#endif /* CONFIG_NRF70_SR_COEX_RF_SWITCH */

/**
 * @function   nrf_wifi_coex_config_non_pta(bool separate_antennas)
 *
 * @brief      Function used to configure non-PTA registers of coexistence hardware.
 *
 * @param[in]  separate_antennas
 *             Indicates whether separate antenans are used or not.
 * @param[in]  is_sr_protocol_ble
 *             Indicates if SR protocol is Bluetooth LE or not.
 *
 * @return     Returns status of configuration.
 *             Returns zero upon successful configuration.
 *             Returns non-zero upon unsuccessful configuration.
 */
int nrf_wifi_coex_config_non_pta(bool separate_antennas, bool is_sr_protocol_ble);

/**
 * @function   nrf_wifi_coex_hw_reset(void)
 *
 * @brief      Function used to reset coexistence hardware.
 *
 * @return     Returns status of configuration.
 *             Returns zero upon successful configuration.
 *             Returns non-zero upon unsuccessful configuration.
 */
int nrf_wifi_coex_hw_reset(void);

#endif /* __COEX_H__ */
