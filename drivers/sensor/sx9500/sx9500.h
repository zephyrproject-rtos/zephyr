/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_SX9500_SX9500_H_
#define ZEPHYR_DRIVERS_SENSOR_SX9500_SX9500_H_

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>

#define SX9500_REG_IRQ_SRC		0x00
#define SX9500_REG_STAT			0x01
#define SX9500_REG_IRQ_MSK		0x03

#define SX9500_REG_PROX_CTRL0		0x06
#define SX9500_REG_PROX_CTRL1		0x07

/* These are used both in the IRQ_SRC register, to identify which
 * interrupt occur, and in the IRQ_MSK register, to enable specific
 * interrupts.
 */
#define SX9500_CONV_DONE_IRQ		(1 << 3)
#define SX9500_NEAR_FAR_IRQ		((1 << 5) | (1 << 6))

struct sx9500_config {
	struct i2c_dt_spec i2c;
#ifdef CONFIG_SX9500_TRIGGER
	struct gpio_dt_spec int_gpio;
#endif
};

struct sx9500_data {
	uint8_t prox_stat;

	struct gpio_callback gpio_cb;

#ifdef CONFIG_SX9500_TRIGGER_OWN_THREAD
	struct k_sem sem;
#endif

#ifdef CONFIG_SX9500_TRIGGER_GLOBAL_THREAD
	struct k_work work;
#endif

#ifdef CONFIG_SX9500_TRIGGER
	const struct device *dev;
	const struct sensor_trigger *trigger_drdy;
	const struct sensor_trigger *trigger_near_far;

	sensor_trigger_handler_t handler_drdy;
	sensor_trigger_handler_t handler_near_far;
#endif
};

#ifdef CONFIG_SX9500_TRIGGER
int sx9500_setup_interrupt(const struct device *dev);
int sx9500_trigger_set(const struct device *dev,
		       const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler);
#else
static inline int sx9500_setup_interrupt(const struct device *dev)
{
	return 0;
}
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_SX9500_SX9500_H_ */
