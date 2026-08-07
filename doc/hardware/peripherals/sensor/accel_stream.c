/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor.h>

#define ACCEL_TRIGGERS						\
	{ SENSOR_TRIG_DRDY, SENSOR_STREAM_DATA_INCLUDE },	\
	{ SENSOR_TRIG_TAP, SENSOR_STREAM_DATA_NOP }
#define ACCEL_ALIAS(id) DT_ALIAS(CONCAT(accel, id))
#define ACCEL_IODEV_SYM(id) CONCAT(accel_iodev, id)
#define ACCEL_IODEV_PTR(id) &ACCEL_IODEV_SYM(id)
#define ACCEL_DEFINE_IODEV(id)					\
	SENSOR_DT_STREAM_IODEV(					\
		ACCEL_IODEV_SYM(id),				\
		ACCEL_ALIAS(id),				\
		ACCEL_TRIGGERS					\
		);

#define NUM_SENSORS 2

LISTIFY(NUM_SENSORS, ACCEL_DEFINE_IODEV, (;));

struct sensor_iodev *iodevs[NUM_SENSORS] = { LISTIFY(NUM_SENSORS, ACCEL_IODEV_PTR, (,)) };

RTIO_DEFINE_WITH_MEMPOOL(accel_ctx, NUM_SENSORS, NUM_SENSORS, NUM_SENSORS, 16, sizeof(void *));

int main(void)
{
	int rc;
	uint32_t accel_frame_iter = 0;
	struct sensor_three_axis_data accel_data[2] = {0};
	struct sensor_decoder_api *decoder;
	struct rtio_cqe *cqe;
	uint8_t *buf;
	uint32_t buf_len;
	struct rtio_sqe *handles[2];

	/* Start the streams */
	for (int i = 0; i < NUM_SENSORS; i++) {
		sensor_stream(iodevs[i], &accel_ctx, NULL, &handles[i]);
	}

	while (1) {
		cqe = rtio_cqe_consume_block(&accel_ctx);

		if (cqe->result != 0) {
			printk("async read failed %d\n", cqe->result);
			return;
		}

		rc = rtio_cqe_get_mempool_buffer(&accel_ctx, cqe, &buf, &buf_len);

		if (rc != 0) {
			printk("get mempool buffer failed %d\n", rc);
			return;
		}

		struct device *sensor = ((struct sensor_read_config *)
					 ((struct rtio_iodev *)cqe->userdata)->data)->sensor;

		rtio_cqe_release(&accel_ctx, cqe);

		rc = sensor_get_decoder(sensor, &decoder);

		if (rc != 0) {
			printk("sensor_get_decoder failed %d\n", rc);
			return;
		}

		/* Frame iterator values when data comes from a FIFO */
		uint32_t accel_fit = 0;

		/* Number of accelerometer data frames */
		uint32_t frame_count;

		rc = decoder->get_frame_count(buf, {SENSOR_CHAN_ACCEL_XYZ, 0},
					      &frame_count);
		if (rc != 0) {
			printk("sensor_get_decoder failed %d\n", rc);
			return;
		}

		/* If a tap has occurred lets print it out */
		if (decoder->has_trigger(buf, SENSOR_TRIG_TAP)) {
			printk("Tap! Sensor %s\n", dev->name);
		}

		/* Decode all available accelerometer sample frames */
		for (int i = 0; i < frame_count; i++) {
			decoder->decode(buf, {SENSOR_CHAN_ACCEL_XYZ, 0},
					accel_fit, 1, &accel_data);
			printk("Accel data for %s " PRIsensor_three_axis_data "\n",
			       dev->name,
			       PRIsensor_three_axis_data_arg(accel_data, 0));
		}

		rtio_release_buffer(&accel_ctx, buf, buf_len);
	}
}
