/*
 * Copyright (c) 2023 Seppo Takalo
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT xptek_xpt2046

#include <zephyr/drivers/spi.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(xpt2046, CONFIG_INPUT_LOG_LEVEL);

struct xpt2046_config {
	const struct spi_dt_spec bus;
	const struct gpio_dt_spec int_gpio;
	uint16_t min_x;
	uint16_t min_y;
	uint16_t max_x;
	uint16_t max_y;
	uint16_t threshold;
	uint16_t screen_size_x;
	uint16_t screen_size_y;
	uint16_t reads;
};
struct xpt2046_data {
	const struct device *dev;
	struct gpio_callback int_gpio_cb;
	struct k_work work;
	struct k_work_delayable dwork;
	uint8_t rbuf[9];
	uint32_t last_x;
	uint32_t last_y;
	bool pressed;
};

enum xpt2046_channel {
	CH_TEMP0 = 0,
	CH_Y,
	CH_VBAT,
	CH_Z1,
	CH_Z2,
	CH_X,
	CH_AUXIN,
	CH_TEMP1
};

struct measurement {
	uint32_t x;
	uint32_t y;
	uint32_t z;
};

#define START		      BIT(7)
#define CHANNEL(ch)	      ((ch & 0x7) << 4)
#define MODE_8_BIT	      BIT(3)
#define SINGLE_ENDED	      BIT(2)
#define POWER_OFF	      0
#define POWER_ON	      0x03
#define CONVERT_U16(buf, idx) ((uint16_t)((buf[idx] & 0x7f) << 5) | (buf[idx + 1] >> 3))

/* Read all Z1, X, Y, Z2 channels using 16 Clocks-per-Conversion mode.
 * See the manual https://www.waveshare.com/w/upload/9/98/XPT2046-EN.pdf for details.
 * Each follow-up command interleaves with previous conversion.
 * So first command starts at byte 0. Second command starts at byte 2.
 */
static uint8_t tbuf[9] = {
	[0] = START | CHANNEL(CH_Z1) | POWER_ON,
	[2] = START | CHANNEL(CH_Z2) | POWER_ON,
	[4] = START | CHANNEL(CH_X) | POWER_ON,
	[6] = START | CHANNEL(CH_Y) | POWER_OFF,
};

static void xpt2046_isr_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	struct xpt2046_data *data = CONTAINER_OF(cb, struct xpt2046_data, int_gpio_cb);
	const struct xpt2046_config *config = data->dev->config;

	gpio_remove_callback(config->int_gpio.port, &data->int_gpio_cb);
	k_work_submit(&data->work);
}

static int xpt2046_read_and_cumulate(const struct spi_dt_spec *bus, const struct spi_buf_set *tx,
				     const struct spi_buf_set *rx, struct measurement *meas)
{
	int ret = spi_transceive_dt(bus, tx, rx);

	if (ret < 0) {
		LOG_ERR("spi_transceive() %d\n", ret);
		return ret;
	}

	uint8_t *buf = rx->buffers->buf;

	meas->z += CONVERT_U16(buf, 1) + 4096 - CONVERT_U16(buf, 3);
	meas->x += CONVERT_U16(buf, 5);
	meas->y += CONVERT_U16(buf, 7);

	return 0;
}

static void xpt2046_release_handler(struct k_work *kw)
{
	struct k_work_delayable *dw = k_work_delayable_from_work(kw);
	struct xpt2046_data *data = CONTAINER_OF(dw, struct xpt2046_data, dwork);
	struct xpt2046_config *config = (struct xpt2046_config *)data->dev->config;

	if (!data->pressed) {
		return;
	}

	/* Check if touch is still pressed */
	if (gpio_pin_get_dt(&config->int_gpio) == 0) {
		data->pressed = false;
		input_report_key(data->dev, INPUT_BTN_TOUCH, 0, true, K_FOREVER);
	} else {
		/* Re-check later */
		k_work_reschedule(&data->dwork, K_MSEC(10));
	}
}

static void xpt2046_work_handler(struct k_work *kw)
{
	struct xpt2046_data *data = CONTAINER_OF(kw, struct xpt2046_data, work);
	struct xpt2046_config *config = (struct xpt2046_config *)data->dev->config;
	int ret;

	const struct spi_buf txb = {.buf = tbuf, .len = sizeof(tbuf)};
	const struct spi_buf rxb = {.buf = data->rbuf, .len = sizeof(data->rbuf)};
	const struct spi_buf_set tx_bufs = {.buffers = &txb, .count = 1};
	const struct spi_buf_set rx_bufs = {.buffers = &rxb, .count = 1};

	/* Run number of reads and calculate average */
	int rounds = config->reads;
	struct measurement meas = {0};

	for (int i = 0; i < rounds; i++) {
		if (xpt2046_read_and_cumulate(&config->bus, &tx_bufs, &rx_bufs, &meas) != 0) {
			return;
		}
	}
	meas.x /= rounds;
	meas.y /= rounds;
	meas.z /= rounds;

	/* Calculate Xp = M * Xt + C using fixed point aritchmetics, where
	 * Xp is the point in screen coordinates, Xt is the touch coordinates.
	 * Use signed int32_t for calculation to ensure that we cover the roll-over to negative
	 * values and return zero instead.
	 */
	int32_t mx = (config->screen_size_x << 16) / (config->max_x - config->min_x);
	int32_t cx = (config->screen_size_x << 16) - mx * config->max_x;
	int32_t x = mx * meas.x + cx;

	x = (x < 0 ? 0 : x) >> 16;

	int32_t my = (config->screen_size_y << 16) / (config->max_y - config->min_y);
	int32_t cy = (config->screen_size_y << 16) - my * config->max_y;
	int32_t y = my * meas.y + cy;

	y = (y < 0 ? 0 : y) >> 16;

	bool pressed = meas.z > config->threshold;

	/* Don't send any other than "pressed" events.
	 * releasing seem to cause just random noise
	 */
	if (pressed) {
		LOG_DBG("raw: x=%4u y=%4u ==> x=%4d y=%4d", meas.x, meas.y, x, y);

		input_report_abs(data->dev, INPUT_ABS_X, x, false, K_FOREVER);
		input_report_abs(data->dev, INPUT_ABS_Y, y, false, K_FOREVER);
		input_report_key(data->dev, INPUT_BTN_TOUCH, 1, true, K_FOREVER);

		data->last_x = x;
		data->last_y = y;
		data->pressed = pressed;

		/* Ensure that we send released event */
		k_work_reschedule(&data->dwork, K_MSEC(100));
	}

	ret = gpio_add_callback(config->int_gpio.port, &data->int_gpio_cb);
	if (ret < 0) {
		LOG_ERR("Could not set gpio callback");
		return;
	}
}

static int xpt2046_init(const struct device *dev)
{
	int r;
	const struct xpt2046_config *config = dev->config;
	struct xpt2046_data *data = dev->data;

	if (!spi_is_ready_dt(&config->bus)) {
		LOG_ERR("SPI controller device not ready");
		return -ENODEV;
	}

	data->dev = dev;
	k_work_init(&data->work, xpt2046_work_handler);
	k_work_init_delayable(&data->dwork, xpt2046_release_handler);

	if (!gpio_is_ready_dt(&config->int_gpio)) {
		LOG_ERR("Interrupt GPIO controller device not ready");
		return -ENODEV;
	}

	r = gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT);
	if (r < 0) {
		LOG_ERR("Could not configure interrupt GPIO pin");
		return r;
	}

	r = gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (r < 0) {
		LOG_ERR("Could not configure interrupt GPIO interrupt.");
		return r;
	}

	gpio_init_callback(&data->int_gpio_cb, xpt2046_isr_handler, BIT(config->int_gpio.pin));

	r = gpio_add_callback(config->int_gpio.port, &data->int_gpio_cb);
	if (r < 0) {
		LOG_ERR("Could not set gpio callback");
		return r;
	}

	LOG_INF("Init '%s' device", dev->name);

	return 0;
}

#define XPT2046_INIT(index)                                                                        \
	static const struct xpt2046_config xpt2046_config_##index = {                              \
		.bus = SPI_DT_SPEC_INST_GET(                                                       \
			index, SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8), 0),        \
		.int_gpio = GPIO_DT_SPEC_INST_GET(index, int_gpios),                               \
		.min_x = DT_INST_PROP(index, min_x),                                               \
		.min_y = DT_INST_PROP(index, min_y),                                               \
		.max_x = DT_INST_PROP(index, max_x),                                               \
		.max_y = DT_INST_PROP(index, max_y),                                               \
		.threshold = DT_INST_PROP(index, z_threshold),                                     \
		.screen_size_x = DT_INST_PROP(index, touchscreen_size_x),                          \
		.screen_size_y = DT_INST_PROP(index, touchscreen_size_y),                          \
		.reads = DT_INST_PROP(index, reads),                                               \
	};                                                                                         \
	static struct xpt2046_data xpt2046_data_##index;                                           \
	DEVICE_DT_INST_DEFINE(index, xpt2046_init, NULL, &xpt2046_data_##index,                    \
			      &xpt2046_config_##index, POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY,    \
			      NULL);                                                               \
	BUILD_ASSERT(DT_INST_PROP(index, min_x) < DT_INST_PROP(index, max_x),                      \
		     "min_x must be less than max_x");                                             \
	BUILD_ASSERT(DT_INST_PROP(index, min_y) < DT_INST_PROP(index, max_y),                      \
		     "min_y must be less than max_y");                                             \
	BUILD_ASSERT(DT_INST_PROP(index, z_threshold) > 10, "Too small threshold");                \
	BUILD_ASSERT(DT_INST_PROP(index, touchscreen_size_x) > 1 &&                                \
			     DT_INST_PROP(index, touchscreen_size_y) > 1,                          \
		     "Screen size undefined");                                                     \
	BUILD_ASSERT(DT_INST_PROP(index, reads) > 0, "Number of reads must be at least one");

DT_INST_FOREACH_STATUS_OKAY(XPT2046_INIT)
