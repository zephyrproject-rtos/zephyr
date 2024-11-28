/*
 * Copyright (c) 2024 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ADT75_ADT75_H_
#define ZEPHYR_DRIVERS_SENSOR_ADT75_ADT75_H_

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

/******************************* ADT75 Register addresses *******************************/
#define ADT75_REG_TEMPERATURE    0x00 /* Status register */
#define ADT75_REG_CONFIGURATION  0x01 /* Status register */
#define ADT75_REG_THYST_SETPOINT 0x02 /* T Hyst Set Point Register */
#define ADT75_REG_OS_SETPOINT    0x03 /* OS Set Point Register */
#define ADT75_REG_ONESHOT        0x04 /* One Shot Register */

/* ADT75 Register Power-On Defaults */
#define ADT75_DEFAULT_TEMPERATURE    0x00
#define ADT75_DEFAULT_CONFIGURATION  0x00
#define ADT75_DEFAULT_THYST_SETPOINT 0x4B00 /* 75 deg C */
#define ADT75_DEFAULT_OS_SETPOINT    0x5000 /* 80 deg C */

/* ADT75_REG_CONFIG definition */
#define ADT75_CONFIG_SHUTDOWN            BIT(0)
#define ADT75_CONFIG_CMP_INT             BIT(1)
#define ADT75_CONFIG_OS_ALERT_POL        BIT(2)
#define ADT75_CONFIG_FAULT_QUEUE(x)      (((x) & 0x3) << 3)
#define ADT75_CONFIG_ONE_SHOT            BIT(5)
#define ADT75_CONFIG_OS_SMBUS_ALERT_MODE BIT(7)

/* ADT75_CONFIG_FAULT_QUEUE(x) options */
#define ADT75_FAULT_QUEUE_1_FAULT  0
#define ADT75_FAULT_QUEUE_2_FAULTS 1
#define ADT75_FAULT_QUEUE_4_FAULTS 2
#define ADT75_FAULT_QUEUE_6_FAULTS 3

/* scale in micro degrees Celsius */
#define ADT75_TEMP_SCALE 62500

struct adt75_data {
	int16_t sample;
};

struct adt75_dev_config {
	struct i2c_dt_spec i2c;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_ADT75_ADT75_H_ */
