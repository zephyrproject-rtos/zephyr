/*
 * Copyright (c) 2026 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_VL53L5CX_DECODER_H_
#define ZEPHYR_DRIVERS_SENSOR_VL53L5CX_DECODER_H_

#include <stdint.h>
#include <zephyr/drivers/sensor.h>

struct vl53l5cx_rtio_data {
	uint64_t timestamp;
	uint8_t distance_idx_num;
	VL53L5CX_ResultsData data;
};

int vl53l5cx_encode(const struct device *dev, const struct sensor_chan_spec *const channels,
		    const size_t num_channels, uint8_t *buf);

int vl53l5cx_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder);

#endif /* ZEPHYR_DRIVERS_SENSOR_VL53L5CX_DECODER_H_ */
