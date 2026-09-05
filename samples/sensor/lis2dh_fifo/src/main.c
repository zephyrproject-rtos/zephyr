/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <inttypes.h>
#include <stdio.h>

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/rtio/rtio.h>

#define ACCEL_STREAM_TRIGGERS                                                                      \
	{SENSOR_TRIG_FIFO_WATERMARK, SENSOR_STREAM_DATA_INCLUDE},                                  \
	{                                                                                          \
		SENSOR_TRIG_FIFO_FULL, SENSOR_STREAM_DATA_INCLUDE                                  \
	}

SENSOR_DT_STREAM_IODEV(accel_stream, DT_ALIAS(accel0), ACCEL_STREAM_TRIGGERS);
RTIO_DEFINE_WITH_MEMPOOL(stream_ctx, 1, 1, 20, 256, sizeof(void *));

static const struct device *const accel = DEVICE_DT_GET(DT_ALIAS(accel0));

static void print_fifo_samples(const uint8_t *buffer)
{
	const struct sensor_decoder_api *decoder;
	struct sensor_three_axis_data accel_data;
	struct sensor_chan_spec channel = {SENSOR_CHAN_ACCEL_XYZ, 0};
	uint16_t frame_count;
	uint32_t fit = 0U;
	int status;

	status = sensor_get_decoder(accel, &decoder);
	if (status < 0) {
		printf("Cannot get LIS2DH decoder: %d\n", status);
		return;
	}

	status = decoder->get_frame_count(buffer, channel, &frame_count);
	if (status < 0) {
		printf("Cannot get FIFO frame count: %d\n", status);
		return;
	}

	while (fit < frame_count) {
		status = decoder->decode(buffer, channel, &fit, 1U, &accel_data);
		if (status < 0) {
			printf("Cannot decode FIFO frame: %d\n", status);
			return;
		}

		printf("%" PRIu64 " ns: (%" PRIq(6) ", %" PRIq(6) ", %" PRIq(6) ") m/s^2\n",
		       PRIsensor_three_axis_data_arg(accel_data, 0));
	}
}

int main(void)
{
	struct rtio_sqe *handle;
	struct rtio_cqe *cqe;
	uint8_t *buffer = NULL;
	uint32_t buffer_len = 0U;
	int status;

	if (!device_is_ready(accel)) {
		printf("LIS2DH device is not ready\n");
		return 0;
	}

	status = sensor_stream(&accel_stream, &stream_ctx, NULL, &handle);
	if (status < 0) {
		printf("Cannot start LIS2DH FIFO stream: %d\n", status);
		return 0;
	}

	printf("LIS2DH Sensor Async FIFO stream started\n");

	while (true) {
		cqe = rtio_cqe_consume_block(&stream_ctx);
		status = cqe->result;
		if (status == 0) {
			status =
				rtio_cqe_get_mempool_buffer(&stream_ctx, cqe, &buffer, &buffer_len);
		}
		rtio_cqe_release(&stream_ctx, cqe);
		if (status < 0) {
			printf("FIFO stream failed: %d\n", status);
			return 0;
		}

		print_fifo_samples(buffer);
		rtio_release_buffer(&stream_ctx, buffer, buffer_len);
	}
}
