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
 * @brief ADT7604 / LTC2984 sensor type identifiers
 *
 * Codes 18 and 27 are overloaded between the two devices and must be
 * disambiguated using the driver's device_type at runtime:
 *   - Code 18: ADT7604_SENSOR_TYPE_COPPER_TRACE (ADT7604) vs
 *              LTC2984_SENSOR_TYPE_RTD_CUSTOM (LTC2984)
 *   - Code 27: ADT7604_SENSOR_TYPE_LEAK_DETECTOR (ADT7604) vs
 *              LTC2984_SENSOR_TYPE_THERMISTOR_CUSTOM_TABLE (LTC2984)
 * @{
 */
#define ADT7604_SENSOR_TYPE_UNASSIGNED          0  /**< Unassigned */
#define LTC2984_SENSOR_TYPE_TC_TYPE_J           1  /**< Type J Thermocouple (LTC2984 only) */
#define LTC2984_SENSOR_TYPE_TC_TYPE_K           2  /**< Type K Thermocouple (LTC2984 only) */
#define LTC2984_SENSOR_TYPE_TC_TYPE_E           3  /**< Type E Thermocouple (LTC2984 only) */
#define LTC2984_SENSOR_TYPE_TC_TYPE_N           4  /**< Type N Thermocouple (LTC2984 only) */
#define LTC2984_SENSOR_TYPE_TC_TYPE_R           5  /**< Type R Thermocouple (LTC2984 only) */
#define LTC2984_SENSOR_TYPE_TC_TYPE_S           6  /**< Type S Thermocouple (LTC2984 only) */
#define LTC2984_SENSOR_TYPE_TC_TYPE_T           7  /**< Type T Thermocouple (LTC2984 only) */
#define LTC2984_SENSOR_TYPE_TC_TYPE_B           8  /**< Type B Thermocouple (LTC2984 only) */
#define LTC2984_SENSOR_TYPE_TC_CUSTOM           9  /**< Custom Thermocouple (LTC2984 only) */
#define ADT7604_SENSOR_TYPE_RTD_PT_10           10 /**< RTD PT-10 */
#define ADT7604_SENSOR_TYPE_RTD_PT_50           11 /**< RTD PT-50 */
#define ADT7604_SENSOR_TYPE_RTD_PT_100          12 /**< RTD PT-100 */
#define ADT7604_SENSOR_TYPE_RTD_PT_200          13 /**< RTD PT-200 */
#define ADT7604_SENSOR_TYPE_RTD_PT_500          14 /**< RTD PT-500 */
#define ADT7604_SENSOR_TYPE_RTD_PT_1000         15 /**< RTD PT-1000 */
#define ADT7604_SENSOR_TYPE_RTD_1000            16 /**< RTD 1000 */
#define ADT7604_SENSOR_TYPE_RTD_NI_120          17 /**< RTD NI-120 */
#define ADT7604_SENSOR_TYPE_COPPER_TRACE        18 /**< Copper Trace (ADT7604 only) */
#define LTC2984_SENSOR_TYPE_RTD_CUSTOM          18 /**< RTD Custom (LTC2984 only) */
#define ADT7604_SENSOR_TYPE_THERMISTOR_44004    19 /**< Thermistor 44004/44033 */
#define ADT7604_SENSOR_TYPE_THERMISTOR_44005    20 /**< Thermistor 44005/44030 */
#define ADT7604_SENSOR_TYPE_THERMISTOR_44007    21 /**< Thermistor 44007/44034 */
#define ADT7604_SENSOR_TYPE_THERMISTOR_44006    22 /**< Thermistor 44006/44031 */
#define ADT7604_SENSOR_TYPE_THERMISTOR_44008    23 /**< Thermistor 44008/44032 */
#define ADT7604_SENSOR_TYPE_THERMISTOR_YSI_400  24 /**< Thermistor YSI-400 */
#define ADT7604_SENSOR_TYPE_THERMISTOR_SPECTRUM 25 /**< Thermistor Spectrum */
#define ADT7604_SENSOR_TYPE_THERMISTOR_CUSTOM   26 /**< Thermistor Custom Steinhart-Hart */
#define ADT7604_SENSOR_TYPE_LEAK_DETECTOR       27 /**< Leak Detector (ADT7604 only) */
#define LTC2984_SENSOR_TYPE_THERMISTOR_CUSTOM_TABLE                                                \
	27                                /**< Thermistor Custom Table (LTC2984 only) */
#define LTC2984_SENSOR_TYPE_DIODE      28 /**< Diode (LTC2984 only) */
#define ADT7604_SENSOR_TYPE_RSENSE     29 /**< Sense Resistor */
#define LTC2984_SENSOR_TYPE_DIRECT_ADC 30 /**< Direct ADC (LTC2984 only) */
/** @} */

/**
 * @name RTD Sensor Configuration
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
 * @name Thermistor Sensor Configuration
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
 * @name Thermocouple Sensor Configuration (bits [21:18])
 * @brief Combined SGL/DIFF + open-circuit check enable + open-circuit detect current for
 * LTC2984 thermocouples. The open-circuit current only applies when OC check is enabled;
 * the "_EXTERNAL" variants disable OC check (detect current supplied externally).
 * @{
 */
#define LTC2984_TC_CONFIG_DIFF_EXTERNAL         0x0 /**< Differential, OC check disabled */
#define LTC2984_TC_CONFIG_DIFF_10_UA            0x4 /**< Differential, OC check, 10 µA */
#define LTC2984_TC_CONFIG_DIFF_100_UA           0x5 /**< Differential, OC check, 100 µA */
#define LTC2984_TC_CONFIG_DIFF_500_UA           0x6 /**< Differential, OC check, 500 µA */
#define LTC2984_TC_CONFIG_DIFF_1_MA             0x7 /**< Differential, OC check, 1 mA */
#define LTC2984_TC_CONFIG_SINGLE_ENDED_EXTERNAL 0x8 /**< Single-ended, OC check disabled */
#define LTC2984_TC_CONFIG_SINGLE_ENDED_10_UA    0xC /**< Single-ended, OC check, 10 µA */
#define LTC2984_TC_CONFIG_SINGLE_ENDED_100_UA   0xD /**< Single-ended, OC check, 100 µA */
#define LTC2984_TC_CONFIG_SINGLE_ENDED_500_UA   0xE /**< Single-ended, OC check, 500 µA */
#define LTC2984_TC_CONFIG_SINGLE_ENDED_1_MA     0xF /**< Single-ended, OC check, 1 mA */
/** @} */

/**
 * @name Diode Sensor Configuration (bits [26:24])
 * @brief Combined SGL/DIFF + number of readings + running average enable for LTC2984 diodes
 * @{
 */
#define LTC2984_DIODE_CONFIG_DIFF_2_READINGS         0x0 /**< Differential, 2 readings, avg off */
#define LTC2984_DIODE_CONFIG_DIFF_2_READINGS_AVG     0x1 /**< Differential, 2 readings, avg on */
#define LTC2984_DIODE_CONFIG_DIFF_3_READINGS         0x2 /**< Differential, 3 readings, avg off */
#define LTC2984_DIODE_CONFIG_DIFF_3_READINGS_AVG     0x3 /**< Differential, 3 readings, avg on */
#define LTC2984_DIODE_CONFIG_SINGLE_ENDED_2_READINGS 0x4 /**< Single-ended, 2 readings, avg off */
#define LTC2984_DIODE_CONFIG_SINGLE_ENDED_2_READINGS_AVG                                           \
	0x5                                              /**< Single-ended, 2 readings, avg on */
#define LTC2984_DIODE_CONFIG_SINGLE_ENDED_3_READINGS 0x6 /**< Single-ended, 3 readings, avg off */
#define LTC2984_DIODE_CONFIG_SINGLE_ENDED_3_READINGS_AVG                                           \
	0x7 /**< Single-ended, 3 readings, avg on */
/** @} */

/**
 * @name Diode Excitation Current (bits [23:22])
 * @brief Base (1x) excitation current for LTC2984 diodes. In 3-readings mode
 * (LTC2984_DIODE_CONFIG_*_3_READINGS*) the device automatically also measures at 4x
 * and 8x this current to cancel parasitic series resistance; only the base value is
 * configured here.
 * @{
 */
#define LTC2984_DIODE_EXCITATION_CURRENT_10_UA 0x0 /**< 10 µA (1x); 40 µA/80 µA at 4x/8x */
#define LTC2984_DIODE_EXCITATION_CURRENT_20_UA 0x1 /**< 20 µA (1x); 80 µA/160 µA at 4x/8x */
#define LTC2984_DIODE_EXCITATION_CURRENT_40_UA 0x2 /**< 40 µA (1x); 160 µA/320 µA at 4x/8x */
#define LTC2984_DIODE_EXCITATION_CURRENT_80_UA 0x3 /**< 80 µA (1x); 320 µA/640 µA at 4x/8x */
/** @} */

/**
 * @name RTD Excitation Current
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
 * @name Copper Trace Excitation Current
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
 * @name Thermistor Excitation Current
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
 * @name Leak Sensor Excitation Current
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
