/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SIWG917_NWP_H
#define SIWG917_NWP_H

#include "sl_wifi.h"

#define SIWX91X_INTERFACE_MASK (0x03)
#define DEFAULT_COUNTRY_CODE	"00"

/**
 * @brief Switch the Wi-Fi operating mode.
 *
 * This function switches the Network Processor (NWP) to the specified Wi-Fi
 * operating mode based on the provided features. It performs a soft reboot
 * of the NWP to apply the new mode along with the updated features.
 *
 * @param[in] oper_mode     Wi-Fi operating mode to switch to.
 * @param[in] hidden_ssid   SSID and its length (used only in WIFI_AP_MODE).
 * @param[in] max_num_sta   Maximum number of supported stations (only for WIFI_AP_MODE).
 *
 * @return 0 on success, negative error code on failure.
 */
int siwx91x_nwp_mode_switch(uint8_t oper_mode, bool hidden_ssid, uint8_t max_num_sta);

/**
 * @brief Map an ISO/IEC 3166-1 alpha-2 country code to a Wi-Fi region code.
 *
 * This function maps a 2-character country code (e.g., "US", "FR", "JP")
 * to the corresponding region code defined in the SDK (sl_wifi_region_code_t).
 * If the country is not explicitly listed, it defaults to the US region.
 *
 * @param[in] country_code  Pointer to a 2-character ISO country code.
 *
 * @return Corresponding sl_wifi_region_code_t value.
 */
sl_wifi_region_code_t siwx91x_map_country_code_to_region(const char *country_code);

/**
 * @brief Get the default SDK region configuration for a region code.
 *
 * Looks up the given sl_wifi_region_code_t and returns the corresponding
 * SDK region configuration, or NULL if not found.
 *
 * @param[in] region_code  Wi-Fi region code (SL_WIFI_REGION_*).
 *
 * @return Pointer to SDK region configuration, or NULL if unsupported.
 */
const sli_si91x_set_region_ap_request_t *siwx91x_find_sdk_region_table(uint8_t region_code);

/**
 * @brief Store the country code internally for GET operation.
 *
 * This function saves the provided country code to a static internal buffer.
 *
 * @param[in] country_code  Pointer to a 2-character ISO country code.
 */
int siwx91x_store_country_code(const char *country_code);

/**
 * @brief Retrieve the currently stored country code.
 *
 * This function returns a pointer to the internally stored 2-character
 * country code set by store_country_code().
 *
 * @return Pointer to the stored country code string.
 */
const char *siwx91x_get_country_code(void);

#endif
