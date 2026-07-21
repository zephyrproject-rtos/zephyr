/*
 * Copyright (c) 2022 ITE Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for the ITE IT8xxx2 voltage comparator.
 * @ingroup it8xxx2_vcmp_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_IT8XXX2_VCMP_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_IT8XXX2_VCMP_H_

/**
 * @defgroup it8xxx2_vcmp_interface IT8XXX2 voltage comparator
 * @ingroup sensor_interface_ext_ite
 * @brief ITE IT8xxx2 voltage comparator
 * @{
 */

/**
 * @name Voltage comparator channels
 *
 * Values for the `vcmp-ch` devicetree property.
 * @{
 */
#define VCMP_CHANNEL_0				0 /**< Channel 0 */
#define VCMP_CHANNEL_1				1 /**< Channel 1 */
#define VCMP_CHANNEL_2				2 /**< Channel 2 */
#define VCMP_CHANNEL_3				3 /**< Channel 3 */
#define VCMP_CHANNEL_4				4 /**< Channel 4 */
#define VCMP_CHANNEL_5				5 /**< Channel 5 */
#define VCMP_CHANNEL_CNT			6 /**< Number of channels */
/** @} */

/**
 * @name Scan period across all channels
 *
 * Values for the `scan-period` devicetree property.
 * @{
 */
#define IT8XXX2_VCMP_SCAN_PERIOD_100US		0x10 /**< 100 µs */
#define IT8XXX2_VCMP_SCAN_PERIOD_200US		0x20 /**< 200 µs */
#define IT8XXX2_VCMP_SCAN_PERIOD_400US		0x30 /**< 400 µs */
#define IT8XXX2_VCMP_SCAN_PERIOD_600US		0x40 /**< 600 µs */
#define IT8XXX2_VCMP_SCAN_PERIOD_800US		0x50 /**< 800 µs */
#define IT8XXX2_VCMP_SCAN_PERIOD_1MS		0x60 /**< 1 ms */
#define IT8XXX2_VCMP_SCAN_PERIOD_1_5MS		0x70 /**< 1.5 ms */
#define IT8XXX2_VCMP_SCAN_PERIOD_2MS		0x80 /**< 2 ms */
#define IT8XXX2_VCMP_SCAN_PERIOD_2_5MS		0x90 /**< 2.5 ms */
#define IT8XXX2_VCMP_SCAN_PERIOD_3MS		0xa0 /**< 3 ms */
#define IT8XXX2_VCMP_SCAN_PERIOD_4MS		0xb0 /**< 4 ms */
#define IT8XXX2_VCMP_SCAN_PERIOD_5MS		0xc0 /**< 5 ms */
/** @} */

/**
 * @name Interrupt trigger mode
 *
 * Values for the `comparison` devicetree property.
 * @{
 */
#define IT8XXX2_VCMP_LESS_OR_EQUAL		0      /**< Trigger when ≤ threshold */
#define IT8XXX2_VCMP_GREATER			1      /**< Trigger when > threshold */
#define IT8XXX2_VCMP_UNDEFINED			0xffff /**< Undefined / unused */
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_IT8XXX2_VCMP_H_ */
