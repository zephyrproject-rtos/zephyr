/*
 * Copyright (c) 2025 Netfeasa Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * hl78xx_cfg.h
 *
 * Helper APIs for RAT, band and APN configuration extracted from hl78xx.c
 * to keep the state machine file smaller and easier to read.
 */
#ifndef ZEPHYR_DRIVERS_MODEM_HL78XX_HL78XX_CFG_H_
#define ZEPHYR_DRIVERS_MODEM_HL78XX_HL78XX_CFG_H_

#include <zephyr/types.h>
#include <stdbool.h>
#include "hl78xx.h"

int hl78xx_rat_cfg(struct hl78xx_data *data, bool *modem_require_restart,
		   enum hl78xx_cell_rat_mode *rat_request);

int hl78xx_band_cfg(struct hl78xx_data *data, bool *modem_require_restart,
		    enum hl78xx_cell_rat_mode rat_config_request);

int hl78xx_set_apn_internal(struct hl78xx_data *data, const char *apn, uint16_t size);

/**
 * @brief Convert a binary bitmap to a trimmed hexadecimal string.
 *
 * Converts a bitmap into a hex string, removing leading zeros for a
 * compact representation. Useful for modem configuration commands.
 *
 * @param bitmap Pointer to the input binary bitmap.
 * @param hex_str Output buffer for the resulting hex string.
 * @param hex_str_len Size of the output buffer in bytes.
 */
void hl78xx_bitmap_to_hex_string_trimmed(const uint8_t *bitmap, char *hex_str, size_t hex_str_len);

/**
 * @brief Trim leading zeros from a hexadecimal string.
 *
 * Removes any '0' characters from the beginning of the provided hex string,
 * returning a pointer to the first non-zero character.
 *
 * @param hex_str Null-terminated hexadecimal string.
 *
 * @return Pointer to the first non-zero digit in the string,
 *         or the last zero if the string is all zeros.
 */
const char *hl78xx_trim_leading_zeros(const char *hex_str);

/**
 * @brief hl78xx_extract_essential_part_apn - Extract the essential part of the APN.
 * @param full_apn Full APN string.
 * @param essential_apn Buffer to store the essential part of the APN.
 * @param max_len Maximum length of the essential APN buffer.
 */
void hl78xx_extract_essential_part_apn(const char *full_apn, char *essential_apn, size_t max_len);

#endif /* ZEPHYR_DRIVERS_MODEM_HL78XX_HL78XX_CFG_H_ */
