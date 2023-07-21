/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 *
 * @brief Utility functions to be used by the Wi-Fi subsystem.
 */

#ifndef ZEPHYR_INCLUDE_NET_WIFI_UTILS_H_
#define ZEPHYR_INCLUDE_NET_WIFI_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup wifi_mgmt
 * @{
 */

/**
 * @name Wi-Fi utility functions.
 *
 * Utility functions for the Wi-Fi subsystem.
 * @{
 */

#define WIFI_UTILS_MAX_BAND_STR_LEN 3
#define WIFI_UTILS_MAX_CHAN_STR_LEN 4

/**
 * @brief Convert a band specification string to a bitmap representing the bands.
 *
 * @details The function will parse a string which specifies Wi-Fi frequency band
 * values as a comma separated string and convert it to a bitmap. The string can
 * use the following characters to represent the bands:
 *
 * - 2: 2.4 GHz
 * - 5: 5 GHz
 * - 6: 6 GHz
 *
 * For the bitmap generated refer to ::wifi_frequency_bands
 * for bit position of each band.
 *
 * E.g. a string "2,5,6" will be converted to a bitmap value of 0x7
 *
 * @param scan_bands_str String which spe.
 * @param band_map Pointer to the bitmap variable to be updated.
 *
 * @retval 0 on success.
 * @retval -errno value in case of failure.
 */
int wifi_utils_parse_scan_bands(char *scan_bands_str, uint8_t *band_map);


/**
 * @brief Convert a string containing a list of SSIDs to an array of SSID strings.
 *
 * @details The function will parse a string which specifies Wi-Fi SSIDs.
 * as a comma separated string and convert it to an array.
 *
 * @param scan_ssids_str List of SSIDs expressed as a comma separated list.
 * @param ssids Pointer to an array where the parsed SSIDs are to be stored.
 *
 * @retval 0 on success.
 * @retval -errno value in case of failure.
 */
int wifi_utils_parse_scan_ssids(char *scan_ssids_str,
				char ssids[][WIFI_SSID_MAX_LEN + 1]);

/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
#endif /* ZEPHYR_INCLUDE_NET_WIFI_UTILS_H_ */
