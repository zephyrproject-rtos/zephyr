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

#define DT_DRV_COMPAT bosch_bmi08x_accel
#include "bmi08x.h"
#include "bmi08x_bus.h"
#include "bmi08x_accel_decoder.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(BMI08X_ACCEL_DECODER, CONFIG_SENSOR_LOG_LEVEL);

enum bmi08x_accel_fifo_header {
	BMI08X_ACCEL_FIFO_FRAME_ACCEL = 0x84,
	BMI08X_ACCEL_FIFO_FRAME_SKIP = 0x40,
	BMI08X_ACCEL_FIFO_FRAME_TIME = 0x44,
	BMI08X_ACCEL_FIFO_FRAME_CONFIG = 0x48,
	BMI08X_ACCEL_FIFO_FRAME_DROP = 0x50,
	BMI08X_ACCEL_FIFO_FRAME_EMPTY = 0x80,
};

struct frame_len {
	enum bmi08x_accel_fifo_header header;
	uint8_t len;
} fifo_frame_len[] = {
	{.header = BMI08X_ACCEL_FIFO_FRAME_ACCEL, .len = 7},
	{.header = BMI08X_ACCEL_FIFO_FRAME_SKIP, .len = 2},
	{.header = BMI08X_ACCEL_FIFO_FRAME_TIME, .len = 4},
	{.header = BMI08X_ACCEL_FIFO_FRAME_CONFIG, .len = 2},
	{.header = BMI08X_ACCEL_FIFO_FRAME_DROP, .len = 2},
	{.header = BMI08X_ACCEL_FIFO_FRAME_EMPTY, .len = 2},
};

void bmi08x_accel_encode_header(const struct device *dev, struct bmi08x_accel_encoded_data *edata,
			       bool is_streaming, uint16_t buf_len)
{
	struct bmi08x_accel_data *data = dev->data;
	uint64_t cycles;

	if (sensor_clock_get_cycles(&cycles) == 0) {
		edata->header.timestamp = sensor_clock_cycles_to_ns(cycles);
	} else {
		edata->header.timestamp = 0;
	}
	edata->header.has_accel = true;
	edata->header.range = data->range;
	edata->header.chip_id = data->accel_chip_id;
	edata->header.is_streaming = is_streaming;
	edata->header.sample_count = is_streaming ? data->stream.fifo_wm : 1;
	edata->header.buf_len = buf_len;
}

static int bmi08x_decoder_get_frame_count(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
					  uint16_t *frame_count)
{
	const struct bmi08x_accel_encoded_data *edata =
		(const struct bmi08x_accel_encoded_data *)buffer;

	if (!edata->header.has_accel || chan_spec.chan_idx != 0) {
		return -ENODATA;
	}

	if (chan_spec.chan_type != SENSOR_CHAN_ACCEL_XYZ) {
		return -EINVAL;
	}

	*frame_count = edata->header.sample_count;
	return 0;
}

static int bmi08x_decoder_get_size_info(struct sensor_chan_spec chan_spec, size_t *base_size,
					size_t *frame_size)
{
	if (chan_spec.chan_idx != 0 || chan_spec.chan_type != SENSOR_CHAN_ACCEL_XYZ) {
		return -EINVAL;
	}

	*base_size = sizeof(struct sensor_three_axis_data);
	*frame_size = sizeof(struct sensor_three_axis_sample_data);
	return 0;
}

static inline void fixed_point_from_encoded_data(const uint16_t encoded_payload[3], uint8_t shift,
						 uint32_t fsr_value_g, q31_t output[3])
{
	for (size_t i = 0 ; i < 3 ; i++) {
		int64_t raw_value;

		raw_value = sign_extend_64(encoded_payload[i], 15);
		raw_value = (raw_value * fsr_value_g << (31 - 5 - 15)) * SENSOR_G / 1000000;

		output[i] = raw_value;
	}
}

static inline int bmi08x_decode_one_shot(const struct bmi08x_accel_encoded_data *edata,
					 uint32_t *fit, struct sensor_three_axis_data *data_output)
{
	uint32_t fsr_value_g = (edata->header.chip_id == BMI085_ACCEL_CHIP_ID) ? 2 : 3;

	if (*fit != 0) {
		return -ENODATA;
	}

	/** Bits we need to represent the integer part of FSR in m/s2:
	 * - 2 - 3 G (19.6 - 29.4 m/s2) = 5 bits.
	 * - 4 - 6 G (39.2 - 58.8 m/s2) = 6 bits.
	 * - 8 - 12 G (78.4 - 117.6 m/s2) = 7 bits.
	 * - 16 - 24 G (156.8 235.2 m/s2) = 8 bits.
	 */
	data_output->shift = 5 + edata->header.range;
	data_output->header.reading_count = 1;
	data_output->header.base_timestamp_ns = edata->header.timestamp;
	fixed_point_from_encoded_data(edata->payload, data_output->shift, fsr_value_g,
				      data_output->readings[0].values);

	return ++(*fit);
}

static inline int fifo_get_frame_len(enum bmi08x_accel_fifo_header header)
{
	for (size_t i = 0 ; i < ARRAY_SIZE(fifo_frame_len) ; i++) {
		if (header == fifo_frame_len[i].header) {
			return fifo_frame_len[i].len;
		}
	}
	return -EINVAL;
}

static inline int bmi08x_decode_fifo(const struct bmi08x_accel_encoded_data *edata, uint32_t *fit,
				     uint16_t max_count, struct sensor_three_axis_data *data_output)
{
	uint8_t reading_count = 0;
	uint32_t fsr_value_g = edata->header.chip_id == BMI085_ACCEL_CHIP_ID ? 2 : 3;

	if (*fit >= edata->header.buf_len) {
		return -ENODATA;
	}

	/** Bits we need to represent the integer part of FSR in m/s2:
	 * - 2 - 3 G (19.6 - 29.4 m/s2) = 5 bits.
	 * - 4 - 6 G (39.2 - 58.8 m/s2) = 6 bits.
	 * - 8 - 12 G (78.4 - 117.6 m/s2) = 7 bits.
	 * - 16 - 24 G (156.8 235.2 m/s2) = 8 bits.
	 */
	data_output->shift = 5 + edata->header.range;
	data_output->header.reading_count = 0;
	data_output->header.base_timestamp_ns = edata->header.timestamp;

	do {
		uint8_t header_byte = edata->fifo[*fit] & 0xFC;
		int frame_len = fifo_get_frame_len(header_byte);

		if (frame_len < 0) {
			LOG_WRN("Invalid frame header: 0x%02X", header_byte);
			return frame_len;
		}

		if (header_byte == BMI08X_ACCEL_FIFO_FRAME_ACCEL &&
		    *fit + frame_len <= edata->header.buf_len) {
			const uint16_t *values = (const uint16_t *)&edata->fifo[*fit + 1];

			fixed_point_from_encoded_data(
				values, data_output->shift, fsr_value_g,
				data_output->readings[reading_count].values);
			reading_count++;
		}
		*fit += frame_len;
	} while (*fit < edata->header.buf_len && reading_count < max_count);

	data_output->header.reading_count = reading_count;
	return reading_count;
}

static int bmi08x_decoder_decode(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
				 uint32_t *fit, uint16_t max_count, void *data_out)
{
	struct sensor_three_axis_data *data_output = (struct sensor_three_axis_data *)data_out;
	const struct bmi08x_accel_encoded_data *edata =
		(const struct bmi08x_accel_encoded_data *)buffer;

	if (chan_spec.chan_type != SENSOR_CHAN_ACCEL_XYZ || chan_spec.chan_idx != 0 ||
	    max_count == 0 || !edata->header.has_accel) {
		return -EINVAL;
	}

	if (edata->header.is_streaming) {
		return bmi08x_decode_fifo(edata, fit, max_count, data_output);
	} else {
		return bmi08x_decode_one_shot(edata, fit, data_output);
	}
}

static bool bmi08x_decoder_has_trigger(const uint8_t *buffer, enum sensor_trigger_type trigger)
{
	const struct bmi08x_accel_encoded_data *edata =
		(const struct bmi08x_accel_encoded_data *)buffer;

	return edata->header.is_streaming && edata->header.fifo_len > 0 &&
	       trigger == SENSOR_TRIG_FIFO_WATERMARK;
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = bmi08x_decoder_get_frame_count,
	.get_size_info = bmi08x_decoder_get_size_info,
	.decode = bmi08x_decoder_decode,
	.has_trigger = bmi08x_decoder_has_trigger,
};

int bmi08x_accel_decoder_get(const struct device *dev, const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);
	*decoder = &SENSOR_DECODER_NAME();
	return 0;
}
