/*
 * Copyright (c) 2026 Michele Vaccari
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ST Zio header pin constants
 * @ingroup st-zio-header
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_ST_ZIO_HEADER_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_ST_ZIO_HEADER_H_

/**
 * @defgroup st-zio-header ST Zio header
 * @brief Constants for pins exposed on ST Zio header
 * @ingroup devicetree-gpio-pin-headers
 * @{
 */

/** ST Zio pin mask (0...99). */
#define ST_ZIO_PIN_MASK 0xFF

/* CN7 connector (upper right) */
#define ST_ZIO_CN7_1  0  /**< CN7 pin 1 */
#define ST_ZIO_CN7_2  1  /**< CN7 pin 2 */
#define ST_ZIO_CN7_3  2  /**< CN7 pin 3 */
#define ST_ZIO_CN7_4  3  /**< CN7 pin 4 */
#define ST_ZIO_CN7_5  4  /**< CN7 pin 5 */
#define ST_ZIO_CN7_6  5  /**< CN7 pin 6 */
#define ST_ZIO_CN7_7  6  /**< CN7 pin 7 */
#define ST_ZIO_CN7_8  7  /**< CN7 pin 8 */
#define ST_ZIO_CN7_9  8  /**< CN7 pin 9 */
#define ST_ZIO_CN7_10 9  /**< CN7 pin 10 */
#define ST_ZIO_CN7_11 10 /**< CN7 pin 11 */
#define ST_ZIO_CN7_12 11 /**< CN7 pin 12 */
#define ST_ZIO_CN7_13 12 /**< CN7 pin 13 */
#define ST_ZIO_CN7_14 13 /**< CN7 pin 14 */
#define ST_ZIO_CN7_15 14 /**< CN7 pin 15 */
#define ST_ZIO_CN7_16 15 /**< CN7 pin 16 */
#define ST_ZIO_CN7_17 16 /**< CN7 pin 17 */
#define ST_ZIO_CN7_18 17 /**< CN7 pin 18 */
#define ST_ZIO_CN7_19 18 /**< CN7 pin 19 */
#define ST_ZIO_CN7_20 19 /**< CN7 pin 20 */

/* CN8 connector (upper left) */
#define ST_ZIO_CN8_1  20 /**< CN8 pin 1 */
#define ST_ZIO_CN8_2  21 /**< CN8 pin 2 */
#define ST_ZIO_CN8_3  22 /**< CN8 pin 3 */
#define ST_ZIO_CN8_4  23 /**< CN8 pin 4 */
#define ST_ZIO_CN8_5  24 /**< CN8 pin 5 */
#define ST_ZIO_CN8_6  25 /**< CN8 pin 6 */
#define ST_ZIO_CN8_7  26 /**< CN8 pin 7 */
#define ST_ZIO_CN8_8  27 /**< CN8 pin 8 */
#define ST_ZIO_CN8_9  28 /**< CN8 pin 9 */
#define ST_ZIO_CN8_10 29 /**< CN8 pin 10 */
#define ST_ZIO_CN8_11 30 /**< CN8 pin 11 */
#define ST_ZIO_CN8_12 31 /**< CN8 pin 12 */
#define ST_ZIO_CN8_13 32 /**< CN8 pin 13 */
#define ST_ZIO_CN8_14 33 /**< CN8 pin 14 */
#define ST_ZIO_CN8_15 34 /**< CN8 pin 15 */
#define ST_ZIO_CN8_16 35 /**< CN8 pin 16 */

/* CN9 connector (lower left) */
#define ST_ZIO_CN9_1  36 /**< CN9 pin 1 */
#define ST_ZIO_CN9_2  37 /**< CN9 pin 2 */
#define ST_ZIO_CN9_3  38 /**< CN9 pin 3 */
#define ST_ZIO_CN9_4  39 /**< CN9 pin 4 */
#define ST_ZIO_CN9_5  40 /**< CN9 pin 5 */
#define ST_ZIO_CN9_6  41 /**< CN9 pin 6 */
#define ST_ZIO_CN9_7  42 /**< CN9 pin 7 */
#define ST_ZIO_CN9_8  43 /**< CN9 pin 8 */
#define ST_ZIO_CN9_9  44 /**< CN9 pin 9 */
#define ST_ZIO_CN9_10 45 /**< CN9 pin 10 */
#define ST_ZIO_CN9_11 46 /**< CN9 pin 11 */
#define ST_ZIO_CN9_12 47 /**< CN9 pin 12 */
#define ST_ZIO_CN9_13 48 /**< CN9 pin 13 */
#define ST_ZIO_CN9_14 49 /**< CN9 pin 14 */
#define ST_ZIO_CN9_15 50 /**< CN9 pin 15 */
#define ST_ZIO_CN9_16 51 /**< CN9 pin 16 */
#define ST_ZIO_CN9_17 52 /**< CN9 pin 17 */
#define ST_ZIO_CN9_18 53 /**< CN9 pin 18 */
#define ST_ZIO_CN9_19 54 /**< CN9 pin 19 */
#define ST_ZIO_CN9_20 55 /**< CN9 pin 20 */
#define ST_ZIO_CN9_21 56 /**< CN9 pin 21 */
#define ST_ZIO_CN9_22 57 /**< CN9 pin 22 */
#define ST_ZIO_CN9_23 58 /**< CN9 pin 23 */
#define ST_ZIO_CN9_24 59 /**< CN9 pin 24 */
#define ST_ZIO_CN9_25 60 /**< CN9 pin 25 */
#define ST_ZIO_CN9_26 61 /**< CN9 pin 26 */
#define ST_ZIO_CN9_27 62 /**< CN9 pin 27 */
#define ST_ZIO_CN9_28 63 /**< CN9 pin 28 */
#define ST_ZIO_CN9_29 64 /**< CN9 pin 29 */
#define ST_ZIO_CN9_30 65 /**< CN9 pin 30 */

/* CN10 connector (lower right) */
#define ST_ZIO_CN10_1  66 /**< CN10 pin 1 */
#define ST_ZIO_CN10_2  67 /**< CN10 pin 2 */
#define ST_ZIO_CN10_3  68 /**< CN10 pin 3 */
#define ST_ZIO_CN10_4  69 /**< CN10 pin 4 */
#define ST_ZIO_CN10_5  70 /**< CN10 pin 5 */
#define ST_ZIO_CN10_6  71 /**< CN10 pin 6 */
#define ST_ZIO_CN10_7  72 /**< CN10 pin 7 */
#define ST_ZIO_CN10_8  73 /**< CN10 pin 8 */
#define ST_ZIO_CN10_9  74 /**< CN10 pin 9 */
#define ST_ZIO_CN10_10 75 /**< CN10 pin 10 */
#define ST_ZIO_CN10_11 76 /**< CN10 pin 11 */
#define ST_ZIO_CN10_12 77 /**< CN10 pin 12 */
#define ST_ZIO_CN10_13 78 /**< CN10 pin 13 */
#define ST_ZIO_CN10_14 79 /**< CN10 pin 14 */
#define ST_ZIO_CN10_15 80 /**< CN10 pin 15 */
#define ST_ZIO_CN10_16 81 /**< CN10 pin 16 */
#define ST_ZIO_CN10_17 82 /**< CN10 pin 17 */
#define ST_ZIO_CN10_18 83 /**< CN10 pin 18 */
#define ST_ZIO_CN10_19 84 /**< CN10 pin 19 */
#define ST_ZIO_CN10_20 85 /**< CN10 pin 20 */
#define ST_ZIO_CN10_21 86 /**< CN10 pin 21 */
#define ST_ZIO_CN10_22 87 /**< CN10 pin 22 */
#define ST_ZIO_CN10_23 88 /**< CN10 pin 23 */
#define ST_ZIO_CN10_24 89 /**< CN10 pin 24 */
#define ST_ZIO_CN10_25 90 /**< CN10 pin 25 */
#define ST_ZIO_CN10_26 91 /**< CN10 pin 26 */
#define ST_ZIO_CN10_27 92 /**< CN10 pin 27 */
#define ST_ZIO_CN10_28 93 /**< CN10 pin 28 */
#define ST_ZIO_CN10_29 94 /**< CN10 pin 29 */
#define ST_ZIO_CN10_30 95 /**< CN10 pin 30 */
#define ST_ZIO_CN10_31 96 /**< CN10 pin 31 */
#define ST_ZIO_CN10_32 97 /**< CN10 pin 32 */
#define ST_ZIO_CN10_33 98 /**< CN10 pin 33 */
#define ST_ZIO_CN10_34 99 /**< CN10 pin 34 */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_ST_ZIO_HEADER_H_ */
