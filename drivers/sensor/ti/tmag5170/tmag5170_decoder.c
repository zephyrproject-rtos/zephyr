/*
 * Copyright (c) 2023 Michal Morsisko
 * Copyright (c) 2026 Swarovski Optik AG & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor_clock.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "tmag5170.h"
#include "tmag5170_decoder.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(TMAG5170_DECODER, CONFIG_SENSOR_LOG_LEVEL);

#define DT_DRV_COMPAT ti_tmag5170

uint8_t tmag5170_encode_channel(enum sensor_channel chan)
{
	switch (chan) {
	case SENSOR_CHAN_MAGN_X:
		return BIT(TMAG5170_RESULT_IDX_X);
	case SENSOR_CHAN_MAGN_Y:
		return BIT(TMAG5170_RESULT_IDX_Y);
	case SENSOR_CHAN_MAGN_Z:
		return BIT(TMAG5170_RESULT_IDX_Z);
	case SENSOR_CHAN_MAGN_XYZ:
		return BIT(TMAG5170_RESULT_IDX_X) | BIT(TMAG5170_RESULT_IDX_Y) |
		       BIT(TMAG5170_RESULT_IDX_Z);
	case SENSOR_CHAN_ROTATION:
		return BIT(TMAG5170_RESULT_IDX_ANGLE);
	case SENSOR_CHAN_AMBIENT_TEMP:
		return BIT(TMAG5170_RESULT_IDX_TEMP);
	default:
		return 0;
	}
}

int tmag5170_encode(const struct device *dev, const struct sensor_chan_spec *const channels,
		    const size_t num_channels, uint8_t *buf)
{
	struct tmag5170_encoded_data *edata = (struct tmag5170_encoded_data *)buf;
	const struct tmag5170_dev_config *cfg = dev->config;
	const struct tmag5170_data *data = dev->data;
	uint64_t cycles;
	int err;

	edata->header.channels = 0;

	for (size_t i = 0; i < num_channels; i++) {
		edata->header.channels |= tmag5170_encode_channel(channels[i].chan_type);
	}

	if (edata->header.channels == 0) {
		return -ENOTSUP;
	}

	err = sensor_clock_get_cycles(&cycles);
	if (err != 0) {
		return err;
	}

	edata->header.timestamp = sensor_clock_cycles_to_ns(cycles);
	edata->header.chip_revision = data->chip_revision;
	edata->header.x_range = cfg->x_range;
	edata->header.y_range = cfg->y_range;
	edata->header.z_range = cfg->z_range;

	return 0;
}

static inline uint16_t tmag5170_frame_reading(const uint8_t *frame)
{
	/* The CRC of the frame has already been verified by the completion
	 * callback of the submission, so only the payload is extracted here.
	 */
	return sys_get_be16(&frame[1]);
}

static int tmag5170_get_shift(const struct tmag5170_encoded_header *header,
			      enum sensor_channel chan, int8_t *shift)
{
	int8_t shift_x;
	int8_t shift_y;
	int8_t shift_z;
	int err;

	switch (chan) {
	case SENSOR_CHAN_MAGN_X:
		return tmag5170_magn_shift(header->x_range, header->chip_revision, shift);
	case SENSOR_CHAN_MAGN_Y:
		return tmag5170_magn_shift(header->y_range, header->chip_revision, shift);
	case SENSOR_CHAN_MAGN_Z:
		return tmag5170_magn_shift(header->z_range, header->chip_revision, shift);
	case SENSOR_CHAN_MAGN_XYZ:
		err = tmag5170_magn_shift(header->x_range, header->chip_revision, &shift_x);
		if (err == 0) {
			err = tmag5170_magn_shift(header->y_range, header->chip_revision, &shift_y);
		}
		if (err == 0) {
			err = tmag5170_magn_shift(header->z_range, header->chip_revision, &shift_z);
		}
		if (err != 0) {
			return err;
		}

		/* All three axes share a single shift within the frame */
		*shift = MAX(shift_x, MAX(shift_y, shift_z));
		return 0;
	case SENSOR_CHAN_ROTATION:
		*shift = TMAG5170_ROTATION_SHIFT;
		return 0;
	case SENSOR_CHAN_AMBIENT_TEMP:
		*shift = TMAG5170_TEMP_SHIFT;
		return 0;
	default:
		return -ENOTSUP;
	}
}

static void tmag5170_convert_micro_to_q31(int64_t micro, int8_t shift, q31_t *out)
{
	int64_t intermediate;

	if (shift < 0) {
		intermediate = micro * ((int64_t)INT32_MAX + 1) * (1 << -shift) / INT64_C(1000000);
	} else {
		intermediate = micro * ((int64_t)INT32_MAX + 1) / ((1 << shift) * INT64_C(1000000));
	}

	*out = CLAMP(intermediate, INT32_MIN, INT32_MAX);
}

static int tmag5170_decode_magn(const struct tmag5170_encoded_data *edata,
				enum tmag5170_result_idx idx, uint8_t chan_range, int8_t shift,
				q31_t *out)
{
	int64_t micro;
	int err;

	err = tmag5170_magn_reading_to_micro_gauss(
		tmag5170_frame_reading(edata->payload.frames[idx]), chan_range,
		edata->header.chip_revision, &micro);
	if (err != 0) {
		return err;
	}

	tmag5170_convert_micro_to_q31(micro, shift, out);

	return 0;
}

static int tmag5170_decoder_get_frame_count(const uint8_t *buffer,
					    struct sensor_chan_spec chan_spec,
					    uint16_t *frame_count)
{
	const struct tmag5170_encoded_data *edata = (const struct tmag5170_encoded_data *)buffer;
	uint8_t channel_request = tmag5170_encode_channel(chan_spec.chan_type);

	if (chan_spec.chan_idx != 0) {
		return -ENOTSUP;
	}

	if (channel_request == 0) {
		return -ENOTSUP;
	}

	if ((edata->header.channels & channel_request) != channel_request) {
		return -ENODATA;
	}

	*frame_count = 1;

	return 0;
}

static int tmag5170_decoder_get_size_info(struct sensor_chan_spec chan_spec, size_t *base_size,
					  size_t *frame_size)
{
	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_MAGN_XYZ:
		*base_size = sizeof(struct sensor_three_axis_data);
		*frame_size = sizeof(struct sensor_three_axis_sample_data);
		return 0;
	case SENSOR_CHAN_MAGN_X:
	case SENSOR_CHAN_MAGN_Y:
	case SENSOR_CHAN_MAGN_Z:
	case SENSOR_CHAN_ROTATION:
	case SENSOR_CHAN_AMBIENT_TEMP:
		*base_size = sizeof(struct sensor_q31_data);
		*frame_size = sizeof(struct sensor_q31_sample_data);
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int tmag5170_decoder_decode(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
				   uint32_t *fit, uint16_t max_count, void *data_out)
{
	const struct tmag5170_encoded_data *edata = (const struct tmag5170_encoded_data *)buffer;
	uint8_t channel_request = tmag5170_encode_channel(chan_spec.chan_type);
	int8_t shift;
	int64_t micro;
	int err;

	if (*fit != 0) {
		return 0;
	}

	if (max_count == 0 || chan_spec.chan_idx != 0) {
		return -EINVAL;
	}

	if (channel_request == 0) {
		return -ENOTSUP;
	}

	if ((edata->header.channels & channel_request) != channel_request) {
		return -ENODATA;
	}

	err = tmag5170_get_shift(&edata->header, chan_spec.chan_type, &shift);
	if (err != 0) {
		return -EINVAL;
	}

	if (chan_spec.chan_type == SENSOR_CHAN_MAGN_XYZ) {
		struct sensor_three_axis_data *out = data_out;

		out->header.base_timestamp_ns = edata->header.timestamp;
		out->header.reading_count = 1;
		out->shift = shift;

		err = tmag5170_decode_magn(edata, TMAG5170_RESULT_IDX_X, edata->header.x_range,
					   shift, &out->readings[0].x);
		if (err == 0) {
			err = tmag5170_decode_magn(edata, TMAG5170_RESULT_IDX_Y,
						   edata->header.y_range, shift,
						   &out->readings[0].y);
		}
		if (err == 0) {
			err = tmag5170_decode_magn(edata, TMAG5170_RESULT_IDX_Z,
						   edata->header.z_range, shift,
						   &out->readings[0].z);
		}
		if (err != 0) {
			return err;
		}
	} else {
		struct sensor_q31_data *out = data_out;

		out->header.base_timestamp_ns = edata->header.timestamp;
		out->header.reading_count = 1;
		out->shift = shift;

		switch (chan_spec.chan_type) {
		case SENSOR_CHAN_MAGN_X:
			err = tmag5170_decode_magn(edata, TMAG5170_RESULT_IDX_X,
						   edata->header.x_range, shift,
						   &out->readings[0].value);
			break;
		case SENSOR_CHAN_MAGN_Y:
			err = tmag5170_decode_magn(edata, TMAG5170_RESULT_IDX_Y,
						   edata->header.y_range, shift,
						   &out->readings[0].value);
			break;
		case SENSOR_CHAN_MAGN_Z:
			err = tmag5170_decode_magn(edata, TMAG5170_RESULT_IDX_Z,
						   edata->header.z_range, shift,
						   &out->readings[0].value);
			break;
		case SENSOR_CHAN_ROTATION:
			tmag5170_angle_reading_to_micro_degrees(
				tmag5170_frame_reading(
					edata->payload.frames[TMAG5170_RESULT_IDX_ANGLE]),
				&micro);
			tmag5170_convert_micro_to_q31(micro, shift, &out->readings[0].value);
			err = 0;
			break;
		case SENSOR_CHAN_AMBIENT_TEMP:
			tmag5170_temp_reading_to_micro_celsius(
				tmag5170_frame_reading(
					edata->payload.frames[TMAG5170_RESULT_IDX_TEMP]),
				&micro);
			tmag5170_convert_micro_to_q31(micro, shift, &out->readings[0].value);
			err = 0;
			break;
		default:
			err = -ENOTSUP;
			break;
		}

		if (err != 0) {
			return err;
		}
	}

	*fit = 1;

	return 1;
}

static bool tmag5170_decoder_has_trigger(const uint8_t *buffer, enum sensor_trigger_type trigger)
{
	ARG_UNUSED(buffer);
	ARG_UNUSED(trigger);

	/* TODO: Report SENSOR_TRIG_DATA_READY once streaming (based on the
	 * optional int-gpios property) has been implemented.
	 */
	return false;
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = tmag5170_decoder_get_frame_count,
	.get_size_info = tmag5170_decoder_get_size_info,
	.decode = tmag5170_decoder_decode,
	.has_trigger = tmag5170_decoder_has_trigger,
};

int tmag5170_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);
	*decoder = &SENSOR_DECODER_NAME();

	return 0;
}
