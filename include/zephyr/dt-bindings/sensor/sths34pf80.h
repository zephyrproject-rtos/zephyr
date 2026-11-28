/*
 * Copyright (c) 2026 Dotcom IoT LLP
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for the ST STHS34PF80 sensor.
 * @ingroup sths34pf80_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_STHS34PF80_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_STHS34PF80_H_

/**
 * @defgroup sths34pf80_interface STHS34PF80
 * @ingroup sensor_interface_ext_st
 * @brief STMicroelectronics STHS34PF80 infrared presence and motion sensor
 * @{
 */

/**
 * @name Output data rate options
 *
 * Values for the `odr` devicetree property, matching sths34pf80_odr_t in
 * sths34pf80_reg.h (hal_st).
 * @{
 */
#define STHS34PF80_DT_ODR_POWER_DOWN 0 /**< Power-down mode, sensor inactive. */
#define STHS34PF80_DT_ODR_0HZ25      1 /**< 0.25 Hz output data rate. */
#define STHS34PF80_DT_ODR_0HZ50      2 /**< 0.50 Hz output data rate. */
#define STHS34PF80_DT_ODR_1HZ        3 /**< 1 Hz output data rate. */
#define STHS34PF80_DT_ODR_2HZ        4 /**< 2 Hz output data rate. */
#define STHS34PF80_DT_ODR_4HZ        5 /**< 4 Hz output data rate. */
#define STHS34PF80_DT_ODR_8HZ        6 /**< 8 Hz output data rate. */
#define STHS34PF80_DT_ODR_15HZ       7 /**< 15 Hz output data rate. */
#define STHS34PF80_DT_ODR_30HZ       8 /**< 30 Hz output data rate. */
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_STHS34PF80_H_ */
