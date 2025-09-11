/*
 * Copyright (c) 2025 Sentry Technologies ApS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/dsp/utils.h>
#include <zephyr/rtio/rtio.h>

static inline struct sensor_fetch_get_compat *
sensor_fetch_get_compat_lookup(const struct device *dev)
{
	STRUCT_SECTION_FOREACH(sensor_fetch_get_compat, compat) {
		if (compat->dev == dev) {
			return compat;
		}
	}

	return NULL;
}

int sensor_sample_fetch_compat(const struct device *dev)
{
	struct sensor_fetch_get_compat *compat = sensor_fetch_get_compat_lookup(dev);

	if (!compat) {
		return -ENOSYS;
	}

	return sensor_read(compat->iodev, compat->rtio, compat->buf, compat->buf_size);
}

int sensor_channel_get_compat(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct sensor_fetch_get_compat *compat = sensor_fetch_get_compat_lookup(dev);
	struct sensor_chan_spec ch = {.chan_type = chan, .chan_idx = 0};
	const struct sensor_decoder_api *decoder;
	/* Size the decode buffer to the biggest structure in sensor_data_types */
	uint8_t decode_buf[sizeof(struct sensor_game_rotation_vector_data)] = {0};
	uint32_t fit = 0;
	int ret;

	if (!compat) {
		return -ENOSYS;
	}

	ret = sensor_get_decoder(dev, &decoder);
	if (ret) {
		return -ENOTSUP;
	}

	ret = decoder->decode(compat->buf, ch, &fit, 1, decode_buf);
	if (ret < 0) {
		return ret;
	}

	/* Handle the various potential data types based off the requested channel */
	if (chan == SENSOR_CHAN_PROX) {
		struct sensor_byte_data *data = (struct sensor_byte_data *)decode_buf;

		val->val2 = 0;
		val->val1 = data->readings[0].is_near;
	} else if (chan == SENSOR_CHAN_GAUGE_CYCLE_COUNT) {
		struct sensor_uint64_data *data = (struct sensor_uint64_data *)decode_buf;

		val->val2 = 0;
		val->val1 = data->readings[0].value;
	} else if (chan == SENSOR_CHAN_GAME_ROTATION_VECTOR) {
		struct sensor_game_rotation_vector_data *data =
			(struct sensor_game_rotation_vector_data *)decode_buf;

		sensor_value_from_float(&val[0],
					Z_SHIFT_Q31_TO_F32(data->readings[0].x, data->shift));
		sensor_value_from_float(&val[1],
					Z_SHIFT_Q31_TO_F32(data->readings[0].y, data->shift));
		sensor_value_from_float(&val[2],
					Z_SHIFT_Q31_TO_F32(data->readings[0].z, data->shift));
		sensor_value_from_float(&val[3],
					Z_SHIFT_Q31_TO_F32(data->readings[0].w, data->shift));
	} else if (SENSOR_CHANNEL_3_AXIS(chan)) {
		struct sensor_three_axis_data *data = (struct sensor_three_axis_data *)decode_buf;

		sensor_value_from_float(&val[0],
					Z_SHIFT_Q31_TO_F32(data->readings[0].x, data->shift));
		sensor_value_from_float(&val[1],
					Z_SHIFT_Q31_TO_F32(data->readings[0].y, data->shift));
		sensor_value_from_float(&val[2],
					Z_SHIFT_Q31_TO_F32(data->readings[0].z, data->shift));
	} else {
		struct sensor_q31_data *data = (struct sensor_q31_data *)decode_buf;

		sensor_value_from_float(val,
					Z_SHIFT_Q31_TO_F32(data->readings[0].value, data->shift));
	}

	return 0;
}
