/*
 * Copyright (c) 2021 ITE Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for the ITE IT8xxx2 tachometer.
 * @ingroup it8xxx2_tach_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_IT8XXX2_TACH_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_IT8XXX2_TACH_H_

/**
 * @defgroup it8xxx2_tach_interface IT8XXX2 tachometer
 * @ingroup sensor_interface_ext_ite
 * @brief ITE IT8xxx2 tachometer
 * @{
 */

/**
 * @name Tachometer channels
 *
 * Values for the `channel` devicetree property.
 * @{
 */
#define IT8XXX2_TACH_CHANNEL_A		0 /**< Tachometer channel A */
#define IT8XXX2_TACH_CHANNEL_B		1 /**< Tachometer channel B */
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_IT8XXX2_TACH_H_ */
