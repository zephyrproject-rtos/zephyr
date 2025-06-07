/*
 * Copyright (c) 2025 COGNID Telematik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Unit test for async sensor simulator
 */

#include <math.h>

#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/ztest.h>

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/sensor_sim_async.h>

#define TEST_ODR 100 /*Hz*/

LOG_MODULE_REGISTER(test_sensor_sim_async, LOG_LEVEL_DBG);

#define SENSOR_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(zephyr_sensor_sim_async)
const struct device *dev = DEVICE_DT_GET(SENSOR_NODE);

static void *setup(void)
{
	struct sensor_value val = {TEST_ODR, 0};

	sensor_attr_set(dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &val);

	return NULL;
}

ZTEST_SUITE(framework_tests, NULL, setup, NULL, NULL, NULL);

SENSOR_DT_READ_IODEV(sensor_dev, SENSOR_NODE, {SENSOR_CHAN_ACCEL_XYZ, 0});
RTIO_DEFINE(sensor_ctx, 1, 1);

/**
 * \brief Test blocking read with sensor_read()
 */
ZTEST(framework_tests, test_one_shot_read)
{
	uint8_t buf[64];
	struct sensor_three_axis_data sensor_data = {0};
	struct sensor_decode_context sensor_decoder = SENSOR_DECODE_CONTEXT_INIT(
		SENSOR_DECODER_DT_GET(SENSOR_NODE), buf, SENSOR_CHAN_ACCEL_XYZ, 0);

	for (int i = 0; i < 32; ++i) {
		int rc;
		const struct sensor_sim_async_sensor_fifo_sample sample = {
			.x = (i / 10.0 + 0.1) * CONFIG_SENSOR_SIM_ASYNC_SCALE,
			.y = (i / 10.0 + 0.2) * CONFIG_SENSOR_SIM_ASYNC_SCALE,
			.z = (i / 10.0 + 0.3) * CONFIG_SENSOR_SIM_ASYNC_SCALE};

		sensor_sim_async_feed_data(dev, &sample, 1, i * 1000000000ULL / 100,
					   SENSOR_CHAN_ACCEL_XYZ);

		/* Blocking read */
		rc = sensor_read(&sensor_dev, &sensor_ctx, buf, sizeof(buf));
		zassert_ok(rc, "sensor_read() failed");

		/* Decode the data into a single q31 */
		sensor_decoder.fit = 0;
		rc = sensor_decode(&sensor_decoder, &sensor_data, 1);
		zassert_ok(rc, "sensor_decode() failed");
		zassert_equal(sensor_data.header.reading_count, 1);
		zassert_equal(sensor_data.header.base_timestamp_ns, i * 1000000000ULL / 100);

		zassert_within(sensor_data.readings[0].x * powf(2.0f, sensor_data.shift - 31),
			       sample.x / (float)CONFIG_SENSOR_SIM_ASYNC_SCALE,
			       1.0f / CONFIG_SENSOR_SIM_ASYNC_SCALE);
		zassert_within(sensor_data.readings[0].y * powf(2.0f, sensor_data.shift - 31),
			       sample.y / (float)CONFIG_SENSOR_SIM_ASYNC_SCALE,
			       1.0f / CONFIG_SENSOR_SIM_ASYNC_SCALE);
		zassert_within(sensor_data.readings[0].z * powf(2.0f, sensor_data.shift - 31),
			       sample.z / (float)CONFIG_SENSOR_SIM_ASYNC_SCALE,
			       1.0f / CONFIG_SENSOR_SIM_ASYNC_SCALE);
	}
}

SENSOR_DT_STREAM_IODEV(sensor_iodev_stream, SENSOR_NODE,
		       {SENSOR_TRIG_FIFO_WATERMARK, SENSOR_STREAM_DATA_INCLUDE});
RTIO_DEFINE_WITH_MEMPOOL(sensor_ctx_mempool, 1, 1, 1, 512, sizeof(void *));

/**
 * \brief Test streaming
 */
ZTEST(framework_tests, test_streaming)
{
	const struct sensor_decoder_api *decoder = SENSOR_DECODER_DT_GET(SENSOR_NODE);
	/* Select as many samples as the fifo watermark, so every sample feed results in one CQE */
	static struct sensor_sim_async_sensor_fifo_sample test_data[DT_PROP(SENSOR_NODE, fifo_wm)];

	/* Start the streams */
	struct rtio_sqe *handle = NULL;

	sensor_stream(&sensor_iodev_stream, &sensor_ctx_mempool, (void *)0x12345678, &handle);

	sensor_sim_async_flush_fifo(dev);

	for (int i = 0; i < 10; ++i) {
		/* Trigger once in a while to check that later with has_trigger() */
		bool tap_triggered = i % 3;

		if (tap_triggered) {
			sensor_sim_async_trigger(dev, SENSOR_TRIG_TAP);
		}

		/* Feed test data to simulated sensor */
		for (int j = 0; j < ARRAY_SIZE(test_data); ++j) {
			test_data[j].x = (i + j / (float)(ARRAY_SIZE(test_data)) + 0.01f) *
					 CONFIG_SENSOR_SIM_ASYNC_SCALE;
			test_data[j].y = (i + j / (float)(ARRAY_SIZE(test_data)) + 0.02f) *
					 CONFIG_SENSOR_SIM_ASYNC_SCALE;
			test_data[j].z = (i + j / (float)(ARRAY_SIZE(test_data)) + 0.03f) *
					 CONFIG_SENSOR_SIM_ASYNC_SCALE;
		}
		sensor_sim_async_feed_data(dev, test_data, ARRAY_SIZE(test_data) - 10,
					   i == 0 ? 0 : -1, SENSOR_CHAN_ACCEL_XYZ);
		sensor_sim_async_feed_data(dev, &test_data[ARRAY_SIZE(test_data) - 10], 10, -1,
					   SENSOR_CHAN_ACCEL_XYZ);

		struct sensor_value val[3];

		zassert_ok(sensor_channel_get(dev, SENSOR_CHAN_ACCEL_XYZ, val));
		zassert_within(val[0].val1 + val[0].val2 / 1000000.0f,
			       test_data[ARRAY_SIZE(test_data) - 1].x /
				       (float)CONFIG_SENSOR_SIM_ASYNC_SCALE,
			       1.0f / CONFIG_SENSOR_SIM_ASYNC_SCALE);
		zassert_within(val[1].val1 + val[1].val2 / 1000000.0f,
			       test_data[ARRAY_SIZE(test_data) - 1].y /
				       (float)CONFIG_SENSOR_SIM_ASYNC_SCALE,
			       1.0f / CONFIG_SENSOR_SIM_ASYNC_SCALE);
		zassert_within(val[2].val1 + val[2].val2 / 1000000.0f,
			       test_data[ARRAY_SIZE(test_data) - 1].z /
				       (float)CONFIG_SENSOR_SIM_ASYNC_SCALE,
			       1.0f / CONFIG_SENSOR_SIM_ASYNC_SCALE);

		/* Wait for a CQE */
		struct rtio_cqe *cqe = rtio_cqe_consume_block(&sensor_ctx_mempool);

		/* Cache the data from the CQE */
		int rc = cqe->result;
		void *userdata = cqe->userdata;

		zassert_ok(rc);
		zassert_equal(userdata, (void *)0x12345678);

		uint8_t *buf = NULL;
		uint32_t buf_len = 0;

		rtio_cqe_get_mempool_buffer(&sensor_ctx_mempool, cqe, &buf, &buf_len);

		/* Release the CQE */
		rtio_cqe_release(&sensor_ctx_mempool, cqe);

		uint16_t frame_count = 0;
		struct sensor_chan_spec chan_spec = {SENSOR_CHAN_ACCEL_XYZ, 0};

		rc = decoder->get_frame_count(buf, chan_spec, &frame_count);
		zassert_ok(rc, "get_frame_count() failed");
		zassert_equal(frame_count, ARRAY_SIZE(test_data));

		size_t base_size, frame_size;

		zassert_ok(decoder->get_size_info(chan_spec, &base_size, &frame_size));

		static uint8_t __aligned(sizeof(void *)) buf2[1024];

		zassert_between_inclusive(base_size + frame_size * (frame_count - 1),
					  3 * sizeof(q31_t) * ARRAY_SIZE(test_data),
					  ARRAY_SIZE(buf2));

		struct sensor_decode_context ctx = SENSOR_DECODE_CONTEXT_INIT(
			SENSOR_DECODER_DT_GET(SENSOR_NODE), buf, SENSOR_CHAN_ACCEL_XYZ, 0);

		sensor_decode(&ctx, buf2, frame_count);

		struct sensor_three_axis_data *decoded_data = (struct sensor_three_axis_data *)buf2;

		zassert_equal(decoded_data->header.reading_count, ARRAY_SIZE(test_data));
		zassert_equal((int64_t)decoded_data->header.base_timestamp_ns,
			      i * ARRAY_SIZE(test_data) * 1000000000ULL / 100);
		for (int j = 0; j < decoded_data->header.reading_count; ++j) {
			zassert_equal((int64_t)decoded_data->readings[j].timestamp_delta,
				      j * 1000000000 / TEST_ODR);
			zassert_within((float)decoded_data->readings[j].x *
					       powf(2.0f, decoded_data->shift - 31),
				       test_data[j].x / (float)CONFIG_SENSOR_SIM_ASYNC_SCALE,
				       1.0f / CONFIG_SENSOR_SIM_ASYNC_SCALE);
			zassert_within((float)decoded_data->readings[j].y *
					       powf(2.0f, decoded_data->shift - 31),
				       test_data[j].y / (float)CONFIG_SENSOR_SIM_ASYNC_SCALE,
				       1.0f / CONFIG_SENSOR_SIM_ASYNC_SCALE);
			zassert_within((float)decoded_data->readings[j].z *
					       powf(2.0f, decoded_data->shift - 31),
				       test_data[j].z / (float)CONFIG_SENSOR_SIM_ASYNC_SCALE,
				       1.0f / CONFIG_SENSOR_SIM_ASYNC_SCALE);
		}

		zassert_equal(decoder->has_trigger(buf, SENSOR_TRIG_TAP), tap_triggered);

		/* Release the memory */
		rtio_release_buffer(&sensor_ctx_mempool, buf, buf_len);

		zassert_false(handle->flags & RTIO_SQE_CANCELED);
	}

	/* test canceling */
	struct rtio_iodev_sqe *iodev_sqe = CONTAINER_OF(handle, struct rtio_iodev_sqe, sqe);

	rtio_iodev_sqe_err(iodev_sqe, -ECANCELED);
	rtio_sqe_cancel(handle);

	struct rtio_cqe *cqe = rtio_cqe_consume_block(&sensor_ctx_mempool);

	zassert_equal(cqe->result, -ECANCELED);
	rtio_cqe_release(&sensor_ctx_mempool, cqe);
	zassert_true(handle->flags & RTIO_SQE_CANCELED);
}

/**
 * \brief Test fetch and get of single values
 */
ZTEST(framework_tests, test_single_get)
{
	struct sensor_value val;

	zassert_ok(sensor_sim_async_set_channel(dev, SENSOR_CHAN_CO2, 26.123456f));
	zassert_ok(sensor_channel_get(dev, SENSOR_CHAN_CO2, &val));
	zassert_equal(val.val1, 26);
	zassert_within(val.val2, 123456, 1000000 / CONFIG_SENSOR_SIM_ASYNC_SCALE);

	zassert_ok(sensor_sim_async_set_channel(dev, SENSOR_CHAN_VOLTAGE, 11.666999f));
	zassert_ok(sensor_channel_get(dev, SENSOR_CHAN_VOLTAGE, &val));
	zassert_equal(val.val1, 11);
	zassert_within(val.val2, 666999, 1000000 / CONFIG_SENSOR_SIM_ASYNC_SCALE);
}
