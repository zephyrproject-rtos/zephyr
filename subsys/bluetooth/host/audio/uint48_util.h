/** @file
 *  @brief Bluetooth Media Control Service - d09r01 IOP1 2019-08-22
 */

/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_UINT48_TOOLS_
#define ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_UINT48_TOOLS_


#include <zephyr/types.h>

#define UINT48_LEN 6
#define UINT48_STR_LEN ((2*UINT48_LEN)+1)

/** @brief Convert from UINT48 (six byte) LSB first array to uint64_t */
uint64_t uint48array_to_u64(uint8_t arr[]);

/** @brief Convert lower six bytes of an uint64_t to an UINT48 LSB first array */
void u64_to_uint48array(uint64_t val, uint8_t arr[]);

/** @brief Convert six bytes of uint64_t to hex string
 *
 * Convert the lower six bytes of an uint64_t to an MSB first hex string
 * representation of an UINT48 LSB first array
 */
void u64_to_uint48array_str(uint64_t val, char str[]);

/** @brief  Convert an UINT48 LSB first array to an MSB first hex string */
void uint48array_str(uint8_t array[], char str[]);


#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_UINT48_TOOLS_ */
