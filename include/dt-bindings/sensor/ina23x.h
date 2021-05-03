/*
 * Copyright 2021 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_INA23X_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_INA23X_H_

#include <dt-bindings/dt-util.h>

/* Mask/Enable bits that asserts the ALERT pin */
#define INA23X_SHUNT_VOLTAGE_OVER    BIT(15)
#define INA23X_SHUNT_VOLTAGE_UNDER   BIT(14)
#define INA23X_BUS_VOLTAGE_OVER      BIT(13)
#define INA23X_BUS_VOLTAGE_UNDER     BIT(12)
#define INA23X_OVER_LIMIT_POWER      BIT(11)
#define INA23X_CONVERSION_READY      BIT(10)
#define INA23X_ALERT_FUNCTION_FLAG   BIT(4)
#define INA23X_CONVERSION_READY_FLAG BIT(3)
#define INA23X_MATH_OVERFLOW_FLAG    BIT(2)
#define INA23X_ALERT_POLARITY        BIT(1)
#define INA23X_ALERT_LATCH_ENABLE    BIT(0)

/* Operating Mode */
#define INA23X_OPER_MODE_POWER_DOWN             0x00
#define INA23X_OPER_MODE_SHUNT_VOLTAGE_TRIG     0x01
#define INA23X_OPER_MODE_BUS_VOLTAGE_TRIG       0x02
#define INA23X_OPER_MODE_SHUNT_BUS_VOLTAGE_TRIG 0x03
#define INA23X_OPER_MODE_SHUNT_VOLTAGE_CONT     0x05
#define INA23X_OPER_MODE_BUS_VOLTAGE_CONT       0x06
#define INA23X_OPER_MODE_SHUNT_BUS_VOLTAGE_CONT 0x07

/* Conversion time for bus and shunt in micro-seconds */
#define INA23X_CONV_TIME_140  0x00
#define INA23X_CONV_TIME_204  0x01
#define INA23X_CONV_TIME_332  0x02
#define INA23X_CONV_TIME_588  0x03
#define INA23X_CONV_TIME_1100 0x04
#define INA23X_CONV_TIME_2116 0x05
#define INA23X_CONV_TIME_4156 0x06
#define INA23X_CONV_TIME_8244 0x07

/* Averaging Mode */
#define INA23X_AVG_MODE_1    0x00
#define INA23X_AVG_MODE_4    0x01
#define INA23X_AVG_MODE_16   0x02
#define INA23X_AVG_MODE_64   0x03
#define INA23X_AVG_MODE_128  0x04
#define INA23X_AVG_MODE_256  0x05
#define INA23X_AVG_MODE_512  0x06
#define INA23X_AVG_MODE_1024 0x07

/**
 * @brief Macro for creating the INA23X configuration value
 *
 * @param mode  Operating mode.
 * @param svct  Conversion time for shunt voltage.
 * @param bvct  Conversion time for bus voltage.
 * @param avg   Averaging mode.
 */
#define INA23X_CONFIG(mode, \
		      svct, \
		      bvct, \
		      avg)  \
	(((avg) << 9) | ((bvct) << 6) | ((svct) << 3) | (mode))

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_INA23X_H_ */
