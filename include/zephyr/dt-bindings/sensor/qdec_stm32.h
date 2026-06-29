/*
 * Copyright 2022 Valerio Setti <vsetti@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for the STM32 quadrature decoder.
 * @ingroup qdec_stm32_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_QDEC_STM32_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_QDEC_STM32_H_

/**
 * @defgroup qdec_stm32_interface QDEC STM32
 * @ingroup sensor_interface_ext_st
 * @brief STM32 timer-based quadrature decoder (QDEC)
 * @{
 */

/**
 * @name Input filter options
 *
 * Digital input filter applied to the timer encoder inputs.
 *
 * FDIVx selects the sampling clock as a fraction of the timer clock (fDTS) and
 * Ny the number of consecutive matching samples required to validate a
 * transition.
 * @{
 */
#define NO_FILTER	0  /**< No filter (sampling at fDTS) */
#define FDIV1_N2	1  /**< fDTS, 2 samples */
#define FDIV1_N4	2  /**< fDTS, 4 samples */
#define FDIV1_N8	3  /**< fDTS, 8 samples */
#define FDIV2_N6	4  /**< fDTS / 2, 6 samples */
#define FDIV2_N8	5  /**< fDTS / 2, 8 samples */
#define FDIV4_N6	6  /**< fDTS / 4, 6 samples */
#define FDIV4_N8	7  /**< fDTS / 4, 8 samples */
#define FDIV8_N6	8  /**< fDTS / 8, 6 samples */
#define FDIV8_N8	9  /**< fDTS / 8, 8 samples */
#define FDIV16_N5	10 /**< fDTS / 16, 5 samples */
#define FDIV16_N6	11 /**< fDTS / 16, 6 samples */
#define FDIV16_N8	12 /**< fDTS / 16, 8 samples */
#define FDIV32_N5	13 /**< fDTS / 32, 5 samples */
#define FDIV32_N6	14 /**< fDTS / 32, 6 samples */
#define FDIV32_N8	15 /**< fDTS / 32, 8 samples */
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_QDEC_STM32_H_ */
