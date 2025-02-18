/*
 * Copyright (c) 2018 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DTS_COMMON_MEM_H_
#define ZEPHYR_DTS_COMMON_MEM_H_

/**
 * @defgroup dts_apis DTS APIs
 * @brief APIs that can only be used in DTS (.dts, .dtsi, .overlay) files
 */

/**
 * @defgroup dts_utils Basic utilities for DTS
 * @brief Unit definitions etc
 *
 * @ingroup dts_apis
 * @{
 */

/**
 * @brief Converts a size value in kilobytes to bytes.
 *
 * @param x Size value in kilobytes.
 * @return Size value in bytes.
 */
#define DT_SIZE_K(x) ((x) * 1024)

/**
 * @brief Converts a size value in megabytes to bytes.
 *
 * @param x Size value in megabytes.
 * @return Size value in bytes.
 */
#define DT_SIZE_M(x) (DT_SIZE_K(x) * 1024)

/**
 * @brief Concatenates two values into a single token.
 *
 * This macro concatenates the two provided arguments `x` and `y` into a single token.
 *
 * @param x First part of the token.
 * @param y Second part of the token.
 * @return The concatenated token.
 */
#define _DT_DO_CONCAT(x, y) x##y

/**
 * @brief Converts a hexadecimal address fragment into a full address.
 *
 * This macro prefixes the provided hexadecimal fragment `a` with `0x` to form a complete address.
 *
 * @param a Hexadecimal address fragment (without `0x` prefix).
 * @return Full hexadecimal address with `0x` prefix.
 */
#define DT_ADDR(a) _DT_DO_CONCAT(0x, a)

/**
 * @}
 */

#endif /* ZEPHYR_DTS_COMMON_MEM_H_ */
