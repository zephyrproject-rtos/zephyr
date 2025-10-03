/*
 * Copyright (c) 2025, CATIE
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor_data_types.h>
#include <zephyr/dsp/print_format.h>
#include <zephyr/logging/log.h>
#include <zephyr/rtio/rtio.h>
#include <stdio.h>

LOG_MODULE_REGISTER(async_hr, LOG_LEVEL_INF);
#define SENSOR_CHANNEL_LIST                                                                        \
	IF_ENABLED(CONFIG_SAMPLE_CHANNEL_RED, (X(RED, "red", 0)))                                                                           \
	IF_ENABLED(CONFIG_SAMPLE_CHANNEL_IR, (X(IR, "ir", 0)))                                                                             \
	IF_ENABLED(CONFIG_SAMPLE_CHANNEL_GREEN, (X(GREEN, "green", 0)))                                                                       \
	IF_ENABLED(CONFIG_SAMPLE_CHANNEL_DIE_TEMP, (X(DIE_TEMP, "temp", 0)))

const int max_width = 5;

#define X(chan, name, idx) name,
const char *channel_names[] = {SENSOR_CHANNEL_LIST};
#undef X

#define X(chan, name, idx) (struct sensor_chan_spec){SENSOR_CHAN_##chan, idx},
const struct sensor_chan_spec chan_list[] = {SENSOR_CHANNEL_LIST};
#undef X

#define X(chan, name, idx) {SENSOR_CHAN_##chan, idx},
SENSOR_DT_READ_IODEV(iodev, DT_CHOSEN(heart_rate_sensor), SENSOR_CHANNEL_LIST);
#undef X
RTIO_DEFINE(ctx, 1, 1);

struct sensor_async_data {
	uint32_t fit;
	struct sensor_q31_data *frame;
};

#include <zephyr/drivers/gpio.h>
const struct gpio_dt_spec sensor_ctrl = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), ctrl_gpios);

static const struct device *check_heart_rate_device(void)
{
	const struct device *const dev = DEVICE_DT_GET(DT_CHOSEN(heart_rate_sensor));

	/* Check that the device is okay */
	if (dev == NULL) {
		/* No such node, or the node does not have status "okay". */
		LOG_ERR("Error: no device found.");
		return NULL;
	}

	if (!device_is_ready(dev)) {
		LOG_ERR("Error: Device \"%s\" is not ready; "
			"check the driver initialization logs for errors.",
			dev->name);
		return NULL;
	}

	/* Setup CTLR */
	if (!gpio_is_ready_dt(&sensor_ctrl)) {
		LOG_ERR("ERROR: PPG CTRL device is not ready");
		return NULL;
	}

	int result = gpio_pin_configure_dt(&sensor_ctrl, GPIO_OUTPUT_ACTIVE);

	if (result < 0) {
		LOG_ERR("ERROR [%d]: Failed to configure PPG CTRL gpio", result);
		return NULL;
	}

	LOG_INF("Found device \"%s\", getting sensor data", dev->name);
	return dev;
}

int main(void)
{
	const struct device *dev = check_heart_rate_device();

	if (dev == NULL) {
		return 0;
	}

	const struct sensor_decoder_api *decoder;

	int rc = sensor_get_decoder(dev, &decoder);

	if (rc != 0) {
		LOG_ERR("%s: sensor_get_decode() failed: %d", dev->name, rc);
		return rc;
	}

	size_t base_size = 0, frame_size = 0;
	struct sensor_async_data data[ARRAY_SIZE(chan_list)];

	for (size_t i = 0; i < ARRAY_SIZE(chan_list); i++) {
		rc = decoder->get_size_info(chan_list[i], &base_size, &frame_size);

		if (rc != 0) {
			LOG_ERR("sensor_get_size_info(%s) failed: [%d]", channel_names[i], rc);
			return rc;
		}

		data[i].fit = 0;
		data[i].frame = (struct sensor_q31_data *)malloc(base_size);
	}

	while (1) {
		uint8_t buf[128];

		rc = sensor_read(&iodev, &ctx, buf, 128);

		if (rc != 0) {
			LOG_ERR("%s: sensor_read() failed: [%d]", dev->name, rc);
			return rc;
		}

		for (size_t i = 0; i < ARRAY_SIZE(chan_list); i++) {
			data[i].fit = 0;
			rc = decoder->decode(buf, chan_list[i], &data[i].fit, 1, data[i].frame);
			
			if (rc <= 0) {
				LOG_ERR("%s: sensor_decode(%s) failed: [%d]", dev->name,
					channel_names[i], rc);
				return rc;
			}

			LOG_INF("[%-*s]: %" PRIsensor_q31_data, max_width, channel_names[i],
				PRIsensor_q31_data_arg(*data[i].frame, chan_list[i].chan_idx));
		}

		k_sleep(K_MSEC(100));
	}
	return 0;
}
