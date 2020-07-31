/* ST Microelectronics STTS751 temperature sensor
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/stts751.pdf
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_STTS751_STTS751_H_
#define ZEPHYR_DRIVERS_SENSOR_STTS751_STTS751_H_

#include <stdint.h>
#include <drivers/i2c.h>
#include <drivers/gpio.h>
#include <drivers/sensor.h>
#include <zephyr/types.h>
#include <sys/util.h>
#include "stts751_reg.h"

union axis1bit16_t {
	int16_t i16bit;
	uint8_t u8bit[2];
};

struct stts751_config {
	char *master_dev_name;
	int (*bus_init)(struct device *dev);
#ifdef CONFIG_STTS751_TRIGGER
	const char *event_port;
	uint8_t event_pin;
	uint8_t int_flags;
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	uint16_t i2c_slv_addr;
#endif
};

struct stts751_data {
	struct device *bus;
	int16_t sample_temp;

	stmdev_ctx_t *ctx;

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	stmdev_ctx_t ctx_i2c;
#endif

#ifdef CONFIG_STTS751_TRIGGER
	struct device *gpio;
	uint32_t pin;
	struct gpio_callback gpio_cb;

	struct sensor_trigger data_ready_trigger;
	sensor_trigger_handler_t thsld_handler;
	struct device *dev;

#if defined(CONFIG_STTS751_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_STTS751_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_STTS751_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif

#endif /* CONFIG_STTS751_TRIGGER */
};

int stts751_i2c_init(struct device *dev);

#ifdef CONFIG_STTS751_TRIGGER
int stts751_trigger_set(struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);

int stts751_init_interrupt(struct device *dev);
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_STTS751_STTS751_H_ */
