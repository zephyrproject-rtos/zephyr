/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ICM20689_DECODER_H_
#define ZEPHYR_DRIVERS_SENSOR_ICM20689_DECODER_H_

#include <stdint.h>
#include <zephyr/drivers/sensor.h>
#include "icm20689.h"

/* Bit positions and raw-value indices in the one-shot encoded sample. */
enum icm20689_encoded_channel {
	ICM20689_ENCODED_CHAN_DIE_TEMP,
	ICM20689_ENCODED_CHAN_ACCEL_X,
	ICM20689_ENCODED_CHAN_ACCEL_Y,
	ICM20689_ENCODED_CHAN_ACCEL_Z,
	ICM20689_ENCODED_CHAN_GYRO_X,
	ICM20689_ENCODED_CHAN_GYRO_Y,
	ICM20689_ENCODED_CHAN_GYRO_Z,
};

/** Metadata needed to decode one ICM20689 sample. */
struct icm20689_decoder_header {
	uint64_t timestamp;
	uint16_t accel_fs;
	uint16_t gyro_fs;
	struct icm20689_axis_alignment axis_align[3];
};

/** Encoded one-shot sample; readings use the indices in icm20689_encoded_channel. */
struct icm20689_encoded_data {
	struct icm20689_decoder_header header;
	uint8_t channels;
	int16_t readings[7];
};

int icm20689_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder);

#endif /* ZEPHYR_DRIVERS_SENSOR_ICM20689_DECODER_H_ */
