/*
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for the ST STTS22H temperature sensor.
 * @ingroup stts22h_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_ST_STTS22H_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_ST_STTS22H_H_

/**
 * @defgroup stts22h_interface STTS22H
 * @ingroup sensor_interface_ext_st
 * @brief STMicroelectronics STTS22H temperature sensor
 * @{
 */

/**
 * @name Output data rate options
 *
 * Values for the `sampling-rate` devicetree property.
 * @{
 */
#define	STTS22H_POWER_DOWN	0x00 /**< Power-down */
#define	STTS22H_ONE_SHOT	0x01 /**< One-shot conversion */
#define	STTS22H_1Hz		0x04 /**< 1 Hz */
#define	STTS22H_25Hz		0x02 /**< 25 Hz */
#define	STTS22H_50Hz		0x12 /**< 50 Hz */
#define	STTS22H_100Hz		0x22 /**< 100 Hz */
#define	STTS22H_200Hz		0x32 /**< 200 Hz */
/** @} */

/** @} */

/* GPIO interrupt configuration */
#define STTS22H_DT_GPIO_INT_EDGE_TO_ACTIVE 0 /*!< Edge-triggered interrupt */
#define STTS22H_DT_GPIO_INT_LEVEL_ACTIVE   1 /*!< Level-triggered interrupt */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_ST_STTS22H_H_ */
