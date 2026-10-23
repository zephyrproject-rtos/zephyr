/*
 * Copyright (c) 2025 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ICTX53XX_H_
#define ZEPHYR_DRIVERS_SENSOR_ICTX53XX_H_

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/rtio/rtio.h>
#include "ict/inv_ict_driver.h"

/* Address value has a read bit */
#define REG_READ_BIT BIT(7)

/* Measurement time */
#define ICTX53XX_MEASUREMENT_TIME_US        3050
#define ICTX53XX_MEASUREMENT_EXTRA_TIME_US  51

/* Temperature */
#define ICTX53XX_TEMPERATURE_OFFSET_UC       25000000 /* 25 C in µC */
#define ICTX53XX_TEMPERATURE_SENSITIVITY_UC  6250 /* 6.25 mC/LSB, in µC */

/* RTIO-specific constants */
#define ICTX53XX_SHIFT 16 /* Q31 shift for sensor field values */

struct ictx53xx_bus {
	/* for communication to the bus controller */
	struct rtio *rtio_ctx;
	struct rtio_iodev *iodev;
};

struct ictx53xx_decoder_header {
	uint64_t timestamp;
	uint8_t channels;
	uint8_t sensitivity;
} __packed;

struct ictx53xx_encoded_data {
	struct ictx53xx_decoder_header header;
	union {
		uint8_t readings[8]; /* Raw I2C data from registers 0x08-0x0F */
		struct {
			int16_t temp;
			struct {
				int16_t x;
				int16_t y;
				int16_t z;
			} mag;
		} __attribute__((__packed__));
	};
};

struct ictx53xx_data {
	struct ictx53xx_encoded_data edata;
	inv_ict_device_t driver;
	inv_ict_mode_t mode;
	inv_ict_odr_t odr;
#ifdef CONFIG_SENSOR_ASYNC_API
	struct ictx53xx_async_fetch_ctx {
		struct rtio_iodev_sqe *iodev_sqe;
		uint64_t timestamp;
		struct k_work_delayable async_fetch_work;
	} work_ctx;
#endif
	struct ictx53xx_bus bus;
};

struct ictx53xx_config {
	inv_ict_mode_t mode;
	int odr_hz;
};

int ictx53xx_i2c_prep_reg_read_rtio_async(const struct ictx53xx_bus *bus, uint8_t reg,
				uint8_t *buf, size_t size, struct rtio_sqe **out);
int ictx53xx_i2c_prep_reg_write_rtio_async(const struct ictx53xx_bus *bus, uint8_t reg,
				const uint8_t *buf, size_t size, struct rtio_sqe **out);
int ictx53xx_reg_read_rtio(const struct ictx53xx_bus *bus, uint8_t start, uint8_t *buf, int size);
int ictx53xx_reg_write_rtio(const struct ictx53xx_bus *bus, uint8_t reg, const uint8_t *buf,
				int size);

int32_t ictx53xx_convert_mag(uint8_t sensitivity_nt_per_lsb, int16_t sample);
int32_t ictx53xx_convert_temp(int16_t sample);

#ifdef CONFIG_SENSOR_ASYNC_API
int ictx53xx_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder);
void ictx53xx_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe);
void ictx53xx_async_fetch(struct k_work *work);
uint8_t ictx53xx_encode_channel(enum sensor_channel chan);
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_ICTX53XX_H_*/
