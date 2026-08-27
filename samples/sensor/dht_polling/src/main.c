/*
 * Copyright (c) 2023 Ian Morris
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>

#include <zephyr/device.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor_data_types.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/dsp/print_format.h>

#define DHT_ALIAS(i) DT_ALIAS(_CONCAT(dht, i))
#define DHT_DEVICE(i, _)                                                      \
	IF_ENABLED(DT_NODE_EXISTS(DHT_ALIAS(i)), (DEVICE_DT_GET(DHT_ALIAS(i)),))

/* Support up to 10 temperature/humidity sensors */
static const struct device *const sensors[] = {LISTIFY(10, DHT_DEVICE, ())};

#define DHT_IODEV(i, _)                                                      \
	IF_ENABLED(DT_NODE_EXISTS(DHT_ALIAS(i)),                                 \
		(SENSOR_DT_READ_IODEV(_CONCAT(dht_iodev, i), DHT_ALIAS(i),           \
		{SENSOR_CHAN_AMBIENT_TEMP, 0},                                       \
		{SENSOR_CHAN_HUMIDITY, 0})))

LISTIFY(10, DHT_IODEV, (;));

#define DHT_IODEV_REF(i, _)                                                   \
	COND_CODE_1(DT_NODE_EXISTS(DHT_ALIAS(i)), (CONCAT(&dht_iodev, i)), (NULL))

static struct rtio_iodev *dht_iodev[] = { LISTIFY(10, DHT_IODEV_REF, (,)) };

RTIO_DEFINE(dht_ctx, 1, 1);

int main(void)
{
	int rc;

	for (size_t i = 0; i < ARRAY_SIZE(sensors); i++) {
		if (!device_is_ready(sensors[i])) {
			printk("sensor: device %s not ready.\n", sensors[i]->name);
			return 0;
		}
	}

	while (1) {
		for (size_t i = 0; i < ARRAY_SIZE(sensors); i++) {
			struct device *dev = (struct device *) sensors[i];

			uint8_t buf[128];

			rc = sensor_read(dht_iodev[i], &dht_ctx, buf, 128);

			if (rc != 0) {
				printk("%s: sensor_read() failed: %d\n", dev->name, rc);
				return rc;
			}

			const struct sensor_decoder_api *decoder;

			rc = sensor_get_decoder(dev, &decoder);

			if (rc != 0) {
				printk("%s: sensor_get_decode() failed: %d\n", dev->name, rc);
				return rc;
			}

			uint32_t temp_fit = 0;
			struct sensor_q31_data temp_data = {0};

			decoder->decode(buf,
					(struct sensor_chan_spec) {SENSOR_CHAN_AMBIENT_TEMP, 0},
					&temp_fit, 1, &temp_data);

			uint32_t hum_fit = 0;
			struct sensor_q31_data hum_data = {0};

			decoder->decode(buf,
					(struct sensor_chan_spec) {SENSOR_CHAN_HUMIDITY, 0},
					&hum_fit, 1, &hum_data);

			printk("%16s: temp is %s%d.%d °C humidity is %s%d.%d %%RH\n", dev->name,
				PRIq_arg(temp_data.readings[0].temperature, 2, temp_data.shift),
				PRIq_arg(hum_data.readings[0].humidity, 2, hum_data.shift));
		}
		k_msleep(1000);
	}
	return 0;
}
