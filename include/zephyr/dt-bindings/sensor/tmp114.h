/*
 * Copyright (c) 2024 Fredrik Gihl
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_TI_TMP114_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_TI_TMP114_H_

/**
 * @defgroup TMP114 Texas Instruments (TI) TMP114 DT Options
 * @ingroup sensor_interface
 * @{
 */

/**
 * @defgroup TMP114_ODR Temperature output data rate
 * @{
 */
#define TMP114_DT_ODR_6_4_MS   0
#define TMP114_DT_ODR_31_25_MS 1
#define TMP114_DT_ODR_62_5_MS  2
#define TMP114_DT_ODR_125_MS   3
#define TMP114_DT_ODR_250_MS   4
#define TMP114_DT_ODR_500_MS   5
#define TMP114_DT_ODR_1000_MS  6
#define TMP114_DT_ODR_2000_MS  7
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_TI_TMP114_H_ */
