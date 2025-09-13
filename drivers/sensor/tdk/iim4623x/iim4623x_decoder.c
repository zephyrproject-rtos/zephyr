/*
 * Copyright (c) 2025 Sentry Technologies ApS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "iim4623x.h"
#include "iim4623x_reg.h"
#include "iim4623x_decoder.h"

#include <zephyr/dsp/utils.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor_clock.h>

LOG_MODULE_REGISTER(iim4623x_decoder, CONFIG_SENSOR_LOG_LEVEL);

#define DT_DRV_COMPAT invensense_iim4623x

static union iim4623x_encoded_channels iim4623x_encode_channel(enum sensor_channel chan)
{
	union iim4623x_encoded_channels enc_chan = {.msk = 0};

	switch (chan) {
	case SENSOR_CHAN_DIE_TEMP:
		enc_chan.temp = 1;
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
		enc_chan.accel = 1;
		break;
	case SENSOR_CHAN_GYRO_XYZ:
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
		enc_chan.gyro = 1;
		break;
	default:
		break;
	};

	return enc_chan;
}

static int iim4623x_decoder_get_frame_count(const uint8_t *buffer,
					    struct sensor_chan_spec chan_spec,
					    uint16_t *frame_count)
{
	struct iim4623x_encoded_data *edata = (struct iim4623x_encoded_data *)buffer;
	union iim4623x_encoded_channels chan_req = iim4623x_encode_channel(chan_spec.chan_type);

	if (chan_spec.chan_idx != 0) {
		return -ENOTSUP;
	}

	if (!(edata->header.chans.msk & chan_req.msk)) {
		return -ENODATA;
	}

	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_DIE_TEMP:
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
	case SENSOR_CHAN_GYRO_XYZ:
		*frame_count = 1;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int iim4623x_decoder_get_size_info(struct sensor_chan_spec chan_spec, size_t *base_size,
					  size_t *frame_size)
{
	if (chan_spec.chan_idx != 0) {
		return -ENOTSUP;
	}

	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_DIE_TEMP:
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
		*base_size = sizeof(struct sensor_q31_data);
		*frame_size = sizeof(struct sensor_q31_sample_data);
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
	case SENSOR_CHAN_GYRO_XYZ:
		*base_size = sizeof(struct sensor_three_axis_data);
		*frame_size = sizeof(struct sensor_three_axis_sample_data);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static uint8_t iim4623x_gyro_shift(uint8_t gyro_fs)
{
	/* Prefer supporting the full-scale range over the greatest precision */
	switch (gyro_fs) {
	case IIM4623X_GYRO_CFG_FS_2000:
		return 11;
	case IIM4623X_GYRO_CFG_FS_1000:
		return 10;
	case IIM4623X_GYRO_CFG_FS_480:
		return 9;
	case IIM4623X_GYRO_CFG_FS_250:
		return 8;
	default:
		break;
	}

	/* Default to greatest precision, should never be reached */
	return 8;
}

static int iim4623x_decode_chan(const struct iim4623x_encoded_data *edata, enum sensor_channel chan,
				struct sensor_q31_data *out)
{
	struct sensor_three_axis_sample_data *tri_axis = (void *)&out->readings[0];
	struct sensor_q31_sample_data *smpl_data = &out->readings[0];

	switch (chan) {
	case SENSOR_CHAN_DIE_TEMP:
		/*
		 * Use shift=7 to gain an effective range of +/- 128 degrees with a resolution of
		 * < 0.001 degrees.
		 * The datasheet specifies a resolution of 126.8 LSB per degree when using the fixed
		 * point output format. Assuming the same resolution applies to the floating point
		 * format, the sensor can only produce a resolution of ~0.0079 degrees.
		 */
		out->shift = 7;
		smpl_data->value = Z_SHIFT_F32_TO_Q31(edata->payload.temp.val, out->shift);
		break;
	case SENSOR_CHAN_ACCEL_X:
		/* Allow representing +/- 16g */
		out->shift = 4;
		smpl_data->value = Z_SHIFT_F32_TO_Q31(edata->payload.accel.x, out->shift);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		out->shift = 4;
		smpl_data->value = Z_SHIFT_F32_TO_Q31(edata->payload.accel.y, out->shift);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		out->shift = 4;
		smpl_data->value = Z_SHIFT_F32_TO_Q31(edata->payload.accel.z, out->shift);
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
		out->shift = 4;
		tri_axis->x = Z_SHIFT_F32_TO_Q31(edata->payload.accel.x, out->shift);
		tri_axis->y = Z_SHIFT_F32_TO_Q31(edata->payload.accel.y, out->shift);
		tri_axis->z = Z_SHIFT_F32_TO_Q31(edata->payload.accel.z, out->shift);
		break;
	case SENSOR_CHAN_GYRO_X:
		out->shift = iim4623x_gyro_shift(edata->header.gyro_fs);
		smpl_data->value = Z_SHIFT_F32_TO_Q31(edata->payload.gyro.x, out->shift);
		break;
	case SENSOR_CHAN_GYRO_Y:
		out->shift = iim4623x_gyro_shift(edata->header.gyro_fs);
		smpl_data->value = Z_SHIFT_F32_TO_Q31(edata->payload.gyro.y, out->shift);
		break;
	case SENSOR_CHAN_GYRO_Z:
		out->shift = iim4623x_gyro_shift(edata->header.gyro_fs);
		smpl_data->value = Z_SHIFT_F32_TO_Q31(edata->payload.gyro.z, out->shift);
		break;
	case SENSOR_CHAN_GYRO_XYZ:
		out->shift = iim4623x_gyro_shift(edata->header.gyro_fs);
		tri_axis->x = Z_SHIFT_F32_TO_Q31(edata->payload.gyro.x, out->shift);
		tri_axis->y = Z_SHIFT_F32_TO_Q31(edata->payload.gyro.y, out->shift);
		tri_axis->z = Z_SHIFT_F32_TO_Q31(edata->payload.gyro.z, out->shift);
		break;
	default:
		return -EINVAL;
	};

	return 0;
}

static int iim4623x_decoder_decode(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
				   uint32_t *fit, uint16_t max_count, void *data_out)
{
	struct iim4623x_encoded_data *edata = (struct iim4623x_encoded_data *)buffer;
	union iim4623x_encoded_channels chan_req = iim4623x_encode_channel(chan_spec.chan_type);
	struct sensor_q31_data *out = data_out;

	if (chan_spec.chan_idx != 0) {
		return -ENOTSUP;
	}

	if (max_count == 0 || *fit != 0) {
		return -EINVAL;
	}

	if (!(chan_req.msk & edata->header.chans.msk)) {
		return -ENODATA;
	}

	/* Convert timestamp from us to ns */
	out->header.base_timestamp_ns = edata->header.timestamp;
	/**
	 * TODO: it should be possible to support more readings, but the internal FIFO of the
	 * iim4623x seems a bit rough to work with for this purpose. For now just support a single
	 * reading.
	 */
	out->header.reading_count = 1;

	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
	case SENSOR_CHAN_DIE_TEMP:
	case SENSOR_CHAN_ACCEL_XYZ:
	case SENSOR_CHAN_GYRO_XYZ:
		iim4623x_decode_chan(edata, chan_spec.chan_type, out);
		break;
	default:
		return -EINVAL;
	};

	*fit = 1;
	return 1;
}

static bool iim4623x_decoder_has_trigger(const uint8_t *buffer, enum sensor_trigger_type trigger)
{
	struct iim4623x_encoded_data *edata = (struct iim4623x_encoded_data *)buffer;

	switch (trigger) {
	case SENSOR_TRIG_DATA_READY:
		return edata->header.data_ready;
	default:
		break;
	}

	return false;
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = iim4623x_decoder_get_frame_count,
	.get_size_info = iim4623x_decoder_get_size_info,
	.decode = iim4623x_decoder_decode,
	.has_trigger = iim4623x_decoder_has_trigger,
};

int iim4623x_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);
	*decoder = &SENSOR_DECODER_NAME();

	return 0;
}

int iim4623x_encode(const struct device *dev, struct iim4623x_encoded_data *edata)
{
	const struct iim4623x_data *data = dev->data;
	uint64_t cycles;
	int ret;

	edata->header = data->edata.header;

	ret = sensor_clock_get_cycles(&cycles);
	if (ret) {
		LOG_ERR_RATELIMIT("Failed getting sensor clock cycles, ret: %d", ret);
		return ret;
	}

	/**
	 * TODO: the sensor includes a micro second timestamp, if it can be converted to "system
	 * time" then this would be more accurate and the header field could be dropped
	 */
	edata->header.timestamp = sensor_clock_cycles_to_ns(cycles);

	return 0;
}
