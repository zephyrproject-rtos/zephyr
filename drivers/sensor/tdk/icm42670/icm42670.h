/*
 * Copyright (c) 2024 TDK Invensense
 * Copyright (c) 2022 Esco Medical ApS
 * Copyright (c) 2020 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ICM42670_H_
#define ZEPHYR_DRIVERS_SENSOR_ICM42670_H_

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>

#include "imu/inv_imu_driver.h"
#ifdef CONFIG_TDK_APEX
#include "imu/inv_imu_apex.h"
#endif

union icm42670_bus {
#if CONFIG_SPI
	struct spi_dt_spec spi;
#endif
#if CONFIG_I2C
	struct i2c_dt_spec i2c;
#endif
};

typedef int (*icm42670_bus_check_fn)(const union icm42670_bus *bus);
typedef int (*icm42670_reg_read_fn)(const union icm42670_bus *bus, uint8_t reg, uint8_t *buf,
				    uint32_t size);
typedef int (*icm42670_reg_write_fn)(const union icm42670_bus *bus, uint8_t reg, uint8_t *buf,
				     uint32_t size);

struct icm42670_bus_io {
	icm42670_bus_check_fn check;
	icm42670_reg_read_fn read;
	icm42670_reg_write_fn write;
};

#if CONFIG_SPI
extern const struct icm42670_bus_io icm42670_bus_io_spi;
#endif

#if CONFIG_I2C
extern const struct icm42670_bus_io icm42670_bus_io_i2c;
#endif

struct icm42670_data {
	struct inv_imu_serif serif;
	struct inv_imu_device driver;
	uint8_t imu_whoami;
	char *imu_name;
	uint8_t chip_id;
	int32_t accel_x;
	int32_t accel_y;
	int32_t accel_z;
	uint16_t accel_hz;
	uint8_t accel_fs;
	uint8_t accel_pwr_mode;
	int32_t gyro_x;
	int32_t gyro_y;
	int32_t gyro_z;
	uint16_t gyro_hz;
	uint16_t gyro_fs;
	int32_t temp;
#ifdef CONFIG_TDK_APEX
	uint8_t dmp_odr_hz;
	uint64_t pedometer_cnt;
	uint8_t pedometer_activity;
	uint8_t pedometer_cadence;
	uint8_t apex_status;
#endif

#ifdef CONFIG_ICM42670_TRIGGER
	const struct device *dev;
	struct gpio_callback gpio_cb;
	sensor_trigger_handler_t data_ready_handler;
	const struct sensor_trigger *data_ready_trigger;
	struct k_mutex mutex;
#endif
#ifdef CONFIG_ICM42670_TRIGGER_OWN_THREAD
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_ICM42670_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#endif
#ifdef CONFIG_ICM42670_TRIGGER_GLOBAL_THREAD
	struct k_work work;
#endif
};

struct icm42670_config {
	union icm42670_bus bus;
	const struct icm42670_bus_io *bus_io;
	struct gpio_dt_spec gpio_int;
	uint8_t accel_fs;
	uint16_t accel_hz;
	uint16_t accel_avg;
	uint16_t accel_filt_bw;
	uint16_t gyro_fs;
	uint16_t gyro_hz;
	uint16_t gyro_filt_bw;
	uint8_t accel_pwr_mode;
	uint8_t apex;
};

#ifdef CONFIG_TDK_APEX

#define ICM42670_APEX_STATUS_MASK_TILT  BIT(0)
#define ICM42670_APEX_STATUS_MASK_SMD   BIT(1)
#define ICM42670_APEX_STATUS_MASK_WOM_X BIT(2)
#define ICM42670_APEX_STATUS_MASK_WOM_Y BIT(3)
#define ICM42670_APEX_STATUS_MASK_WOM_Z BIT(4)

int icm42670_apex_enable(inv_imu_device_t *s);
int icm42670_apex_fetch_from_dmp(const struct device *dev);
void icm42670_apex_pedometer_cadence_convert(struct sensor_value *val, uint8_t raw_val,
					     uint8_t dmp_odr_hz);
int icm42670_apex_enable_pedometer(const struct device *dev, inv_imu_device_t *s);
int icm42670_apex_enable_tilt(inv_imu_device_t *s);
int icm42670_apex_enable_smd(inv_imu_device_t *s);
int icm42670_apex_enable_wom(inv_imu_device_t *s);
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_ICM42670_H_ */
