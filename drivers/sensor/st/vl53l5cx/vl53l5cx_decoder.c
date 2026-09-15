/*
 * Copyright (c) 2026 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vl53l5cx.h"
#include "vl53l5cx_decoder.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(vl53l5cx_decoder, CONFIG_SENSOR_LOG_LEVEL);

static int vl53l5cx_decoder_get_frame_count(const uint8_t *buffer,
					    struct sensor_chan_spec chan_spec,
					    uint16_t *frame_count)
{
	const struct vl53l5cx_rtio_data *data = (const struct vl53l5cx_rtio_data *)buffer;

	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_DIE_TEMP:
		if (chan_spec.chan_idx != 0) {
			return -ENOTSUP;
		}
		break;
	case SENSOR_CHAN_DISTANCE:
		if (chan_spec.chan_idx >= data->distance_idx_num) {
			return -ENOTSUP;
		}
		break;
	default:
		return -ENOTSUP;
	}

	*frame_count = 1;

	return 0;
}

static int32_t mm_to_m_q29(uint32_t mm)
{
	return (int32_t)(((((int64_t)mm) << 29) + 500) / 1000);
}

static int vl53l5cx_decode_sample(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
				  uint32_t *fit, uint16_t max_count, void *data_out)
{
	const struct vl53l5cx_rtio_data *edata = (const struct vl53l5cx_rtio_data *)buffer;
	struct sensor_q31_data *out = data_out;

	if (*fit != 0) {
		return 0;
	}
	if (max_count == 0) {
		return -EINVAL;
	}

	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_DIE_TEMP:
		if (chan_spec.chan_idx != 0) {
			return -EINVAL;
		}

		out->header.base_timestamp_ns = edata->timestamp;
		out->header.reading_count = 1;

		out->shift = 15;

		out->readings[0].temperature = edata->data.silicon_temp_degc << 16;
		break;
	case SENSOR_CHAN_DISTANCE:
		if (chan_spec.chan_idx >= edata->distance_idx_num) {
			return -EINVAL;
		}

		out->header.base_timestamp_ns = edata->timestamp;
		out->header.reading_count = 1;

		out->shift = 2;

		out->readings[0].distance =
			mm_to_m_q29(edata->data.distance_mm[chan_spec.chan_idx]);
		break;
	default:
		return -EINVAL;
	}

	*fit = 1;
	return 1;
}

static int vl53l5cx_decoder_decode(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
				   uint32_t *fit, uint16_t max_count, void *data_out)
{
	return vl53l5cx_decode_sample(buffer, chan_spec, fit, max_count, data_out);
}

static int vl53l5cx_decoder_get_size_info(struct sensor_chan_spec chan_spec, size_t *base_size,
					  size_t *frame_size)
{
	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_DIE_TEMP:
	case SENSOR_CHAN_DISTANCE:
		*base_size = sizeof(struct sensor_q31_data);
		*frame_size = sizeof(struct sensor_q31_sample_data);
		break;
	default:
		return -ENOTSUP;
	}
	return 0;
}

static bool vl53l5cx_decoder_has_trigger(const uint8_t *buffer, enum sensor_trigger_type trigger)
{
	return false;
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = vl53l5cx_decoder_get_frame_count,
	.get_size_info = vl53l5cx_decoder_get_size_info,
	.decode = vl53l5cx_decoder_decode,
	.has_trigger = vl53l5cx_decoder_has_trigger,
};

int vl53l5cx_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);
	*decoder = &SENSOR_DECODER_NAME();

	return 0;
}
