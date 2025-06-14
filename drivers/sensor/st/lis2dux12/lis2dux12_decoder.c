/* ST Microelectronics LIS2DUX12 6-axis IMU sensor driver
 *
 * Copyright (c) 2023 Google LLC
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "lis2dux12.h"
#include "lis2dux12_decoder.h"
#include <zephyr/dt-bindings/sensor/lis2dux12.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LIS2DUX12_DECODER, CONFIG_SENSOR_LOG_LEVEL);

#ifdef CONFIG_LIS2DUX12_STREAM
static const uint8_t accel_divisor[] = {
	[LIS2DUX12_DT_BDR_XL_ODR] = 1,
	[LIS2DUX12_DT_BDR_XL_ODR_DIV_2] = 2,
	[LIS2DUX12_DT_BDR_XL_ODR_DIV_4] = 4,
	[LIS2DUX12_DT_BDR_XL_ODR_DIV_8] = 8,
	[LIS2DUX12_DT_BDR_XL_ODR_DIV_16] = 16,
	[LIS2DUX12_DT_BDR_XL_ODR_DIV_32] = 32,
	[LIS2DUX12_DT_BDR_XL_ODR_DIV_64] = 64,
	[LIS2DUX12_DT_BDR_XL_ODR_OFF] = 0,
};

static uint32_t accel_period_ns(uint8_t odr, uint8_t scaler)
{
	switch (odr) {
	default:
	case LIS2DUX12_DT_ODR_OFF:
		return UINT32_C(0);

	case LIS2DUX12_DT_ODR_1Hz_ULP:
		return ((UINT32_C(1000000000000) / 1000) * accel_divisor[scaler]);

	case LIS2DUX12_DT_ODR_3Hz_ULP:
		return ((UINT32_C(1000000000000) / 3000) * accel_divisor[scaler]);

	case LIS2DUX12_DT_ODR_25Hz_ULP:
		return ((UINT32_C(1000000000000) / 25000) * accel_divisor[scaler]);

	case LIS2DUX12_DT_ODR_6Hz:
		return ((UINT32_C(1000000000000) / 6000) * accel_divisor[scaler]);

	case LIS2DUX12_DT_ODR_12Hz5:
		return ((UINT32_C(1000000000000) / 12500) * accel_divisor[scaler]);

	case LIS2DUX12_DT_ODR_25Hz:
		return ((UINT32_C(1000000000000) / 25000) * accel_divisor[scaler]);

	case LIS2DUX12_DT_ODR_50Hz:
		return ((UINT32_C(1000000000000) / 50000) * accel_divisor[scaler]);

	case LIS2DUX12_DT_ODR_100Hz:
		return ((UINT32_C(1000000000000) / 100000) * accel_divisor[scaler]);

	case LIS2DUX12_DT_ODR_200Hz:
		return ((UINT32_C(1000000000000) / 200000) * accel_divisor[scaler]);

	case LIS2DUX12_DT_ODR_400Hz:
		return ((UINT32_C(1000000000000) / 400000) * accel_divisor[scaler]);

	case LIS2DUX12_DT_ODR_800Hz:
		return ((UINT32_C(1000000000000) / 800000) * accel_divisor[scaler]);

	}

	return 0;
}
#endif /* CONFIG_LIS2DUX12_STREAM */

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
	[LIS2DUX12_DT_FS_2G] = 5,
	[LIS2DUX12_DT_FS_4G] = 6,
	[LIS2DUX12_DT_FS_8G] = 7,
	[LIS2DUX12_DT_FS_16G] = 8,
};

#if defined(CONFIG_LIS2DUX12_ENABLE_TEMP)
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
	/* LIS2DUX12_DT_FS_2G */
	SENSOR_SCALE_UG_TO_UMS2(GAIN_UNIT),
	/* LIS2DUX12_DT_FS_4G / LSM6DSV32X_DT_FS_4G */
	SENSOR_SCALE_UG_TO_UMS2(2 * GAIN_UNIT),
	/* LIS2DUX12_DT_FS_8G / LSM6DSV32X_DT_FS_8G*/
	SENSOR_SCALE_UG_TO_UMS2(4 * GAIN_UNIT),
	/* LIS2DUX12_DT_FS_16G / LSM6DSV32X_DT_FS_16G */
	SENSOR_SCALE_UG_TO_UMS2(8 * GAIN_UNIT),
	/* LSM6DSV32X_DT_FS_32G */
	SENSOR_SCALE_UG_TO_UMS2(16 * GAIN_UNIT),
};

/* Calculate scaling factor to transform micro-dps/LSB unit into micro-rads/LSB */
#define SENSOR_SCALE_UDPS_TO_URADS(udps_lsb) \
	(int32_t)(((udps_lsb) * SENSOR_PI / 180LL) / 1000000LL)

static int lis2dux12_decoder_get_frame_count(const uint8_t *buffer,
					      struct sensor_chan_spec chan_spec,
					      uint16_t *frame_count)
{
	struct lis2dux12_fifo_data *data = (struct lis2dux12_fifo_data *)buffer;
	struct lis2dux12_rtio_data *rdata = (struct lis2dux12_rtio_data *)buffer;
	const struct lis2dux12_decoder_header *header = &data->header;

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

#ifdef CONFIG_LIS2DUX12_STREAM
	const struct lis2dux12_fifo_data *edata = (const struct lis2dux12_fifo_data *)buffer;
	const uint8_t *buffer_end;
	uint8_t fifo_tag;
	uint8_t tot_accel_fifo_words = 0;
	uint8_t tot_ts_fifo_words = 0;

	buffer += sizeof(struct lis2dux12_fifo_data);
	buffer_end = buffer + LIS2DUX12_FIFO_SIZE(edata->fifo_count);

	/* count total FIFO word for each tag */
	while (buffer < buffer_end) {
		fifo_tag = (buffer[0] >> 3);

		switch (fifo_tag) {
		case LIS2DUXXX_XL_ONLY_2X_TAG:
			tot_accel_fifo_words += 2;
			break;
		case LIS2DUXXX_XL_TEMP_TAG:
			tot_accel_fifo_words++;
			break;
		case LIS2DUXXX_TIMESTAMP_TAG:
			tot_ts_fifo_words++;
			break;
		default:
			break;
		}

		buffer += LIS2DUX12_FIFO_ITEM_LEN;
	}

	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		*frame_count = tot_accel_fifo_words;
		break;

#if defined(CONFIG_LIS2DUX12_ENABLE_TEMP)
	case SENSOR_CHAN_DIE_TEMP:
		*frame_count = (data->fifo_mode_sel == 0) ? tot_accel_fifo_words : 0;
		break;
#endif

	default:
		*frame_count = 0;
		break;
	}
#endif

	return 0;
}

#ifdef CONFIG_LIS2DUX12_STREAM
static int lis2dux12_decode_fifo(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
				  uint32_t *fit, uint16_t max_count, void *data_out)
{
	const struct lis2dux12_fifo_data *edata = (const struct lis2dux12_fifo_data *)buffer;
	const uint8_t *buffer_end;
	const struct lis2dux12_decoder_header *header = &edata->header;
	int count = 0;
	uint8_t fifo_tag;
	uint16_t xl_count = 0;
	uint16_t tot_fifo_samples = 0;
	int ret;

	/* count total FIFO word for each tag */
	ret = lis2dux12_decoder_get_frame_count(buffer, chan_spec, &tot_fifo_samples);
	if (ret < 0) {
		return 0;
	}

	buffer += sizeof(struct lis2dux12_fifo_data);
	buffer_end = buffer + LIS2DUX12_FIFO_SIZE(edata->fifo_count);

	/*
	 * Timestamp in header is set when FIFO threshold is reached, so
	 * set time baseline going back in past according to total number
	 * of FIFO word for each type.
	 */
	if (SENSOR_CHANNEL_IS_ACCEL(chan_spec.chan_type)) {
		((struct sensor_data_header *)data_out)->base_timestamp_ns =
			edata->header.timestamp -
			(tot_fifo_samples - 1) *
				accel_period_ns(edata->accel_odr, edata->accel_batch_odr);
#if defined(CONFIG_LIS2DUX12_ENABLE_TEMP)
	} else if (chan_spec.chan_type == SENSOR_CHAN_DIE_TEMP) {
		((struct sensor_data_header *)data_out)->base_timestamp_ns =
			edata->header.timestamp -
			(tot_fifo_samples - 1) *
				accel_period_ns(edata->accel_odr, edata->accel_batch_odr);
#endif
	}

	while (count < max_count && buffer < buffer_end) {
		const uint8_t *frame_end = buffer;
		uint8_t skip_frame;

		skip_frame = 0;
		frame_end += LIS2DUX12_FIFO_ITEM_LEN;

		fifo_tag = (buffer[0] >> 3);

		switch (fifo_tag) {
		case LIS2DUXXX_XL_ONLY_2X_TAG: {
			struct sensor_three_axis_data *out = data_out;
			int16_t x, y, z;
			const int32_t scale = accel_scaler[header->range];

			xl_count += 2;
			if ((uintptr_t)buffer < *fit) {
				/* This frame was already decoded, move on to the next frame */
				buffer = frame_end;
				continue;
			}

			if (!SENSOR_CHANNEL_IS_ACCEL(chan_spec.chan_type)) {
				buffer = frame_end;
				continue;
			}

			out->shift = accel_range[header->range];

			out->readings[count].timestamp_delta =
				(xl_count - 2) * accel_period_ns(edata->accel_odr,
								 edata->accel_batch_odr);

			x = *(int16_t *)&buffer[0];
			y = *(int16_t *)&buffer[1];
			z = *(int16_t *)&buffer[2];

			out->readings[count].x = Q31_SHIFT_MICROVAL(scale * x, out->shift);
			out->readings[count].y = Q31_SHIFT_MICROVAL(scale * y, out->shift);
			out->readings[count].z = Q31_SHIFT_MICROVAL(scale * z, out->shift);
			count++;

			out->readings[count].timestamp_delta =
				(xl_count - 1) * accel_period_ns(edata->accel_odr,
								 edata->accel_batch_odr);

			x = *(int16_t *)&buffer[3];
			y = *(int16_t *)&buffer[4];
			z = *(int16_t *)&buffer[5];
			out->readings[count].x = Q31_SHIFT_MICROVAL(scale * x, out->shift);
			out->readings[count].y = Q31_SHIFT_MICROVAL(scale * y, out->shift);
			out->readings[count].z = Q31_SHIFT_MICROVAL(scale * z, out->shift);
			break;
		}

		case LIS2DUXXX_XL_TEMP_TAG: {
			struct sensor_three_axis_data *out = data_out;
			int16_t x, y, z;
#if defined(CONFIG_LIS2DUX12_ENABLE_TEMP)
			struct sensor_q31_data *t_out = data_out;
			int16_t  t;
			int64_t t_uC;
#endif
			const int32_t scale = accel_scaler[header->range];

			xl_count++;
			if ((uintptr_t)buffer < *fit) {
				/* This frame was already decoded, move on to the next frame */
				buffer = frame_end;
				continue;
			}

			if (edata->fifo_mode_sel == 1) {
				out->readings[count].timestamp_delta =
					(xl_count - 1) * accel_period_ns(edata->accel_odr,
									edata->accel_batch_odr);

				if (!SENSOR_CHANNEL_IS_ACCEL(chan_spec.chan_type)) {
					buffer = frame_end;
					continue;
				}

				x = (int16_t)buffer[1] + (int16_t)buffer[2] * 256;
				y = (int16_t)buffer[3] + (int16_t)buffer[4] * 256;
				z = (int16_t)buffer[5] + (int16_t)buffer[6] * 256;

				out->shift = accel_range[header->range];

				out->readings[count].x = Q31_SHIFT_MICROVAL(scale * x, out->shift);
				out->readings[count].y = Q31_SHIFT_MICROVAL(scale * y, out->shift);
				out->readings[count].z = Q31_SHIFT_MICROVAL(scale * z, out->shift);
			} else {
				if (!SENSOR_CHANNEL_IS_ACCEL(chan_spec.chan_type)
#if defined(CONFIG_LIS2DUX12_ENABLE_TEMP)
				   && (chan_spec.chan_type != SENSOR_CHAN_DIE_TEMP)
#endif
				   ) {
					buffer = frame_end;
					continue;
				}

				if (SENSOR_CHANNEL_IS_ACCEL(chan_spec.chan_type)) {
					out->readings[count].timestamp_delta =
						(xl_count - 1) * accel_period_ns(edata->accel_odr,
									edata->accel_batch_odr);

					x = (int16_t)buffer[1];
					x = (x + (int16_t)buffer[2] * 256) * 16;
					y = (int16_t)buffer[2] / 16;
					y = (y + ((int16_t)buffer[3] * 16)) * 16;
					z = (int16_t)buffer[4];
					z = (z + (int16_t)buffer[5] * 256) * 16;

					out->shift = accel_range[header->range];

					out->readings[count].x = Q31_SHIFT_MICROVAL(scale * x,
										    out->shift);
					out->readings[count].y = Q31_SHIFT_MICROVAL(scale * y,
										    out->shift);
					out->readings[count].z = Q31_SHIFT_MICROVAL(scale * z,
										    out->shift);
#if defined(CONFIG_LIS2DUX12_ENABLE_TEMP)
				} else {
					t_out->readings[count].timestamp_delta =
						(xl_count - 1) * accel_period_ns(edata->accel_odr,
									edata->accel_batch_odr);

					t = ((int16_t)buffer[5] / 16 +
					     (int16_t)buffer[6] * 16) * 16;

					t_out->shift = temp_range;

					/* transform temperature LSB into micro-Celsius */
					t_uC = SENSOR_TEMP_UCELSIUS(t);
					t_out->readings[count].temperature =
							Q31_SHIFT_MICROVAL(t_uC, t_out->shift);

#endif /* CONFIG_LIS2DUX12_ENABLE_TEMP */
				}
			}
			break;
		}

		default:
			/* skip unhandled FIFO tag */
			buffer = frame_end;
			LOG_DBG("unknown FIFO tag %02x", fifo_tag);
			continue;
		}

		buffer = frame_end;
		*fit = (uintptr_t)frame_end;
		count++;
	}

	return count;
}
#endif /* CONFIG_LIS2DUX12_STREAM */

static int lis2dux12_decode_sample(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
				    uint32_t *fit, uint16_t max_count, void *data_out)
{
	const struct lis2dux12_rtio_data *edata = (const struct lis2dux12_rtio_data *)buffer;
	const struct lis2dux12_decoder_header *header = &edata->header;

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

		out->readings[0].x = Q31_SHIFT_MICROVAL(scale * edata->acc[0], out->shift);
		out->readings[0].y = Q31_SHIFT_MICROVAL(scale * edata->acc[1], out->shift);
		out->readings[0].z = Q31_SHIFT_MICROVAL(scale * edata->acc[2], out->shift);
		*fit = 1;
		return 1;
	}
#if defined(CONFIG_LIS2DUX12_ENABLE_TEMP)
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

static int lis2dux12_decoder_decode(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
				     uint32_t *fit, uint16_t max_count, void *data_out)
{
#ifdef CONFIG_LIS2DUX12_STREAM
	const struct lis2dux12_decoder_header *header =
		(const struct lis2dux12_decoder_header *)buffer;

	if (header->is_fifo) {
		return lis2dux12_decode_fifo(buffer, chan_spec, fit, max_count, data_out);
	}
#endif

	return lis2dux12_decode_sample(buffer, chan_spec, fit, max_count, data_out);
}

static int lis2dux12_decoder_get_size_info(struct sensor_chan_spec chan_spec, size_t *base_size,
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

static bool lis2dux12_decoder_has_trigger(const uint8_t *buffer, enum sensor_trigger_type trigger)
{
#ifdef CONFIG_LIS2DUX12_STREAM
	const struct lis2dux12_decoder_header *header =
		(const struct lis2dux12_decoder_header *)buffer;

	switch (trigger) {
	case SENSOR_TRIG_DATA_READY:
		return header->int_status & 0x01;
	case SENSOR_TRIG_FIFO_WATERMARK:
		return header->int_status & 0x80;
	case SENSOR_TRIG_FIFO_FULL:
		return header->int_status & 0x40;
	default:
		return false;
	}
#endif

	return false;
}

#define DT_DRV_COMPAT st_lis2dux12

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = lis2dux12_decoder_get_frame_count,
	.get_size_info = lis2dux12_decoder_get_size_info,
	.decode = lis2dux12_decoder_decode,
	.has_trigger = lis2dux12_decoder_has_trigger,
};

int lis2dux12_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);
	*decoder = &SENSOR_DECODER_NAME();

	return 0;
}
