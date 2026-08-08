/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for the Sensirion STC31 CO2 sensor.
 * @ingroup stc31_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_STC31_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_STC31_H_

/**
 * @defgroup stc31_interface STC31
 * @ingroup sensor_interface_ext_sensirion
 * @brief Sensirion STC31 CO2 sensor
 * @{
 */

/**
 * @name Binary gas / measurement mode
 *
 * Values for the `binary-gas` devicetree property (STC31-C datasheet §3.3.2).
 * @{
 */

/* Low-noise modes */
#define STC31_DT_BINARY_GAS_CO2_IN_N2_100_LN  0x0000 /**< CO2 in N2, 0...100 vol% (low-noise) */
#define STC31_DT_BINARY_GAS_CO2_IN_AIR_100_LN 0x0001 /**< CO2 in air, 0...100 vol% (low-noise) */
#define STC31_DT_BINARY_GAS_CO2_IN_N2_25_LN   0x0002 /**< CO2 in N2, 0...25 vol% (low-noise) */
#define STC31_DT_BINARY_GAS_CO2_IN_AIR_25_LN  0x0003 /**< CO2 in air, 0...25 vol% (low-noise) */

/* Low-cross-sensitivity modes (recommended for STC31-C) */
#define STC31_DT_BINARY_GAS_CO2_IN_N2_100  0x0010 /**< CO2 in N2, 0...100 vol% */
#define STC31_DT_BINARY_GAS_CO2_IN_AIR_100 0x0011 /**< CO2 in air, 0...100 vol% */
#define STC31_DT_BINARY_GAS_CO2_IN_N2_40   0x0012 /**< CO2 in N2, 0...40 vol% */
#define STC31_DT_BINARY_GAS_CO2_IN_AIR_40  0x0013 /**< CO2 in air, 0...40 vol% */

/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_STC31_H_ */
