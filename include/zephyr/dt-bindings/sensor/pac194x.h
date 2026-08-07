/*
 * Copyright (c) 2026 Google, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for the Microchip PAC194x power monitor.
 * @ingroup pac194x_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_PAC194X_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_PAC194X_H_

/**
 * @defgroup pac194x_interface PAC194X
 * @ingroup sensor_interface_ext_microchip
 * @brief Microchip PAC194x power monitor
 * @{
 */

/**
 * @name Full-scale range mode
 *
 * Values for the `microchip,vbus-mode` and `microchip,vsense-mode` devicetree properties.
 * @{
 */
#define PAC_UNIPOLAR_FSR	0 /**< Unipolar full-scale range */
#define PAC_BIPOLAR_FSR		1 /**< Bipolar full-scale range */
#define PAC_BIPOLAR_HALF_FSR	2 /**< Bipolar half (FSR / 2) range */
/** @} */

/**
 * @name Accumulation source
 *
 * Values for the `microchip,accumulation-mode` devicetree property.
 * @{
 */
#define PAC_ACCUM_VPOWER	0 /**< Accumulate power data */
#define PAC_ACCUM_VSENSE	1 /**< Accumulate current (sense voltage) data */
#define PAC_ACCUM_VBUS		2 /**< Accumulate bus voltage data */
/** @} */

/**
 * @name Refresh mode
 *
 * Values for the `microchip,refresh-mode` devicetree property.
 * @{
 */
#define PAC_REFRESH_MANUAL	0 /**< Manual refresh */
#define PAC_REFRESH_AUTO_WAIT	1 /**< Automatic refresh, wait for valid data */
#define PAC_REFRESH_AUTO_NOWAIT	2 /**< Automatic refresh, do not wait */
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_PAC194X_H_ */
