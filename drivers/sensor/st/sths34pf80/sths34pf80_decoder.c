/*
 * Copyright (c) 2026 Mahendra Sondagar <mahendra@dotcom.co.in>
 * Copyright (c) 2026 Parin Baudhanwala <parin.baudhanwala@dnkmail.in>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Sensor decoder of the STHS34PF80 driver.
 *
 * Converts the raw frame produced by the submit() function into q31 values
 * for the sensor Read & Decode API.
 */

#define DT_DRV_COMPAT st_sths34pf80

#include <errno.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/sths34pf80.h>
#include <zephyr/sys/byteorder.h>
#include "sths34pf80_decoder.h"

#define STHS34PF80_PRES_FLAG_BIT       2
#define STHS34PF80_MOT_FLAG_BIT        1
#define STHS34PF80_TAMB_SHOCK_FLAG_BIT 0

/*
 * Every value is reported as a q31 number: real_value = q31 * 2^shift / 2^31.
 * "lsb" is the number of raw LSB per unit (degree C for temperatures).
 */
static const struct {
	int32_t lsb;
	int8_t shift;
} chan_desc[STHS34PF80_IDX_COUNT] = {
	[STHS34PF80_IDX_OBJECT] = {2000, 5}, /* +-16.4 degC fits in +-32 */
	[STHS34PF80_IDX_AMBIENT] = {100, 9}, /* +-327 degC fits in +-512 */
	[STHS34PF80_IDX_OBJECT_COMP] = {2000, 5},
	[STHS34PF80_IDX_PRESENCE] = {1, 15}, /* raw int16 counts */
	[STHS34PF80_IDX_MOTION] = {1, 15},
	[STHS34PF80_IDX_TEMP_SHOCK] = {1, 15},
	[STHS34PF80_IDX_PRESENCE_FLAG] = {1, 1}, /* 0 or 1 */
	[STHS34PF80_IDX_MOTION_FLAG] = {1, 1},
	[STHS34PF80_IDX_TEMP_SHOCK_FLAG] = {1, 1},
};

int sths34pf80_chan_to_idx(enum sensor_channel chan)
{
	switch ((int)chan) {
	case SENSOR_CHAN_STHS34PF80_OBJECT_TEMP:
		return STHS34PF80_IDX_OBJECT;
	case SENSOR_CHAN_AMBIENT_TEMP:
		return STHS34PF80_IDX_AMBIENT;
	case SENSOR_CHAN_STHS34PF80_OBJECT_TEMP_COMP:
		return STHS34PF80_IDX_OBJECT_COMP;
	case SENSOR_CHAN_STHS34PF80_PRESENCE:
		return STHS34PF80_IDX_PRESENCE;
	case SENSOR_CHAN_STHS34PF80_MOTION:
		return STHS34PF80_IDX_MOTION;
	case SENSOR_CHAN_STHS34PF80_TEMP_SHOCK:
		return STHS34PF80_IDX_TEMP_SHOCK;
	case SENSOR_CHAN_STHS34PF80_PRESENCE_FLAG:
		return STHS34PF80_IDX_PRESENCE_FLAG;
	case SENSOR_CHAN_STHS34PF80_MOTION_FLAG:
		return STHS34PF80_IDX_MOTION_FLAG;
	case SENSOR_CHAN_STHS34PF80_TEMP_SHOCK_FLAG:
		return STHS34PF80_IDX_TEMP_SHOCK_FLAG;
	default:
		return -ENOTSUP;
	}
}

/**
 * @brief Extract one raw value from the bytes read from the sensor.
 *
 * @param edata Raw frame.
 * @param idx   Value to extract, an enum sths34pf80_idx entry.
 *
 * @return The raw register value, or the flag (0 or 1).
 */
static int16_t sths34pf80_raw_get(const struct sths34pf80_rtio_data *edata, int idx)
{
	const uint8_t func_status = edata->block1[0];

	switch (idx) {
	case STHS34PF80_IDX_OBJECT:
		return (int16_t)sys_get_le16(&edata->block1[1]);
	case STHS34PF80_IDX_AMBIENT:
		return (int16_t)sys_get_le16(&edata->block1[3]);
	case STHS34PF80_IDX_OBJECT_COMP:
		return (int16_t)sys_get_le16(&edata->block2[0]);
	case STHS34PF80_IDX_PRESENCE:
		return (int16_t)sys_get_le16(&edata->block2[2]);
	case STHS34PF80_IDX_MOTION:
		return (int16_t)sys_get_le16(&edata->block2[4]);
	case STHS34PF80_IDX_TEMP_SHOCK:
		return (int16_t)sys_get_le16(&edata->block2[6]);
	case STHS34PF80_IDX_PRESENCE_FLAG:
		return (func_status >> STHS34PF80_PRES_FLAG_BIT) & 1;
	case STHS34PF80_IDX_MOTION_FLAG:
		return (func_status >> STHS34PF80_MOT_FLAG_BIT) & 1;
	case STHS34PF80_IDX_TEMP_SHOCK_FLAG:
	default:
		return (func_status >> STHS34PF80_TAMB_SHOCK_FLAG_BIT) & 1;
	}
}

static int sths34pf80_decoder_get_frame_count(const uint8_t *buffer,
					      struct sensor_chan_spec chan_spec,
					      uint16_t *frame_count)
{
	const struct sths34pf80_rtio_data *edata = (const struct sths34pf80_rtio_data *)buffer;
	int idx;

	if (chan_spec.chan_idx != 0) {
		return -EINVAL;
	}

	idx = sths34pf80_chan_to_idx(chan_spec.chan_type);
	if (idx < 0) {
		return -EINVAL;
	}

	*frame_count = (edata->valid & BIT(idx)) ? 1 : 0;

	return 0;
}

static int sths34pf80_decoder_get_size_info(struct sensor_chan_spec chan_spec, size_t *base_size,
					    size_t *frame_size)
{
	if (sths34pf80_chan_to_idx(chan_spec.chan_type) < 0) {
		return -ENOTSUP;
	}

	*base_size = sizeof(struct sensor_q31_data);
	*frame_size = sizeof(struct sensor_q31_sample_data);

	return 0;
}

static int sths34pf80_decoder_decode(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
				     uint32_t *fit, uint16_t max_count, void *data_out)
{
	const struct sths34pf80_rtio_data *edata = (const struct sths34pf80_rtio_data *)buffer;
	struct sensor_q31_data *out = data_out;
	int idx;

	if (*fit != 0) {
		return 0;
	}

	if (max_count == 0 || chan_spec.chan_idx != 0) {
		return -EINVAL;
	}

	idx = sths34pf80_chan_to_idx(chan_spec.chan_type);
	if (idx < 0) {
		return -EINVAL;
	}

	if ((edata->valid & BIT(idx)) == 0) {
		return -ENODATA;
	}

	out->header.base_timestamp_ns = edata->timestamp;
	out->header.reading_count = 1;
	out->shift = chan_desc[idx].shift;
	out->readings[0].timestamp_delta = 0;
	out->readings[0].value = (q31_t)(((int64_t)sths34pf80_raw_get(edata, idx) *
					  ((int64_t)1 << (31 - chan_desc[idx].shift))) /
					 chan_desc[idx].lsb);

	*fit = 1;

	return 1;
}

static bool sths34pf80_decoder_has_trigger(const uint8_t *buffer, enum sensor_trigger_type trigger)
{
	ARG_UNUSED(buffer);
	ARG_UNUSED(trigger);

	return false;
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = sths34pf80_decoder_get_frame_count,
	.get_size_info = sths34pf80_decoder_get_size_info,
	.decode = sths34pf80_decoder_decode,
	.has_trigger = sths34pf80_decoder_has_trigger,
};

int sths34pf80_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);

	*decoder = &SENSOR_DECODER_NAME();

	return 0;
}
