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

#if CONFIG_SAMPLE_STREAM
LOG_MODULE_REGISTER(stream_hr, LOG_LEVEL_INF);
#define TRIGGER_DATA_NOP_INCLUDE_DROP(_nop, _include) \
		COND_CODE_1(_nop, (SENSOR_STREAM_DATA_NOP), \
		(COND_CODE_1(_include, (SENSOR_STREAM_DATA_INCLUDE), \
		(SENSOR_STREAM_DATA_DROP))))
#define TRIGGER_TYPE_LIST \
	IF_ENABLED(CONFIG_SAMPLE_TRIGGER_DRDY, (X(DATA_READY, "DRDY", \
		TRIGGER_DATA_NOP_INCLUDE_DROP(CONFIG_SAMPLE_TRIG_DRDY_NOP, CONFIG_SAMPLE_TRIG_DRDY_INCLUDE)))) \
	IF_ENABLED(CONFIG_SAMPLE_TRIGGER_FIFO_WATTERMARK, (X(FIFO_WATERMARK, "WATERMARK", \
		TRIGGER_DATA_NOP_INCLUDE_DROP(CONFIG_SAMPLE_TRIG_FIFO_WATTERMARK_NOP, CONFIG_SAMPLE_TRIG_FIFO_WATTERMARK_INCLUDE)))) \
	IF_ENABLED(CONFIG_SAMPLE_TRIGGER_OVERFLOW, (X(OVERFLOW, "OVERFLOW", \
		TRIGGER_DATA_NOP_INCLUDE_DROP(CONFIG_SAMPLE_TRIG_OVERFLOW_NOP, CONFIG_SAMPLE_TRIG_OVERFLOW_INCLUDE))))
#define X(trig, name, data) name,
const char *trigger_names[] = {TRIGGER_TYPE_LIST};
#undef X
#define X(trig, name, data) (uint8_t)(data),
const uint8_t trigger_state[] = {TRIGGER_TYPE_LIST};
#undef X
#else
LOG_MODULE_REGISTER(async_hr, LOG_LEVEL_INF);
#endif /* CONFIG_SAMPLE_STREAM */

#define SENSOR_CHANNEL_LIST                                                                        \
	IF_ENABLED(CONFIG_SAMPLE_CHANNEL_RED, (X(RED, "red", 0)))                                                                           \
	IF_ENABLED(CONFIG_SAMPLE_CHANNEL_IR, (X(IR, "ir", 0)))                                                                             \
	IF_ENABLED(CONFIG_SAMPLE_CHANNEL_GREEN, (X(GREEN, "green", 0)))                                                                       \
	IF_ENABLED(CONFIG_SAMPLE_CHANNEL_DIE_TEMP, (X(DIE_TEMP, "temp", 0)))
#define X(chan, name, idx) name,
const char *channel_names[] = {SENSOR_CHANNEL_LIST};
#undef X
#define X(chan, name, idx) (struct sensor_chan_spec){SENSOR_CHAN_##chan, idx},
const struct sensor_chan_spec chan_list[] = {SENSOR_CHANNEL_LIST};
#undef X

const int max_width = 5;

#if CONFIG_SAMPLE_STREAM
#define NUM_SENSORS 1
#define X(trig, name, data) (struct sensor_stream_trigger){SENSOR_TRIG_##trig, data},
SENSOR_DT_STREAM_IODEV(iodev, DT_CHOSEN(heart_rate_sensor), TRIGGER_TYPE_LIST);
#undef X
RTIO_DEFINE_WITH_MEMPOOL(ctx, NUM_SENSORS, NUM_SENSORS, NUM_SENSORS*20, 256, sizeof(void *));
struct rtio_sqe *handle;
#else
#define X(chan, name, idx) {SENSOR_CHAN_##chan, idx},
SENSOR_DT_READ_IODEV(iodev, DT_CHOSEN(heart_rate_sensor), SENSOR_CHANNEL_LIST);
#undef X
RTIO_DEFINE(ctx, 1, 1);
#endif

struct sensor_async_data {
	uint32_t fit;
	struct sensor_q31_data *frame;
};

#include <zephyr/drivers/gpio.h>
const struct gpio_dt_spec sensor_ctrl = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), ctrl_gpios);

static const struct device *check_heart_rate_device(void)
{
	/* Check if heart rate sensor is available in chosen node */
	if (!DT_NODE_EXISTS(DT_CHOSEN(heart_rate_sensor))) {
		LOG_ERR("Error: no device found in chosen node");
		return NULL;
	}

#if CONFIG_SAMPLE_STREAM
	for (size_t i = 0; i < ARRAY_SIZE(trigger_names); i++) {
		LOG_WRN("Trigger: [%s][%d]", trigger_names[i], trigger_state[i]);
	}
#endif /* CONFIG_SAMPLE_STREAM */

	/* Get heart rate sensor device */
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

#if CONFIG_SAMPLE_STREAM
	struct rtio_cqe *cqe;

	/* Start the streams */
	LOG_INF("Device (%s): start stream", dev->name);
	sensor_stream(&iodev, &ctx, NULL, &handle);
#endif /* CONFIG_SAMPLE_STREAM */

	while (1) {
#if CONFIG_SAMPLE_STREAM
		uint8_t *buf;
		uint32_t buf_len;

		cqe = rtio_cqe_consume_block(&ctx);

		if (cqe->result != 0) {
			LOG_ERR("%s: rtio_cqe_consume_block() failed", dev->name);
			return cqe->result;
		}

		rc = rtio_cqe_get_mempool_buffer(&ctx, cqe, &buf, &buf_len);

		if (rc != 0) {
			LOG_ERR("%s: rtio_cqe_mempool_buffer() failed", dev->name);
			return rc;
		}

		rtio_cqe_release(&ctx, cqe);
#else
		uint8_t buf[128];
		rc = sensor_read(&iodev, &ctx, buf, 128);

		if (rc != 0) {
			LOG_ERR("%s: sensor_read() failed: [%d]", dev->name, rc);
			return rc;
		}
#endif /* CONFIG_SAMPLE_STREAM */

		for (size_t i = 0; i < ARRAY_SIZE(chan_list); i++) {
			data[i].fit = 0;
			rc = decoder->decode(buf, chan_list[i], &data[i].fit, 1, data[i].frame);
			
			if (rc <= 0) {
//				LOG_ERR("%s: sensor_decode(%s) failed: [%d]", dev->name,
//					channel_names[i], rc);
				continue;
			}

			LOG_INF("[%-*s]: %" PRIsensor_q31_data, max_width, channel_names[i],
				PRIsensor_q31_data_arg(*data[i].frame, chan_list[i].chan_idx));
		}

#if CONFIG_SAMPLE_STREAM
		rtio_release_buffer(&ctx, buf, buf_len);
#else
		k_sleep(K_MSEC(100));
#endif /* CONFIG_SAMPLE_STREAM */
	}

	return 0;
}
