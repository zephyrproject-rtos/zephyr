/*
 * Copyright (c) 2022 TOKITA Hiroshi <tokita.hiroshi@fujitsu.com
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

#define ACCEL_ALIAS(i) DT_ALIAS(_CONCAT(accel, i))
#define ACCELEROMETER_DEVICE(i, _)                                                           \
	IF_ENABLED(DT_NODE_EXISTS(ACCEL_ALIAS(i)), (DEVICE_DT_GET(ACCEL_ALIAS(i)),))
#define NUM_SENSORS 1

/* support up to 10 accelerometer sensors */
static const struct device *const sensors[] = {LISTIFY(10, ACCELEROMETER_DEVICE, ())};

#ifdef CONFIG_SENSOR_ASYNC_API

#define ACCEL_IODEV_SYM(id) CONCAT(accel_iodev, id)
#define ACCEL_IODEV_PTR(id, _) &ACCEL_IODEV_SYM(id)

#define ACCEL_DEFINE_IODEV(id, _)                 \
	SENSOR_DT_READ_IODEV(                     \
		ACCEL_IODEV_SYM(id),              \
		ACCEL_ALIAS(id),                  \
		{SENSOR_CHAN_ACCEL_XYZ, 0});

LISTIFY(NUM_SENSORS, ACCEL_DEFINE_IODEV, (;));

struct rtio_iodev *iodevs[NUM_SENSORS] = { LISTIFY(NUM_SENSORS, ACCEL_IODEV_PTR, (,)) };

RTIO_DEFINE_WITH_MEMPOOL(accel_ctx, NUM_SENSORS, NUM_SENSORS, NUM_SENSORS*20, 256, sizeof(void *));

static int print_accels_stream(const struct device *dev, struct rtio_iodev *iodev)
{
	int rc = 0;
	struct sensor_three_axis_data accel_data = {0};
	const struct sensor_decoder_api *decoder;

	while (1) {
		for (size_t i = 0; i < ARRAY_SIZE(sensors); i++) {
			uint8_t buf[128];

			rc = sensor_read(iodevs[i], &accel_ctx, buf, 128);

			if (rc != 0) {
				printk("%s: sensor_read() failed: %d\n", dev->name, rc);
				return rc;
			}

			rc = sensor_get_decoder(dev, &decoder);

			if (rc != 0) {
				printk("%s: sensor_get_decode() failed: %d\n", dev->name, rc);
				return rc;
			}

			uint32_t accel_fit = 0;

			/* decode and print Accelerometer FIFO frames */
			decoder->decode(buf, (struct sensor_chan_spec) {SENSOR_CHAN_ACCEL_XYZ, 0},
				&accel_fit, 8, &accel_data);

			printk("XL data for %s %lluns (%" PRIq(6) ", %" PRIq(6)
			       ", %" PRIq(6) ")\n", dev->name,
			       PRIsensor_three_axis_data_arg(accel_data, 0));
		}
		k_msleep(500);
	}
}
#else

static const enum sensor_channel channels[] = {
	SENSOR_CHAN_ACCEL_X,
	SENSOR_CHAN_ACCEL_Y,
	SENSOR_CHAN_ACCEL_Z,
};

static int print_accels(const struct device *dev)
{
	int ret;
	struct sensor_value accel[3];

	ret = sensor_sample_fetch(dev);
	if (ret < 0) {
		printk("%s: sensor_sample_fetch() failed: %d\n", dev->name, ret);
		return ret;
	}

	for (size_t i = 0; i < ARRAY_SIZE(channels); i++) {
		ret = sensor_channel_get(dev, channels[i], &accel[i]);
		if (ret < 0) {
			printk("%s: sensor_channel_get(%c) failed: %d\n", dev->name, 'X' + i, ret);
			return ret;
		}
	}

	printk("%16s [m/s^2]:    (%12.6f, %12.6f, %12.6f)\n", dev->name,
	       sensor_value_to_double(&accel[0]), sensor_value_to_double(&accel[1]),
	       sensor_value_to_double(&accel[2]));

	return 0;
}
#endif /*CONFIG_SENSOR_ASYNC_API*/

static int set_sampling_freq(const struct device *dev)
{
	int ret;
	struct sensor_value odr;

	ret = sensor_attr_get(dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &odr);

	/* If we don't get a frequency > 0, we set one */
	if (ret != 0 || (odr.val1 == 0 && odr.val2 == 0)) {
		odr.val1 = 100;
		odr.val2 = 0;

		ret = sensor_attr_set(dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY,
				      &odr);

		if (ret != 0) {
			printk("%s : failed to set sampling frequency\n", dev->name);
		}
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
		set_sampling_freq(sensors[i]);
	}

#ifndef CONFIG_COVERAGE
	while (1) {
#else
	for (int i = 0; i < 5; i++) {
#endif
		for (size_t i = 0; i < ARRAY_SIZE(sensors); i++) {
#ifdef CONFIG_SENSOR_ASYNC_API
			ret = print_accels_stream(sensors[i], iodevs[i]);
#else
			ret = print_accels(sensors[i]);
#endif /*CONFIG_SENSOR_ASYNC_API*/
			if (ret < 0) {
				return 0;
			}
		}
		k_msleep(1000);
	}
	return 0;
}
