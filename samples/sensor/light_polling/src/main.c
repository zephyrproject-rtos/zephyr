/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor_data_types.h>
#include <zephyr/dsp/print_format.h>
#include <zephyr/kernel.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/sys/util_macro.h>

#define SLEEP_TIME K_MSEC(1000)

/* The light-sensor alias and up to 10 light-sensorN aliases */
#define LIGHT_ALIAS      DT_ALIAS(light_sensor)
#define LIGHT_ALIAS_N(i) DT_ALIAS(_CONCAT(light_sensor, i))

#define LIGHT_DEVICE(node)   IF_ENABLED(DT_NODE_EXISTS(node), (DEVICE_DT_GET(node),))
#define LIGHT_DEVICE_N(i, _) LIGHT_DEVICE(LIGHT_ALIAS_N(i))

static const struct device *const sensors[] = {
	LIGHT_DEVICE(LIGHT_ALIAS)
	LISTIFY(10, LIGHT_DEVICE_N, ())
};

#define LIGHT_IODEV(name, node)                                                                    \
	IF_ENABLED(DT_NODE_EXISTS(node),                                                           \
		   (SENSOR_DT_READ_IODEV(name, node, {SENSOR_CHAN_LIGHT, 0});))
#define LIGHT_IODEV_N(i, _) LIGHT_IODEV(_CONCAT(light_iodev, i), LIGHT_ALIAS_N(i))

LIGHT_IODEV(light_iodev, LIGHT_ALIAS)
LISTIFY(10, LIGHT_IODEV_N, ())

#define LIGHT_IODEV_REF(name, node) IF_ENABLED(DT_NODE_EXISTS(node), (&name,))
#define LIGHT_IODEV_REF_N(i, _)     LIGHT_IODEV_REF(_CONCAT(light_iodev, i), LIGHT_ALIAS_N(i))

static struct rtio_iodev *const iodevs[] = {
	LIGHT_IODEV_REF(light_iodev, LIGHT_ALIAS)
	LISTIFY(10, LIGHT_IODEV_REF_N, ())
};

RTIO_DEFINE(light_ctx, 1, 1);

int main(void)
{
	const struct sensor_chan_spec light_chan = {SENSOR_CHAN_LIGHT, 0};

	for (size_t i = 0; i < ARRAY_SIZE(sensors); i++) {
		if (!device_is_ready(sensors[i])) {
			printk("sensor: device %s not ready.\n", sensors[i]->name);
			return 0;
		}
	}

	while (1) {
		for (size_t i = 0; i < ARRAY_SIZE(sensors); i++) {
			const struct device *dev = sensors[i];
			const struct sensor_decoder_api *decoder;
			struct sensor_q31_data data = {0};
			uint32_t fit = 0;
			uint8_t buf[128];
			int rc;

			rc = sensor_read(iodevs[i], &light_ctx, buf, sizeof(buf));
			if (rc != 0) {
				printk("%s: sensor_read() failed: %d\n", dev->name, rc);
				continue;
			}

			rc = sensor_get_decoder(dev, &decoder);
			if (rc != 0) {
				printk("%s: sensor_get_decoder() failed: %d\n", dev->name, rc);
				continue;
			}

			rc = decoder->decode(buf, light_chan, &fit, 1, &data);
			if (rc != 1) {
				printk("%s: light decode failed: %d\n", dev->name, rc);
				continue;
			}

			printk("%16s: %" PRIq(2) " lx\n", dev->name,
			       PRIq_arg(data.readings[0].light, 2, data.shift));
		}

		k_sleep(SLEEP_TIME);
	}

	return 0;
}
