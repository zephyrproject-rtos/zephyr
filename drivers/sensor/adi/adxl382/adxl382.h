/*
 * Copyright (c) 2026 Deere & Company
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ADXL382_H_
#define ZEPHYR_DRIVERS_SENSOR_ADXL382_H_

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>

#define ADXL382_DEVID_AD 0x00
#define ADXL382_MISC0 0x20
#define ADXL382_OP_MODE 0x26
#define ADXL382_DIG_EN 0x27
#define ADXL382_RESET 0x2a
#define ADXL382_XDATA_H 0x15

#define ADXL382_READ 0x01
#define ADXL382_REG_READ(reg) (((reg) << 1) | ADXL382_READ)
#define ADXL382_REG_WRITE(reg) ((reg) << 1)
#define ADXL382_RESET_CODE 0x52
#define ADXL382_DIG_EN_XYZ 0x70
#define ADXL382_MODE_60G_LP 0x84
#define ADXL382_COUNTS_PER_G 500
#define ADXL382_MAX_READ_SIZE 6

/* +/-60 g full scale (642.6 m/s^2 at INT16_MAX) fits a q31 with shift 10. */
#define ADXL382_Q31_SHIFT 10
/* round(SENSOR_G / 500 counts/g * 2^(31 - shift) / 1e6) */
#define ADXL382_Q31_SCALE INT32_C(41132)

struct adxl382_xyz_accel_data {
	int16_t x;
	int16_t y;
	int16_t z;
};

/* One-shot RTIO buffer consumed by the decoder. */
struct adxl382_encoded_data {
	uint64_t timestamp;
	struct adxl382_xyz_accel_data sample;
};

struct adxl382_transfer_function {
	int (*read_reg_multiple)(const struct device *dev, uint8_t reg,
				 uint8_t *values, uint16_t count);
	int (*read_reg)(const struct device *dev, uint8_t reg, uint8_t *value);
	int (*write_reg)(const struct device *dev, uint8_t reg, uint8_t value);
};

struct adxl382_dev_config {
	struct spi_dt_spec spi;
	int (*bus_init)(const struct device *dev);
};

struct adxl382_data {
	struct k_mutex lock;
	const struct adxl382_transfer_function *hw_tf;
	struct adxl382_xyz_accel_data sample;
	bool valid;
};

int adxl382_spi_init(const struct device *dev);

/* Read XYZ directly from the device, signed counts at 500 counts/g. */
int adxl382_get_accel_data(const struct device *dev, struct adxl382_xyz_accel_data *accel);

#ifdef CONFIG_SENSOR_ASYNC_API
void adxl382_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe);
int adxl382_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder);
#endif

#endif
