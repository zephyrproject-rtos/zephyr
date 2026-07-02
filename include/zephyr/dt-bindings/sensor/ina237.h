/*
 * Copyright (c) 2021 Grinn
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for the TI INA237 power monitor.
 * @ingroup ina237_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_INA237_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_INA237_H_

#include <zephyr/dt-bindings/dt-util.h>

/**
 * @defgroup ina237_interface INA237
 * @ingroup sensor_interface_ext_ti
 * @brief Texas Instruments INA237 current and power monitor
 * @{
 */

/**
 * @name Operating mode
 * @anchor ina237_oper_mode
 *
 * Operating-mode argument for the INA237_ADC_CONFIG() macro.
 * @{
 */
#define INA237_CFG_HIGH_PRECISION			BIT(4) /**< High-precision mode flag */
#define INA237_OPER_MODE_SHUTDOWN			0x00 /**< Shutdown */
#define INA237_OPER_MODE_BUS_VOLTAGE_TRIG		0x01 /**< Bus voltage, triggered */
#define INA237_OPER_MODE_SHUNT_VOLTAGE_TRIG		0x02 /**< Shunt voltage, triggered */
#define INA237_OPER_MODE_SHUNT_BUS_VOLTAGE_TRIG		0x03 /**< Shunt and bus, triggered */
#define INA237_OPER_MODE_TEMP_TRIG			0x04 /**< Temperature, triggered */
#define INA237_OPER_MODE_TEMP_BUS_VOLTAGE_TRIG		0x05 /**< Temp and bus, triggered */
#define INA237_OPER_MODE_TEMP_SHUNT_VOLTAGE_TRIG	0x06 /**< Temp and shunt, triggered */
#define INA237_OPER_MODE_BUS_SHUNT_VOLTAGE_TEMP_TRIG	0x07 /**< Bus, shunt and temp, triggered */
#define INA237_OPER_MODE_BUS_VOLTAGE_CONT		0x09 /**< Bus voltage, continuous */
#define INA237_OPER_MODE_SHUNT_VOLTAGE_CONT		0x0A /**< Shunt voltage, continuous */
#define INA237_OPER_MODE_SHUNT_BUS_VOLTAGE_CONT		0x0B /**< Shunt and bus, continuous */
#define INA237_OPER_MODE_TEMP_CONT			0x0C /**< Temperature, continuous */
#define INA237_OPER_MODE_BUS_VOLTAGE_TEMP_CONT		0x0D /**< Bus and temp, continuous */
#define INA237_OPER_MODE_TEMP_SHUNT_VOLTAGE_CONT	0x0E /**< Temp and shunt, continuous */
#define INA237_OPER_MODE_BUS_SHUNT_VOLTAGE_TEMP_CONT	0x0F /**< Bus, shunt and temp, continuous */
/** @} */

/**
 * @name Bus, shunt and temperature conversion time
 * @anchor ina237_conv_time
 *
 * Conversion-time argument for the INA237_ADC_CONFIG() macro.
 * @{
 */
#define INA237_CONV_TIME_50   0x00 /**< 50 µs */
#define INA237_CONV_TIME_84   0x01 /**< 84 µs */
#define INA237_CONV_TIME_150  0x02 /**< 150 µs */
#define INA237_CONV_TIME_280  0x03 /**< 280 µs */
#define INA237_CONV_TIME_540  0x04 /**< 540 µs */
#define INA237_CONV_TIME_1052 0x05 /**< 1052 µs */
#define INA237_CONV_TIME_2074 0x06 /**< 2074 µs */
#define INA237_CONV_TIME_4120 0x07 /**< 4120 µs */
/** @} */

/**
 * @name Averaging mode (number of samples)
 * @anchor ina237_avg
 *
 * Averaging-mode argument for the INA237_ADC_CONFIG() macro.
 * @{
 */
#define INA237_AVG_MODE_1    0x00 /**< 1 sample */
#define INA237_AVG_MODE_4    0x01 /**< 4 samples */
#define INA237_AVG_MODE_16   0x02 /**< 16 samples */
#define INA237_AVG_MODE_64   0x03 /**< 64 samples */
#define INA237_AVG_MODE_128  0x04 /**< 128 samples */
#define INA237_AVG_MODE_256  0x05 /**< 256 samples */
#define INA237_AVG_MODE_512  0x06 /**< 512 samples */
#define INA237_AVG_MODE_1024 0x07 /**< 1024 samples */
/** @} */

/**
 * @name Reset mode
 * @anchor ina237_reset
 *
 * Reset-mode argument for the INA237_CONFIG() macro.
 * @{
 */
#define INA237_RST_NORMAL_OPERATION	0x00 /**< Normal operation */
#define INA237_RST_SYSTEM_RESET		0x01 /**< Trigger a system reset */
/** @} */

/**
 * @name Initial ADC conversion delay (steps of 2 ms)
 * @anchor ina237_init_adc_delay
 *
 * Initial ADC-delay argument for the INA237_CONFIG() macro.
 * @{
 */
#define INA237_INIT_ADC_DELAY_0_S	0x00 /**< 0 s (no delay) */
#define INA237_INIT_ADC_DELAY_2_MS	0x01 /**< 2 ms */
#define INA237_INIT_ADC_DELAY_510_MS	0xFF /**< 510 ms */
/** @} */

/**
 * @name Shunt full-scale range across IN+ and IN−
 * @anchor ina237_adc_range
 *
 * Shunt full-scale range argument for the INA237_CONFIG() macro.
 * @{
 */
#define INA237_ADC_RANGE_163_84	0x00 /**< ±163.84 mV */
#define INA237_ADC_RANGE_40_96	0x01 /**< ±40.96 mV */
/** @} */

/**
 * @brief Build the INA237 configuration register value.
 *
 * @param rst_mode  Reset mode (see @ref ina237_reset).
 * @param convdly   Initial ADC conversion delay (see @ref ina237_init_adc_delay).
 * @param adc_range Shunt full-scale range (see @ref ina237_adc_range).
 */
#define INA237_CONFIG(rst_mode, \
		      convdly, \
		      adc_range) \
	(((rst_mode) << 15) | ((convdly) << 6) | ((adc_range) << 4))

/**
 * @brief Build the INA237 ADC configuration register value.
 *
 * @param mode   Operating mode (see @ref ina237_oper_mode).
 * @param vshct  Shunt voltage conversion time (see @ref ina237_conv_time).
 * @param vbusct Bus voltage conversion time (see @ref ina237_conv_time).
 * @param vtct   Temperature conversion time (see @ref ina237_conv_time).
 * @param avg    Averaging mode (see @ref ina237_avg).
 */
#define INA237_ADC_CONFIG(mode, \
		      vshct, \
		      vbusct, \
		      vtct, \
		      avg)  \
	(((mode) << 12) | ((vbusct) << 9) | ((vshct) << 6) | ((vtct) << 3) | (avg))

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_INA237_H_ */
