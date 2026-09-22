/*
 * Copyright (c) 2023 TOKITA Hiroshi <tokita.hiroshi@fujitsu.com>
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup regulator_rpi_pico
 * @brief Header file for Raspberry Pi Pico on-chip regulator Devicetree helpers.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_RPI_PICO_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_RPI_PICO_H_

/**
 * @defgroup regulator_rpi_pico Raspberry Pi Pico regulator Devicetree helpers
 * @brief Raspberry Pi Pico on-chip regulator driver Devicetree helpers
 * @ingroup devicetree-regulator
 * @{
 */

/**
 * @name Raspberry Pi Pico regulator modes
 * @{
 */
/** Normal operating mode */
#define REGULATOR_RPI_PICO_MODE_NORMAL 0x0
/** High impedance mode */
#define REGULATOR_RPI_PICO_MODE_HI_Z 0x2

/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_RPI_PICO_H_ */
