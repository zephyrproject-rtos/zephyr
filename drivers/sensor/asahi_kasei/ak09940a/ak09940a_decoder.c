/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT asahi_kasei_ak09940a

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor_data_types.h>

#include "ak09940a.h"

/* Magnetic field range is +/-13.1 G */
#define AK09940A_MAGN_SHIFT 4
/* Temperature range is -44.7 to 105.3 degrees Celsius */
#define AK09940A_TEMP_SHIFT 7

static q31_t ak09940a_micro_to_q31(int64_t micro, int8_t shift)
{
	return (q31_t)((micro * (INT64_C(1) << (31 - shift))) / 1000000);
}

static q31_t ak09940a_magn_q31(const struct ak09940a_encoded_data *edata, uint8_t axis)
{
	int64_t micro_gauss =
		(int64_t)ak09940a_frame_magn(edata->frame, axis) * AK09940A_MICRO_GAUSS_PER_LSB;

	return ak09940a_micro_to_q31(micro_gauss, AK09940A_MAGN_SHIFT);
}

static int ak09940a_decoder_get_frame_count(const uint8_t *buffer,
					    struct sensor_chan_spec chan_spec,
					    uint16_t *frame_count)
{
	ARG_UNUSED(buffer);

	if (chan_spec.chan_idx != 0) {
		return -ENOTSUP;
	}

	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_MAGN_X:
	case SENSOR_CHAN_MAGN_Y:
	case SENSOR_CHAN_MAGN_Z:
	case SENSOR_CHAN_MAGN_XYZ:
	case SENSOR_CHAN_DIE_TEMP:
		*frame_count = 1;
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int ak09940a_decoder_get_size_info(struct sensor_chan_spec chan_spec, size_t *base_size,
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
	case SENSOR_CHAN_DIE_TEMP:
		*base_size = sizeof(struct sensor_q31_data);
		*frame_size = sizeof(struct sensor_q31_sample_data);
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int ak09940a_decoder_decode(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
				   uint32_t *fit, uint16_t max_count, void *data_out)
{
	const struct ak09940a_encoded_data *edata = (const struct ak09940a_encoded_data *)buffer;

	if (*fit != 0) {
		return 0;
	}

	if ((max_count == 0) || (chan_spec.chan_idx != 0)) {
		return -EINVAL;
	}

	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_MAGN_X:
	case SENSOR_CHAN_MAGN_Y:
	case SENSOR_CHAN_MAGN_Z: {
		struct sensor_q31_data *out = data_out;

		out->header.base_timestamp_ns = edata->timestamp;
		out->header.reading_count = 1;
		out->shift = AK09940A_MAGN_SHIFT;
		out->readings[0].timestamp_delta = 0;
		out->readings[0].value =
			ak09940a_magn_q31(edata, chan_spec.chan_type - SENSOR_CHAN_MAGN_X);
		break;
	}
	case SENSOR_CHAN_MAGN_XYZ: {
		struct sensor_three_axis_data *out = data_out;

		out->header.base_timestamp_ns = edata->timestamp;
		out->header.reading_count = 1;
		out->shift = AK09940A_MAGN_SHIFT;
		out->readings[0].timestamp_delta = 0;
		out->readings[0].x = ak09940a_magn_q31(edata, 0);
		out->readings[0].y = ak09940a_magn_q31(edata, 1);
		out->readings[0].z = ak09940a_magn_q31(edata, 2);
		break;
	}
	case SENSOR_CHAN_DIE_TEMP: {
		struct sensor_q31_data *out = data_out;

		out->header.base_timestamp_ns = edata->timestamp;
		out->header.reading_count = 1;
		out->shift = AK09940A_TEMP_SHIFT;
		out->readings[0].timestamp_delta = 0;
		out->readings[0].temperature = ak09940a_micro_to_q31(
			ak09940a_frame_temp_micro(edata->frame), AK09940A_TEMP_SHIFT);
		break;
	}
	default:
		return -EINVAL;
	}

	*fit = 1;

	return 1;
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = ak09940a_decoder_get_frame_count,
	.get_size_info = ak09940a_decoder_get_size_info,
	.decode = ak09940a_decoder_decode,
};

int ak09940a_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);

	*decoder = &SENSOR_DECODER_NAME();

	return 0;
}
