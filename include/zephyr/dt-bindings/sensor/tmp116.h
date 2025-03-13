/*
 * Copyright (c) 2024 Vitrolife A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_TI_TMP116_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_TI_TMP116_H_

/**
 * @defgroup TMP116 Texas Instruments (TI) TMP116 DT Options
 * @ingroup sensor_interface
 * @{
 */

/**
 * @defgroup TMP116_ODR Temperature output data rate
 * @{
 */
#define TMP116_DT_ODR_15_5_MS  0
#define TMP116_DT_ODR_125_MS   0x80
#define TMP116_DT_ODR_250_MS   0x100
#define TMP116_DT_ODR_500_MS   0x180
#define TMP116_DT_ODR_1000_MS  0x200
#define TMP116_DT_ODR_4000_MS  0x280
#define TMP116_DT_ODR_8000_MS  0x300
#define TMP116_DT_ODR_16000_MS 0x380
/** @} */

/**
 * @defgroup TMP116_OS Temperature average sample count
 * @{
 */
#define TMP116_DT_OVERSAMPLING_1   0
#define TMP116_DT_OVERSAMPLING_8   0x20
#define TMP116_DT_OVERSAMPLING_32  0x40
#define TMP116_DT_OVERSAMPLING_64  0x60
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_TI_TMP116_H_ */
