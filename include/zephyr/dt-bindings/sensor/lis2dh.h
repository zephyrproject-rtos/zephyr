/*
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for the ST LIS2DH 3-axis accelerometer.
 * @ingroup lis2dh_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_ST_LIS2DH_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_ST_LIS2DH_H_

/**
 * @addtogroup lis2dh_interface
 * @{
 */

/**
 * @name GPIO interrupt configuration
 *
 * Values for the `int1-gpio-config` and `int2-gpio-config` devicetree properties.
 * @{
 */
#define LIS2DH_DT_GPIO_INT_EDGE_BOTH		0 /**< Trigger on both signal edges */
#define LIS2DH_DT_GPIO_INT_EDGE_RISING		1 /**< Trigger on the rising edge */
#define LIS2DH_DT_GPIO_INT_EDGE_FALLING		2 /**< Trigger on the falling edge */
#define LIS2DH_DT_GPIO_INT_LEVEL_HIGH		3 /**< Trigger while the signal is high */
#define LIS2DH_DT_GPIO_INT_LEVEL_LOW		4 /**< Trigger while the signal is low */
/** @} */

/**
 * @name Any-motion detection mode
 *
 * Values for the `anym-mode` devicetree property.
 * @{
 */
#define LIS2DH_DT_ANYM_OR_COMBINATION		0 /**< OR combination of enabled axes */
#define LIS2DH_DT_ANYM_6D_MOVEMENT		1 /**< 6-direction movement recognition */
#define LIS2DH_DT_ANYM_AND_COMBINATION		2 /**< AND combination of enabled axes */
#define LIS2DH_DT_ANYM_6D_POSITION		3 /**< 6-direction position recognition */
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_ST_LIS2DH_H_ */
