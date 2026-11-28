/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ASAHI_KASEI_AK09940A_AK09940A_H_
#define ZEPHYR_DRIVERS_SENSOR_ASAHI_KASEI_AK09940A_AK09940A_H_

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "ak09940a_reg.h"

#define AK09940A_BUS_I2C DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#define AK09940A_BUS_SPI DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)

/* 10 nT per LSB */
#define AK09940A_MICRO_GAUSS_PER_LSB 100

/* Measurement data, ST1 to ST2 */
#define AK09940A_FRAME_SIZE (AK09940A_REG_ST2 - AK09940A_REG_ST1 + 1)
#define AK09940A_FRAME_ST1  (AK09940A_REG_ST1 - AK09940A_REG_ST1)
#define AK09940A_FRAME_HXL  (AK09940A_REG_HXL - AK09940A_REG_ST1)
#define AK09940A_FRAME_TMPS (AK09940A_REG_TMPS - AK09940A_REG_ST1)

/* Sensor drive settings */
struct ak09940a_drive {
	/* CNTL1 and CNTL3 MT bits */
	uint8_t cntl1;
	uint8_t cntl3;
	/* Maximum single measurement time */
	uint16_t measure_time_us;
	/* Number of continuous modes available, slowest first */
	uint8_t odr_count;
};

union ak09940a_bus {
#if AK09940A_BUS_I2C
	struct i2c_dt_spec i2c;
#endif
#if AK09940A_BUS_SPI
	struct spi_dt_spec spi;
#endif
};

struct ak09940a_bus_io {
	int (*check)(const union ak09940a_bus *bus);
	int (*read)(const union ak09940a_bus *bus, uint8_t reg, uint8_t *buf, size_t len);
	int (*write)(const union ak09940a_bus *bus, uint8_t reg, uint8_t val);
};

struct ak09940a_config {
	union ak09940a_bus bus;
	const struct ak09940a_bus_io *bus_io;
	struct gpio_dt_spec reset_gpio;
	const struct ak09940a_drive *drive;
	bool is_spi;
};

struct ak09940a_data {
	uint8_t frame[AK09940A_FRAME_SIZE];
	/* Operation mode, power-down or continuous */
	uint8_t mode;
#ifdef CONFIG_SENSOR_ASYNC_API
	struct rtio *rtio_ctx;
	struct rtio_iodev *iodev;
	atomic_t busy;
	/* The pending read triggers a single measurement */
	bool async_single;
#endif
};

struct ak09940a_encoded_data {
	uint64_t timestamp;
	uint8_t frame[AK09940A_FRAME_SIZE];
};

/* Raw 18-bit reading of an axis (0 = X, 1 = Y, 2 = Z) */
static inline int32_t ak09940a_frame_magn(const uint8_t *frame, uint8_t axis)
{
	return sign_extend(sys_get_le24(&frame[AK09940A_FRAME_HXL + 3U * axis]), 17);
}

/* Die temperature in micro degrees Celsius: 30 - TMPS / 1.7 */
static inline int64_t ak09940a_frame_temp_micro(const uint8_t *frame)
{
	return INT64_C(30000000) - ((int64_t)(int8_t)frame[AK09940A_FRAME_TMPS] * 10000000) / 17;
}

#ifdef CONFIG_SENSOR_ASYNC_API
void ak09940a_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe);

int ak09940a_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder);
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_ASAHI_KASEI_AK09940A_AK09940A_H_ */
