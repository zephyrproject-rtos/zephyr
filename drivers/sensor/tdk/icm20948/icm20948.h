/*
 * Copyright (c) 2026 T3 Gemstone
 *	T3 Gemstone Developer Team <support@t3gemstone.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ICM20948_H_
#define ZEPHYR_DRIVERS_SENSOR_ICM20948_H_

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>

/* No bank has been selected yet, or the last attempt to select one failed */
#define ICM20948_BANK_UNKNOWN 0xFF

struct icm20948_config {
	struct spi_dt_spec spi;
	struct gpio_dt_spec irq; /* Only meaningful when a trigger is enabled */
	uint8_t gyro_fs;
	uint8_t accel_fs;
	uint8_t gyro_dlpf;
	uint8_t accel_dlpf;
};

struct icm20948_data {
	struct k_sem lock;
	uint8_t bank;
	int16_t accel[3];
	int16_t gyro[3];
	int16_t temp;
#ifdef CONFIG_ICM20948_MAGN_EN
	int16_t magn[3];
	uint8_t magn_st2; /* Holds the overflow flag of the last conversion */
	bool magn_valid;  /* Clear until a sample has been read off a live bus */
#endif
#ifdef CONFIG_ICM20948_TRIGGER
	const struct device *dev; /* Back reference, the callback only gets its own */
	struct gpio_callback irq_cb;
	struct k_mutex trig_lock; /* Keeps the handler and its trigger in step */
	const struct sensor_trigger *drdy_trigger;
	sensor_trigger_handler_t drdy_handler;
#if defined(CONFIG_ICM20948_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_ICM20948_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem irq_sem;
#elif defined(CONFIG_ICM20948_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif
#endif
};

/*
 * Register accessors. The user bank encoded in the high byte of reg is
 * selected before the transfer if it is not selected already, so callers
 * never deal with banks themselves.
 *
 * The caller is expected to hold the device lock.
 */
int icm20948_read(const struct device *dev, uint16_t reg, uint8_t *buf, size_t len);
int icm20948_write(const struct device *dev, uint16_t reg, uint8_t val);
int icm20948_update(const struct device *dev, uint16_t reg, uint8_t mask, uint8_t val);

#ifdef CONFIG_ICM20948_TRIGGER
/* Arms the interrupt line of the device and the thread that services it */
int icm20948_trigger_init(const struct device *dev);

int icm20948_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler);
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_ICM20948_H_ */
