/* ST Microelectronics LIS2DW12 3-axis accelerometer driver
 *
 * Copyright (c) 2026 Sloc GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_LIS2DW12_LIS2DW12_ASYNC_H_
#define ZEPHYR_DRIVERS_SENSOR_LIS2DW12_LIS2DW12_ASYNC_H_

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/rtio/rtio.h>

struct lis2dw12_encoded_data {
	uint64_t timestamp_ns;
	int16_t accel[3];
	int16_t temp;
	uint16_t gain;
	uint8_t has_accel;
	uint8_t has_temp;
} __packed;

void lis2dw12_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe);
int lis2dw12_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder);

#endif /* ZEPHYR_DRIVERS_SENSOR_LIS2DW12_LIS2DW12_ASYNC_H_ */
