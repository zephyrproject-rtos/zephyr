/*
 * Bosch BMA400 3-axis accelerometer driver
 * SPDX-FileCopyrightText: Copyright 2026 Luca Gessi lucagessi90@gmail.com
 * SPDX-FileCopyrightText: Copyright 2026 The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bosch_bma400

#include "bma400_decoder.h"
#include "bma400_defs.h"
#include "bma400.h"
#include <errno.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(bma400, CONFIG_SENSOR_LOG_LEVEL);

#define IS_ACCEL(chan)                                                                             \
	((chan) == SENSOR_CHAN_ACCEL_X || (chan) == SENSOR_CHAN_ACCEL_Y ||                         \
	 (chan) == SENSOR_CHAN_ACCEL_Z || (chan) == SENSOR_CHAN_ACCEL_XYZ)

#ifdef CONFIG_BMA400_STREAM

static const uint64_t accel_period_ns[] = {[BMA400_ODR_12_5HZ] = UINT64_C(1000000000000) / 12500,
					   [BMA400_ODR_25HZ] = UINT64_C(1000000000) / 25,
					   [BMA400_ODR_50HZ] = UINT64_C(1000000000) / 50,
					   [BMA400_ODR_100HZ] = UINT64_C(1000000000) / 100,
					   [BMA400_ODR_200HZ] = UINT64_C(1000000000) / 200,
					   [BMA400_ODR_400HZ] = UINT64_C(1000000000) / 400,
					   [BMA400_ODR_800HZ] = UINT64_C(1000000000) / 800};

#endif /* CONFIG_BMA400_STREAM */

/*
 * RTIO decoder
 */

static int bma400_decoder_get_frame_count(const uint8_t *buffer, struct sensor_chan_spec ch,
					  uint16_t *frame_count)
{
	const struct bma400_fifo_data *edata = (const struct bma400_fifo_data *)buffer;
#ifdef CONFIG_BMA400_STREAM
	const struct bma400_decoder_header *header = &edata->header;
#endif
	if (ch.chan_idx != 0) {
		return -ENOTSUP;
	}
#ifdef CONFIG_BMA400_STREAM
	if (!header->is_fifo) {
		switch (ch.chan_type) {
		case SENSOR_CHAN_ACCEL_X:
		case SENSOR_CHAN_ACCEL_Y:
		case SENSOR_CHAN_ACCEL_Z:
		case SENSOR_CHAN_ACCEL_XYZ:
		case SENSOR_CHAN_DIE_TEMP:
			*frame_count = edata->fifo_count > 0 ? 1 : 0;
			return 0;
		default:
			return -ENOTSUP;
		}
	}
#else
	/* If not streaming, we only have one frame per read */
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
#endif /* CONFIG_BMA400_STREAM */
	if (!IS_ACCEL(ch.chan_type)) {
		return -ENOTSUP;
	}
	/* Skip the header */
	buffer += sizeof(struct bma400_fifo_data) + 1; /* +1 for SPI dummy byte */

	uint16_t count = 0;
	const uint8_t *end = buffer + edata->fifo_count;

	while (buffer < end) {
		uint8_t size = BMA400_FIFO_HEADER_LENGTH;
		/* Verify if FIFO frame has sensor data bit */
		const bool has_accel = (BMA400_FIFO_EMPTY_FRAME & buffer[0]) != 0;
		const bool is_empty = (BMA400_FIFO_EMPTY_FRAME == buffer[0]);
		const bool is_ctrl = (BMA400_FIFO_CONTROL_FRAME == buffer[0]);

		if (has_accel) {
			size += BMA400_FIFO_A_LENGTH;
			++count;
		} else if (is_ctrl) {
			size += BMA400_FIFO_CF_LENGTH;
		} else if (is_empty) {
			size += BMA400_FIFO_CF_LENGTH;
		} else {
			/* Error frame not OK */
			return -ENODATA;
		}

		buffer += size;
	}

	*frame_count = count;

	return 0;
}

static int bma400_decoder_get_size_info(struct sensor_chan_spec ch, size_t *base_size,
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

static int bma400_get_shift(struct sensor_chan_spec ch, uint8_t accel_fs, int8_t *shift)
{
	switch (ch.chan_type) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		switch (accel_fs) {
		case BMA400_RANGE_2G:
			/* 2 G's = 19.62 m/s^2. Use shift of 5 (+/-32) */
			*shift = 5;
			return 0;
		case BMA400_RANGE_4G:
			*shift = 6;
			return 0;
		case BMA400_RANGE_8G:
			*shift = 7;
			return 0;
		case BMA400_RANGE_16G:
			*shift = 8;
			return 0;
		default:
			return -EINVAL;
		}
	case SENSOR_CHAN_DIE_TEMP:
		*shift = BMA400_TEMP_SHIFT;
		return 0;
	default:
		return -EINVAL;
	}
}

static void bma400_convert_raw_accel_to_q31(int16_t raw_val, q31_t *out)
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
	 * All that's left is to divide by the bma400's maximum range BIT(11).
	 */

	int16_t value = (int16_t)sys_le16_to_cpu(raw_val << 4) >> 4;

	const int64_t scale = (int64_t)(9.80665 * BIT64(31 - 4));

	*out = CLAMP(((int64_t)value * scale) >> 11, INT32_MIN, INT32_MAX);
}

static void bma400_unpack_accel_data(const uint8_t *pkt, uint8_t data_start_index, q31_t *out)
{
	uint8_t offset = BMA400_FIFO_HEADER_LENGTH + (data_start_index * 2);

	int16_t value = (pkt[offset + 1] << 4) | FIELD_GET(GENMASK(3, 0), pkt[offset]);

	bma400_convert_raw_accel_to_q31((int16_t)value, out);
}

static int bma400_one_shot_decode(const uint8_t *buffer, struct sensor_chan_spec ch, uint32_t *fit,
				  uint16_t max_count, void *data_out)
{
	const struct bma400_encoded_data *edata = (const struct bma400_encoded_data *)buffer;
	const struct bma400_decoder_header *header = &edata->header;
	int rc = 0;

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
		rc = bma400_get_shift((struct sensor_chan_spec){.chan_type = SENSOR_CHAN_ACCEL_XYZ,
								.chan_idx = 3},
				      header->accel_fs, &out->shift);
		if (rc != 0) {
			return -EINVAL;
		}

		/* The presence of SPI dummy byte makes tha packet the same as FIFO */
		bma400_unpack_accel_data(&edata->accel_xyz_raw_data[0], 0, &out->readings[0].x);
		bma400_unpack_accel_data(&edata->accel_xyz_raw_data[0], 1, &out->readings[0].y);
		bma400_unpack_accel_data(&edata->accel_xyz_raw_data[0], 2, &out->readings[0].z);
		*fit = 1;
		return 1;
	}
	default:
		return -EINVAL;
	}
}

#ifdef CONFIG_BMA400_STREAM

static int bma400_fifo_decode(const uint8_t *buffer, struct sensor_chan_spec ch, uint32_t *fit,
			      uint16_t max_count, void *data_out)
{
	const struct bma400_fifo_data *edata = (const struct bma400_fifo_data *)buffer;
	const uint8_t *buffer_end = buffer + sizeof(struct bma400_fifo_data) + edata->fifo_count;
	struct sensor_three_axis_data *data = (struct sensor_three_axis_data *)data_out;
	uint64_t period_ns = accel_period_ns[edata->accel_odr];
	uint32_t accel_frame_count = 0;
	uint32_t tot_accel_count = 0;

	if ((uintptr_t)buffer_end <= *fit || ch.chan_idx != 0) {
		return 0;
	}
	if (!IS_ACCEL(ch.chan_type)) {
		return -ENOTSUP;
	}

	bma400_get_shift(
		(struct sensor_chan_spec){.chan_type = SENSOR_CHAN_ACCEL_XYZ, .chan_idx = 3},
		edata->header.accel_fs, &data->shift);

	((struct sensor_data_header *)data_out)->base_timestamp_ns = edata->header.timestamp;
	buffer += sizeof(struct bma400_fifo_data) + 1; /* +1 for SPI dummy byte */
	/* Counts only real increment over fit */
	while (accel_frame_count < max_count && buffer < buffer_end) {
		const bool has_accel = (BMA400_FIFO_EMPTY_FRAME & buffer[0]) != 0;
		const bool is_empty = (BMA400_FIFO_EMPTY_FRAME == buffer[0]);
		const bool is_ctrl = (BMA400_FIFO_CONTROL_FRAME == buffer[0]);
		const uint8_t *frame_end = buffer;

		if (is_empty) {
			frame_end += BMA400_FIFO_CF_LENGTH + BMA400_FIFO_HEADER_LENGTH;
		} else if (has_accel) {
			frame_end += BMA400_FIFO_A_LENGTH + BMA400_FIFO_HEADER_LENGTH;
			tot_accel_count++;
		} else if (is_ctrl) {
			frame_end += BMA400_FIFO_CF_LENGTH + BMA400_FIFO_HEADER_LENGTH;
		} else {
			/* Error frame not OK */
			return -ENODATA;
		}

		if ((uintptr_t)buffer < *fit) {
			buffer = frame_end;
			continue;
		}

		if (has_accel) {
			data->readings[accel_frame_count].timestamp_delta =
				(tot_accel_count - 1) * period_ns;
			bma400_unpack_accel_data(buffer, 0, &data->readings[accel_frame_count].x);
			bma400_unpack_accel_data(buffer, 1, &data->readings[accel_frame_count].y);
			bma400_unpack_accel_data(buffer, 2, &data->readings[accel_frame_count].z);
			accel_frame_count++;
		}

		buffer = frame_end;
		*fit = (uintptr_t)frame_end;
	}

	return accel_frame_count;
}

#endif /* CONFIG_BMA400_STREAM */

static int bma400_decoder_decode(const uint8_t *buffer, struct sensor_chan_spec ch, uint32_t *fit,
				 uint16_t max_count, void *data_out)
{
#ifdef CONFIG_BMA400_STREAM
	const struct bma400_decoder_header *header = (const struct bma400_decoder_header *)buffer;

	if (header->is_fifo) {
		return bma400_fifo_decode(buffer, ch, fit, max_count, data_out);
	}
#endif

	return bma400_one_shot_decode(buffer, ch, fit, max_count, data_out);
}

static bool bma400_decoder_has_trigger(const uint8_t *buffer, enum sensor_trigger_type trigger)
{
	if (trigger == SENSOR_TRIG_MOTION) {
		return ((const struct bma400_decoder_header *)buffer)->has_motion_trigger;
	}
	if (trigger == SENSOR_TRIG_FIFO_WATERMARK) {
		return ((const struct bma400_decoder_header *)buffer)->has_fifo_wm_trigger;
	}
	if (trigger == SENSOR_TRIG_FIFO_FULL) {
		return ((const struct bma400_decoder_header *)buffer)->has_fifo_full_trigger;
	}
	return false;
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = bma400_decoder_get_frame_count,
	.get_size_info = bma400_decoder_get_size_info,
	.decode = bma400_decoder_decode,
	.has_trigger = bma400_decoder_has_trigger,
};

int bma400_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);
	*decoder = &SENSOR_DECODER_NAME();

	return 0;
}
