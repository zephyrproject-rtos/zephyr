/*
 * Copyright (c) Paul Timke <ptimkec@live.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_PAJ7620_H_
#define ZEPHYR_DRIVERS_SENSOR_PAJ7620_H_

#include <zephyr/drivers/sensor/paj7620.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#if CONFIG_PAJ7620_TRIGGER
#include <zephyr/drivers/gpio.h>
#endif

/** Sensor hardcoded Part ID */
#define PAJ7620_PART_ID ((PAJ7620_VAL_PART_ID_MSB << 8) | (PAJ7620_VAL_PART_ID_LSB & 0x00FF))

/** Sensor FPS values */
#define PAJ7620_NORMAL_SPEED 0xAC /* Normal speed 120 fps */
#define PAJ7620_GAME_SPEED   0x30 /* Game mode speed 240 fps */

/** Gesture Entry/Exit default values in milliseconds */
#define PAJ7620_DEFAULT_GEST_ENTRY_TIME_MS 0
#define PAJ7620_DEFAULT_GEST_EXIT_TIME_MS  200

/** Sensor stabilization time microseconds (us) */
#define PAJ7620_SENSOR_STABILIZATION_TIME_US 700

enum paj7620_mem_bank {
	PAJ7620_MEMBANK_0 = 0,
	PAJ7620_MEMBANK_1,
};

struct paj7620_config {
	const struct i2c_dt_spec i2c;
#if CONFIG_PAJ7620_TRIGGER
	struct gpio_dt_spec int_gpio;
#endif
};

struct paj7620_data {
	struct k_sem sem;
	uint64_t gest_entry_time; /* User set gesture entry delay in ms (default: 0) */
	uint64_t gest_exit_time;  /* User set gesture exit delay in ms (default: 200) */
	enum paj7620_gesture gesture;

#ifdef CONFIG_PAJ7620_TRIGGER
	const struct device *dev;
	struct gpio_callback gpio_cb;
	sensor_trigger_handler_t motion_handler;
	const struct sensor_trigger *motion_trig;
#endif
#ifdef CONFIG_PAJ7620_TRIGGER_OWN_THREAD
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_PAJ7620_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem trig_sem;
#endif
#ifdef CONFIG_PAJ7620_TRIGGER_GLOBAL_THREAD
	struct k_work work;
#endif
};

int paj7620_byte_read(const struct device *dev, uint8_t reg, uint8_t *byte);
int paj7620_byte_write(const struct device *dev, uint8_t reg, uint8_t byte);

#ifdef CONFIG_PAJ7620_TRIGGER
int paj7620_trigger_init(const struct device *dev);
int paj7620_trigger_set(const struct device *dev,
		const struct sensor_trigger *trig,
		sensor_trigger_handler_t handler);
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_PAJ7620_H_ */
