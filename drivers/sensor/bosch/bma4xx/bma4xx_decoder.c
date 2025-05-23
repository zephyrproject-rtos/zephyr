/*
 * Copyright (c) 2024 Cienet
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bosch_bma4xx

#include "bma4xx_decoder.h"
#include "bma4xx_defs.h"
#include "bma4xx.h"
#include <errno.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(bma4xx, CONFIG_SENSOR_LOG_LEVEL);

#define IS_ACCEL(chan)                                                                             \
	((chan) == SENSOR_CHAN_ACCEL_X || (chan) == SENSOR_CHAN_ACCEL_Y ||                         \
	 (chan) == SENSOR_CHAN_ACCEL_Z || (chan) == SENSOR_CHAN_ACCEL_XYZ)

#ifdef CONFIG_BMA4XX_STREAM

static uint64_t accel_period_ns[] = {
	[BMA4XX_ODR_0_78125] = UINT64_C(100000000000000) / 78125,
	[BMA4XX_ODR_1_5625] = UINT64_C(10000000000000) / 15625,
	[BMA4XX_ODR_3_125] = UINT64_C(10000000000000) / 31250,
	[BMA4XX_ODR_6_25] = UINT64_C(10000000000000) / 62500,
	[BMA4XX_ODR_12_5] = UINT64_C(1000000000000) / 12500,
	[BMA4XX_ODR_25] = UINT64_C(1000000000) / 25,
	[BMA4XX_ODR_50] = UINT64_C(1000000000) / 50,
	[BMA4XX_ODR_100] = UINT64_C(1000000000) / 100,
	[BMA4XX_ODR_200] = UINT64_C(1000000000) / 200,
	[BMA4XX_ODR_400] = UINT64_C(1000000000) / 400,
	[BMA4XX_ODR_800] = UINT64_C(1000000000) / 800,
	[BMA4XX_ODR_1600] = UINT64_C(10000000) / 16,
	[BMA4XX_ODR_3200] = UINT64_C(10000000) / 32,
	[BMA4XX_ODR_6400] = UINT64_C(10000000) / 64,
	[BMA4XX_ODR_12800] = UINT64_C(10000000) / 128,
};

#endif /* CONFIG_BMA4XX_STREAM */

/*
 * RTIO decoder
 */

static int bma4xx_decoder_get_frame_count(const uint8_t *buffer, struct sensor_chan_spec ch,
					  uint16_t *frame_count)
{
	const struct bma4xx_fifo_data *edata = (const struct bma4xx_fifo_data *)buffer;
	const struct bma4xx_decoder_header *header = &edata->header;

	if (ch.chan_idx != 0) {
		return -ENOTSUP;
	}

	if (!header->is_fifo) {
		switch (ch.chan_type) {
		case SENSOR_CHAN_ACCEL_X:
		case SENSOR_CHAN_ACCEL_Y:
		case SENSOR_CHAN_ACCEL_Z:
		case SENSOR_CHAN_ACCEL_XYZ:
		case SENSOR_CHAN_DIE_TEMP:
			*frame_count = 1;
			return 0;
		default:
			return -ENOTSUP;
		}
	}

	if (!IS_ACCEL(ch.chan_type)) {
		return -ENOTSUP;
	}
	/* Skip the header */
	buffer += sizeof(struct bma4xx_fifo_data);

	uint16_t count = 0;
	const uint8_t *end = buffer + edata->fifo_count;

	while (buffer < end) {
		uint8_t size = BMA4XX_FIFO_HEADER_LENGTH;
		const bool has_accel = FIELD_GET(BMA4XX_BIT_FIFO_HEADER_ACCEL, buffer[0]) == 1;
		const bool has_aux = FIELD_GET(BMA4XX_BIT_FIFO_HEADER_AUX, buffer[0]) == 1;

		if (FIELD_GET(BMA4XX_BIT_FIFO_HEADER_REGULAR, buffer[0])) {
			if (has_accel && has_aux) {
				size += BMA4XX_FIFO_MA_LENGTH;
			} else if (has_accel) {
				size += BMA4XX_FIFO_A_LENGTH;
			} else if (has_aux) {
				size += BMA4XX_FIFO_M_LENGTH;
			}
			++count;
		} else if (FIELD_GET(BMA4XX_BIT_FIFO_HEADER_CONTROL, buffer[0])) {
			if (FIELD_GET(BMA4XX_BIT_FIFO_HEADER_SENSORTIME, buffer[0])) {
				size += BMA4XX_FIFO_ST_LENGTH;
			} else if (FIELD_GET(BMA4XX_BIT_FIFO_HEAD_OVER_READ_MSB, buffer[0])) {
				size = *end;
			} else {
				size += BMA4XX_FIFO_CF_LENGTH;
			}
		}

		buffer += size;
	}

	*frame_count = count;

	return 0;
}

static int bma4xx_decoder_get_size_info(struct sensor_chan_spec ch, size_t *base_size,
					size_t *frame_size)
{
	switch (ch.chan_type) {
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

static int bma4xx_get_shift(struct sensor_chan_spec ch, uint8_t accel_fs, int8_t *shift)
{
	switch (ch.chan_type) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		switch (accel_fs) {
		case BMA4XX_RANGE_2G:
			/* 2 G's = 19.62 m/s^2. Use shift of 5 (+/-32) */
			*shift = 5;
			return 0;
		case BMA4XX_RANGE_4G:
			*shift = 6;
			return 0;
		case BMA4XX_RANGE_8G:
			*shift = 7;
			return 0;
		case BMA4XX_RANGE_16G:
			*shift = 8;
			return 0;
		default:
			return -EINVAL;
		}
	case SENSOR_CHAN_DIE_TEMP:
		*shift = BMA4XX_TEMP_SHIFT;
		return 0;
	default:
		return -EINVAL;
	}
}

static void bma4xx_convert_raw_accel_to_q31(int16_t raw_val, q31_t *out)
{
	/* The full calculation is (assuming floating math):
	 *   value_ms2 = raw_value * range * 9.80665 / BIT(11)
	 * We can treat 'range * 9.80665' as a scale, the scale is calculated by first getting 1g
	 * represented as a q31 value with the same shift as our result:
	 *   1g = (9.80665 * BIT(31)) >> shift
	 * Next, we need to multiply it by our range in g, which for this driver is one of
	 * [2, 4, 8, 16] and maps to a left shift of [1, 2, 3, 4]:
	 *   1g <<= log2(range)
	 * Note we used a right shift by 'shift' and left shift by log2(range). 'shift' is
	 * [5, 6, 7, 8] for range values [2, 4, 8, 16] since it's the final shift in m/s2. It is
	 * calculated via:
	 *   shift = ceil(log2(range * 9.80665))
	 * This means that we can shorten the above 1g alterations to:
	 *   1g = (1g >> ceil(log2(range * 9.80665))) << log2(range)
	 * For the range values [2, 4, 8, 16], the following is true:
	 *   (x >> ceil(log2(range * 9.80665))) << log2(range)
	 *   = x >> 4
	 * Since the range cancels out in the right and left shift, we've now reduced the following:
	 *   range * 9.80665 = 9.80665 * BIT(31 - 4)
	 * All that's left is to divide by the bma4xx's maximum range BIT(11).
	 */

	int16_t value = (int16_t)sys_le16_to_cpu(raw_val << 4) >> 4;

	const int64_t scale = (int64_t)(9.80665 * BIT64(31 - 4));

	*out = CLAMP(((int64_t)value * scale) >> 11, INT32_MIN, INT32_MAX);
}

#ifdef CONFIG_BMA4XX_STREAM

static void bma4xx_unpack_accel_data(const uint8_t *pkt, uint8_t data_start_index, q31_t *out)
{
	uint8_t offset = BMA4XX_FIFO_HEADER_LENGTH + (data_start_index * 2);
	const bool has_aux = FIELD_GET(BMA4XX_BIT_FIFO_HEADER_AUX, pkt[0]) == 1;

	if (has_aux) {
		offset += BMA4XX_FIFO_M_LENGTH;
	}

	int16_t value = (pkt[offset + 1] << 4) | FIELD_GET(GENMASK(7, 4), pkt[offset]);

	bma4xx_convert_raw_accel_to_q31((int16_t)value, out);
}

#endif /* CONFIG_BMA4XX_STREAM */

#ifdef CONFIG_BMA4XX_TEMPERATURE
/**
 * @brief Convert the 8-bit temp register value into a Q31 celsius value
 */
static void bma4xx_convert_raw_temp_to_q31(int8_t raw_val, q31_t *out)
{
	/* Value of 0 equals 23 degrees C. Each bit count equals 1 degree C */

	int64_t intermediate =
		((int64_t)raw_val + 23) * ((int64_t)INT32_MAX + 1) / (1 << BMA4XX_TEMP_SHIFT);

	*out = CLAMP(intermediate, INT32_MIN, INT32_MAX);
}
#endif /* CONFIG_BMA4XX_TEMPERATURE */

static int bma4xx_one_shot_decode(const uint8_t *buffer, struct sensor_chan_spec ch, uint32_t *fit,
				  uint16_t max_count, void *data_out)
{
	const struct bma4xx_encoded_data *edata = (const struct bma4xx_encoded_data *)buffer;
	const struct bma4xx_decoder_header *header = &edata->header;
	int16_t raw_x, raw_y, raw_z;
	int rc;

	if (*fit != 0) {
		return 0;
	}
	if (max_count == 0 || ch.chan_idx != 0) {
		return -EINVAL;
	}

	switch (ch.chan_type) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ: {
		struct sensor_three_axis_data *out = (struct sensor_three_axis_data *)data_out;

		out->header.base_timestamp_ns = edata->header.timestamp;
		out->header.reading_count = 1;
		rc = bma4xx_get_shift((struct sensor_chan_spec){.chan_type = SENSOR_CHAN_ACCEL_XYZ,
								.chan_idx = 3},
				      header->accel_fs, &out->shift);
		if (rc != 0) {
			return -EINVAL;
		}

		raw_x = (((int16_t)edata->accel_xyz_raw_data[1]) << 4) |
			FIELD_GET(GENMASK(7, 4), edata->accel_xyz_raw_data[0]);
		raw_y = (((int16_t)edata->accel_xyz_raw_data[3]) << 4) |
			FIELD_GET(GENMASK(7, 4), edata->accel_xyz_raw_data[2]);
		raw_z = (((int16_t)edata->accel_xyz_raw_data[5]) << 4) |
			FIELD_GET(GENMASK(7, 4), edata->accel_xyz_raw_data[4]);

		bma4xx_convert_raw_accel_to_q31(raw_x, &out->readings[0].x);
		bma4xx_convert_raw_accel_to_q31(raw_y, &out->readings[0].y);
		bma4xx_convert_raw_accel_to_q31(raw_z, &out->readings[0].z);

		*fit = 1;
		return 1;
	}
#ifdef CONFIG_BMA4XX_TEMPERATURE
	case SENSOR_CHAN_DIE_TEMP: {
		struct sensor_q31_data *out = (struct sensor_q31_data *)data_out;

		out->header.base_timestamp_ns = edata->header.timestamp;
		out->header.reading_count = 1;
		rc = bma4xx_get_shift((struct sensor_chan_spec){.chan_type = SENSOR_CHAN_DIE_TEMP,
								.chan_idx = 12},
				      0, &out->shift);

		if (rc != 0) {
			return -EINVAL;
		}

		bma4xx_convert_raw_temp_to_q31(edata->temp, &out->readings[0].temperature);

		*fit = 1;
		return 1;
	}
#endif /* CONFIG_BMA4XX_TEMPERATURE */
	default:
		return -EINVAL;
	}
}

#ifdef CONFIG_BMA4XX_STREAM

static int bma4xx_fifo_decode(const uint8_t *buffer, struct sensor_chan_spec ch, uint32_t *fit,
			      uint16_t max_count, void *data_out)
{
	const struct bma4xx_fifo_data *edata = (const struct bma4xx_fifo_data *)buffer;
	const uint8_t *buffer_end = buffer + sizeof(struct bma4xx_fifo_data) + edata->fifo_count;
	int accel_frame_count = 0;
	int count = 0;

	if ((uintptr_t)buffer_end <= *fit || ch.chan_idx != 0) {
		return 0;
	}

	if (!IS_ACCEL(ch.chan_type)) {
		return -ENOTSUP;
	}

	((struct sensor_data_header *)data_out)->base_timestamp_ns = edata->header.timestamp;

	buffer += sizeof(struct bma4xx_fifo_data);

	while (count < max_count && buffer < buffer_end) {
		const bool has_accel = FIELD_GET(BMA4XX_BIT_FIFO_HEADER_ACCEL, buffer[0]) == 1;
		const bool has_aux = FIELD_GET(BMA4XX_BIT_FIFO_HEADER_AUX, buffer[0]) == 1;
		const uint8_t *frame_end = buffer;

		if (FIELD_GET(BMA4XX_BIT_FIFO_HEADER_REGULAR, buffer[0])) {
			if (has_accel && has_aux) {
				frame_end += BMA4XX_FIFO_MA_LENGTH + BMA4XX_FIFO_HEADER_LENGTH;
				accel_frame_count++;
			} else if (has_accel) {
				frame_end += BMA4XX_FIFO_A_LENGTH + BMA4XX_FIFO_HEADER_LENGTH;
				accel_frame_count++;
			} else if (has_aux) {
				frame_end += BMA4XX_FIFO_M_LENGTH + BMA4XX_FIFO_HEADER_LENGTH;
			}
		} else if (FIELD_GET(BMA4XX_BIT_FIFO_HEADER_CONTROL, buffer[0])) {
			if (FIELD_GET(BMA4XX_BIT_FIFO_HEADER_SENSORTIME, buffer[0])) {
				frame_end += BMA4XX_FIFO_ST_LENGTH + BMA4XX_FIFO_HEADER_LENGTH;
			} else if (FIELD_GET(BMA4XX_BIT_FIFO_HEAD_OVER_READ_MSB, buffer[0])) {
				frame_end = buffer_end;
			} else {
				frame_end += BMA4XX_FIFO_CF_LENGTH + BMA4XX_FIFO_HEADER_LENGTH;
			}
		}

		if ((uintptr_t)buffer < *fit) {
			/* This frame was already decoded, move on to the next frame */
			buffer = frame_end;
			continue;
		}

		if (has_accel) {
			struct sensor_three_axis_data *data =
				(struct sensor_three_axis_data *)data_out;

			uint64_t period_ns = accel_period_ns[edata->accel_odr];

			bma4xx_get_shift(
				(struct sensor_chan_spec){.chan_type = SENSOR_CHAN_ACCEL_XYZ,
							  .chan_idx = 3},
				edata->header.accel_fs, &data->shift);
			data->readings[count].timestamp_delta = (accel_frame_count - 1) * period_ns;
			bma4xx_unpack_accel_data(buffer, 0, &data->readings[count].x);
			bma4xx_unpack_accel_data(buffer, 1, &data->readings[count].y);
			bma4xx_unpack_accel_data(buffer, 2, &data->readings[count].z);
		}

		buffer = frame_end;
		*fit = (uintptr_t)frame_end;
		count++;
	}

	return count;
}

#endif /* CONFIG_BMA4XX_STREAM */

static int bma4xx_decoder_decode(const uint8_t *buffer, struct sensor_chan_spec ch, uint32_t *fit,
				 uint16_t max_count, void *data_out)
{
#ifdef CONFIG_BMA4XX_STREAM

	const struct bma4xx_decoder_header *header = (const struct bma4xx_decoder_header *)buffer;

	if (header->is_fifo) {
		return bma4xx_fifo_decode(buffer, ch, fit, max_count, data_out);
	}
#endif

	return bma4xx_one_shot_decode(buffer, ch, fit, max_count, data_out);
}

static bool bma4xx_decoder_has_trigger(const uint8_t *buffer, enum sensor_trigger_type trigger)
{
	return false;
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = bma4xx_decoder_get_frame_count,
	.get_size_info = bma4xx_decoder_get_size_info,
	.decode = bma4xx_decoder_decode,
	.has_trigger = bma4xx_decoder_has_trigger,
};

int bma4xx_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);
	*decoder = &SENSOR_DECODER_NAME();

	return 0;
}
