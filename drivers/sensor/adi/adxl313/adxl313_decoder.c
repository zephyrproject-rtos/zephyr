/*
 * Copyright (c) 2025 Lothar Rubusch <l.rubusch@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "adxl313.h"

/*
 * The q-scale factor will always be the same, as the nominal LSB/g
 * changes at the same rate the selected shift parameter per range:
 *
 * - At 0.5G: 256 LSB/g, 10-bits resolution.
 * - At 1g: 128 LSB/g, 10-bits resolution.
 * - At 2g: 64 LSB/g, 10-bits resolution.
 * - At 4g 32 LSB/g, 10-bits resolution.
 */
static const uint32_t qscale_factor_no_full_res[] = {
	[ADXL313_RANGE_0_5G] = UINT32_C(2570754),
	[ADXL313_RANGE_1G] = UINT32_C(2570754),
	[ADXL313_RANGE_2G] = UINT32_C(2570754),
	[ADXL313_RANGE_4G] = UINT32_C(2570754),
};

/*
 * Sensitivities based on Range:
 *
 * - At 0.5G: 256 LSB/g, 10-bits resolution.
 * - At 1g: 256 LSB/g, 11-bits resolution.
 * - At 2g: 256 LSB/g, 12-bits resolution.
 * - At 4g 256 LSB/g, 13-bits resolution.
 */
static const uint32_t qscale_factor_full_res[] = {
	[ADXL313_RANGE_0_5G] = UINT32_C(2570754),
	[ADXL313_RANGE_1G] = UINT32_C(1285377),
	[ADXL313_RANGE_2G] = UINT32_C(642688),
	[ADXL313_RANGE_4G] = UINT32_C(321344),
};

void adxl313_accel_convert_q31(q31_t *out, int16_t sample, enum adxl313_range range,
			       bool is_full_res)
{
	if (is_full_res) {
		switch (range) {
		case ADXL313_RANGE_0_5G:
			if (sample & BIT(9)) {
				sample |= ADXL313_COMPLEMENT_MASK(10);
			}
			break;
		case ADXL313_RANGE_1G:
			if (sample & BIT(10)) {
				sample |= ADXL313_COMPLEMENT_MASK(11);
			}
			break;
		case ADXL313_RANGE_2G:
			if (sample & BIT(11)) {
				sample |= ADXL313_COMPLEMENT_MASK(12);
			}
			break;
		case ADXL313_RANGE_4G:
			if (sample & BIT(12)) {
				sample |= ADXL313_COMPLEMENT_MASK(13);
			}
			break;
		}
		*out = sample * qscale_factor_full_res[range];
	} else {
		if (sample & BIT(9)) {
			sample |= ADXL313_COMPLEMENT;
		}
		*out = sample * qscale_factor_no_full_res[range];
	}
}

static int adxl313_decoder_get_frame_count(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
					   uint16_t *frame_count)
{
	int32_t ret = -ENOTSUP;

	if (chan_spec.chan_idx != 0) {
		return ret;
	}

	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		*frame_count = 1;
		ret = 0;
		break;

	default:
		break;
	}

	return ret;
}

static int adxl313_decode_sample(const struct adxl313_xyz_accel_data *data,
				 struct sensor_chan_spec chan_spec, uint32_t *fit,
				 uint16_t max_count, void *data_out)
{
	struct sensor_three_axis_data *out = (struct sensor_three_axis_data *)data_out;
	enum adxl313_range selected_range = data->selected_range;

	memset(out, 0, sizeof(*out));
	out->header.base_timestamp_ns = k_ticks_to_ns_floor64(k_uptime_ticks());
	out->header.reading_count = 1;
	out->shift = range_to_shift[selected_range];

	if (*fit > 0) {
		return -ENOTSUP;
	}

	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_ACCEL_XYZ:
		adxl313_accel_convert_q31(&out->readings->x, data->x, selected_range,
					  data->is_full_res);
		adxl313_accel_convert_q31(&out->readings->y, data->y, selected_range,
					  data->is_full_res);
		adxl313_accel_convert_q31(&out->readings->z, data->z, selected_range,
					  data->is_full_res);
		break;
	default:
		return -ENOTSUP;
	}

	*fit = 1;

	return 1;
}

static int adxl313_decoder_decode(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
				  uint32_t *fit, uint16_t max_count, void *data_out)
{
	const struct adxl313_xyz_accel_data *data = (const struct adxl313_xyz_accel_data *)buffer;

	return adxl313_decode_sample(data, chan_spec, fit, max_count, data_out);
}

static bool adxl313_decoder_has_trigger(const uint8_t *buffer, enum sensor_trigger_type trigger)
{
	const struct adxl313_fifo_data *data = (const struct adxl313_fifo_data *)buffer;

	switch (trigger) {
	case SENSOR_TRIG_DATA_READY:
		return FIELD_GET(ADXL313_INT_DATA_RDY, data->int_status);
	case SENSOR_TRIG_FIFO_WATERMARK:
		return FIELD_GET(ADXL313_INT_WATERMARK, data->int_status);
	case SENSOR_TRIG_FIFO_FULL:
		return FIELD_GET(ADXL313_INT_OVERRUN, data->int_status);
	default:
		return false;
	}
}

static int adxl313_get_size_info(struct sensor_chan_spec channel, size_t *base_size,
				 size_t *frame_size)
{
	__ASSERT_NO_MSG(base_size != NULL);
	__ASSERT_NO_MSG(frame_size != NULL);

	if (channel.chan_type >= SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	switch (channel.chan_type) {
	case SENSOR_CHAN_ACCEL_XYZ:
		*base_size = sizeof(struct sensor_three_axis_data);
		*frame_size = sizeof(struct sensor_three_axis_sample_data);
		return 0;
	default:
		break;
	}

	return -ENOTSUP;
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = adxl313_decoder_get_frame_count,
	.decode = adxl313_decoder_decode,
	.has_trigger = adxl313_decoder_has_trigger,
	.get_size_info = adxl313_get_size_info,
};

int adxl313_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);
	*decoder = &SENSOR_DECODER_NAME();

	return 0;
}
