/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup pmbus
 * @brief PMBus data format conversion.
 */

#ifndef ZEPHYR_INCLUDE_PMBUS_PMBUS_FORMAT_H_
#define ZEPHYR_INCLUDE_PMBUS_PMBUS_FORMAT_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup pmbus
 * @{
 */

/**
 * @name Data format conversion
 *
 * Values are in milli units: mV, mA, mW or milli degree Celsius.
 * @{
 */

/**
 * @brief Convert a LINEAR11 word to a value.
 *
 * @param raw LINEAR11 word.
 * @param val Value in milli units.
 *
 * @retval 0 on success.
 * @retval -ERANGE if the value does not fit in 32 bits.
 */
int pmbus_linear11_to_milli(uint16_t raw, int32_t *val);

/**
 * @brief Convert a value to a LINEAR11 word with the best precision.
 *
 * @param val Value in milli units.
 * @param raw LINEAR11 word.
 *
 * @retval 0 on success.
 * @retval -ERANGE if the value is too large.
 */
int pmbus_milli_to_linear11(int32_t val, uint16_t *raw);

/**
 * @brief Convert a ULINEAR16 word to a value.
 *
 * @param raw Mantissa.
 * @param exp Exponent, from the VOUT_MODE parameter field.
 * @param val Value in milli units.
 *
 * @retval 0 on success.
 * @retval -ERANGE if the value does not fit in 32 bits.
 */
int pmbus_ulinear16_to_milli(uint16_t raw, int8_t exp, int32_t *val);

/**
 * @brief Convert a value to a ULINEAR16 word.
 *
 * @param val Value in milli units.
 * @param exp Exponent, from the VOUT_MODE parameter field.
 * @param raw Mantissa.
 *
 * @retval 0 on success.
 * @retval -ERANGE if the value cannot be encoded.
 */
int pmbus_milli_to_ulinear16(int32_t val, int8_t exp, uint16_t *raw);

/** @} */

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_PMBUS_PMBUS_FORMAT_H_ */
