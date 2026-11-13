/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for the ADI ADT7604 temperature measurement system.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_ADT7604_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_ADT7604_H_

/**
 * @defgroup adt7604_dt_bindings ADT7604 Devicetree Bindings
 * @ingroup sensor_interface
 * @{
 */

/**
 * @name Sensor Types
 * @brief ADT7604 sensor type identifiers
 * @{
 */
#define ADT7604_SENSOR_TYPE_UNASSIGNED          0  /**< Unassigned */
#define ADT7604_SENSOR_TYPE_RTD_PT_10           10 /**< RTD PT-10 */
#define ADT7604_SENSOR_TYPE_RTD_PT_50           11 /**< RTD PT-50 */
#define ADT7604_SENSOR_TYPE_RTD_PT_100          12 /**< RTD PT-100 */
#define ADT7604_SENSOR_TYPE_RTD_PT_200          13 /**< RTD PT-200 */
#define ADT7604_SENSOR_TYPE_RTD_PT_500          14 /**< RTD PT-500 */
#define ADT7604_SENSOR_TYPE_RTD_PT_1000         15 /**< RTD PT-1000 */
#define ADT7604_SENSOR_TYPE_RTD_1000            16 /**< RTD 1000 */
#define ADT7604_SENSOR_TYPE_RTD_NI_120          17 /**< RTD NI-120 */
#define ADT7604_SENSOR_TYPE_COPPER_TRACE        18 /**< Copper Trace */
#define ADT7604_SENSOR_TYPE_THERMISTOR_44004    19 /**< Thermistor 44004/44033 */
#define ADT7604_SENSOR_TYPE_THERMISTOR_44005    20 /**< Thermistor 44005/44030 */
#define ADT7604_SENSOR_TYPE_THERMISTOR_44007    21 /**< Thermistor 44007/44034 */
#define ADT7604_SENSOR_TYPE_THERMISTOR_44006    22 /**< Thermistor 44006/44031 */
#define ADT7604_SENSOR_TYPE_THERMISTOR_44008    23 /**< Thermistor 44008/44032 */
#define ADT7604_SENSOR_TYPE_THERMISTOR_YSI_400  24 /**< Thermistor YSI-400 */
#define ADT7604_SENSOR_TYPE_THERMISTOR_SPECTRUM 25 /**< Thermistor Spectrum */
#define ADT7604_SENSOR_TYPE_THERMISTOR_CUSTOM   26 /**< Thermistor Custom */
#define ADT7604_SENSOR_TYPE_LEAK_DETECTOR       27 /**< Leak Detector */
#define ADT7604_SENSOR_TYPE_RSENSE              29 /**< Sense Resistor */
/** @} */

/**
 * @name RTD Sensor Configuration (Table 49)
 * @brief Sensor configuration values for RTD sensors combining wire type and excitation mode
 * Bits [21:20] = Number of wires, Bits [19:18] = Excitation mode
 * @{
 */
#define ADT7604_RTD_CONFIG_2_WIRE_EXTERNAL     0x0 /**< 2-Wire, External, No rotation, No sharing */
#define ADT7604_RTD_CONFIG_2_WIRE_INTERNAL     0x1 /**< 2-Wire, Internal, No rotation, Sharing */
#define ADT7604_RTD_CONFIG_3_WIRE_EXTERNAL     0x4 /**< 3-Wire, External, No rotation, No sharing */
#define ADT7604_RTD_CONFIG_3_WIRE_INTERNAL     0x5 /**< 3-Wire, Internal, No rotation, Sharing */
#define ADT7604_RTD_CONFIG_4_WIRE_EXTERNAL     0x8 /**< 4-Wire, External, No rotation, No sharing */
#define ADT7604_RTD_CONFIG_4_WIRE_INTERNAL     0x9 /**< 4-Wire, Internal, No rotation, Sharing */
#define ADT7604_RTD_CONFIG_4_WIRE_INTERNAL_ROT 0xA /**< 4-Wire, Internal, Rotation, Sharing */
/** @} */

/**
 * @name Thermistor Sensor Configuration (Table 73)
 * @brief Sensor configuration values for thermistor sensors
 * Bits [21:19] = SGL + Excitation Mode
 * @{
 */
#define ADT7604_THERMISTOR_CONFIG_DIFF_NO_SHARE_NO_ROTATE                                          \
	0x0                                                /**< Differential, No share, No rotate */
#define ADT7604_THERMISTOR_CONFIG_DIFF_SHARE_ROTATE    0x1 /**< Differential, Share, Rotate */
#define ADT7604_THERMISTOR_CONFIG_DIFF_SHARE_NO_ROTATE 0x2 /**< Differential, Share, No rotate */
#define ADT7604_THERMISTOR_CONFIG_SINGLE_ENDED         0x4 /**< Single-ended */
/** @} */

/**
 * @name RTD Excitation Current (Table 50)
 * @brief Excitation current values for RTD sensors (bits [17:14])
 * @{
 */
#define ADT7604_RTD_EXCITATION_CURRENT_5_UA   0x01 /**< 5 µA */
#define ADT7604_RTD_EXCITATION_CURRENT_10_UA  0x02 /**< 10 µA */
#define ADT7604_RTD_EXCITATION_CURRENT_25_UA  0x03 /**< 25 µA */
#define ADT7604_RTD_EXCITATION_CURRENT_50_UA  0x04 /**< 50 µA */
#define ADT7604_RTD_EXCITATION_CURRENT_100_UA 0x05 /**< 100 µA */
#define ADT7604_RTD_EXCITATION_CURRENT_250_UA 0x06 /**< 250 µA */
#define ADT7604_RTD_EXCITATION_CURRENT_500_UA 0x07 /**< 500 µA */
#define ADT7604_RTD_EXCITATION_CURRENT_1_MA   0x08 /**< 1 mA */
/** @} */

/**
 * @name Copper Trace Excitation Current (Table 32)
 * @brief Excitation current values for copper trace sensors ≥1Ω (bits [17:14])
 * @{
 */
#define ADT7604_CT_EXCITATION_CURRENT_5_UA   0x01 /**< 5 µA */
#define ADT7604_CT_EXCITATION_CURRENT_10_UA  0x02 /**< 10 µA */
#define ADT7604_CT_EXCITATION_CURRENT_25_UA  0x03 /**< 25 µA */
#define ADT7604_CT_EXCITATION_CURRENT_50_UA  0x04 /**< 50 µA */
#define ADT7604_CT_EXCITATION_CURRENT_100_UA 0x05 /**< 100 µA */
#define ADT7604_CT_EXCITATION_CURRENT_250_UA 0x06 /**< 250 µA */
#define ADT7604_CT_EXCITATION_CURRENT_500_UA 0x07 /**< 500 µA */
#define ADT7604_CT_EXCITATION_CURRENT_1_MA   0x08 /**< 1 mA */
/** @} */

/**
 * @name Thermistor Excitation Current (Table 74)
 * @brief Excitation current values for thermistor sensors (bits [18:15])
 * @{
 */
#define ADT7604_THERMISTOR_EXCITATION_CURRENT_250_NA 0x01 /**< 250 nA */
#define ADT7604_THERMISTOR_EXCITATION_CURRENT_500_NA 0x02 /**< 500 nA */
#define ADT7604_THERMISTOR_EXCITATION_CURRENT_1_UA   0x03 /**< 1 µA */
#define ADT7604_THERMISTOR_EXCITATION_CURRENT_5_UA   0x04 /**< 5 µA */
#define ADT7604_THERMISTOR_EXCITATION_CURRENT_10_UA  0x05 /**< 10 µA */
#define ADT7604_THERMISTOR_EXCITATION_CURRENT_25_UA  0x06 /**< 25 µA */
#define ADT7604_THERMISTOR_EXCITATION_CURRENT_50_UA  0x07 /**< 50 µA */
#define ADT7604_THERMISTOR_EXCITATION_CURRENT_100_UA 0x08 /**< 100 µA */
#define ADT7604_THERMISTOR_EXCITATION_CURRENT_250_UA 0x09 /**< 250 µA */
#define ADT7604_THERMISTOR_EXCITATION_CURRENT_500_UA 0x0A /**< 500 µA */
#define ADT7604_THERMISTOR_EXCITATION_CURRENT_1_MA   0x0B /**< 1 mA */
#define ADT7604_THERMISTOR_EXCITATION_CURRENT_AUTO   0x0C /**< Auto-range */
/** @} */

/**
 * @name Leak Sensor Excitation Current (Table 39)
 * @brief Excitation current values for leak sensors (bits [18:15])
 * @{
 */
#define ADT7604_LEAK_EXCITATION_CURRENT_250_NA 0x01 /**< 250 nA */
#define ADT7604_LEAK_EXCITATION_CURRENT_500_NA 0x02 /**< 500 nA */
#define ADT7604_LEAK_EXCITATION_CURRENT_1_UA   0x03 /**< 1 µA */
#define ADT7604_LEAK_EXCITATION_CURRENT_5_UA   0x04 /**< 5 µA */
#define ADT7604_LEAK_EXCITATION_CURRENT_10_UA  0x05 /**< 10 µA */
#define ADT7604_LEAK_EXCITATION_CURRENT_25_UA  0x06 /**< 25 µA */
#define ADT7604_LEAK_EXCITATION_CURRENT_50_UA  0x07 /**< 50 µA */
#define ADT7604_LEAK_EXCITATION_CURRENT_100_UA 0x08 /**< 100 µA */
#define ADT7604_LEAK_EXCITATION_CURRENT_250_UA 0x09 /**< 250 µA */
#define ADT7604_LEAK_EXCITATION_CURRENT_500_UA 0x0A /**< 500 µA */
#define ADT7604_LEAK_EXCITATION_CURRENT_1_MA   0x0B /**< 1 mA */
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_ADT7604_H_ */
