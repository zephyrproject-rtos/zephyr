/* ST Microelectronics LIS2DS12 3-axis accelerometer driver
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lis2ds12.pdf
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_LIS2DS12_LIS2DS12_H_
#define ZEPHYR_DRIVERS_SENSOR_LIS2DS12_LIS2DS12_H_

#include <zephyr/types.h>
#include <drivers/sensor.h>
#include <drivers/gpio.h>
#include "lis2ds12_reg.h"

/* Return ODR reg value based on data rate set */
#define LIS2DS12_HR_ODR_TO_REG(_odr) \
	((_odr <= 12) ? LIS2DS12_XL_ODR_12Hz5_HR : \
	 ((31 - __builtin_clz(_odr / 25))) + 2)

struct lis2ds12_config {
	char *comm_master_dev_name;
	int (*bus_init)(const struct device *dev);
#ifdef CONFIG_LIS2DS12_TRIGGER
	const char *irq_port;
	gpio_pin_t irq_pin;
	gpio_dt_flags_t irq_flags;
#endif
};

struct lis2ds12_data {
	stmdev_ctx_t *ctx;
	const struct device *comm_master;
	int sample_x;
	int sample_y;
	int sample_z;
	float gain;

#ifdef CONFIG_LIS2DS12_TRIGGER
	const struct device *gpio;
	struct gpio_callback gpio_cb;

	struct sensor_trigger data_ready_trigger;
	sensor_trigger_handler_t data_ready_handler;
	const struct device *dev;

#if defined(CONFIG_LIS2DS12_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_LIS2DS12_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem trig_sem;
#elif defined(CONFIG_LIS2DS12_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif

#endif /* CONFIG_LIS2DS12_TRIGGER */
};

int lis2ds12_spi_init(const struct device *dev);
int lis2ds12_i2c_init(const struct device *dev);

#ifdef CONFIG_LIS2DS12_TRIGGER
int lis2ds12_trigger_set(const struct device *dev,
			 const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler);

int lis2ds12_trigger_init(const struct device *dev);
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_LIS2DS12_LIS2DS12_H_ */
