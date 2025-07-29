/* ST Microelectronics IIS3DWB accelerometer sensor
 *
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/iis3dwb.pdf
 */

#include "iis3dwb.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(IIS3DWB_DECODER, CONFIG_SENSOR_LOG_LEVEL);

/*
 * Expand val to q31_t according to its range; this is achieved multiplying by 2^31/2^range.
 */
#define Q31_SHIFT_VAL(val, range) \
	(q31_t) (roundf((val) * ((int64_t)1 << (31 - (range)))))

/*
 * Expand micro_val (a generic micro unit) to q31_t according to its range; this is achieved
 * multiplying by 2^31/2^range. Then transform it to val.
 */
#define Q31_SHIFT_MICROVAL(micro_val, range) \
	(q31_t) ((int64_t)(micro_val) * ((int64_t)1 << (31 - (range))) / 1000000LL)

/* bit range for Accelerometer for a given range value */
static const int8_t accel_range[] = {
	[IIS3DWB_DT_FS_2G] = 5,
	[IIS3DWB_DT_FS_4G] = 6,
	[IIS3DWB_DT_FS_8G] = 7,
	[IIS3DWB_DT_FS_16G] = 8,
};

#if defined(CONFIG_IIS3DWB_ENABLE_TEMP)
/* bit range for Temperature sensor */
static const int8_t temp_range = 9;

/* transform temperature LSB into micro-Celsius */
#define SENSOR_TEMP_UCELSIUS(t_lsb) \
	(int64_t) (25000000LL + (((int64_t)(t_lsb) * 1000000LL) / 355LL))

#endif

/* Calculate scaling factor to transform micro-g/LSB unit into micro-ms2/LSB */
#define SENSOR_SCALE_UG_TO_UMS2(ug_lsb)	\
	(int32_t)((ug_lsb) * SENSOR_G / 1000000LL)

/*
 * Accelerometer scaling factors table for a given range value
 * GAIN_UNIT_XL is expressed in ug/LSB.
 */
static const int32_t accel_scaler[] = {
	[IIS3DWB_DT_FS_2G] = SENSOR_SCALE_UG_TO_UMS2(GAIN_UNIT),
	[IIS3DWB_DT_FS_4G] = SENSOR_SCALE_UG_TO_UMS2(2 * GAIN_UNIT),
	[IIS3DWB_DT_FS_8G] = SENSOR_SCALE_UG_TO_UMS2(4 * GAIN_UNIT),
	[IIS3DWB_DT_FS_16G] = SENSOR_SCALE_UG_TO_UMS2(8 * GAIN_UNIT),
};

/* Calculate scaling factor to transform micro-dps/LSB unit into micro-rads/LSB */
#define SENSOR_SCALE_UDPS_TO_URADS(udps_lsb) \
	(int32_t)(((udps_lsb) * SENSOR_PI / 180LL) / 1000000LL)

static int iis3dwb_decoder_get_frame_count(const uint8_t *buffer,
					      struct sensor_chan_spec chan_spec,
					      uint16_t *frame_count)
{
	struct iis3dwb_fifo_data *data = (struct iis3dwb_fifo_data *)buffer;
	struct iis3dwb_rtio_data *rdata = (struct iis3dwb_rtio_data *)buffer;
	const struct iis3dwb_decoder_header *header = &data->header;

	if (chan_spec.chan_idx != 0) {
		return -ENOTSUP;
	}

	if (!header->is_fifo) {
		switch (chan_spec.chan_type) {
		case SENSOR_CHAN_ACCEL_X:
		case SENSOR_CHAN_ACCEL_Y:
		case SENSOR_CHAN_ACCEL_Z:
		case SENSOR_CHAN_ACCEL_XYZ:
			*frame_count = rdata->has_accel ? 1 : 0;
			return 0;

		case SENSOR_CHAN_DIE_TEMP:
			*frame_count = rdata->has_temp ? 1 : 0;
			return 0;

		default:
			*frame_count = 0;
			return -ENOTSUP;
		}

		return 0;
	}

	return 0;
}

static int iis3dwb_decode_sample(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
				    uint32_t *fit, uint16_t max_count, void *data_out)
{
	const struct iis3dwb_rtio_data *edata = (const struct iis3dwb_rtio_data *)buffer;
	const struct iis3dwb_decoder_header *header = &edata->header;

	if (*fit != 0) {
		return 0;
	}
	if (max_count == 0 || chan_spec.chan_idx != 0) {
		return -EINVAL;
	}

	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ: {
		const int32_t scale = accel_scaler[header->range];

		if (edata->has_accel == 0) {
			return -ENODATA;
		}

		struct sensor_three_axis_data *out = data_out;

		out->header.base_timestamp_ns = edata->header.timestamp;
		out->header.reading_count = 1;

		out->shift = accel_range[header->range];

		out->readings[0].x = Q31_SHIFT_MICROVAL(scale * edata->accel[0], out->shift);
		out->readings[0].y = Q31_SHIFT_MICROVAL(scale * edata->accel[1], out->shift);
		out->readings[0].z = Q31_SHIFT_MICROVAL(scale * edata->accel[2], out->shift);
		*fit = 1;
		return 1;
	}
#if defined(CONFIG_IIS3DWB_ENABLE_TEMP)
	case SENSOR_CHAN_DIE_TEMP: {
		int64_t t_uC;

		if (edata->has_temp == 0) {
			return -ENODATA;
		}

		struct sensor_q31_data *out = data_out;

		out->header.base_timestamp_ns = edata->header.timestamp;
		out->header.reading_count = 1;

		out->shift = temp_range;

		/* transform temperature LSB into micro-Celsius */
		t_uC = SENSOR_TEMP_UCELSIUS(edata->temp);

		out->readings[0].temperature = Q31_SHIFT_MICROVAL(t_uC, out->shift);
		*fit = 1;
		return 1;
	}
#endif
	default:
		return -EINVAL;
	}
}

static int iis3dwb_decoder_decode(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
				     uint32_t *fit, uint16_t max_count, void *data_out)
{
	return iis3dwb_decode_sample(buffer, chan_spec, fit, max_count, data_out);
}

static int iis3dwb_decoder_get_size_info(struct sensor_chan_spec chan_spec, size_t *base_size,
					    size_t *frame_size)
{
	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		*base_size = sizeof(struct sensor_three_axis_data);
		*frame_size = sizeof(struct sensor_three_axis_sample_data);
		return 0;
	case SENSOR_CHAN_DIE_TEMP:
		*base_size = sizeof(struct sensor_q31_data);
		*frame_size = sizeof(struct sensor_q31_sample_data);
		return 0;
	default:
		return -ENOTSUP;
	}
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = iis3dwb_decoder_get_frame_count,
	.get_size_info = iis3dwb_decoder_get_size_info,
	.decode = iis3dwb_decoder_decode,
};

int iis3dwb_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);
	*decoder = &SENSOR_DECODER_NAME();

	return 0;
}
