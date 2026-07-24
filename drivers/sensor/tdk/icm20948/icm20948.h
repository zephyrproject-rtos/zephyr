/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ICM20948_ICM20948_H_
#define ZEPHYR_DRIVERS_SENSOR_ICM20948_ICM20948_H_

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/dt-bindings/sensor/icm20948.h>
#include "icm20948_reg.h"

struct icm20948_data {
	int16_t accel_x;
	int16_t accel_y;
	int16_t accel_z;
	uint16_t accel_sensitivity_shift;

	int16_t temp;

	int16_t gyro_x;
	int16_t gyro_y;
	int16_t gyro_z;
	uint16_t gyro_sensitivity_x10;

	int16_t magn_x;
	int16_t magn_y;
	int16_t magn_z;
	int16_t magn_scale_x;
	int16_t magn_scale_y;
	int16_t magn_scale_z;
	uint8_t magn_st2;

#ifdef CONFIG_ICM20948_TRIGGER
	const struct device *dev;
	struct gpio_callback gpio_cb;

	/* Data ready trigger */
	const struct sensor_trigger *drdy_trigger;
	sensor_trigger_handler_t drdy_handler;

	/* Motion (Wake-on-Motion) trigger */
	const struct sensor_trigger *motion_trigger;
	sensor_trigger_handler_t motion_handler;

#if defined(CONFIG_ICM20948_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_ICM20948_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_ICM20948_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif

#endif /* CONFIG_ICM20948_TRIGGER */
};

enum icm20948_bus_type {
	ICM20948_BUS_SPI,
	ICM20948_BUS_I2C,
};

struct icm20948_config {
	enum icm20948_bus_type bus_type;
	union {
		struct i2c_dt_spec i2c;
		struct spi_dt_spec spi;
	};
	uint8_t gyro_div;
	uint8_t gyro_dlpf;
	uint8_t gyro_fs;
	uint16_t accel_div;
	uint8_t accel_dlpf;
	uint8_t accel_fs;
	uint8_t mag_mode;
#ifdef CONFIG_ICM20948_TRIGGER
	const struct gpio_dt_spec int_pin;
#endif /* CONFIG_ICM20948_TRIGGER */
};

#ifdef CONFIG_ICM20948_TRIGGER
int icm20948_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler);
int icm20948_init_interrupt(const struct device *dev);
int icm20948_config_wom(const struct device *dev, uint8_t threshold_mg);
#endif /* CONFIG_ICM20948_TRIGGER */

int icm20948_read_reg(const struct device *dev, uint16_t reg, uint8_t *val);
int icm20948_read_field(const struct device *dev, uint16_t reg, uint8_t mask, uint8_t *val);
int icm20948_read_block(const struct device *dev, uint16_t reg, uint8_t *buf, uint8_t len);
int icm20948_write_reg(const struct device *dev, uint16_t reg, uint8_t val);
int icm20948_write_field(const struct device *dev, uint16_t reg, uint8_t mask, uint8_t val);
int icm20948_set_bank(const struct device *dev, uint16_t reg);

int ak09916_init(const struct device *dev);
int ak09916_convert_magn(struct sensor_value *val, int16_t raw_val, int16_t scale, uint8_t st2);

#endif /* ZEPHYR_DRIVERS_SENSOR_ICM20948_ICM20948_H_ */
