/*
 * Copyright (c) 2026 Deere & Company
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_adxl382

#include <errno.h>
#include <zephyr/drivers/sensor.h>

#include "adxl382.h"

static inline q31_t adxl382_accel_q31(int16_t raw)
{
	return (q31_t)raw * ADXL382_Q31_SCALE;
}

static bool adxl382_is_accel(enum sensor_channel chan)
{
	return chan == SENSOR_CHAN_ACCEL_X || chan == SENSOR_CHAN_ACCEL_Y ||
	       chan == SENSOR_CHAN_ACCEL_Z || chan == SENSOR_CHAN_ACCEL_XYZ;
}

static int adxl382_decoder_get_frame_count(const uint8_t *buffer,
					   struct sensor_chan_spec chan_spec,
					   uint16_t *frame_count)
{
	ARG_UNUSED(buffer);

	if (chan_spec.chan_idx != 0 || !adxl382_is_accel(chan_spec.chan_type)) {
		return -ENOTSUP;
	}
	*frame_count = 1;
	return 0;
}

static int adxl382_decoder_get_size_info(struct sensor_chan_spec chan_spec, size_t *base_size,
					 size_t *frame_size)
{
	if (!adxl382_is_accel(chan_spec.chan_type)) {
		return -ENOTSUP;
	}
	return sensor_natively_supported_channel_size_info(chan_spec, base_size, frame_size);
}

static int adxl382_decoder_decode(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
				  uint32_t *fit, uint16_t max_count, void *data_out)
{
	const struct adxl382_encoded_data *edata = (const struct adxl382_encoded_data *)buffer;
	const struct adxl382_xyz_accel_data *sample = &edata->sample;
	int16_t raw;

	if (chan_spec.chan_idx != 0 || !adxl382_is_accel(chan_spec.chan_type)) {
		return -ENOTSUP;
	}
	if (*fit != 0 || max_count == 0) {
		return 0;
	}

	if (chan_spec.chan_type == SENSOR_CHAN_ACCEL_XYZ) {
		struct sensor_three_axis_data *out = data_out;

		out->header.base_timestamp_ns = edata->timestamp;
		out->header.reading_count = 1;
		out->shift = ADXL382_Q31_SHIFT;
		out->readings[0].timestamp_delta = 0;
		out->readings[0].x = adxl382_accel_q31(sample->x);
		out->readings[0].y = adxl382_accel_q31(sample->y);
		out->readings[0].z = adxl382_accel_q31(sample->z);
	} else {
		struct sensor_q31_data *out = data_out;

		raw = chan_spec.chan_type == SENSOR_CHAN_ACCEL_X ? sample->x :
		      chan_spec.chan_type == SENSOR_CHAN_ACCEL_Y ? sample->y : sample->z;
		out->header.base_timestamp_ns = edata->timestamp;
		out->header.reading_count = 1;
		out->shift = ADXL382_Q31_SHIFT;
		out->readings[0].timestamp_delta = 0;
		out->readings[0].value = adxl382_accel_q31(raw);
	}
	*fit = 1;
	return 1;
}

static bool adxl382_decoder_has_trigger(const uint8_t *buffer, enum sensor_trigger_type trigger)
{
	ARG_UNUSED(buffer);
	ARG_UNUSED(trigger);
	/* One-shot reads only; FIFO streaming is not implemented. */
	return false;
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = adxl382_decoder_get_frame_count,
	.get_size_info = adxl382_decoder_get_size_info,
	.decode = adxl382_decoder_decode,
	.has_trigger = adxl382_decoder_has_trigger,
};

int adxl382_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);
	*decoder = &SENSOR_DECODER_NAME();
	return 0;
}
