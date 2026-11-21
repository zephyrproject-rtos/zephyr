/*
 * Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "max30210.h"

#include <zephyr/logging/log.h>
#include <zephyr/ztest.h>

LOG_MODULE_REGISTER(MAX30210, CONFIG_SENSOR_LOG_LEVEL);

struct fifo_buffer {
	struct max30210_fifo_data header;
	uint8_t samples[MAX30210_FIFO_DEPTH * MAX30210_BYTES_PER_SAMPLE];
};

static const struct sensor_chan_spec temperature = {
	.chan_type = SENSOR_CHAN_AMBIENT_TEMP,
	.chan_idx = 0,
};

static void check_batch(uint64_t period_ns, uint16_t max_count)
{
	struct fifo_buffer buffer = {
		.header = {
			.timestamp_ns = UINT64_C(5000000000000),
			.sample_period_ns = period_ns,
			.sample_count = MAX30210_FIFO_DEPTH,
		},
	};
	uint8_t output[sizeof(struct sensor_q31_data) +
		       MAX30210_FIFO_DEPTH * sizeof(struct sensor_q31_sample_data)]
		__aligned(8);
	struct sensor_q31_data *decoded = (struct sensor_q31_data *)output;
	const struct sensor_decoder_api *decoder;
	uint64_t first_timestamp = buffer.header.timestamp_ns -
				   (MAX30210_FIFO_DEPTH - 1) * period_ns;
	uint32_t fit = 0;
	uint16_t frames;

	for (uint16_t i = 0; i < MAX30210_FIFO_DEPTH; i++) {
		int16_t raw = ((int16_t)i - 32) * MAX30210_TEMP_COUNTS_PER_C;

		sys_put_be16(raw, &buffer.samples[i * MAX30210_BYTES_PER_SAMPLE + 1]);
	}

	zassert_ok(max30210_get_decoder(NULL, &decoder));
	zassert_ok(decoder->get_frame_count((const uint8_t *)&buffer, temperature, &frames));
	zassert_equal(frames, MAX30210_FIFO_DEPTH);

	while (fit < frames) {
		uint32_t start = fit;
		uint16_t expected = MIN(max_count, frames - start);
		int count = decoder->decode((const uint8_t *)&buffer, temperature, &fit,
					    max_count, decoded);

		expected = MIN(expected, UINT32_MAX / period_ns + 1);
		zassert_equal(count, expected);
		zassert_equal(fit, start + count);
		zassert_equal(decoded->header.reading_count, count);
		zassert_equal(decoded->header.base_timestamp_ns,
			      first_timestamp + start * period_ns);
		for (int i = 0; i < count; i++) {
			int32_t degrees = (int32_t)start + i - 32;
			const uint8_t *sample =
				&buffer.samples[(start + i) * MAX30210_BYTES_PER_SAMPLE];
			int32_t raw = sign_extend(sys_get_be16(sample + 1), 15);

			zassert_equal(decoded->readings[i].timestamp_delta, i * period_ns);
			zassert_equal(decoded->header.base_timestamp_ns +
					      decoded->readings[i].timestamp_delta,
				      first_timestamp + (start + i) * period_ns);
			int64_t expected_temp = degrees * (INT64_C(1) << (31 - decoded->shift));

			zassert_equal(decoded->readings[i].temperature, expected_temp,
				      "sample %u: bytes %02x %02x %02x, raw %d, decoded %d, "
				      "expected %lld",
				      (unsigned int)(start + i), sample[0], sample[1], sample[2],
				      raw,
				      decoded->readings[i].temperature,
				      (long long)expected_temp);
		}
	}
	zassert_equal(decoder->decode((const uint8_t *)&buffer, temperature, &fit, max_count,
				     decoded),
		      -ENODATA);
}

ZTEST(max30210_decoder, test_q31_scaling)
{
	volatile int32_t raw = -6400;
	int64_t scaled = ((int64_t)raw * (INT64_C(1) << 23)) / MAX30210_TEMP_COUNTS_PER_C;

	zassert_equal(scaled, -INT64_C(268435456), "raw %d, scaled %lld",
		      (int)raw, (long long)scaled);
}

ZTEST(max30210_decoder, test_full_fifo_at_8_hz)
{
	check_batch(UINT64_C(125000000), MAX30210_FIFO_DEPTH);
}

ZTEST(max30210_decoder, test_single_frame_iteration)
{
	check_batch(UINT64_C(125000000), 1);
}

ZTEST(max30210_decoder, test_slowest_sampling_rate)
{
	check_batch(UINT64_C(64000000000), MAX30210_FIFO_DEPTH);
}

ZTEST(max30210_decoder, test_timestamp_underflow)
{
	struct fifo_buffer buffer = {
		.header = {
			.timestamp_ns = 1,
			.sample_period_ns = UINT64_C(125000000),
			.sample_count = 2,
		},
	};
	struct sensor_q31_data decoded;
	const struct sensor_decoder_api *decoder;
	uint32_t fit = 0;

	zassert_ok(max30210_get_decoder(NULL, &decoder));
	zassert_equal(decoder->decode((const uint8_t *)&buffer, temperature, &fit, 1, &decoded),
		      -ERANGE);
	zassert_equal(fit, 0);
}

ZTEST_SUITE(max30210_decoder, NULL, NULL, NULL, NULL, NULL);
