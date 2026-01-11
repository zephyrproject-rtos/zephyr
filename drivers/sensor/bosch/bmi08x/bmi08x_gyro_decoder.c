/*
 * Copyright (c) 2025 Croxel, Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor_clock.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/sys/check.h>

#define DT_DRV_COMPAT bosch_bmi08x_gyro
#include "bmi08x.h"
#include "bmi08x_bus.h"
#include "bmi08x_gyro_stream.h"
#include "bmi08x_gyro_decoder.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(BMI08X_GYRO_DECODER, CONFIG_SENSOR_LOG_LEVEL);

void bmi08x_gyro_encode_header(const struct device *dev, struct bmi08x_gyro_encoded_data *edata,
			       bool is_streaming)
{
	struct bmi08x_gyro_data *data = dev->data;
	uint64_t cycles;

	if (sensor_clock_get_cycles(&cycles) == 0) {
		edata->header.timestamp = sensor_clock_cycles_to_ns(cycles);
	} else {
		edata->header.timestamp = 0;
	}
	edata->header.has_gyro = true;
	edata->header.range = data->range;
	edata->header.is_streaming = is_streaming;
	edata->header.sample_count = is_streaming ? data->stream.fifo_wm : 1;
}

static int bmi08x_decoder_get_frame_count(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
					  uint16_t *frame_count)
{
	const struct bmi08x_gyro_encoded_data *edata =
		(const struct bmi08x_gyro_encoded_data *)buffer;

	if (!edata->header.has_gyro || chan_spec.chan_idx != 0) {
		return -ENODATA;
	}
	if (chan_spec.chan_type != SENSOR_CHAN_GYRO_XYZ) {
		return -EINVAL;
	}

	*frame_count = edata->header.sample_count;
	return 0;
}

static int bmi08x_decoder_get_size_info(struct sensor_chan_spec chan_spec, size_t *base_size,
					size_t *frame_size)
{
	if (chan_spec.chan_idx != 0 || chan_spec.chan_type != SENSOR_CHAN_GYRO_XYZ) {
		return -EINVAL;
	}

	*base_size = sizeof(struct sensor_three_axis_data);
	*frame_size = sizeof(struct sensor_three_axis_sample_data);
	return 0;
}

static int bmi08x_decoder_decode(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
				 uint32_t *fit, uint16_t max_count, void *data_out)
{
	const struct bmi08x_gyro_encoded_data *edata =
		(const struct bmi08x_gyro_encoded_data *)buffer;
	struct sensor_three_axis_data *data_output = (struct sensor_three_axis_data *)data_out;
	uint32_t fit0 = *fit;

	if (chan_spec.chan_type != SENSOR_CHAN_GYRO_XYZ || chan_spec.chan_idx != 0 ||
	    max_count == 0) {
		return -EINVAL;
	}
	if (!edata->header.has_gyro || *fit >= edata->header.sample_count) {
		return -ENODATA;
	}
	if (edata->header.is_streaming &&
	    ((edata->header.fifo_status & 0x7F) == 0)) {
		return -ENODATA;
	}
	uint32_t max_samples = MIN(edata->header.sample_count, edata->header.fifo_status & 0x7F);

	/** Bits we need to represent the integer part of FSR in rad/s:
	 * - 2000 dps (34.91 rad/s) = 6 bits.
	 * - 1000 dps (17.45 rad/s) = 5 bits.
	 * -  500 dps (8.73 rad/s) = 4 bits.
	 * -  250 dps (4.36 rad/s) = 3 bits.
	 * -  125 dps (2.18 rad/s) = 2 bits.
	 */
	data_output->shift = 6 - edata->header.range;
	data_output->header.base_timestamp_ns = edata->header.timestamp;

	do {
		for (size_t i = 0 ; i < 3 ; i++) {
			int64_t raw_value;

			raw_value = sign_extend_64(edata->fifo[*fit].payload[i], 15);
			raw_value = (raw_value * 2000 << (31 - 6 - 15)) * SENSOR_PI / 1000000 / 180;

			data_output->readings[*fit - fit0].values[i] = raw_value;
		}
	} while (++(*fit) < MIN(max_samples, fit0 + max_count));

	data_output->header.reading_count = *fit - fit0;

	return data_output->header.reading_count;
}

static bool bmi08x_decoder_has_trigger(const uint8_t *buffer, enum sensor_trigger_type trigger)
{
	const struct bmi08x_gyro_encoded_data *edata =
		(const struct bmi08x_gyro_encoded_data *)buffer;

	return trigger == SENSOR_TRIG_FIFO_WATERMARK &&
	       edata->header.has_gyro &&
	       edata->header.is_streaming &&
	       edata->header.int_status & BIT(4) &&
	       (edata->header.fifo_status & BIT_MASK(7)) > 0;
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = bmi08x_decoder_get_frame_count,
	.get_size_info = bmi08x_decoder_get_size_info,
	.decode = bmi08x_decoder_decode,
	.has_trigger = bmi08x_decoder_has_trigger,
};

int bmi08x_gyro_decoder_get(const struct device *dev, const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);
	*decoder = &SENSOR_DECODER_NAME();
	return 0;
}
