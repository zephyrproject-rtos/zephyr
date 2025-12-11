/*
 * Copyright (c) 2018 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ADT7420_ADT7420_H_
#define ZEPHYR_DRIVERS_SENSOR_ADT7420_ADT7420_H_

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

/* ADT7420 registers */
#define ADT7420_REG_TEMP_MSB		0x00 /* Temperature value MSB */
#define ADT7420_REG_TEMP_LSB		0x01 /* Temperature value LSB */
#define ADT7420_REG_STATUS		0x02 /* Status */
#define ADT7420_REG_CONFIG		0x03 /* Configuration */
#define ADT7420_REG_T_HIGH_MSB		0x04 /* Temperature HIGH setpoint MSB */
#define ADT7420_REG_T_HIGH_LSB		0x05 /* Temperature HIGH setpoint LSB */
#define ADT7420_REG_T_LOW_MSB		0x06 /* Temperature LOW setpoint MSB */
#define ADT7420_REG_T_LOW_LSB		0x07 /* Temperature LOW setpoint LSB */
#define ADT7420_REG_T_CRIT_MSB		0x08 /* Temperature CRIT setpoint MSB */
#define ADT7420_REG_T_CRIT_LSB		0x09 /* Temperature CRIT setpoint LSB */
#define ADT7420_REG_HIST		0x0A /* Temperature HYST setpoint */
#define ADT7420_REG_ID			0x0B /* ID */
#define ADT7420_REG_RESET		0x2F /* Software reset */

/* ADT7420_REG_STATUS definition */
#define ADT7420_STATUS_T_LOW		BIT(4)
#define ADT7420_STATUS_T_HIGH		BIT(5)
#define ADT7420_STATUS_T_CRIT		BIT(6)
#define ADT7420_STATUS_RDY		BIT(7)

/* ADT7420_REG_CONFIG definition */
#define ADT7420_CONFIG_FAULT_QUEUE(x)	((x) & 0x3)
#define ADT7420_CONFIG_CT_POL		BIT(2)
#define ADT7420_CONFIG_INT_POL		BIT(3)
#define ADT7420_CONFIG_INT_CT_MODE	BIT(4)
#define ADT7420_CONFIG_OP_MODE(x)	(((x) & 0x3) << 5)
#define ADT7420_CONFIG_RESOLUTION	BIT(7)

/* ADT7420_CONFIG_FAULT_QUEUE(x) options */
#define ADT7420_FAULT_QUEUE_1_FAULT	0
#define ADT7420_FAULT_QUEUE_2_FAULTS	1
#define ADT7420_FAULT_QUEUE_3_FAULTS	2
#define ADT7420_FAULT_QUEUE_4_FAULTS	3

/* ADT7420_CONFIG_OP_MODE(x) options */
#define ADT7420_OP_MODE_CONT_CONV	0
#define ADT7420_OP_MODE_ONE_SHOT	1
#define ADT7420_OP_MODE_1_SPS		2
#define ADT7420_OP_MODE_SHUTDOWN	3

/* ADT7420 default ID */
#define ADT7420_DEFAULT_ID		0xCB

/* scale in micro degrees Celsius */
#define ADT7420_TEMP_SCALE		15625

struct adt7420_data {
	int16_t sample;
#ifdef CONFIG_ADT7420_TRIGGER
	struct gpio_callback gpio_cb;

	sensor_trigger_handler_t th_handler;
	const struct sensor_trigger *th_trigger;

	const struct device *dev;

#if defined(CONFIG_ADT7420_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_ADT7420_THREAD_STACK_SIZE);
	struct k_sem gpio_sem;
	struct k_thread thread;
#elif defined(CONFIG_ADT7420_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif
#endif /* CONFIG_ADT7420_TRIGGER */

};

struct adt7420_dev_config {
	struct i2c_dt_spec i2c;
#ifdef CONFIG_ADT7420_TRIGGER
	struct gpio_dt_spec int_gpio;
#endif
};

#ifdef CONFIG_ADT7420_TRIGGER
int adt7420_trigger_set(const struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);

int adt7420_init_interrupt(const struct device *dev);
#endif /* CONFIG_ADT7420_TRIGGER */


#endif /* ZEPHYR_DRIVERS_SENSOR_ADT7420_ADT7420_H_ */
