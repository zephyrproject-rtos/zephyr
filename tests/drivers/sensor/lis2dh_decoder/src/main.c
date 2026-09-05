/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

#include "lis2dh_decoder.c"

#define FIFO_SAMPLES 32U

static void put_sample(uint8_t *buffer, size_t index, int16_t x, int16_t y, int16_t z)
{
	uint8_t *sample =
		buffer + sizeof(struct lis2dh_encoded_header) + index * LIS2DH_ENCODED_SAMPLE_SIZE;

	sys_put_le16((uint16_t)x, sample);
	sys_put_le16((uint16_t)y, sample + 2U);
	sys_put_le16((uint16_t)z, sample + 4U);
}

ZTEST(lis2dh_decoder, test_decode_fifo_frames)
{
	uint8_t buffer[sizeof(struct lis2dh_encoded_header) + 2U * LIS2DH_ENCODED_SAMPLE_SIZE] = {
		0};
	struct lis2dh_encoded_header *header = (struct lis2dh_encoded_header *)buffer;
	struct sensor_three_axis_data data;
	struct sensor_chan_spec channel = {SENSOR_CHAN_ACCEL_XYZ, 0};
	const struct sensor_decoder_api *decoder = NULL;
	uint16_t frame_count = UINT16_MAX;
	uint32_t fit = 0U;
	int status;

	header->timestamp_ns = 1000U;
	header->period_ns = 100U;
	header->scale = 1000000U;
	header->sample_count = 2U;
	header->trigger = SENSOR_TRIG_FIFO_WATERMARK;
	header->shift = 5;
	header->is_fifo = 1U;
	put_sample(buffer, 0U, 0x0010, (int16_t)0xfff0, 0x0020);
	put_sample(buffer, 1U, 0x0030, (int16_t)0xffd0, 0x0040);

	zassert_ok(lis2dh_get_decoder(NULL, &decoder));
	zassert_ok(decoder->get_frame_count(buffer, channel, &frame_count));
	zassert_equal(frame_count, 2U);
	zassert_true(decoder->has_trigger(buffer, SENSOR_TRIG_FIFO_WATERMARK));
	zassert_false(decoder->has_trigger(buffer, SENSOR_TRIG_FIFO_FULL));

	status = decoder->decode(buffer, channel, &fit, 1U, &data);
	zassert_equal(status, 1);
	zassert_equal(data.header.base_timestamp_ns, 900U);
	zassert_equal(data.header.reading_count, 1U);
	zassert_equal(data.readings[0].timestamp_delta, 0U);
	zassert_true(data.readings[0].x > 0);
	zassert_true(data.readings[0].y < 0);
	zassert_equal(fit, 1U);

	status = decoder->decode(buffer, channel, &fit, 1U, &data);
	zassert_equal(status, 1);
	zassert_equal(data.header.base_timestamp_ns, 1000U);
	zassert_equal(data.readings[0].timestamp_delta, 0U);
	zassert_true(data.readings[0].x > 0);
	zassert_true(data.readings[0].y < 0);
	zassert_equal(fit, 2U);
}

ZTEST(lis2dh_decoder, test_empty_and_invalid_channel)
{
	uint8_t buffer[sizeof(struct lis2dh_encoded_header)] = {0};
	struct lis2dh_encoded_header *header = (struct lis2dh_encoded_header *)buffer;
	struct sensor_chan_spec accel = {SENSOR_CHAN_ACCEL_XYZ, 0};
	struct sensor_chan_spec temperature = {SENSOR_CHAN_DIE_TEMP, 0};
	const struct sensor_decoder_api *decoder = NULL;
	uint16_t frame_count = UINT16_MAX;

	header->is_fifo = 1U;
	header->trigger = SENSOR_TRIG_FIFO_FULL;

	zassert_ok(lis2dh_get_decoder(NULL, &decoder));
	zassert_ok(decoder->get_frame_count(buffer, accel, &frame_count));
	zassert_equal(frame_count, 0U);
	zassert_true(decoder->has_trigger(buffer, SENSOR_TRIG_FIFO_FULL));
	zassert_equal(decoder->get_frame_count(buffer, temperature, &frame_count), -ENOTSUP);
}

ZTEST(lis2dh_decoder, test_decode_full_fifo)
{
	uint8_t buffer[sizeof(struct lis2dh_encoded_header) +
		       FIFO_SAMPLES * LIS2DH_ENCODED_SAMPLE_SIZE] = {0};
	uint8_t decoded[sizeof(struct sensor_three_axis_data) +
			(FIFO_SAMPLES - 1U) * sizeof(struct sensor_three_axis_sample_data)];
	struct lis2dh_encoded_header *header = (struct lis2dh_encoded_header *)buffer;
	struct sensor_three_axis_data *data = (struct sensor_three_axis_data *)decoded;
	struct sensor_chan_spec channel = {SENSOR_CHAN_ACCEL_XYZ, 0};
	const struct sensor_decoder_api *decoder = NULL;
	uint16_t frame_count = UINT16_MAX;
	uint32_t fit = 0U;
	size_t i;

	header->timestamp_ns = 3200U;
	header->period_ns = 100U;
	header->scale = 100000U;
	header->sample_count = FIFO_SAMPLES;
	header->trigger = SENSOR_TRIG_FIFO_FULL;
	header->shift = 5;
	header->is_fifo = 1U;

	for (i = 0U; i < FIFO_SAMPLES; i++) {
		put_sample(buffer, i, (int16_t)((i + 1U) << 4), 0, 0);
	}

	zassert_ok(lis2dh_get_decoder(NULL, &decoder));
	zassert_ok(decoder->get_frame_count(buffer, channel, &frame_count));
	zassert_equal(frame_count, FIFO_SAMPLES);
	zassert_equal(decoder->decode(buffer, channel, &fit, frame_count, data), frame_count);
	zassert_equal(data->header.base_timestamp_ns, 100U);
	zassert_equal(data->header.reading_count, FIFO_SAMPLES);
	zassert_equal(data->readings[FIFO_SAMPLES - 1U].timestamp_delta, 3100U);
	zassert_true(data->readings[0].x < data->readings[FIFO_SAMPLES - 1U].x);
}

ZTEST(lis2dh_decoder, test_timestamp_wraparound)
{
	uint8_t buffer[sizeof(struct lis2dh_encoded_header) + 2U * LIS2DH_ENCODED_SAMPLE_SIZE] = {
		0};
	struct lis2dh_encoded_header *header = (struct lis2dh_encoded_header *)buffer;
	uint8_t decoded[sizeof(struct sensor_three_axis_data) +
			sizeof(struct sensor_three_axis_sample_data)] __aligned(8);
	struct sensor_three_axis_data *data = (void *)decoded;
	struct sensor_chan_spec channel = {SENSOR_CHAN_ACCEL_XYZ, 0};
	const struct sensor_decoder_api *decoder = NULL;
	uint32_t fit = 0U;

	header->timestamp_ns = 49U;
	header->period_ns = 100U;
	header->scale = 1000000U;
	header->sample_count = 2U;
	header->shift = 5;
	header->is_fifo = 1U;
	put_sample(buffer, 0U, 0, 0, 0);
	put_sample(buffer, 1U, 0x0010, 0, 0);

	zassert_ok(lis2dh_get_decoder(NULL, &decoder));
	zassert_equal(decoder->decode(buffer, channel, &fit, 2U, data), 2);
	zassert_equal(data->header.base_timestamp_ns, UINT64_MAX - 50U);
	zassert_equal(data->readings[0].timestamp_delta, 0U);
	zassert_equal(data->readings[1].timestamp_delta, 100U);
}

ZTEST(lis2dh_decoder, test_slow_odr_batches)
{
	uint8_t buffer[sizeof(struct lis2dh_encoded_header) +
		       FIFO_SAMPLES * LIS2DH_ENCODED_SAMPLE_SIZE] = {0};
	uint8_t decoded[sizeof(struct sensor_three_axis_data) +
			(FIFO_SAMPLES - 1U) * sizeof(struct sensor_three_axis_sample_data)]
		__aligned(8);
	struct lis2dh_encoded_header *header = (void *)buffer;
	struct sensor_three_axis_data *data = (void *)decoded;
	struct sensor_chan_spec channel = {SENSOR_CHAN_ACCEL_XYZ, 0};
	uint32_t fit = 0U;

	header->timestamp_ns = UINT64_C(32000000000);
	header->period_ns = UINT64_C(1000000000);
	header->sample_count = FIFO_SAMPLES;
	header->shift = 5;
	while (fit < FIFO_SAMPLES) {
		uint32_t first = fit;
		int count = lis2dh_decoder_decode(buffer, channel, &fit, FIFO_SAMPLES, data);

		zassert_equal(count, MIN(5U, FIFO_SAMPLES - first));
		zassert_equal(data->header.base_timestamp_ns, (first + 1U) * UINT64_C(1000000000));
		zassert_equal(data->readings[count - 1].timestamp_delta,
			      (count - 1U) * UINT64_C(1000000000));
	}
}

ZTEST(lis2dh_decoder, test_runtime_scales)
{
	const uint32_t scales[] = {9576U, 19153U, 38306U, 114921U};

	for (size_t i = 0; i < ARRAY_SIZE(scales); i++) {
		int8_t shift = lis2dh_encoded_shift(scales[i]);
		q31_t positive = lis2dh_decode_accel(INT16_MAX, scales[i], shift);
		q31_t negative = lis2dh_decode_accel(INT16_MIN, scales[i], shift);
		int64_t recovered =
			((int64_t)positive * 1000000 * (INT64_C(1) << shift)) / (INT64_C(1) << 31);

		zassert_true(positive > 0);
		zassert_true(negative < 0);
		zassert_within(recovered, (int64_t)2047 * scales[i], 1);
	}
}

ZTEST_SUITE(lis2dh_decoder, NULL, NULL, NULL, NULL, NULL);
