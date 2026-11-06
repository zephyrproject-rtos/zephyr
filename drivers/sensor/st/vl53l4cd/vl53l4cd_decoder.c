/*
 * Copyright (c) 2026 Mahendra Sondagar <mahendra@dotcom.co.in>
 * Copyright (c) 2026 Darshan Maru <darshan.maru@dnkmail.in>
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * DT_DRV_COMPAT is required here: SENSOR_DECODER_API_DT_DEFINE() and
 * SENSOR_DECODER_NAME() are built from it.
 */
#define DT_DRV_COMPAT st_vl53l4cd

#include <errno.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/vl53l4cd.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "vl53l4cd.h"

struct vl53l4cd_sample {
	uint32_t signal_rate_kcps;
	uint32_t ambient_rate_kcps;
	uint32_t signal_per_spad_kcps;
	uint32_t ambient_per_spad_kcps;
	uint16_t range_mm;
	uint16_t sigma_mm;
	uint8_t range_status;
	uint16_t spad_count;
};

static void vl53l4cd_unpack(const struct vl53l4cd_encoded_data *edata, struct vl53l4cd_sample *s)
{
	/* Maps the raw hardware range status (5 bits) to the ST ULD status code */
	static const uint8_t status_rtn[24] = {255, 255, 255, 5,   2,   4,   1,   7,
					       3,   0,   255, 255, 9,   13,  255, 255,
					       255, 255, 10,  6,   255, 255, 11,  12};
	const uint8_t *r = edata->result;
	uint8_t status = r[0] & 0x1F; /* 0x89 */

	if (status < ARRAY_SIZE(status_rtn)) {
		status = status_rtn[status];
	} else {
		status = VL53L4CD_RANGE_STATUS_UNKNOWN;
	}

	s->range_status = status;
	s->spad_count = sys_get_be16(&r[3]);
	s->signal_rate_kcps = (uint32_t)sys_get_be16(&r[5]) * 8U;  /* 0x8E */
	s->ambient_rate_kcps = (uint32_t)sys_get_be16(&r[7]) * 8U; /* 0x90 */
	s->sigma_mm = sys_get_be16(&r[9]) / 4U;                    /* 0x92 */
	s->range_mm = sys_get_be16(&r[13]);                        /* 0x96 */

	/* Per-SPAD rates; avoid division by zero when no SPADs are reported */
	if (s->spad_count != 0) {
		s->signal_per_spad_kcps = s->signal_rate_kcps / s->spad_count;
		s->ambient_per_spad_kcps = s->ambient_rate_kcps / s->spad_count;
	} else {
		s->signal_per_spad_kcps = 0;
		s->ambient_per_spad_kcps = 0;
	}
}

static bool vl53l4cd_chan_supported(int chan_type)
{
	switch (chan_type) {
	case SENSOR_CHAN_DISTANCE:
	case SENSOR_CHAN_VL53L4CD_RANGE_STATUS:
	case SENSOR_CHAN_VL53L4CD_SPAD_COUNT:
	case SENSOR_CHAN_VL53L4CD_SIGNAL_RATE:
	case SENSOR_CHAN_VL53L4CD_AMBIENT_RATE:
	case SENSOR_CHAN_VL53L4CD_SIGNAL_PER_SPAD:
	case SENSOR_CHAN_VL53L4CD_AMBIENT_PER_SPAD:
	case SENSOR_CHAN_VL53L4CD_SIGMA:
		return true;
	default:
		return false;
	}
}

static q31_t vl53l4cd_int_to_q31(uint32_t value)
{
	return (q31_t)((int64_t)value << (31 - VL53L4CD_RAW_Q31_SHIFT));
}

static q31_t vl53l4cd_mm_to_q31(uint16_t mm)
{
	int64_t scaled = (int64_t)mm * (1LL << (31 - VL53L4CD_DISTANCE_Q31_SHIFT));

	return (q31_t)((scaled + 500) / 1000);
}

static int vl53l4cd_decoder_get_frame_count(const uint8_t *buffer,
					    struct sensor_chan_spec chan_spec,
					    uint16_t *frame_count)
{
	ARG_UNUSED(buffer);

	if (chan_spec.chan_idx != 0 || !vl53l4cd_chan_supported(chan_spec.chan_type)) {
		return -ENOTSUP;
	}

	*frame_count = 1;
	return 0;
}

static int vl53l4cd_decoder_get_size_info(struct sensor_chan_spec chan_spec, size_t *base_size,
					  size_t *frame_size)
{
	if (!vl53l4cd_chan_supported(chan_spec.chan_type)) {
		return -ENOTSUP;
	}

	*base_size = sizeof(struct sensor_q31_data);
	*frame_size = sizeof(struct sensor_q31_sample_data);
	return 0;
}

static int vl53l4cd_decoder_decode(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
				   uint32_t *fit, uint16_t max_count, void *data_out)
{
	const struct vl53l4cd_encoded_data *edata = (const struct vl53l4cd_encoded_data *)buffer;
	struct sensor_q31_data *out = data_out;
	struct vl53l4cd_sample sample;

	if (*fit != 0) {
		return 0;
	}

	if (max_count == 0 || chan_spec.chan_idx != 0) {
		return -EINVAL;
	}

	vl53l4cd_unpack(edata, &sample);

	out->header.base_timestamp_ns = edata->timestamp_ns;
	out->header.reading_count = 1;
	out->readings[0].timestamp_delta = 0;

	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_DISTANCE:
		out->shift = VL53L4CD_DISTANCE_Q31_SHIFT;
		out->readings[0].value = vl53l4cd_mm_to_q31(sample.range_mm);
		break;
	case SENSOR_CHAN_VL53L4CD_RANGE_STATUS:
		out->shift = VL53L4CD_RAW_Q31_SHIFT;
		out->readings[0].value = vl53l4cd_int_to_q31(sample.range_status);
		break;
	case SENSOR_CHAN_VL53L4CD_SPAD_COUNT:
		out->shift = VL53L4CD_RAW_Q31_SHIFT;
		out->readings[0].value = vl53l4cd_int_to_q31(sample.spad_count);
		break;
	case SENSOR_CHAN_VL53L4CD_SIGNAL_RATE:
		out->shift = VL53L4CD_RAW_Q31_SHIFT;
		out->readings[0].value = vl53l4cd_int_to_q31(sample.signal_rate_kcps);
		break;
	case SENSOR_CHAN_VL53L4CD_AMBIENT_RATE:
		out->shift = VL53L4CD_RAW_Q31_SHIFT;
		out->readings[0].value = vl53l4cd_int_to_q31(sample.ambient_rate_kcps);
		break;
	case SENSOR_CHAN_VL53L4CD_SIGNAL_PER_SPAD:
		out->shift = VL53L4CD_RAW_Q31_SHIFT;
		out->readings[0].value = vl53l4cd_int_to_q31(sample.signal_per_spad_kcps);
		break;
	case SENSOR_CHAN_VL53L4CD_AMBIENT_PER_SPAD:
		out->shift = VL53L4CD_RAW_Q31_SHIFT;
		out->readings[0].value = vl53l4cd_int_to_q31(sample.ambient_per_spad_kcps);
		break;
	case SENSOR_CHAN_VL53L4CD_SIGMA:
		out->shift = VL53L4CD_RAW_Q31_SHIFT;
		out->readings[0].value = vl53l4cd_int_to_q31(sample.sigma_mm);
		break;
	default:
		return -ENOTSUP;
	}

	*fit = 1;
	return 1;
}

static bool vl53l4cd_decoder_has_trigger(const uint8_t *buffer, enum sensor_trigger_type trigger)
{
	ARG_UNUSED(buffer);
	ARG_UNUSED(trigger);

	return false;
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = vl53l4cd_decoder_get_frame_count,
	.get_size_info = vl53l4cd_decoder_get_size_info,
	.decode = vl53l4cd_decoder_decode,
	.has_trigger = vl53l4cd_decoder_has_trigger,
};

int vl53l4cd_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);

	*decoder = &SENSOR_DECODER_NAME();
	return 0;
}
