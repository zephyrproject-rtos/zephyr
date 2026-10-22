/*
 * Copyright (c) 2021 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for the Nuvoton NPCX tachometer.
 * @ingroup npcx_tach_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_NPCX_TACH_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_NPCX_TACH_H_

/**
 * @defgroup npcx_tach_interface NPCX tachometer
 * @ingroup sensor_interface_ext_nuvoton
 * @brief Nuvoton NPCX tachometer
 * @{
 */

/**
 * @name Tachometer port
 *
 * Values for the `port` devicetree property.
 * @{
 */
#define NPCX_TACH_PORT_A     0 /**< Tachometer port A */
#define NPCX_TACH_PORT_B     1 /**< Tachometer port B */
/** @} */

/**
 * @name Operating frequency
 *
 * Values for the `sample-clk` devicetree property.
 * @{
 */
#define NPCX_TACH_FREQ_LFCLK 32768 /**< Low-frequency clock (32768 Hz) */
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_NPCX_TACH_H_ */
