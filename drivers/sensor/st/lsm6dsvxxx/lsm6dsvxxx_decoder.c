/* ST Microelectronics LSM6DSVXXX family IMU sensor
 *
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lsm6dsv320x.pdf
 * https://www.st.com/resource/en/datasheet/lsm6dsv80x.pdf
 */

#include "lsm6dsvxxx.h"
#include <zephyr/dt-bindings/sensor/lsm6dsvxxx.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LSM6DSVXXX_DECODER, CONFIG_SENSOR_LOG_LEVEL);

#ifdef CONFIG_LSM6DSVXXX_STREAM
static const uint32_t accel_period_ns[] = {
	[LSM6DSVXXX_DT_XL_BATCHED_AT_1Hz875] = UINT32_C(1000000000000) / 1875,
	[LSM6DSVXXX_DT_XL_BATCHED_AT_7Hz5] = UINT32_C(1000000000000) / 7500,
	[LSM6DSVXXX_DT_XL_BATCHED_AT_15Hz] = UINT32_C(1000000000) / 15,
	[LSM6DSVXXX_DT_XL_BATCHED_AT_30Hz] = UINT32_C(1000000000) / 30,
	[LSM6DSVXXX_DT_XL_BATCHED_AT_60Hz] = UINT32_C(1000000000) / 60,
	[LSM6DSVXXX_DT_XL_BATCHED_AT_120Hz] = UINT32_C(1000000000) / 120,
	[LSM6DSVXXX_DT_XL_BATCHED_AT_240Hz] = UINT32_C(1000000000) / 240,
	[LSM6DSVXXX_DT_XL_BATCHED_AT_480Hz] = UINT32_C(1000000000) / 480,
	[LSM6DSVXXX_DT_XL_BATCHED_AT_960Hz] = UINT32_C(1000000000) / 960,
	[LSM6DSVXXX_DT_XL_BATCHED_AT_1920Hz] = UINT32_C(1000000000) / 1920,
	[LSM6DSVXXX_DT_XL_BATCHED_AT_3840Hz] = UINT32_C(1000000000) / 3840,
	[LSM6DSVXXX_DT_XL_BATCHED_AT_7680Hz] = UINT32_C(1000000000) / 7680,
};

static const uint32_t gyro_period_ns[] = {
	[LSM6DSVXXX_DT_GY_BATCHED_AT_1Hz875] = UINT32_C(1000000000000) / 1875,
	[LSM6DSVXXX_DT_GY_BATCHED_AT_7Hz5] = UINT32_C(1000000000000) / 7500,
	[LSM6DSVXXX_DT_GY_BATCHED_AT_15Hz] = UINT32_C(1000000000) / 15,
	[LSM6DSVXXX_DT_GY_BATCHED_AT_30Hz] = UINT32_C(1000000000) / 30,
	[LSM6DSVXXX_DT_GY_BATCHED_AT_60Hz] = UINT32_C(1000000000) / 60,
	[LSM6DSVXXX_DT_GY_BATCHED_AT_120Hz] = UINT32_C(1000000000) / 120,
	[LSM6DSVXXX_DT_GY_BATCHED_AT_240Hz] = UINT32_C(1000000000) / 240,
	[LSM6DSVXXX_DT_GY_BATCHED_AT_480Hz] = UINT32_C(1000000000) / 480,
	[LSM6DSVXXX_DT_GY_BATCHED_AT_960Hz] = UINT32_C(1000000000) / 960,
	[LSM6DSVXXX_DT_GY_BATCHED_AT_1920Hz] = UINT32_C(1000000000) / 1920,
	[LSM6DSVXXX_DT_GY_BATCHED_AT_3840Hz] = UINT32_C(1000000000) / 3840,
	[LSM6DSVXXX_DT_GY_BATCHED_AT_7680Hz] = UINT32_C(1000000000) / 7680,
};

#if defined(CONFIG_LSM6DSVXXX_ENABLE_TEMP)
static const uint32_t temp_period_ns[] = {
	[LSM6DSVXXX_DT_TEMP_BATCHED_AT_1Hz875] = UINT32_C(1000000000000) / 1875,
	[LSM6DSVXXX_DT_TEMP_BATCHED_AT_15Hz] = UINT32_C(1000000000) / 15,
	[LSM6DSVXXX_DT_TEMP_BATCHED_AT_60Hz] = UINT32_C(1000000000) / 60,
};
#endif

static const uint32_t sflp_period_ns[] = {
	[LSM6DSVXXX_DT_SFLP_ODR_AT_15Hz] = UINT32_C(1000000000) / 15,
	[LSM6DSVXXX_DT_SFLP_ODR_AT_30Hz] = UINT32_C(1000000000) / 30,
	[LSM6DSVXXX_DT_SFLP_ODR_AT_60Hz] = UINT32_C(1000000000) / 60,
	[LSM6DSVXXX_DT_SFLP_ODR_AT_120Hz] = UINT32_C(1000000000) / 120,
	[LSM6DSVXXX_DT_SFLP_ODR_AT_240Hz] = UINT32_C(1000000000) / 240,
	[LSM6DSVXXX_DT_SFLP_ODR_AT_480Hz] = UINT32_C(1000000000) / 480,
};
#endif /* CONFIG_LSM6DSVXXX_STREAM */

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

/* bit shift for Gyroscope for a given fs */
static const int8_t gyro_bit_shift[] = {
	2, /* 125 dps */
	3, /* 250 dps */
	4, /* 500 dps */
	5, /* 1000 dps */
	6, /* 2000 dps */
	7, /* 4000 dps */
};

#if defined(CONFIG_LSM6DSVXXX_ENABLE_TEMP)
/* bit shift for Temperature sensor */
static const int8_t temp_bit_shift = 9;

/* transform temperature LSB into micro-Celsius */
#define SENSOR_TEMP_UCELSIUS(t_lsb) \
	(int64_t) (25000000LL + (((int64_t)(t_lsb) * 1000000LL) / 355LL))

#endif

/*
 * Accelerometer scaling factors table (indexed by full scale)
 * GAIN_UNIT_G is expressed in udps/LSB.
 */
static const int32_t gyro_scaler[] = {
	SENSOR_SCALE_UDPS_TO_URADS(4375), /* 125 dps */
	SENSOR_SCALE_UDPS_TO_URADS(8750), /* 250 dps */
	SENSOR_SCALE_UDPS_TO_URADS(17500), /* 500 dps */
	SENSOR_SCALE_UDPS_TO_URADS(35000), /* 1000 dps */
	SENSOR_SCALE_UDPS_TO_URADS(70000), /* 2000 dps */
	SENSOR_SCALE_UDPS_TO_URADS(140000), /* 4000 dps */
};

static int lsm6dsvxxx_decoder_get_frame_count(const uint8_t *buffer,
					      struct sensor_chan_spec chan_spec,
					      uint16_t *frame_count)
{
	const struct lsm6dsvxxx_fifo_data *data = (const struct lsm6dsvxxx_fifo_data *)buffer;
	const struct lsm6dsvxxx_rtio_data *rdata = (const struct lsm6dsvxxx_rtio_data *)buffer;
	const struct lsm6dsvxxx_decoder_header *header = &data->header;

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

#ifdef CONFIG_LSM6DSVXXX_STREAM
	const struct lsm6dsvxxx_fifo_data *edata = (const struct lsm6dsvxxx_fifo_data *)buffer;
	const uint8_t *buffer_end;
	uint8_t fifo_tag;
	uint8_t tot_accel_fifo_words = 0, tot_gyro_fifo_words = 0;
	uint8_t tot_sflp_gbias = 0, tot_sflp_gravity = 0, tot_sflp_game_rotation = 0;

#if defined(CONFIG_LSM6DSVXXX_ENABLE_TEMP)
	uint8_t tot_temp_fifo_words = 0;
#endif

	buffer += sizeof(struct lsm6dsvxxx_fifo_data);
	buffer_end = buffer + LSM6DSVXXX_FIFO_SIZE(edata->fifo_count);

	/* count total FIFO word for each tag */
	while (buffer < buffer_end) {
		fifo_tag = (buffer[0] >> 3);

		switch (fifo_tag) {
		case LSM6DSVXXX_XL_HG_TAG:
		case LSM6DSVXXX_XL_NC_TAG:
			tot_accel_fifo_words++;
			break;
		case LSM6DSVXXX_GY_NC_TAG:
			tot_gyro_fifo_words++;
			break;
#if defined(CONFIG_LSM6DSVXXX_ENABLE_TEMP)
		case LSM6DSVXXX_TEMPERATURE_TAG:
			tot_temp_fifo_words++;
			break;
#endif
		case LSM6DSVXXX_SFLP_GYROSCOPE_BIAS_TAG:
			tot_sflp_gbias++;
			break;
		case LSM6DSVXXX_SFLP_GRAVITY_VECTOR_TAG:
			tot_sflp_gravity++;
			break;
		case LSM6DSVXXX_SFLP_GAME_ROTATION_VECTOR_TAG:
			tot_sflp_game_rotation++;
			break;
		default:
			break;
		}

		buffer += LSM6DSVXXX_FIFO_ITEM_LEN;
	}

	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		*frame_count = tot_accel_fifo_words;
		break;

	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
	case SENSOR_CHAN_GYRO_XYZ:
		*frame_count = tot_gyro_fifo_words;
		break;

#if defined(CONFIG_LSM6DSVXXX_ENABLE_TEMP)
	case SENSOR_CHAN_DIE_TEMP:
		*frame_count = tot_temp_fifo_words;
		break;
#endif
	case SENSOR_CHAN_GAME_ROTATION_VECTOR:
		*frame_count = tot_sflp_game_rotation;
		break;
	case SENSOR_CHAN_GRAVITY_VECTOR:
		*frame_count = tot_sflp_gravity;
		break;
	case SENSOR_CHAN_GBIAS_XYZ:
		*frame_count = tot_sflp_gbias;
		break;
	default:
		*frame_count = 0;
		break;
	}
#endif

	return 0;
}

#ifdef CONFIG_LSM6DSVXXX_STREAM
static float32_t calculate_quat_w(float32_t *x, float32_t *y, float32_t *z)
{
	float32_t  sumsq;

	sumsq = powf(*x, 2) + powf(*y, 2) + powf(*z, 2);

	/*
	 * Theoretically sumsq should never be greater than 1, but due to
	 * lack of precision it might happen. So, add a software correction
	 * which consists in normalizing the (x, y, z) vector.
	 */
	if (sumsq > 1.0f) {
		float32_t n = sqrtf(sumsq);

		*x /= n;
		*y /= n;
		*z /= n;
		sumsq = 1.0f;
	}

	/* unity vector quaternions */
	return sqrtf(1.0f - sumsq);
}

static int generate_accel_output(const uint8_t *buffer, int count, uint16_t xl_count,
				 struct sensor_chan_spec chan_spec,
				 const struct lsm6dsvxxx_decoder_header *header,
				 void *data_out)
{
	struct sensor_three_axis_data *out = data_out;
	const struct lsm6dsvxxx_fifo_data *edata = (const struct lsm6dsvxxx_fifo_data *)buffer;
	int16_t x, y, z;
	const struct lsm6dsvxxx_config *cfg = header->cfg;
	const int32_t scale = cfg->accel_scaler[header->accel_fs];

	if (!SENSOR_CHANNEL_IS_ACCEL(chan_spec.chan_type)) {
		return 1;
	}

	out->readings[count].timestamp_delta =
		(xl_count - 1) * accel_period_ns[edata->accel_batch_odr];

	x = *(const int16_t *)&buffer[1];
	y = *(const int16_t *)&buffer[3];
	z = *(const int16_t *)&buffer[5];

	out->shift = cfg->accel_bit_shift[header->accel_fs];

	out->readings[count].x = Q31_SHIFT_MICROVAL(scale * x, out->shift);
	out->readings[count].y = Q31_SHIFT_MICROVAL(scale * y, out->shift);
	out->readings[count].z = Q31_SHIFT_MICROVAL(scale * z, out->shift);

	return 0;
}

static int generate_gyro_output(const uint8_t *buffer, int count, uint16_t gy_count,
				 struct sensor_chan_spec chan_spec,
				 const struct lsm6dsvxxx_decoder_header *header,
				 void *data_out)
{
	struct sensor_three_axis_data *out = data_out;
	const struct lsm6dsvxxx_fifo_data *edata = (const struct lsm6dsvxxx_fifo_data *)buffer;
	int16_t x, y, z;
	const int32_t scale = gyro_scaler[header->gyro_fs];

	if (!SENSOR_CHANNEL_IS_GYRO(chan_spec.chan_type)) {
		return 1;
	}

	out->readings[count].timestamp_delta =
		(gy_count - 1) * gyro_period_ns[edata->gyro_batch_odr];

	x = *(const int16_t *)&buffer[1];
	y = *(const int16_t *)&buffer[3];
	z = *(const int16_t *)&buffer[5];

	out->shift = gyro_bit_shift[header->gyro_fs];

	out->readings[count].x = Q31_SHIFT_MICROVAL(scale * x, out->shift);
	out->readings[count].y = Q31_SHIFT_MICROVAL(scale * y, out->shift);
	out->readings[count].z = Q31_SHIFT_MICROVAL(scale * z, out->shift);

	return 0;
}

#if defined(CONFIG_LSM6DSVXXX_ENABLE_TEMP)
static int generate_temp_output(const uint8_t *buffer, int count, uint16_t temp_count,
				 struct sensor_chan_spec chan_spec,
				 const struct lsm6dsvxxx_decoder_header *header,
				 void *data_out)
{
	struct sensor_q31_data *out = data_out;
	const struct lsm6dsvxxx_fifo_data *edata = (const struct lsm6dsvxxx_fifo_data *)buffer;
	int16_t t;
	int64_t t_uC;

	if (chan_spec.chan_type != SENSOR_CHAN_DIE_TEMP) {
		return 1;
	}

	out->readings[count].timestamp_delta =
		(temp_count - 1) * temp_period_ns[edata->temp_batch_odr];

	t = *(const int16_t *)&buffer[1];
	t_uC = SENSOR_TEMP_UCELSIUS(t);

	out->shift = temp_bit_shift;

	out->readings[count].temperature = Q31_SHIFT_MICROVAL(t_uC, out->shift);

	return 0;
}
#endif

static int generate_game_rot_output(const uint8_t *buffer, int count, uint16_t game_rot_count,
				    struct sensor_chan_spec chan_spec,
				    const struct lsm6dsvxxx_decoder_header *header,
				    void *data_out)
{
	struct sensor_game_rotation_vector_data *out = data_out;
	const struct lsm6dsvxxx_fifo_data *edata = (const struct lsm6dsvxxx_fifo_data *)buffer;
	const struct lsm6dsvxxx_config *cfg = header->cfg;
	union { float32_t f; uint32_t i; } x, y, z;
	float32_t w;

	if (chan_spec.chan_type != SENSOR_CHAN_GAME_ROTATION_VECTOR) {
		return 1;
	}

	out->readings[count].timestamp_delta =
		(game_rot_count - 1) * sflp_period_ns[edata->sflp_batch_odr];

	x.i = cfg->chip_api->from_f16_to_f32(buffer[1] | (buffer[2] << 8));
	y.i = cfg->chip_api->from_f16_to_f32(buffer[3] | (buffer[4] << 8));
	z.i = cfg->chip_api->from_f16_to_f32(buffer[5] | (buffer[6] << 8));

	/* unity vector quaternion */
	w = calculate_quat_w(&x.f, &y.f, &z.f);

	/*
	 * Quaternions are numbers between -1 and 1. So let's select the signed
	 * Q0.31 format (m = 0, n (fractional bits) == 31)
	 */
	out->shift = 0;

	out->readings[count].x = Q31_SHIFT_VAL(x.f, out->shift);
	out->readings[count].y = Q31_SHIFT_VAL(y.f, out->shift);
	out->readings[count].z = Q31_SHIFT_VAL(z.f, out->shift);
	out->readings[count].w = Q31_SHIFT_VAL(w, out->shift);

	return 0;
}

static int generate_gbias_output(const uint8_t *buffer, int count, uint16_t gbias_count,
				 struct sensor_chan_spec chan_spec,
				 const struct lsm6dsvxxx_decoder_header *header,
				 void *data_out)
{
	struct sensor_three_axis_data *out = data_out;
	const struct lsm6dsvxxx_fifo_data *edata = (const struct lsm6dsvxxx_fifo_data *)buffer;
	int16_t x, y, z;
	const int32_t scale = gyro_scaler[0]; /* 125 dpds */

	if (chan_spec.chan_type != SENSOR_CHAN_GBIAS_XYZ) {
		return 1;
	}

	out->readings[count].timestamp_delta =
		(gbias_count - 1) * sflp_period_ns[edata->sflp_batch_odr];

	x = buffer[1] | (buffer[2] << 8);
	y = buffer[3] | (buffer[4] << 8);
	z = buffer[5] | (buffer[6] << 8);

	out->shift = gyro_bit_shift[0]; /* 125 dpds */

	out->readings[count].x = Q31_SHIFT_MICROVAL(scale * x, out->shift);
	out->readings[count].y = Q31_SHIFT_MICROVAL(scale * y, out->shift);
	out->readings[count].z = Q31_SHIFT_MICROVAL(scale * z, out->shift);

	return 0;
}

static int generate_gravity_output(const uint8_t *buffer, int count, uint16_t gravity_count,
				   struct sensor_chan_spec chan_spec,
				   const struct lsm6dsvxxx_decoder_header *header,
				   void *data_out)
{
	struct sensor_three_axis_data *out = data_out;
	const struct lsm6dsvxxx_fifo_data *edata = (const struct lsm6dsvxxx_fifo_data *)buffer;
	const struct lsm6dsvxxx_config *cfg = header->cfg;
	float32_t x, y, z;

	if (chan_spec.chan_type != SENSOR_CHAN_GRAVITY_VECTOR) {
		return 1;
	}

	out->readings[count].timestamp_delta =
		(gravity_count - 1) * sflp_period_ns[edata->sflp_batch_odr];

	x = cfg->chip_api->from_sflp_to_mg(buffer[1] | (buffer[2] << 8));
	y = cfg->chip_api->from_sflp_to_mg(buffer[3] | (buffer[4] << 8));
	z = cfg->chip_api->from_sflp_to_mg(buffer[5] | (buffer[6] << 8));

	out->shift = 12;

	out->readings[count].x = Q31_SHIFT_VAL(x, out->shift);
	out->readings[count].y = Q31_SHIFT_VAL(y, out->shift);
	out->readings[count].z = Q31_SHIFT_VAL(z, out->shift);

	return 0;
}

static int lsm6dsvxxx_decode_fifo(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
				  uint32_t *fit, uint16_t max_count, void *data_out)
{
	const struct lsm6dsvxxx_fifo_data *edata = (const struct lsm6dsvxxx_fifo_data *)buffer;
	const uint8_t *buffer_end;
	const struct lsm6dsvxxx_decoder_header *header = &edata->header;
	int count = 0;
	uint8_t fifo_tag;
	uint16_t xl_count = 0, gy_count = 0;
	uint16_t tot_chan_fifo_words = 0;

#if defined(CONFIG_LSM6DSVXXX_ENABLE_TEMP)
	uint16_t temp_count = 0;
#endif
	uint16_t game_rot_count = 0, gravity_count = 0, gbias_count = 0;
	int ret;

	/* count total FIFO word for each tag */
	ret = lsm6dsvxxx_decoder_get_frame_count(buffer, chan_spec, &tot_chan_fifo_words);
	if (ret < 0) {
		return 0;
	}

	buffer += sizeof(struct lsm6dsvxxx_fifo_data);
	buffer_end = buffer + LSM6DSVXXX_FIFO_SIZE(edata->fifo_count);

	/*
	 * Timestamp in header is set when FIFO threshold is reached, so
	 * set time baseline going back in past according to total number
	 * of FIFO word for each type.
	 */
	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		((struct sensor_data_header *)data_out)->base_timestamp_ns =
			edata->header.timestamp -
			(tot_chan_fifo_words - 1) * accel_period_ns[edata->accel_batch_odr];
		break;
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
	case SENSOR_CHAN_GYRO_XYZ:
		((struct sensor_data_header *)data_out)->base_timestamp_ns =
			edata->header.timestamp -
			(tot_chan_fifo_words - 1) * gyro_period_ns[edata->gyro_batch_odr];
		break;
#if defined(CONFIG_LSM6DSVXXX_ENABLE_TEMP)
	case SENSOR_CHAN_DIE_TEMP:
		((struct sensor_data_header *)data_out)->base_timestamp_ns =
			edata->header.timestamp -
			(tot_chan_fifo_words - 1) * temp_period_ns[edata->temp_batch_odr];
		break;
#endif
	case SENSOR_CHAN_GAME_ROTATION_VECTOR:
	case SENSOR_CHAN_GRAVITY_VECTOR:
	case SENSOR_CHAN_GBIAS_XYZ:
		((struct sensor_data_header *)data_out)->base_timestamp_ns =
			edata->header.timestamp -
			(tot_chan_fifo_words - 1) * sflp_period_ns[edata->sflp_batch_odr];
		break;
	default:
		/* unhandled FIFO tag */
		((struct sensor_data_header *)data_out)->base_timestamp_ns = 0;
	}

	while (count < max_count && buffer < buffer_end) {
		const uint8_t *frame_end = buffer;
		uint8_t skip = 0;

		frame_end += LSM6DSVXXX_FIFO_ITEM_LEN;

		if ((uintptr_t)buffer < *fit) {
			/* This frame was already decoded, move on to the next frame */
			buffer = frame_end;
			continue;
		}

		fifo_tag = (buffer[0] >> 3);

		switch (fifo_tag) {
		case LSM6DSVXXX_XL_HG_TAG:
		case LSM6DSVXXX_XL_NC_TAG:
			xl_count++;
			skip = generate_accel_output(buffer, count, xl_count,
						     chan_spec, header, data_out);
			break;

		case LSM6DSVXXX_GY_NC_TAG:
			gy_count++;
			skip = generate_gyro_output(buffer, count, gy_count,
						    chan_spec, header, data_out);
			break;

#if defined(CONFIG_LSM6DSVXXX_ENABLE_TEMP)
		case LSM6DSVXXX_TEMPERATURE_TAG:
			temp_count++;
			skip = generate_temp_output(buffer, count, temp_count,
						    chan_spec, header, data_out);
			break;
#endif
		case LSM6DSVXXX_SFLP_GAME_ROTATION_VECTOR_TAG:
			game_rot_count++;
			skip = generate_game_rot_output(buffer, count, game_rot_count,
							chan_spec, header, data_out);
			break;

		case LSM6DSVXXX_SFLP_GYROSCOPE_BIAS_TAG:
			gbias_count++;
			skip = generate_gbias_output(buffer, count, gbias_count,
						     chan_spec, header, data_out);
			break;

		case LSM6DSVXXX_SFLP_GRAVITY_VECTOR_TAG:
			gravity_count++;
			skip = generate_gravity_output(buffer, count, gravity_count,
						       chan_spec, header, data_out);
			break;

		default:
			/* skip unhandled FIFO tag */
			buffer = frame_end;
			LOG_DBG("unknown FIFO tag %02x", fifo_tag);
			continue;
		}

		buffer = frame_end;
		if (!skip) {
			*fit = (uintptr_t)frame_end;
			count++;
		}
	}

	return count;
}
#endif /* CONFIG_LSM6DSVXXX_STREAM */

static int lsm6dsvxxx_decode_sample(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
				    uint32_t *fit, uint16_t max_count, void *data_out)
{
	const struct lsm6dsvxxx_rtio_data *edata = (const struct lsm6dsvxxx_rtio_data *)buffer;
	const struct lsm6dsvxxx_decoder_header *header = &edata->header;
	const struct lsm6dsvxxx_config *cfg = header->cfg;

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
		const int32_t scale = cfg->accel_scaler[header->accel_fs];

		if (edata->has_accel == 0) {
			return -ENODATA;
		}

		struct sensor_three_axis_data *out = data_out;

		out->header.base_timestamp_ns = edata->header.timestamp;
		out->header.reading_count = 1;

		out->shift = cfg->accel_bit_shift[header->accel_fs];

		out->readings[0].x = Q31_SHIFT_MICROVAL(scale * edata->accel[0], out->shift);
		out->readings[0].y = Q31_SHIFT_MICROVAL(scale * edata->accel[1], out->shift);
		out->readings[0].z = Q31_SHIFT_MICROVAL(scale * edata->accel[2], out->shift);
		*fit = 1;
		return 1;
	}
#if defined(CONFIG_LSM6DSVXXX_ENABLE_TEMP)
	case SENSOR_CHAN_DIE_TEMP: {
		int64_t t_uC;

		if (edata->has_temp == 0) {
			return -ENODATA;
		}

		struct sensor_q31_data *out = data_out;

		out->header.base_timestamp_ns = edata->header.timestamp;
		out->header.reading_count = 1;

		out->shift = temp_bit_shift;

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

static int lsm6dsvxxx_decoder_decode(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
				     uint32_t *fit, uint16_t max_count, void *data_out)
{
#ifdef CONFIG_LSM6DSVXXX_STREAM
	const struct lsm6dsvxxx_decoder_header *header =
		(const struct lsm6dsvxxx_decoder_header *)buffer;

	if (header->is_fifo) {
		return lsm6dsvxxx_decode_fifo(buffer, chan_spec, fit, max_count, data_out);
	} else {
		return lsm6dsvxxx_decode_sample(buffer, chan_spec, fit, max_count, data_out);
	}
#endif

	return lsm6dsvxxx_decode_sample(buffer, chan_spec, fit, max_count, data_out);
}

static int lsm6dsvxxx_decoder_get_size_info(struct sensor_chan_spec chan_spec, size_t *base_size,
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
static bool lsm6dsvxxx_decoder_has_trigger(const uint8_t *buffer, enum sensor_trigger_type trigger)
{
	return false;
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = lsm6dsvxxx_decoder_get_frame_count,
	.get_size_info = lsm6dsvxxx_decoder_get_size_info,
	.decode = lsm6dsvxxx_decoder_decode,
	.has_trigger = lsm6dsvxxx_decoder_has_trigger,
};

int lsm6dsvxxx_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);
	*decoder = &SENSOR_DECODER_NAME();

	return 0;
}
