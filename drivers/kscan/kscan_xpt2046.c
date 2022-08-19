/*
 * Copyright (c) 2020 SenCai(1065402494@qq.com)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT xpt_xpt2046

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/kscan.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(xpt2046, CONFIG_KSCAN_LOG_LEVEL);

/* XPT2046 used registers */
#define CMD_READ_X  0xD0
#define CMD_READ_Y  0x90
#define CMD_READ_Z1 0xB0
#define CMD_READ_Z2 0xC0

/** XPT2046 configuration (DT). */
struct xpt2046_config {
	/** SPI bus. */
	struct spi_dt_spec spi;
	/** Interrupt GPIO information. */
	struct gpio_dt_spec int_gpio;
	/* Touch screen Size (in pixels). */
	uint16_t x_size;
	uint16_t y_size;
	/* Touch screen measurement value. */
	uint16_t x_min;
	uint16_t x_max;
	uint16_t y_min;
	uint16_t y_max;
};

/** XPT2046 data. */
struct xpt2046_data {
	/** Device pointer. */
	const struct device *dev;
	/** KSCAN Callback. */
	kscan_callback_t callback;
	/** Work queue (for deferred read). */
	struct k_work work;
	/** Timer (polling mode). */
	struct k_timer timer;
};

static int xpt2046_process(const struct device *dev)
{
	const struct xpt2046_config *config = dev->config;
	struct xpt2046_data *data = dev->data;

	int ret;
	bool pressed = false;
	uint16_t point_x = 0, point_y = 0;
	uint8_t tx_data[5U] = {0};
	uint8_t rx_data[5U];

	/* is touch */
	int val = gpio_pin_get_dt(&config->int_gpio);
	if (val != 0) {

		/* get coordinates */
		tx_data[0U] = CMD_READ_X;
		tx_data[2U] = CMD_READ_Y;

		const struct spi_buf tx_buf[] = {{.buf = tx_data, .len = 5}};
		const struct spi_buf rx_buf[] = {{.buf = rx_data, .len = 5}};

		const struct spi_buf_set tx = {.buffers = tx_buf, .count = 1};
		const struct spi_buf_set rx = {.buffers = rx_buf, .count = 1};

		ret = spi_transceive_dt(&config->spi, &tx, &rx);
		if (ret < 0) {
			return ret;
		}

		point_x = (((uint16_t)rx_data[1] << 8U) | rx_data[2]) >> 3;
		point_y = (((uint16_t)rx_data[3] << 8U) | rx_data[4]) >> 3;

		/* Organizing Coordinate Data */
		point_x -= (point_x > config->x_min) ? config->x_min : 0;
		point_y -= (point_y > config->y_min) ? config->y_min : 0;

		point_x = point_x * config->x_size / (config->x_max - config->x_min);
		point_y = point_y * config->y_size / (config->y_max - config->y_min);

		pressed = true;
	}

	/* callback */
	data->callback(dev, point_x, point_y, pressed);

	return 0;
}

static void xpt2046_work_handler(struct k_work *work)
{
	struct xpt2046_data *data = CONTAINER_OF(work, struct xpt2046_data, work);

	xpt2046_process(data->dev);
}

static void xpt2046_timer_handler(struct k_timer *timer)
{
	struct xpt2046_data *data = CONTAINER_OF(timer, struct xpt2046_data, timer);

	k_work_submit(&data->work);
}

static int xpt2046_configure(const struct device *dev, kscan_callback_t callback)
{
	struct xpt2046_data *data = dev->data;

	if (!callback) {
		LOG_ERR("Invalid callback (NULL)");
		return -EINVAL;
	}

	data->callback = callback;

	return 0;
}

static int xpt2046_enable_callback(const struct device *dev)
{
	struct xpt2046_data *data = dev->data;

	k_timer_start(&data->timer, K_MSEC(CONFIG_KSCAN_XPT2046_PERIOD),
		      K_MSEC(CONFIG_KSCAN_XPT2046_PERIOD));

	return 0;
}

static int xpt2046_disable_callback(const struct device *dev)
{
	struct xpt2046_data *data = dev->data;

	k_timer_stop(&data->timer);

	return 0;
}

static int xpt2046_init(const struct device *dev)
{
	const struct xpt2046_config *config = dev->config;
	struct xpt2046_data *data = dev->data;

	if (!spi_is_ready(&config->spi)) {
		LOG_ERR("SPI bus %s not ready", config->spi.bus->name);
		return -ENODEV;
	}

	data->dev = dev;

	k_work_init(&data->work, xpt2046_work_handler);

	int r;

	if (!device_is_ready(config->int_gpio.port)) {
		LOG_ERR("Interrupt GPIO controller device not ready");
		return -ENODEV;
	}

	r = gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT);
	if (r < 0) {
		LOG_ERR("Could not configure interrupt GPIO pin");
		return r;
	}

	k_timer_init(&data->timer, xpt2046_timer_handler, NULL);

	return 0;
}

static const struct kscan_driver_api xpt2046_driver_api = {
	.config = xpt2046_configure,
	.enable_callback = xpt2046_enable_callback,
	.disable_callback = xpt2046_disable_callback,
};

#define XPT2046_INIT(index)                                                                        \
	static const struct xpt2046_config xpt2046_config_##index = {                              \
		.spi = SPI_DT_SPEC_INST_GET(index, SPI_OP_MODE_MASTER | SPI_WORD_SET(8), 0),       \
		.int_gpio = GPIO_DT_SPEC_INST_GET(index, pendown_gpios),                           \
		.x_size = DT_INST_PROP(index, touchscreen_size_x),                                 \
		.y_size = DT_INST_PROP(index, touchscreen_size_y),                                 \
		.x_min = DT_INST_PROP(index, x_min),                                               \
		.x_max = DT_INST_PROP(index, x_max),                                               \
		.y_min = DT_INST_PROP(index, y_min),                                               \
		.y_max = DT_INST_PROP(index, y_max),                                               \
	};                                                                                         \
	static struct xpt2046_data xpt2046_data_##index;                                           \
	DEVICE_DT_INST_DEFINE(index, xpt2046_init, NULL, &xpt2046_data_##index,                    \
			      &xpt2046_config_##index, POST_KERNEL, CONFIG_KSCAN_INIT_PRIORITY,    \
			      &xpt2046_driver_api);

DT_INST_FOREACH_STATUS_OKAY(XPT2046_INIT)
