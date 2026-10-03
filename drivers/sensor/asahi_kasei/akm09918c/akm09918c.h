/*
 * Copyright (c) 2023 Google LLC
 * Copyright (c) 2024 Florian Weber <Florian.Weber@live.de>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_AKM09918C_AKM09918C_H_
#define ZEPHYR_DRIVERS_SENSOR_AKM09918C_AKM09918C_H_

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/rtio/rtio.h>

#include "akm09918c_reg.h"

/* Time it takes to get a measurement in single-measure mode */
#define AKM09918C_MEASURE_TIME_US 9000

/* Conversion values */
#define AKM09918C_MICRO_GAUSS_PER_BIT INT64_C(1500)

/* Maximum and minimum raw register values for magnetometer data per datasheet */
#define AKM09918C_MAGN_MAX_DATA_REG (32752)
#define AKM09918C_MAGN_MIN_DATA_REG (-32752)

/* Maximum and minimum magnetometer values in microgauss. +/-32752 is the maximum range of the
 * data registers (slightly less than the range of int16). This works out to +/- 49,128,000 uGs
 */
#define AKM09918C_MAGN_MAX_MICRO_GAUSS (AKM09918C_MAGN_MAX_DATA_REG * AKM09918C_MICRO_GAUSS_PER_BIT)
#define AKM09918C_MAGN_MIN_MICRO_GAUSS (AKM09918C_MAGN_MIN_DATA_REG * AKM09918C_MICRO_GAUSS_PER_BIT)

struct akm09918c_data {
	int16_t x_sample;
	int16_t y_sample;
	int16_t z_sample;
	uint8_t mode;
#ifdef CONFIG_SENSOR_ASYNC_API
	struct akm09918c_async_fetch_ctx {
		struct rtio_iodev_sqe *iodev_sqe;
		uint64_t timestamp;
		struct k_work_delayable async_fetch_work;
	} work_ctx;
	/* for communication to the bus controller */
	struct rtio *rtio_ctx;
	struct rtio_iodev *iodev;
#endif
};

struct akm09918c_config {
	struct i2c_dt_spec i2c;
};

int akm09918c_start_measurement_blocking(const struct device *dev, enum sensor_channel chan);

int akm09918c_fetch_measurement_blocking(const struct device *dev, int16_t *x, int16_t *y,
					 int16_t *z);
/*
 * RTIO types
 */

struct akm09918c_decoder_header {
	uint64_t timestamp;
} __packed;

struct akm09918c_encoded_data {
	struct akm09918c_decoder_header header;
	struct __packed {
		uint8_t st1;
		int16_t data[3];
		uint8_t tmps; /* not used  - only for padding */
		uint8_t st2;  /* not used but includes overflow data */
	} reading;
};

void akm09918_async_fetch(struct k_work *work);

int akm09918c_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder);

void akm09918c_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe);
void akm09918_after_start_cb(struct rtio *rtio_ctx, const struct rtio_sqe *sqe, int result,
			     void *arg0);
void akm09918_complete_cb(struct rtio *rtio_ctx, const struct rtio_sqe *sqe, int result,
			  void *arg0);

#endif /* ZEPHYR_DRIVERS_SENSOR_AKM09918C_AKM09918C_H_ */
