/*
 * Copyright (c) 2021 Grinn
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_INA237_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_INA237_H_

#include <zephyr/dt-bindings/dt-util.h>

/* Operating Mode */
#define INA237_CFG_HIGH_PRECISION			BIT(4)
#define INA237_OPER_MODE_SHUTDOWN			0x00
#define INA237_OPER_MODE_BUS_VOLTAGE_TRIG		0x01
#define INA237_OPER_MODE_SHUNT_VOLTAGE_TRIG		0x02
#define INA237_OPER_MODE_SHUNT_BUS_VOLTAGE_TRIG		0x03
#define INA237_OPER_MODE_TEMP_TRIG			0x04
#define INA237_OPER_MODE_TEMP_BUS_VOLTAGE_TRIG		0x05
#define INA237_OPER_MODE_TEMP_SHUNT_VOLTAGE_TRIG	0x06
#define INA237_OPER_MODE_BUS_SHUNT_VOLTAGE_TEMP_TRIG	0x07
#define INA237_OPER_MODE_BUS_VOLTAGE_CONT		0x09
#define INA237_OPER_MODE_SHUNT_VOLTAGE_CONT		0x0A
#define INA237_OPER_MODE_SHUNT_BUS_VOLTAGE_CONT		0x0B
#define INA237_OPER_MODE_TEMP_CONT			0x0C
#define INA237_OPER_MODE_BUS_VOLTAGE_TEMP_CONT		0x0D
#define INA237_OPER_MODE_TEMP_SHUNT_VOLTAGE_CONT	0x0E
#define INA237_OPER_MODE_BUS_SHUNT_VOLTAGE_TEMP_CONT	0x0F

/* Conversion time for bus, shunt and temp in micro-seconds */
#define INA237_CONV_TIME_50   0x00
#define INA237_CONV_TIME_84   0x01
#define INA237_CONV_TIME_150  0x02
#define INA237_CONV_TIME_280  0x03
#define INA237_CONV_TIME_540  0x04
#define INA237_CONV_TIME_1052 0x05
#define INA237_CONV_TIME_2074 0x06
#define INA237_CONV_TIME_4120 0x07

/* Averaging Mode */
#define INA237_AVG_MODE_1    0x00
#define INA237_AVG_MODE_4    0x01
#define INA237_AVG_MODE_16   0x02
#define INA237_AVG_MODE_64   0x03
#define INA237_AVG_MODE_128  0x04
#define INA237_AVG_MODE_256  0x05
#define INA237_AVG_MODE_512  0x06
#define INA237_AVG_MODE_1024 0x07

/* Reset Mode */
#define INA237_RST_NORMAL_OPERATION	0x00
#define INA237_RST_SYSTEM_RESET		0x01

/* Delay for initial ADC conversion in steps of 2 ms */
#define INA237_INIT_ADC_DELAY_0_S	0x00
#define INA237_INIT_ADC_DELAY_2_MS	0x01
#define INA237_INIT_ADC_DELAY_510_MS	0xFF

/* Shunt full scale range selection across IN+ and IN–. */
#define INA237_ADC_RANGE_163_84	0x00
#define INA237_ADC_RANGE_40_96	0x01

/**
 * @brief Macro for creating the INA237 configuration value
 *
 * @param rst_mode	Reset mode.
 * @param convdly	Delay for initial ADC conversion in steps of 2 ms.
 * @param adc_range	Shunt full scale range selection across IN+ and IN–.
 *
 */
#define INA237_CONFIG(rst_mode, \
		      convdly, \
		      adc_range) \
	(((rst_mode) << 15) | ((convdly) << 6) | ((adc_range) << 4))

/**
 * @brief Macro for creating the INA237 ADC configuration value
 *
 * @param mode   Operating mode.
 * @param vshct  Conversion time for shunt voltage.
 * @param vbusct Conversion time for bus voltage.
 * @param vtct   Conversion time for temperature.
 * @param avg    Averaging mode.
 */
#define INA237_ADC_CONFIG(mode, \
		      vshct, \
		      vbusct, \
		      vtct, \
		      avg)  \
	(((mode) << 12) | ((vbusct) << 9) | ((vshct) << 6) | ((vtct) << 3) | (avg))

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_INA237_H_ */
