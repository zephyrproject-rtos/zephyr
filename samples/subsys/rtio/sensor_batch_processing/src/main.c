/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/rtio/rtio_executor_simple.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main);

#define N		(8)
#define M		(N/2)
#define SQ_SZ		(N)
#define CQ_SZ		(N)

#define NODE_ID		DT_COMPAT_GET_ANY_STATUS_OKAY(vnd_sensor)
#define SAMPLE_PERIOD	DT_PROP(NODE_ID, sample_period)
#define SAMPLE_SIZE	DT_PROP(NODE_ID, sample_size)
#define PROCESS_TIME	((M - 1) * SAMPLE_PERIOD)

RTIO_EXECUTOR_SIMPLE_DEFINE(simple_exec);
RTIO_DEFINE(ez_io, (struct rtio_executor *)&simple_exec, SQ_SZ, CQ_SZ);

static uint8_t bufs[N][SAMPLE_SIZE];

void main(void)
{
	const struct device *const vnd_sensor = DEVICE_DT_GET(NODE_ID);
	struct rtio_iodev *iodev = vnd_sensor->data;

	/* Fill the entire submission queue. */
	for (int n = 0; n < N; n++) {
		struct rtio_sqe *sqe = rtio_spsc_acquire(ez_io.sq);

		rtio_sqe_prep_read(sqe, iodev, RTIO_PRIO_HIGH, bufs[n],
				   SAMPLE_SIZE, bufs[n]);
		rtio_spsc_produce(ez_io.sq);
	}

	while (true) {
		int m = 0;
		uint8_t *userdata[M];

		LOG_INF("Submitting %d read requests", M);
		rtio_submit(&ez_io, M);

		/* Consume completion events until there is enough sensor data
		 * available to execute a batch processing algorithm, such as
		 * an FFT.
		 */
		while (m < M) {
			struct rtio_cqe *cqe = rtio_spsc_consume(ez_io.cq);

			if (cqe == NULL) {
				LOG_DBG("No completion events available");
				k_msleep(SAMPLE_PERIOD);
				continue;
			}
			LOG_DBG("Consumed completion event %d", m);

			if (cqe->result < 0) {
				LOG_ERR("Operation failed");
			}

			userdata[m] = cqe->userdata;
			rtio_spsc_release(ez_io.cq);
			m++;
		}

		/* Here is where we would execute a batch processing algorithm.
		 * Model as a long sleep that takes multiple sensor sample
		 * periods. The sensor driver can continue reading new data
		 * during this time because we submitted more buffers into the
		 * queue than we needed for the batch processing algorithm.
		 */
		LOG_INF("Start processing %d samples", M);
		for (m = 0; m < M; m++) {
			LOG_HEXDUMP_DBG(userdata[m], SAMPLE_SIZE, "Sample data:");
		}
		k_msleep(PROCESS_TIME);
		LOG_INF("Finished processing %d samples", M);

		/* Recycle the sensor data buffers and refill the submission
		 * queue.
		 */
		for (m = 0; m < M; m++) {
			struct rtio_sqe *sqe = rtio_spsc_acquire(ez_io.sq);

			rtio_sqe_prep_read(sqe, iodev, RTIO_PRIO_HIGH,
					   userdata[m], SAMPLE_SIZE,
					   userdata[m]);
			rtio_spsc_produce(ez_io.sq);
		}
	}
}
