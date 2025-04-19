/*
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/kernel.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/drivers/sensor.h>

#define STREAMDEV_ALIAS(i) DT_ALIAS(_CONCAT(stream, i))
#define STREAMDEV_DEVICE(i, _) \
	IF_ENABLED(DT_NODE_EXISTS(STREAMDEV_ALIAS(i)), (DEVICE_DT_GET(STREAMDEV_ALIAS(i)),))
#define NUM_SENSORS 1

/* support up to 10 sensors */
static const struct device *const sensors[] = { LISTIFY(10, STREAMDEV_DEVICE, ()) };

#define STREAM_IODEV_SYM(id) CONCAT(accel_iodev, id)
#define STREAM_IODEV_PTR(id, _) &STREAM_IODEV_SYM(id)

#define STREAM_TRIGGERS					   \
	{ SENSOR_TRIG_DATA_READY, SENSOR_STREAM_DATA_INCLUDE }

#define STREAM_DEFINE_IODEV(id, _)    \
	SENSOR_DT_STREAM_IODEV(	      \
		STREAM_IODEV_SYM(id), \
		STREAMDEV_ALIAS(id),  \
		STREAM_TRIGGERS);

LISTIFY(NUM_SENSORS, STREAM_DEFINE_IODEV, (;));

struct rtio_iodev *iodevs[NUM_SENSORS] = { LISTIFY(NUM_SENSORS, STREAM_IODEV_PTR, (,)) };

RTIO_DEFINE_WITH_MEMPOOL(stream_ctx, NUM_SENSORS, NUM_SENSORS,
			 NUM_SENSORS * 20, 256, sizeof(void *));

struct sensor_chan_spec accel_chan = { SENSOR_CHAN_ACCEL_XYZ, 0 };

static uint8_t accel_buf[128] = { 0 };

static int print_accels_stream(const struct device *dev, struct rtio_iodev *iodev)
{
	int rc = 0;
	const struct sensor_decoder_api *decoder;
	struct rtio_cqe *cqe;
	uint8_t *buf;
	uint32_t buf_len;
	struct rtio_sqe *handles[NUM_SENSORS];
	struct sensor_three_axis_data *accel_data = (struct sensor_three_axis_data *)accel_buf;

	/* Start the streams */
	for (int i = 0; i < NUM_SENSORS; i++) {
		printk("sensor_stream\n");
		sensor_stream(iodevs[i], &stream_ctx, NULL, &handles[i]);
	}

	while (1) {
		cqe = rtio_cqe_consume_block(&stream_ctx);

		if (cqe->result != 0) {
			printk("async read failed %d\n", cqe->result);
			return cqe->result;
		}

		rc = rtio_cqe_get_mempool_buffer(&stream_ctx, cqe, &buf, &buf_len);

		if (rc != 0) {
			printk("get mempool buffer failed %d\n", rc);
			return rc;
		}

		const struct device *sensor = dev;

		rtio_cqe_release(&stream_ctx, cqe);

		rc = sensor_get_decoder(sensor, &decoder);

		if (rc != 0) {
			printk("sensor_get_decoder failed %d\n", rc);
			return rc;
		}

		/* Frame iterator values */
		uint32_t accel_fit = 0;

		/* Number of sensor data frames */
		uint16_t xl_count, frame_count;

		rc = decoder->get_frame_count(buf, accel_chan, &xl_count);

		if (rc != 0) {
			printk("sensor_get_frame failed %d\n", rc);
			return rc;
		}

		frame_count = xl_count;

		/* If a tap has occurred lets print it out */
		if (decoder->has_trigger(buf, SENSOR_TRIG_TAP)) {
			printk("Tap! Sensor %s\n", dev->name);
		}


		int8_t c = 0;

		/* decode and print Accelerometer frames */
		c = decoder->decode(buf, accel_chan, &accel_fit, 1, accel_data);

		printk("XL data for %s %lluns (%" PRIq(6) ", %" PRIq(6)
		       ", %" PRIq(6) ")\n", dev->name,
		       PRIsensor_three_axis_data_arg(*accel_data, 0));

		rtio_release_buffer(&stream_ctx, buf, buf_len);
	}

	return rc;
}

static int check_sensor_is_off(const struct device *dev)
{
	int ret;
	struct sensor_value odr;

	ret = sensor_attr_get(dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &odr);

	/* Check if accel is off */
	if (ret != 0 || (odr.val1 == 0 && odr.val2 == 0)) {
		printk("%s WRN : accelerometer device is off\n", dev->name);
	}

	return 0;
}

int main(void)
{
	int ret;

	for (size_t i = 0; i < ARRAY_SIZE(sensors); i++) {
		if (!device_is_ready(sensors[i])) {
			printk("sensor: device %s not ready.\n", sensors[i]->name);
			return 0;
		}
		check_sensor_is_off(sensors[i]);
	}

	while (1) {
		for (size_t i = 0; i < ARRAY_SIZE(sensors); i++) {
			ret = print_accels_stream(sensors[i], iodevs[i]);

			if (ret < 0) {
				return 0;
			}
		}
		k_msleep(1000);
	}
	return 0;
}
