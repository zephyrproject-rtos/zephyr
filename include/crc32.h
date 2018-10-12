/*
 * Copyright (c) 2018 Workaround GmbH.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief CRC 32 computation function
 */

#ifndef ZEPHYR_INCLUDE_CRC32_H_
#define ZEPHYR_INCLUDE_CRC32_H_

#include <zephyr/types.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @defgroup checksum Checksum
 */

/**
 * @defgroup crc32 CRC 32
 * @ingroup checksum
 * @{
 */

/**
 * @brief Generate IEEE conform CRC32 checksum.
 *
 * @param  *data        Pointer to data on which the CRC should be calculated.
 * @param  len          Data length.
 *
 * @return CRC32 value.
 *
 */
u32_t crc32_ieee(const u8_t *data, size_t len);

/**
 * @brief Update an IEEE conforming CRC32 checksum.
 *
 * @param crc   CRC32 checksum that needs to be updated.
 * @param *data Pointer to data on which the CRC should be calculated.
 * @param len   Data length.
 *
 * @return CRC32 value.
 *
 */
u32_t crc32_ieee_update(u32_t crc, const u8_t *data, size_t len);

/**
 * @}
 */
#endif
