/* sensor_lsm6dsr.h - header file for LSM6DSR accelerometer, gyroscope and
 * temperature sensor driver
 */

/*
 * Copyright (c) 2025 Helbling Technik AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_LSM6DSR_LSM6DSR_H_
#define ZEPHYR_DRIVERS_SENSOR_LSM6DSR_LSM6DSR_H_

#include <zephyr/drivers/sensor.h>
#include <zephyr/types.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#include <zephyr/drivers/spi.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#include <zephyr/drivers/i2c.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */

#define LSM6DSR_REG_FIFO_CTRL4             0x0A
#define LSM6DSR_MASK_FIFO_CTRL4_FIFO_MODE  (BIT(2) | BIT(1) | BIT(0))
#define LSM6DSR_SHIFT_FIFO_CTRL4_FIFO_MODE 0

#define LSM6DSR_REG_WHO_AM_I 0x0F
#define LSM6DSR_VAL_WHO_AM_I 0x6B

#define LSM6DSR_REG_CTRL1_XL          0x10
#define LSM6DSR_MASK_CTRL1_XL_ODR_XL  (BIT(7) | BIT(6) | BIT(5) | BIT(4))
#define LSM6DSR_SHIFT_CTRL1_XL_ODR_XL 4
#define LSM6DSR_MASK_CTRL1_XL_FS_XL   (BIT(3) | BIT(2))
#define LSM6DSR_SHIFT_CTRL1_XL_FS_XL  2

#define LSM6DSR_REG_CTRL2_G         0x11
#define LSM6DSR_MASK_CTRL2_G_ODR_G  (BIT(7) | BIT(6) | BIT(5) | BIT(4))
#define LSM6DSR_SHIFT_CTRL2_G_ODR_G 4
#define LSM6DSR_MASK_CTRL2_G_FS_G   (BIT(3) | BIT(2))
#define LSM6DSR_SHIFT_CTRL2_G_FS_G  2
#define LSM6DSR_MASK_CTRL2_FS125    BIT(1)
#define LSM6DSR_SHIFT_CTRL2_FS125   1
#define LSM6DSR_MASK_CTRL2_FS4000   BIT(0)
#define LSM6DSR_SHIFT_CTRL2_FS4000  0

#define LSM6DSR_REG_CTRL3_C                0x12
#define LSM6DSR_MASK_CTRL3_C_BOOT          BIT(7)
#define LSM6DSR_SHIFT_CTRL3_C_BOOT         7
#define LSM6DSR_MASK_CTRL3_C_BDU           BIT(6)
#define LSM6DSR_SHIFT_CTRL3_C_BDU          6
#define LSM6DSR_MASK_CTRL3_C_IF_INC        BIT(2)
#define LSM6DSR_SHIFT_CTRL3_C_IF_INC       2
#define LSM6DSR_MASK_CTRL3_C_MUST_BE_ZERO  BIT(1)
#define LSM6DSR_SHIFT_CTRL3_C_MUST_BE_ZERO 1

#define LSM6DSR_REG_CTRL6_C              0x15
#define LSM6DSR_MASK_CTRL6_C_XL_HM_MODE  BIT(4)
#define LSM6DSR_SHIFT_CTRL6_C_XL_HM_MODE 4

#define LSM6DSR_REG_CTRL7_G           0x16
#define LSM6DSR_MASK_CTRL7_G_HM_MODE  BIT(7)
#define LSM6DSR_SHIFT_CTRL7_G_HM_MODE 7

#define LSM6DSR_REG_OUTX_L_G  0x22
#define LSM6DSR_REG_OUTX_H_G  0x23
#define LSM6DSR_REG_OUTY_L_G  0x24
#define LSM6DSR_REG_OUTY_H_G  0x25
#define LSM6DSR_REG_OUTZ_L_G  0x26
#define LSM6DSR_REG_OUTZ_H_G  0x27
#define LSM6DSR_REG_OUTX_L_XL 0x28
#define LSM6DSR_REG_OUTX_H_XL 0x29
#define LSM6DSR_REG_OUTY_L_XL 0x2A
#define LSM6DSR_REG_OUTY_H_XL 0x2B
#define LSM6DSR_REG_OUTZ_L_XL 0x2C
#define LSM6DSR_REG_OUTZ_H_XL 0x2D

/* Accel sensor sensitivity grain is 61 ug/LSB */
#define SENSI_GRAIN_XL 61LL

/* Gyro sensor sensitivity grain is 4375 udps/LSB */
#define SENSI_GRAIN_G 4375LL

#if CONFIG_LSM6DSR_ACCEL_FS == 0
#define LSM6DSR_ACCEL_FS_RUNTIME          1
#define LSM6DSR_DEFAULT_ACCEL_FULLSCALE   0
#define LSM6DSR_DEFAULT_ACCEL_SENSITIVITY SENSI_GRAIN_XL
#elif CONFIG_LSM6DSR_ACCEL_FS == 2
#define LSM6DSR_DEFAULT_ACCEL_FULLSCALE   0
#define LSM6DSR_DEFAULT_ACCEL_SENSITIVITY SENSI_GRAIN_XL
#elif CONFIG_LSM6DSR_ACCEL_FS == 4
#define LSM6DSR_DEFAULT_ACCEL_FULLSCALE   2
#define LSM6DSR_DEFAULT_ACCEL_SENSITIVITY (2.0 * SENSI_GRAIN_XL)
#elif CONFIG_LSM6DSR_ACCEL_FS == 8
#define LSM6DSR_DEFAULT_ACCEL_FULLSCALE   3
#define LSM6DSR_DEFAULT_ACCEL_SENSITIVITY (4.0 * SENSI_GRAIN_XL)
#elif CONFIG_LSM6DSR_ACCEL_FS == 16
#define LSM6DSR_DEFAULT_ACCEL_FULLSCALE   1
#define LSM6DSR_DEFAULT_ACCEL_SENSITIVITY (8.0 * SENSI_GRAIN_XL)
#endif

#if (CONFIG_LSM6DSR_ACCEL_ODR == 0)
#define LSM6DSR_ACCEL_ODR_RUNTIME 1
#endif

#if CONFIG_LSM6DSR_GYRO_FS == 0
#define LSM6DSR_GYRO_FS_RUNTIME          1
#define LSM6DSR_DEFAULT_GYRO_FULLSCALE   4
#define LSM6DSR_DEFAULT_GYRO_SENSITIVITY SENSI_GRAIN_G
#elif CONFIG_LSM6DSR_GYRO_FS == 125
#define LSM6DSR_DEFAULT_GYRO_FULLSCALE   4
#define LSM6DSR_DEFAULT_GYRO_SENSITIVITY SENSI_GRAIN_G
#elif CONFIG_LSM6DSR_GYRO_FS == 250
#define LSM6DSR_DEFAULT_GYRO_FULLSCALE   0
#define LSM6DSR_DEFAULT_GYRO_SENSITIVITY (2.0 * SENSI_GRAIN_G)
#elif CONFIG_LSM6DSR_GYRO_FS == 500
#define LSM6DSR_DEFAULT_GYRO_FULLSCALE   1
#define LSM6DSR_DEFAULT_GYRO_SENSITIVITY (4.0 * SENSI_GRAIN_G)
#elif CONFIG_LSM6DSR_GYRO_FS == 1000
#define LSM6DSR_DEFAULT_GYRO_FULLSCALE   2
#define LSM6DSR_DEFAULT_GYRO_SENSITIVITY (8.0 * SENSI_GRAIN_G)
#elif CONFIG_LSM6DSR_GYRO_FS == 2000
#define LSM6DSR_DEFAULT_GYRO_FULLSCALE   3
#define LSM6DSR_DEFAULT_GYRO_SENSITIVITY (16.0 * SENSI_GRAIN_G)
#elif CONFIG_LSM6DSR_GYRO_FS == 4000
#define LSM6DSR_DEFAULT_GYRO_FULLSCALE   5
#define LSM6DSR_DEFAULT_GYRO_SENSITIVITY (32.0 * SENSI_GRAIN_G)
#endif

#if (CONFIG_LSM6DSR_GYRO_ODR == 0)
#define LSM6DSR_GYRO_ODR_RUNTIME 1
#endif

union lsm6dsr_bus_cfg {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	struct i2c_dt_spec i2c;
#endif

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	struct spi_dt_spec spi;
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */
};

struct lsm6dsr_config {
	int (*bus_init)(const struct device *dev);
	const union lsm6dsr_bus_cfg bus_cfg;
};

struct lsm6dsr_data;

struct lsm6dsr_transfer_function {
	int (*read_data)(const struct device *dev, uint8_t reg_addr, uint8_t *value, uint8_t len);
	int (*write_data)(const struct device *dev, uint8_t reg_addr, uint8_t *value, uint8_t len);
	int (*read_reg)(const struct device *dev, uint8_t reg_addr, uint8_t *value);
	int (*update_reg)(const struct device *dev, uint8_t reg_addr, uint8_t mask, uint8_t value);
};

struct lsm6dsr_data {
	int accel_sample_x;
	int accel_sample_y;
	int accel_sample_z;
	float accel_sensitivity;
	int gyro_sample_x;
	int gyro_sample_y;
	int gyro_sample_z;
	float gyro_sensitivity;
	const struct lsm6dsr_transfer_function *hw_tf;
	uint16_t accel_freq;
	uint16_t gyro_freq;
};

int lsm6dsr_spi_init(const struct device *dev);
int lsm6dsr_i2c_init(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_SENSOR_LSM6DSR_LSM6DSR_H_ */
