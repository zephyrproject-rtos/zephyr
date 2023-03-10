/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/rtio/rtio_executor_simple.h>

#include <stdint.h>
#include <stdio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(TDK_ROBOKIT1, CONFIG_SENSOR_LOG_LEVEL);

/* Power of 2 number of stream buffers */
#define N_BUFS 4

RTIO_EXECUTOR_SIMPLE_DEFINE(r_exec);

/* Show no over/under flows with a 4 buffer queue */
RTIO_DEFINE(r4, (struct rtio_executor *)&r_exec, N_BUFS, N_BUFS);

/* Show over/underflows with a single buffer queue */
RTIO_DEFINE(r1, (struct rtio_executor *)&r_exec, 1, 1);

#define FIFO_ITERS 4096

struct fifo_header {
	uint8_t int_status;
	uint16_t gyro_odr: 4;
	uint16_t accel_odr: 4;
	uint16_t gyro_fs: 3;
	uint16_t accel_fs: 3;
	uint16_t packet_format: 2;
} __packed;

/* The fifo size (2048) bytes plus 3 for the header */
#define ICM42688_FIFO_BUF_LEN 2051
static uint8_t icm42688_fifo_bufs[N_BUFS][ICM42688_FIFO_BUF_LEN];

const struct device *icm42688 = DEVICE_DT_GET_ONE(invensense_icm42688);
const struct device *akm09918 = DEVICE_DT_GET_ONE(asahi_kasei_akm09918c);

void poll_imu(void)
{
	uint32_t cycle_start, cycle_end;
	struct sensor_value accel[3];
	struct sensor_value gyro[3];
	struct sensor_value imu_temp;

	/* Fetch everything */
	cycle_start = k_cycle_get_32();
	sensor_sample_fetch_chan(icm42688, SENSOR_CHAN_ALL);
	cycle_end = k_cycle_get_32();

	sensor_channel_get(icm42688, SENSOR_CHAN_ACCEL_XYZ, accel);
	sensor_channel_get(icm42688, SENSOR_CHAN_GYRO_XYZ, gyro);
	sensor_channel_get(icm42688, SENSOR_CHAN_DIE_TEMP, &imu_temp);

	LOG_INF("ICM42688: Fetch took %u cycles", cycle_end - cycle_start);
	LOG_INF("ICM42688: Accel (m/s^2): x: %d.%06d, y: %d.%06d, z: %d.%06d",
		accel[0].val1, accel[0].val2,
		accel[1].val1, accel[1].val2,
		accel[2].val1, accel[2].val2);
	LOG_INF("ICM42688: Gyro (rad/s): x: %d.%06d, y: %d.%06d, z: %d.%06d",
		gyro[0].val1, gyro[0].val2,
		gyro[1].val1, gyro[1].val2,
		gyro[2].val1, gyro[2].val2);
	LOG_INF("ICM42688: Temp (C): %d.%06d",
		imu_temp.val1, imu_temp.val2);
}

void poll_mag(void)
{
	uint32_t cycle_start, cycle_end;
	struct sensor_value magn[3];

	/* Fetch everything */
	cycle_start = k_cycle_get_32();
	sensor_sample_fetch_chan(akm09918, SENSOR_CHAN_ALL);
	cycle_end = k_cycle_get_32();

	sensor_channel_get(akm09918, SENSOR_CHAN_MAGN_XYZ, magn);

	LOG_INF("AKM09918: Fetch took %u cycles", cycle_end - cycle_start);
	LOG_INF("AKM09918: Mag (Gauss): x: %d.%06d, y: %d.%06d, z: %d.%06d",
		magn[0].val1, magn[0].val2,
		magn[1].val1, magn[1].val2,
		magn[2].val1, magn[2].val2);
}

void fifo_stream(struct rtio *r, uint32_t n_bufs)
{
	struct rtio_iodev *iodev;
	struct rtio_sqe *sqe;

	LOG_INF("FIFO with RTIO context %p, context size %u", r, sizeof(*r));

	/* obtain reference to the stream */
	(void)sensor_fifo_iodev(icm42688, &iodev);

	LOG_INF("Setting up RX requests");

	__ASSERT_NO_MSG(iodev != NULL);

	/* Feed initial read requests */
	for (int i = 0; i < n_bufs; i++) {
		sqe = rtio_spsc_acquire(r->sq);
		rtio_sqe_prep_read(sqe, iodev, 0, &icm42688_fifo_bufs[i][0], ICM42688_FIFO_BUF_LEN,
				   (void *)(uintptr_t)i);
		rtio_spsc_produce(r->sq);
	}

	/* Submits requests */
	rtio_submit(r, 0);

	/* Setup requests, starting stream */
	LOG_INF("Polling RX Requests");

	/* Enable the fifo automatic triggering (via gpio) */
	sensor_fifo_start(icm42688);

	uint32_t overflows = 0;

	/* Now poll for ready fifo buffers for a little bit, at 32KHz sampling
	 * and a 1024 buffer read with 16 bytes a sample, the fifo holds
	 * 64 samples. At 32KHz it triggers every 2ms! this means
	 * Every buffer has ~2ms to process or we lose the next sample.
	 *
	 * Every 8th buffer we busy wait just over 2ms to show how no data
	 * is lost even with some variable latency involved. This could
	 * come from data processing or attempting to transport the data
	 * to another device.
	 *
	 * From the sensor a lack of buffer to read into is an underflow
	 * and from the application perspective being unable to keep up
	 * is an overflow.
	 */
	for (int i = 0; i < FIFO_ITERS; i++) {
		struct rtio_cqe *cqe = rtio_cqe_consume_block(r);
		int32_t result = cqe->result;
		uintptr_t buf_idx = (uintptr_t)cqe->userdata;
		uint8_t *buf = &icm42688_fifo_bufs[buf_idx][0];

		rtio_spsc_release(r->cq);

		/* The first byte is the interrupt status and can be
		 * checked for a FIFO FULL signifying an overflow
		 */
		struct fifo_header *hdr = (void *)&buf[0];

		if (hdr->int_status & BIT(1)) {
			overflows++;
		}


		if (i % 64 == 0) {
			LOG_INF("Poll Mag: Iteration %d, Underflows (sensor overflows) %u, Buf "
			       "%lu, int status %x, result %d\n",
			       i, overflows, buf_idx, buf[0], result);
			poll_mag();
			k_busy_wait(1000);
		}

		/* Now to recycle the buffer by putting it back in the queue */
		struct rtio_sqe *sqe = rtio_spsc_acquire(r->sq);

		__ASSERT_NO_MSG(sqe != NULL);
		rtio_sqe_prep_read(sqe, iodev, 0, &icm42688_fifo_bufs[buf_idx][0],
				   ICM42688_FIFO_BUF_LEN, (void *)(uintptr_t)buf_idx);
		rtio_spsc_produce(r->sq);

		rtio_submit(r, 0);

	}

	LOG_INF("Checking In, Sensor Overflows %u", overflows);

	/* All done streaming */
	sensor_fifo_stop(icm42688);

	LOG_INF("DONE! FIFO should be DISABLED");
}


void main(void)
{
	LOG_INF("TDK RoboKit1 Sample");

	if (!device_is_ready(icm42688)) {
		LOG_INF("%s: device not ready.", icm42688->name);
		return;
	}

	if (!device_is_ready(akm09918)) {
		LOG_INF("%s: device not ready.", akm09918->name);
		return;
	}

	struct sensor_value sample_freq = { .val1 = 10, .val2 = 0 };

	sensor_attr_set(akm09918, SENSOR_CHAN_MAGN_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY,
		&sample_freq);

	LOG_INF("Fetch + Read");

	/* A few polling readings */
	for (int i = 0; i < 10; i++) {
		poll_imu();
		poll_mag();
		k_sleep(K_MSEC(100));
	}

	LOG_INF("Showing under/over flows with 1 buffer queue");
	fifo_stream(&r1, 1);

	LOG_INF("Showing no under/over flows with 4 buffer queue");
	fifo_stream(&r4, 4);

	LOG_INF("Done!");

	while (true) {
		k_msleep(1000);
	}
}
