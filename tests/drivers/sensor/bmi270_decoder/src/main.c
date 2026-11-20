/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <string.h>

#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

#include "bmi270_decoder.h"

#define FRAME_BYTES 12
#define MAX_FRAMES  8
#define ODR_HZ      100
#define PERIOD_NS   (1000000000U / ODR_HZ)

/* Headerless FIFO frame: gyroscope x, y, z then accelerometer x, y, z. */
struct frame {
	int16_t gyr[3];
	int16_t acc[3];
};

/* Dummy frames the BMI270 writes in headerless mode for an element without a valid sample. */
static const struct frame acc_dummy = {.gyr = {1, 2, 3}, .acc = {0x7F01, -32768, -32768}};
static const struct frame gyr_dummy = {.gyr = {0x7F02, -32768, -32768}, .acc = {4, 5, 6}};
static const struct frame both_dummy = {.gyr = {0x7F02, -32768, -32768},
					.acc = {0x7F01, -32768, -32768}};

static uint8_t block[sizeof(struct bmi270_fifo_encoded_data) + MAX_FRAMES * FRAME_BYTES]
	__aligned(8);
static uint8_t reference[sizeof(block)] __aligned(8);

static const struct sensor_chan_spec acc_chan = {SENSOR_CHAN_ACCEL_XYZ, 0};
static const struct sensor_chan_spec gyr_chan = {SENSOR_CHAN_GYRO_XYZ, 0};

static const struct sensor_decoder_api *decoder(void)
{
	const struct sensor_decoder_api *api;

	zassert_ok(bmi270_get_decoder(NULL, &api));
	return api;
}

static struct frame sample(int16_t value)
{
	return (struct frame){.gyr = {value, value, value}, .acc = {value, value, value}};
}

/* Fills buf with a headerless FIFO block at 100 Hz, 4 g, 2000 dps holding the given frames. */
static void fill(uint8_t *buf, const struct frame *frames, size_t count)
{
	struct bmi270_fifo_encoded_data *edata = (struct bmi270_fifo_encoded_data *)buf;

	memset(buf, 0, sizeof(block));
	edata->header.timestamp = 1000000;
	edata->header.is_fifo = 1;
	edata->header.is_headerless = 1;
	edata->header.acc_range = 1;
	edata->header.gyr_range_idx = 0;
	edata->header.acc_odr_hz = ODR_HZ;
	edata->header.gyr_odr_hz = ODR_HZ;
	edata->fifo_byte_count = count * FRAME_BYTES;

	for (size_t i = 0; i < count; i++) {
		uint8_t *f = &edata->fifo_data[i * FRAME_BYTES];

		for (int axis = 0; axis < 3; axis++) {
			sys_put_le16(frames[i].gyr[axis], &f[axis * 2]);
			sys_put_le16(frames[i].acc[axis], &f[6 + axis * 2]);
		}
	}
}

/* Decoder output for up to MAX_FRAMES readings; readings[] continues past its declared size. */
struct decoded {
	struct sensor_three_axis_data data;
	struct sensor_three_axis_sample_data more[MAX_FRAMES - 1];
};

static const struct sensor_three_axis_sample_data *reading(const struct decoded *out, int i)
{
	const uint8_t *first =
		(const uint8_t *)out + offsetof(struct sensor_three_axis_data, readings);

	return (const struct sensor_three_axis_sample_data *)first + i;
}

static void assert_same_values(const struct decoded *out, const struct decoded *ref, int i)
{
	zassert_equal(reading(out, i)->x, reading(ref, i)->x, "sample %d", i);
	zassert_equal(reading(out, i)->y, reading(ref, i)->y, "sample %d", i);
	zassert_equal(reading(out, i)->z, reading(ref, i)->z, "sample %d", i);
}

static uint16_t frame_count(const uint8_t *buf, struct sensor_chan_spec chan)
{
	uint16_t count;

	zassert_ok(decoder()->get_frame_count(buf, chan, &count));
	return count;
}

ZTEST(bmi270_decoder, test_headerless_without_dummy_frames)
{
	const struct frame frames[] = {sample(100), sample(200), sample(300)};
	struct decoded out;
	uint32_t fit = 0;

	fill(block, frames, ARRAY_SIZE(frames));

	zassert_equal(frame_count(block, acc_chan), 3);
	zassert_equal(decoder()->decode(block, acc_chan, &fit, 3, &out), 3);
	zassert_equal(fit, 3);
	zassert_not_equal(reading(&out, 0)->x, 0);
	for (int i = 0; i < 3; i++) {
		/* The raw values are 100, 200 and 300 on every axis and the scale is linear. */
		zassert_equal(reading(&out, i)->x, (i + 1) * reading(&out, 0)->x, "sample %d", i);
		zassert_equal(reading(&out, i)->y, (i + 1) * reading(&out, 0)->y, "sample %d", i);
		zassert_equal(reading(&out, i)->z, (i + 1) * reading(&out, 0)->z, "sample %d", i);
		zassert_equal(reading(&out, i)->timestamp_delta, i * PERIOD_NS, "sample %d", i);
	}
}

ZTEST(bmi270_decoder, test_headerless_accel_dummy_frames_are_skipped)
{
	const struct frame frames[] = {acc_dummy, acc_dummy, sample(100), sample(200)};
	const struct frame expected[] = {sample(100), sample(200)};
	struct decoded out, ref;
	uint32_t fit = 0, ref_fit = 0;

	fill(block, frames, ARRAY_SIZE(frames));
	fill(reference, expected, ARRAY_SIZE(expected));

	zassert_equal(frame_count(block, acc_chan), 2);
	zassert_equal(frame_count(block, gyr_chan), 4,
		      "gyroscope samples in accel dummy frames are valid");

	zassert_equal(decoder()->decode(block, acc_chan, &fit, 4, &out), 2);
	zassert_equal(decoder()->decode(reference, acc_chan, &ref_fit, 4, &ref), 2);
	zassert_equal(fit, 2);
	for (int i = 0; i < 2; i++) {
		assert_same_values(&out, &ref, i);
		/* The skipped frames still took one sample period each. */
		zassert_equal(reading(&out, i)->timestamp_delta, (2 + i) * PERIOD_NS, "sample %d",
			      i);
	}
}

ZTEST(bmi270_decoder, test_headerless_gyro_dummy_frames_are_skipped)
{
	const struct frame frames[] = {gyr_dummy, sample(100), gyr_dummy, sample(200)};
	const struct frame expected[] = {sample(100), sample(200)};
	struct decoded out, ref;
	uint32_t fit = 0, ref_fit = 0;

	fill(block, frames, ARRAY_SIZE(frames));
	fill(reference, expected, ARRAY_SIZE(expected));

	zassert_equal(frame_count(block, gyr_chan), 2);
	zassert_equal(frame_count(block, acc_chan), 4,
		      "accelerometer samples in gyro dummy frames are valid");

	zassert_equal(decoder()->decode(block, gyr_chan, &fit, 4, &out), 2);
	zassert_equal(decoder()->decode(reference, gyr_chan, &ref_fit, 4, &ref), 2);
	for (int i = 0; i < 2; i++) {
		assert_same_values(&out, &ref, i);
	}
	zassert_equal(reading(&out, 0)->timestamp_delta, 1 * PERIOD_NS);
	zassert_equal(reading(&out, 1)->timestamp_delta, 3 * PERIOD_NS);
}

ZTEST(bmi270_decoder, test_headerless_frames_dummy_for_both_elements_are_skipped)
{
	const struct frame frames[] = {sample(100), both_dummy, sample(200)};
	const struct frame expected[] = {sample(100), sample(200)};
	const struct sensor_chan_spec chans[] = {acc_chan, gyr_chan};

	fill(block, frames, ARRAY_SIZE(frames));
	fill(reference, expected, ARRAY_SIZE(expected));

	for (int c = 0; c < (int)ARRAY_SIZE(chans); c++) {
		struct decoded out, ref;
		uint32_t fit = 0, ref_fit = 0;

		zassert_equal(frame_count(block, chans[c]), 2, "channel %d", c);
		zassert_equal(decoder()->decode(block, chans[c], &fit, 4, &out), 2, "channel %d",
			      c);
		zassert_equal(decoder()->decode(reference, chans[c], &ref_fit, 4, &ref), 2,
			      "channel %d", c);
		for (int i = 0; i < 2; i++) {
			assert_same_values(&out, &ref, i);
		}
		zassert_equal(reading(&out, 0)->timestamp_delta, 0, "channel %d", c);
		zassert_equal(reading(&out, 1)->timestamp_delta, 2 * PERIOD_NS, "channel %d", c);
	}
}

ZTEST(bmi270_decoder, test_headerless_trailing_partial_frame_is_ignored)
{
	const struct frame frames[] = {sample(100), sample(200)};
	struct bmi270_fifo_encoded_data *edata = (struct bmi270_fifo_encoded_data *)block;
	struct decoded out;
	uint32_t fit = 0;

	fill(block, frames, ARRAY_SIZE(frames));
	edata->fifo_byte_count += FRAME_BYTES / 2;

	zassert_equal(frame_count(block, acc_chan), 2);
	zassert_equal(frame_count(block, gyr_chan), 2);
	zassert_equal(decoder()->decode(block, acc_chan, &fit, 4, &out), 2);
	zassert_equal(fit, 2);
}

ZTEST(bmi270_decoder, test_headerless_decode_in_steps_across_dummy_frames)
{
	const struct frame frames[] = {sample(100), acc_dummy, sample(200), sample(300)};
	const uint32_t expected_delta[] = {0, 2 * PERIOD_NS, 3 * PERIOD_NS};
	struct decoded out;
	uint32_t fit = 0;

	fill(block, frames, ARRAY_SIZE(frames));

	for (int i = 0; i < 3; i++) {
		zassert_equal(decoder()->decode(block, acc_chan, &fit, 1, &out), 1, "step %d", i);
		zassert_equal(fit, i + 1, "step %d", i);
		zassert_equal(reading(&out, 0)->timestamp_delta, expected_delta[i], "step %d", i);
	}
	zassert_equal(decoder()->decode(block, acc_chan, &fit, 1, &out), 0);
}

ZTEST_SUITE(bmi270_decoder, NULL, NULL, NULL, NULL, NULL);
