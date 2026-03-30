/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup wuc_interface
 * @brief NXP LLWU wakeup source encodings.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_WUC_NXP_LLWU_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_WUC_NXP_LLWU_H_

/**
 * @brief Wakeup source encoding layout.
 *
 * Bit[31]: 0 = PIN, 1 = MODULE
 *
 * For PIN (bit[31] = 0):
 *   - Bit[7:0]  : pin index (0-255)
 *   - Bit[9:8]  : edge type (1 = rising, 2 = falling, 3 = any)
 *
 * For MODULE (bit[31] = 1):
 *   - Bit[7:0]  : module index (0-255)
 */

/** @name Wakeup source type encodings */
/** @{ */
/** Wakeup source type encoding for a PIN source. */
#define NXP_LLWU_WAKEUP_SOURCE_TYPE_PIN    (0U << 31)
/** Wakeup source type encoding for a MODULE source. */
#define NXP_LLWU_WAKEUP_SOURCE_TYPE_MODULE (1U << 31)
/** @} */

/** @name PIN edge encodings */
/** @{ */
/** PIN edge encoding for rising edge. */
#define NXP_LLWU_PIN_EDGE_RISING  1
/** PIN edge encoding for falling edge. */
#define NXP_LLWU_PIN_EDGE_FALLING 2
/** PIN edge encoding for any edge. */
#define NXP_LLWU_PIN_EDGE_ANY     3
/** @} */

/**
 * @brief Encode a PIN wakeup source.
 *
 * @param pin_index PIN index (0-255).
 * @param edge_type Edge encoding (see NXP_LLWU_PIN_EDGE_*).
 */
#define NXP_LLWU_PIN_WAKEUP(pin_index, edge_type)                                                  \
	(NXP_LLWU_WAKEUP_SOURCE_TYPE_PIN | (((edge_type) & 0x3) << 8) | ((pin_index) & 0xFF))

/**
 * @brief Encode a MODULE wakeup source.
 *
 * @param module_index Module index (0-255).
 */
#define NXP_LLWU_MODULE_WAKEUP(module_index)                                                       \
	(NXP_LLWU_WAKEUP_SOURCE_TYPE_MODULE | ((module_index) & 0xFF))

/** @name PIN wakeup sources (PIN 0-15) */
/** @{ */
/** PIN 0 wakeup source on rising edge. */
#define NXP_LLWU_PIN_0_RISING  NXP_LLWU_PIN_WAKEUP(1, NXP_LLWU_PIN_EDGE_RISING)
/** PIN 0 wakeup source on falling edge. */
#define NXP_LLWU_PIN_0_FALLING NXP_LLWU_PIN_WAKEUP(1, NXP_LLWU_PIN_EDGE_FALLING)
/** PIN 0 wakeup source on any edge. */
#define NXP_LLWU_PIN_0_ANY     NXP_LLWU_PIN_WAKEUP(1, NXP_LLWU_PIN_EDGE_ANY)

/** PIN 1 wakeup source on rising edge. */
#define NXP_LLWU_PIN_1_RISING  NXP_LLWU_PIN_WAKEUP(2, NXP_LLWU_PIN_EDGE_RISING)
/** PIN 1 wakeup source on falling edge. */
#define NXP_LLWU_PIN_1_FALLING NXP_LLWU_PIN_WAKEUP(2, NXP_LLWU_PIN_EDGE_FALLING)
/** PIN 1 wakeup source on any edge. */
#define NXP_LLWU_PIN_1_ANY     NXP_LLWU_PIN_WAKEUP(2, NXP_LLWU_PIN_EDGE_ANY)

/** PIN 2 wakeup source on rising edge. */
#define NXP_LLWU_PIN_2_RISING  NXP_LLWU_PIN_WAKEUP(3, NXP_LLWU_PIN_EDGE_RISING)
/** PIN 2 wakeup source on falling edge. */
#define NXP_LLWU_PIN_2_FALLING NXP_LLWU_PIN_WAKEUP(3, NXP_LLWU_PIN_EDGE_FALLING)
/** PIN 2 wakeup source on any edge. */
#define NXP_LLWU_PIN_2_ANY     NXP_LLWU_PIN_WAKEUP(3, NXP_LLWU_PIN_EDGE_ANY)

/** PIN 3 wakeup source on rising edge. */
#define NXP_LLWU_PIN_3_RISING  NXP_LLWU_PIN_WAKEUP(4, NXP_LLWU_PIN_EDGE_RISING)
/** PIN 3 wakeup source on falling edge. */
#define NXP_LLWU_PIN_3_FALLING NXP_LLWU_PIN_WAKEUP(4, NXP_LLWU_PIN_EDGE_FALLING)
/** PIN 3 wakeup source on any edge. */
#define NXP_LLWU_PIN_3_ANY     NXP_LLWU_PIN_WAKEUP(4, NXP_LLWU_PIN_EDGE_ANY)

/** PIN 4 wakeup source on rising edge. */
#define NXP_LLWU_PIN_4_RISING  NXP_LLWU_PIN_WAKEUP(5, NXP_LLWU_PIN_EDGE_RISING)
/** PIN 4 wakeup source on falling edge. */
#define NXP_LLWU_PIN_4_FALLING NXP_LLWU_PIN_WAKEUP(5, NXP_LLWU_PIN_EDGE_FALLING)
/** PIN 4 wakeup source on any edge. */
#define NXP_LLWU_PIN_4_ANY     NXP_LLWU_PIN_WAKEUP(5, NXP_LLWU_PIN_EDGE_ANY)

/** PIN 5 wakeup source on rising edge. */
#define NXP_LLWU_PIN_5_RISING  NXP_LLWU_PIN_WAKEUP(6, NXP_LLWU_PIN_EDGE_RISING)
/** PIN 5 wakeup source on falling edge. */
#define NXP_LLWU_PIN_5_FALLING NXP_LLWU_PIN_WAKEUP(6, NXP_LLWU_PIN_EDGE_FALLING)
/** PIN 5 wakeup source on any edge. */
#define NXP_LLWU_PIN_5_ANY     NXP_LLWU_PIN_WAKEUP(6, NXP_LLWU_PIN_EDGE_ANY)

/** PIN 6 wakeup source on rising edge. */
#define NXP_LLWU_PIN_6_RISING  NXP_LLWU_PIN_WAKEUP(7, NXP_LLWU_PIN_EDGE_RISING)
/** PIN 6 wakeup source on falling edge. */
#define NXP_LLWU_PIN_6_FALLING NXP_LLWU_PIN_WAKEUP(7, NXP_LLWU_PIN_EDGE_FALLING)
/** PIN 6 wakeup source on any edge. */
#define NXP_LLWU_PIN_6_ANY     NXP_LLWU_PIN_WAKEUP(7, NXP_LLWU_PIN_EDGE_ANY)

/** PIN 7 wakeup source on rising edge. */
#define NXP_LLWU_PIN_7_RISING  NXP_LLWU_PIN_WAKEUP(8, NXP_LLWU_PIN_EDGE_RISING)
/** PIN 7 wakeup source on falling edge. */
#define NXP_LLWU_PIN_7_FALLING NXP_LLWU_PIN_WAKEUP(8, NXP_LLWU_PIN_EDGE_FALLING)
/** PIN 7 wakeup source on any edge. */
#define NXP_LLWU_PIN_7_ANY     NXP_LLWU_PIN_WAKEUP(8, NXP_LLWU_PIN_EDGE_ANY)

/** PIN 8 wakeup source on rising edge. */
#define NXP_LLWU_PIN_8_RISING  NXP_LLWU_PIN_WAKEUP(9, NXP_LLWU_PIN_EDGE_RISING)
/** PIN 8 wakeup source on falling edge. */
#define NXP_LLWU_PIN_8_FALLING NXP_LLWU_PIN_WAKEUP(9, NXP_LLWU_PIN_EDGE_FALLING)
/** PIN 8 wakeup source on any edge. */
#define NXP_LLWU_PIN_8_ANY     NXP_LLWU_PIN_WAKEUP(9, NXP_LLWU_PIN_EDGE_ANY)

/** PIN 9 wakeup source on rising edge. */
#define NXP_LLWU_PIN_9_RISING  NXP_LLWU_PIN_WAKEUP(10, NXP_LLWU_PIN_EDGE_RISING)
/** PIN 9 wakeup source on falling edge. */
#define NXP_LLWU_PIN_9_FALLING NXP_LLWU_PIN_WAKEUP(10, NXP_LLWU_PIN_EDGE_FALLING)
/** PIN 9 wakeup source on any edge. */
#define NXP_LLWU_PIN_9_ANY     NXP_LLWU_PIN_WAKEUP(10, NXP_LLWU_PIN_EDGE_ANY)

/** PIN 10 wakeup source on rising edge. */
#define NXP_LLWU_PIN_10_RISING  NXP_LLWU_PIN_WAKEUP(11, NXP_LLWU_PIN_EDGE_RISING)
/** PIN 10 wakeup source on falling edge. */
#define NXP_LLWU_PIN_10_FALLING NXP_LLWU_PIN_WAKEUP(11, NXP_LLWU_PIN_EDGE_FALLING)
/** PIN 10 wakeup source on any edge. */
#define NXP_LLWU_PIN_10_ANY     NXP_LLWU_PIN_WAKEUP(11, NXP_LLWU_PIN_EDGE_ANY)

/** PIN 11 wakeup source on rising edge. */
#define NXP_LLWU_PIN_11_RISING  NXP_LLWU_PIN_WAKEUP(12, NXP_LLWU_PIN_EDGE_RISING)
/** PIN 11 wakeup source on falling edge. */
#define NXP_LLWU_PIN_11_FALLING NXP_LLWU_PIN_WAKEUP(12, NXP_LLWU_PIN_EDGE_FALLING)
/** PIN 11 wakeup source on any edge. */
#define NXP_LLWU_PIN_11_ANY     NXP_LLWU_PIN_WAKEUP(12, NXP_LLWU_PIN_EDGE_ANY)

/** PIN 12 wakeup source on rising edge. */
#define NXP_LLWU_PIN_12_RISING  NXP_LLWU_PIN_WAKEUP(13, NXP_LLWU_PIN_EDGE_RISING)
/** PIN 12 wakeup source on falling edge. */
#define NXP_LLWU_PIN_12_FALLING NXP_LLWU_PIN_WAKEUP(13, NXP_LLWU_PIN_EDGE_FALLING)
/** PIN 12 wakeup source on any edge. */
#define NXP_LLWU_PIN_12_ANY     NXP_LLWU_PIN_WAKEUP(13, NXP_LLWU_PIN_EDGE_ANY)

/** PIN 13 wakeup source on rising edge. */
#define NXP_LLWU_PIN_13_RISING  NXP_LLWU_PIN_WAKEUP(14, NXP_LLWU_PIN_EDGE_RISING)
/** PIN 13 wakeup source on falling edge. */
#define NXP_LLWU_PIN_13_FALLING NXP_LLWU_PIN_WAKEUP(14, NXP_LLWU_PIN_EDGE_FALLING)
/** PIN 13 wakeup source on any edge. */
#define NXP_LLWU_PIN_13_ANY     NXP_LLWU_PIN_WAKEUP(14, NXP_LLWU_PIN_EDGE_ANY)

/** PIN 14 wakeup source on rising edge. */
#define NXP_LLWU_PIN_14_RISING  NXP_LLWU_PIN_WAKEUP(15, NXP_LLWU_PIN_EDGE_RISING)
/** PIN 14 wakeup source on falling edge. */
#define NXP_LLWU_PIN_14_FALLING NXP_LLWU_PIN_WAKEUP(15, NXP_LLWU_PIN_EDGE_FALLING)
/** PIN 14 wakeup source on any edge. */
#define NXP_LLWU_PIN_14_ANY     NXP_LLWU_PIN_WAKEUP(15, NXP_LLWU_PIN_EDGE_ANY)

/** PIN 15 wakeup source on rising edge. */
#define NXP_LLWU_PIN_15_RISING  NXP_LLWU_PIN_WAKEUP(16, NXP_LLWU_PIN_EDGE_RISING)
/** PIN 15 wakeup source on falling edge. */
#define NXP_LLWU_PIN_15_FALLING NXP_LLWU_PIN_WAKEUP(16, NXP_LLWU_PIN_EDGE_FALLING)
/** PIN 15 wakeup source on any edge. */
#define NXP_LLWU_PIN_15_ANY     NXP_LLWU_PIN_WAKEUP(16, NXP_LLWU_PIN_EDGE_ANY)
/** @} */

/** @name MODULE wakeup sources (MODULE 0-7) */
/** @{ */
/** MODULE 0 wakeup source. */
#define NXP_LLWU_MODULE_0 NXP_LLWU_MODULE_WAKEUP(1)
/** MODULE 1 wakeup source. */
#define NXP_LLWU_MODULE_1 NXP_LLWU_MODULE_WAKEUP(2)
/** MODULE 2 wakeup source. */
#define NXP_LLWU_MODULE_2 NXP_LLWU_MODULE_WAKEUP(3)
/** MODULE 3 wakeup source. */
#define NXP_LLWU_MODULE_3 NXP_LLWU_MODULE_WAKEUP(4)
/** MODULE 4 wakeup source. */
#define NXP_LLWU_MODULE_4 NXP_LLWU_MODULE_WAKEUP(5)
/** MODULE 5 wakeup source. */
#define NXP_LLWU_MODULE_5 NXP_LLWU_MODULE_WAKEUP(6)
/** MODULE 6 wakeup source. */
#define NXP_LLWU_MODULE_6 NXP_LLWU_MODULE_WAKEUP(7)
/** MODULE 7 wakeup source. */
#define NXP_LLWU_MODULE_7 NXP_LLWU_MODULE_WAKEUP(8)
/** @} */

/** @name Wakeup source field helpers */
/** @{ */
/** Check if a wakeup source encodes a PIN source. */
#define NXP_LLWU_IS_PIN_SOURCE(source)    (((source) & (1U << 31)) == 0)
/** Check if a wakeup source encodes a MODULE source. */
#define NXP_LLWU_IS_MODULE_SOURCE(source) (((source) & (1U << 31)) != 0)
/** Extract the PIN index from a wakeup source. */
#define NXP_LLWU_GET_PIN_INDEX(source)    ((source) & 0xFF)
/** Extract the PIN edge encoding from a wakeup source. */
#define NXP_LLWU_GET_PIN_EDGE(source)     (((source) >> 8) & 0x3)
/** Extract the MODULE index from a wakeup source. */
#define NXP_LLWU_GET_MODULE_INDEX(source) ((source) & 0xFF)
/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_WUC_NXP_LLWU_H_ */
