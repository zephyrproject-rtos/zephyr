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
 * @brief Append a string containing an SSID to an array of SSID strings.
 *
 * @param scan_ssids_str string to be appended in the list of scanned SSIDs.
 * @param ssids Pointer to an array where the SSIDs pointers are to be stored.
 * @param num_ssids Maximum number of SSIDs that can be stored.
 *
 * @retval 0 on success.
 * @retval -errno value in case of failure.
 */
int wifi_utils_parse_scan_ssids(char *scan_ssids_str,
				const char *ssids[],
				uint8_t num_ssids);


/**
 * @brief Convert a string containing a specification of scan channels to an array.
 *
 * @details The function will parse a string which specifies channels to be scanned
 * as a string and convert it to an array.
 *
 * The channel string has to be formatted using the colon (:), comma(,), hyphen (-) and
 * underscore (_) delimiters as follows:
 *	- A colon identifies the value preceding it as a band. A band value
 *	  (2: 2.4 GHz, 5: 5 GHz 6: 6 GHz) has to precede the channels in that band (e.g. 2: etc)
 *	- Hyphens (-) are used to identify channel ranges (e.g. 2-7, 32-48 etc)
 *	- Commas are used to separate channel values within a band. Channels can be specified
 *	  as individual values (2,6,48 etc) or channel ranges using hyphens (1-14, 32-48 etc)
 *	- Underscores (_) are used to specify multiple band-channel sets (e.g. 2:1,2_5:36,40 etc)
 *	- No spaces should be used anywhere, i.e. before/after commas,
 *	  before/after hyphens etc.
 *
 * An example channel specification specifying channels in the 2.4 GHz and 5 GHz bands is
 * as below:
 *	2:1,5,7,9-11_5:36-48,100,163-167
 *
 * @param scan_chan_str List of channels expressed in the format described above.
 * @param chan Pointer to an array where the parsed channels are to be stored.
 * @param max_channels Maximum number of channels to store
 *
 * @retval 0 on success.
 * @retval -errno value in case of failure.
 */
int wifi_utils_parse_scan_chan(char *scan_chan_str,
			       struct wifi_band_channel *chan,
			       uint8_t max_channels);

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
