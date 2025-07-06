/*
 * Copyright 2025 CogniPilot Foundation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <math.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#define BATCH_DURATION 50

double q31_to_double(int32_t q31_value, int8_t shift)
{
	return ((double)q31_value) / (double)(1 << (31 - shift));
}

static struct rtio_sqe *streaming_handle;

static struct sensor_stream_trigger stream_trigger = {
	.opt = SENSOR_STREAM_DATA_INCLUDE,
	.trigger = SENSOR_TRIG_FIFO_WATERMARK,
};

static struct sensor_read_config stream_config = {
	.sensor = DEVICE_DT_GET(DT_NODELABEL(icm42688_1)),
	.is_streaming = true,
	.triggers = &stream_trigger,
	.count = 0,
	.max = 1,
};

static RTIO_IODEV_DEFINE(iodev_stream, &__sensor_iodev_api, &stream_config);
RTIO_DEFINE_WITH_MEMPOOL(rtio, 4, 4, 32, 64, 4);

static void callback(int result, uint8_t *buf, uint32_t buf_len, void *userdata)
{
	static uint8_t decoded_buffer[127];

	zassert_not_null(stream_config.sensor->api);
	const struct sensor_decoder_api *decoder;

	zassert_ok(sensor_get_decoder(stream_config.sensor, &decoder));
	int channels[] = {
		SENSOR_CHAN_GYRO_XYZ,
		SENSOR_CHAN_ACCEL_XYZ,
	};

	for (int i = 0; i < ARRAY_SIZE(channels); i++) {
		int chan = channels[i];
		struct sensor_chan_spec ch = {
			.chan_idx = 0,
			.chan_type = chan,
		};

		uint32_t fit = 0;
		int count = 0;

		while (decoder->decode(buf, ch, &fit, 1, decoded_buffer) > 0) {
			zassert(count++ < 127, "fifo overflow");
			if (ch.chan_type == SENSOR_CHAN_ACCEL_XYZ) {
				struct sensor_three_axis_data *data =
					(struct sensor_three_axis_data *)decoded_buffer;
				double x = q31_to_double(data->readings[0].values[0], data->shift);
				double y = q31_to_double(data->readings[0].values[1], data->shift);
				double z = q31_to_double(data->readings[0].values[2], data->shift);

				zassert(fabs(x) < 1.0, "fail", "accel x out of range: %10.4f", x);
				zassert(fabs(y) < 1.0, "fail", "accel y out of range: %10.4f", y);
				zassert(fabs(z - 9.8) < 1.0, "fail",
					"accel z out of range: %10.4f", z);
			} else if (ch.chan_type == SENSOR_CHAN_GYRO_XYZ) {
				struct sensor_three_axis_data *data =
					(struct sensor_three_axis_data *)decoded_buffer;
				double x = q31_to_double(data->readings[0].values[0], data->shift);
				double y = q31_to_double(data->readings[0].values[1], data->shift);
				double z = q31_to_double(data->readings[0].values[2], data->shift);

				zassert(fabs(x) < 0.1, "fail", "gyro x out of range: %10.4f", x);
				zassert(fabs(y) < 0.1, "fail", "gyro y out of range: %10.4f", y);
				zassert(fabs(z) < 0.1, "fail", "gyro z out of range: %10.4f", z);
			}
		}
	}
}

ZTEST(icm42688_stream, test_icm42688_stream)
{
	stream_config.count = 1;
	struct sensor_value val_in = { BATCH_DURATION, 0 };

	zassert_ok(sensor_attr_set(
		stream_config.sensor,
		SENSOR_CHAN_ALL,
		SENSOR_ATTR_BATCH_DURATION, &val_in));
	struct sensor_value val_out;

	zassert_ok(sensor_attr_get(
		stream_config.sensor,
		SENSOR_CHAN_ALL,
		SENSOR_ATTR_BATCH_DURATION, &val_out));
	zassert_equal(val_in.val1, val_out.val1);
	zassert_ok(sensor_stream(&iodev_stream, &rtio, NULL,
		&streaming_handle));
	sensor_processing_with_callback(&rtio, callback);
	zassert_ok(rtio_sqe_cancel(streaming_handle));
}

static void *icm42688_stream_setup(void)
{
	return NULL;
}


ZTEST_SUITE(icm42688_stream, NULL, icm42688_stream_setup, NULL, NULL, NULL);
