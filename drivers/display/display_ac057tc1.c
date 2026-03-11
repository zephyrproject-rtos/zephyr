/*
 * Copyright (c) 2026 Fabian Blatz <fabianblatz@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT eink_ac057tc1

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/display/ac057tc1.h>
#include <zephyr/drivers/mipi_dbi.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <string.h>

LOG_MODULE_REGISTER(ac057tc1, CONFIG_DISPLAY_LOG_LEVEL);

/* Timing constants */
#define AC057TC1_RESET_DELAY      1
#define AC057TC1_RESET_WAIT       200
#define AC057TC1_BUSY_TIMEOUT     5000  /* 5 seconds for normal operations */
#define AC057TC1_REFRESH_TIMEOUT  40000 /* 40 seconds for display refresh */
#define AC057TC1_POWER_OFF_DELAY  200
#define AC057TC1_INIT_DELAY       100
#define AC057TC1_DEEP_SLEEP_DELAY 10
#define AC057TC1_DEEP_SLEEP_WAIT  100

/* Register definitions */
#define AC057TC1_CMD_PANEL_SET          0x00
#define AC057TC1_CMD_POWER_SET          0x01
#define AC057TC1_CMD_POWER_OFF          0x02
#define AC057TC1_CMD_POWER_OFF_SEQ      0x03
#define AC057TC1_CMD_POWER_ON           0x04
#define AC057TC1_CMD_BOOSTER_SOFTSTART  0x06
#define AC057TC1_CMD_DEEP_SLEEP         0x07
#define AC057TC1_CMD_DATA_START_TRANS   0x10
#define AC057TC1_CMD_DISPLAY_REF        0x12
#define AC057TC1_CMD_TEMP_SENSOR        0x40
#define AC057TC1_CMD_TEMP_SENSOR_EN     0x41
#define AC057TC1_CMD_VCOM_DATA_INTERVAL 0x50
#define AC057TC1_CMD_TCON               0x60
#define AC057TC1_CMD_RESOLUTION_SET     0x61
#define AC057TC1_CMD_E3                 0xE3

/* Deep sleep check byte */
#define AC057TC1_DEEP_SLEEP_CHECK 0xA5

/* Pixel format: 4 bits per pixel (one nibble per pixel, 2 pixels per byte) */
#define AC057TC1_PIXELS_PER_BYTE 2

struct ac057tc1_config {
	const struct device *mipi_dev;
	struct mipi_dbi_config dbi_config;
	struct gpio_dt_spec busy_gpio;
	uint16_t width;
	uint16_t height;
};

struct ac057tc1_data {
	bool blanking_on;
	struct k_sem busy_sem;
	struct gpio_callback busy_cb;
};

static inline int ac057tc1_write_cmd(const struct device *dev, uint8_t cmd, const uint8_t *data,
				     size_t len)
{
	const struct ac057tc1_config *config = dev->config;

	return mipi_dbi_command_write(config->mipi_dev, &config->dbi_config, cmd, data, len);
}

static inline int ac057tc1_write_cmd_uint8(const struct device *dev, uint8_t cmd, uint8_t data)
{
	return ac057tc1_write_cmd(dev, cmd, &data, 1);
}

static void ac057tc1_busy_cb(const struct device *gpio_dev, struct gpio_callback *cb, uint32_t pins)
{
	struct ac057tc1_data *data = CONTAINER_OF(cb, struct ac057tc1_data, busy_cb);

	k_sem_give(&data->busy_sem);
}

static int ac057tc1_busy_wait(const struct device *dev, uint32_t timeout_ms)
{
	const struct ac057tc1_config *config = dev->config;
	struct ac057tc1_data *data = dev->data;
	int ret;

	if (config->busy_gpio.port == NULL) {
		LOG_WRN("Busy GPIO not configured, using fixed delay");
		k_msleep(timeout_ms);
		return 0;
	}

	if (!device_is_ready(config->busy_gpio.port)) {
		LOG_WRN("Busy GPIO port not ready, using fixed delay");
		k_msleep(timeout_ms);
		return 0;
	}

	if (gpio_pin_get_dt(&config->busy_gpio) != 0) {
		LOG_DBG("Busy GPIO already ready (HIGH)");
		return 0;
	}

	k_sem_reset(&data->busy_sem);

	ret = gpio_pin_interrupt_configure_dt(&config->busy_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure busy GPIO interrupt: %d", ret);
		return ret;
	}

	ret = k_sem_take(&data->busy_sem, K_MSEC(timeout_ms));

	gpio_pin_interrupt_configure_dt(&config->busy_gpio, GPIO_INT_DISABLE);

	if (ret < 0) {
		LOG_ERR("Timeout waiting for busy signal");
		return -ETIMEDOUT;
	}

	LOG_DBG("Busy GPIO ready (HIGH)");
	return 0;
}

static int ac057tc1_hw_init(const struct device *dev)
{
	const struct ac057tc1_config *config = dev->config;
	int ret;

	ret = mipi_dbi_reset(config->mipi_dev, AC057TC1_RESET_DELAY);
	if (ret < 0) {
		LOG_ERR("Failed to reset display: %d", ret);
		return ret;
	}
	k_msleep(AC057TC1_RESET_WAIT);

	ret = ac057tc1_busy_wait(dev, AC057TC1_BUSY_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("Display not ready after reset");
		return ret;
	}

	uint8_t panel_set[] = {0xEF, 0x08};

	ret = ac057tc1_write_cmd(dev, AC057TC1_CMD_PANEL_SET, panel_set, sizeof(panel_set));
	if (ret < 0) {
		return ret;
	}

	uint8_t power_set[] = {0x37, 0x00, 0x05, 0x05};

	ret = ac057tc1_write_cmd(dev, AC057TC1_CMD_POWER_SET, power_set, sizeof(power_set));
	if (ret < 0) {
		return ret;
	}

	ret = ac057tc1_write_cmd_uint8(dev, AC057TC1_CMD_POWER_OFF_SEQ, 0x00);
	if (ret < 0) {
		return ret;
	}

	uint8_t booster[] = {0xC7, 0xC7, 0x1D};

	ret = ac057tc1_write_cmd(dev, AC057TC1_CMD_BOOSTER_SOFTSTART, booster, sizeof(booster));
	if (ret < 0) {
		return ret;
	}

	ret = ac057tc1_write_cmd_uint8(dev, AC057TC1_CMD_TEMP_SENSOR_EN, 0x00);
	if (ret < 0) {
		return ret;
	}

	ret = ac057tc1_write_cmd_uint8(dev, AC057TC1_CMD_VCOM_DATA_INTERVAL, 0x37);
	if (ret < 0) {
		return ret;
	}

	ret = ac057tc1_write_cmd_uint8(dev, AC057TC1_CMD_TCON, 0x20);
	if (ret < 0) {
		return ret;
	}

	uint8_t res_set[] = {(config->width >> 8) & 0xFF, config->width & 0xFF,
			     (config->height >> 8) & 0xFF, config->height & 0xFF};

	ret = ac057tc1_write_cmd(dev, AC057TC1_CMD_RESOLUTION_SET, res_set, sizeof(res_set));
	if (ret < 0) {
		return ret;
	}

	ret = ac057tc1_write_cmd_uint8(dev, AC057TC1_CMD_E3, 0xAA);
	if (ret < 0) {
		return ret;
	}

	k_msleep(AC057TC1_INIT_DELAY);

	ret = ac057tc1_write_cmd_uint8(dev, AC057TC1_CMD_VCOM_DATA_INTERVAL, 0x37);
	if (ret < 0) {
		return ret;
	}

	return mipi_dbi_release(config->mipi_dev, &config->dbi_config);
}

static int ac057tc1_deep_sleep(const struct device *dev)
{
	int ret;

	k_msleep(AC057TC1_DEEP_SLEEP_DELAY);

	ret = ac057tc1_write_cmd_uint8(dev, AC057TC1_CMD_DEEP_SLEEP, AC057TC1_DEEP_SLEEP_CHECK);
	if (ret < 0) {
		return ret;
	}

	k_msleep(AC057TC1_DEEP_SLEEP_WAIT);

	return 0;
}

static int ac057tc1_init(const struct device *dev)
{
	const struct ac057tc1_config *config = dev->config;
	struct ac057tc1_data *data = dev->data;
	int ret;

	LOG_DBG("Initializing AC057TC1 display");

	if (!device_is_ready(config->mipi_dev)) {
		LOG_ERR("MIPI DBI device not ready");
		return -ENODEV;
	}

	if (config->busy_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&config->busy_gpio)) {
			LOG_ERR("Busy GPIO not ready");
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&config->busy_gpio, GPIO_INPUT);
		if (ret < 0) {
			LOG_ERR("Failed to configure busy GPIO: %d", ret);
			return ret;
		}

		/* Initialize semaphore for interrupt-driven busy wait */
		k_sem_init(&data->busy_sem, 0, 1);

		/* Set up GPIO callback for busy pin */
		gpio_init_callback(&data->busy_cb, ac057tc1_busy_cb, BIT(config->busy_gpio.pin));
		ret = gpio_add_callback(config->busy_gpio.port, &data->busy_cb);
		if (ret < 0) {
			LOG_ERR("Failed to add busy GPIO callback: %d", ret);
			return ret;
		}
	}

	data->blanking_on = true;

	ret = ac057tc1_hw_init(dev);
	if (ret < 0) {
		LOG_ERR("Hardware init failed: %d", ret);
		return ret;
	}

	LOG_INF("AC057TC1 display initialized (%ux%u)", config->width, config->height);

	return 0;
}

static int ac057tc1_write(const struct device *dev, const uint16_t x, const uint16_t y,
			  const struct display_buffer_descriptor *desc, const void *buf)
{
	const struct ac057tc1_config *config = dev->config;
	struct ac057tc1_data *data = dev->data;
	int ret;
	size_t buf_len;

	LOG_DBG("write x=%u y=%u w=%u h=%u pitch=%u", x, y, desc->width, desc->height, desc->pitch);

	/* Validate parameters */
	if (x != 0 || y != 0 || desc->width != config->width || desc->height != config->height) {
		LOG_ERR("Partial updates not supported. Full screen writes only.");
		return -ENOTSUP;
	}

	/* Calculate buffer length - 4 bits per pixel, 2 pixels per byte */
	buf_len = (desc->width * desc->height) / AC057TC1_PIXELS_PER_BYTE;

	if (buf == NULL || desc->buf_size < buf_len) {
		LOG_ERR("Invalid buffer: buf=%p size=%u expected=%u", buf, desc->buf_size, buf_len);
		return -EINVAL;
	}

	/* Wake panel from deep sleep if needed */
	ret = ac057tc1_hw_init(dev);
	if (ret < 0) {
		LOG_ERR("Failed to wake panel: %d", ret);
		return ret;
	}

	/* Set resolution */
	uint8_t res_set[] = {(config->width >> 8) & 0xFF, config->width & 0xFF,
			     (config->height >> 8) & 0xFF, config->height & 0xFF};

	ret = ac057tc1_write_cmd(dev, AC057TC1_CMD_RESOLUTION_SET, res_set, sizeof(res_set));
	if (ret < 0) {
		return ret;
	}

	/* Send pixel data with DATA_START_TRANS command */
	ret = ac057tc1_write_cmd(dev, AC057TC1_CMD_DATA_START_TRANS, buf, buf_len);
	if (ret < 0) {
		LOG_ERR("Failed to write display data: %d", ret);
		return ret;
	}

	if (!data->blanking_on) {
		ret = ac057tc1_write_cmd(dev, AC057TC1_CMD_POWER_ON, NULL, 0);
		if (ret < 0) {
			return ret;
		}

		/* Wait for power on */
		ret = ac057tc1_busy_wait(dev, AC057TC1_BUSY_TIMEOUT);
		if (ret < 0) {
			return ret;
		}

		/* Display refresh */
		ret = ac057tc1_write_cmd(dev, AC057TC1_CMD_DISPLAY_REF, NULL, 0);
		if (ret < 0) {
			return ret;
		}

		/* 7-color e-ink displays take 20-40 seconds to refresh */
		ret = ac057tc1_busy_wait(dev, AC057TC1_REFRESH_TIMEOUT);
		if (ret < 0) {
			return ret;
		}

		/* Power off after refresh */
		ret = ac057tc1_write_cmd(dev, AC057TC1_CMD_POWER_OFF, NULL, 0);
		if (ret < 0) {
			return ret;
		}

		/* Wait for power off - busy goes LOW when done */
		k_msleep(AC057TC1_POWER_OFF_DELAY);

		/* Put panel to deep sleep */
		ret = ac057tc1_deep_sleep(dev);
		if (ret < 0) {
			LOG_WRN("Failed to enter deep sleep: %d", ret);
		}
	}

	return mipi_dbi_release(config->mipi_dev, &config->dbi_config);
}

static int ac057tc1_blanking_on(const struct device *dev)
{
	struct ac057tc1_data *data = dev->data;

	LOG_DBG("Blanking on");
	data->blanking_on = true;

	return 0;
}

static int ac057tc1_blanking_off(const struct device *dev)
{
	struct ac057tc1_data *data = dev->data;

	LOG_DBG("Blanking off");
	data->blanking_on = false;

	return 0;
}

static void ac057tc1_get_capabilities(const struct device *dev, struct display_capabilities *caps)
{
	const struct ac057tc1_config *config = dev->config;

	memset(caps, 0, sizeof(struct display_capabilities));
	caps->x_resolution = config->width;
	caps->y_resolution = config->height;
	caps->supported_pixel_formats = PIXEL_FORMAT_L_4;
	caps->current_pixel_format = PIXEL_FORMAT_L_4;
	caps->screen_info = SCREEN_INFO_EPD;
}

static int ac057tc1_set_pixel_format(const struct device *dev, const enum display_pixel_format pf)
{
	if (pf == PIXEL_FORMAT_L_4) {
		return 0;
	}

	LOG_ERR("Pixel format not supported");
	return -ENOTSUP;
}

#ifdef CONFIG_PM_DEVICE
static int ac057tc1_pm_action(const struct device *dev, enum pm_device_action action)
{
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = ac057tc1_hw_init(dev);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		ret = ac057tc1_deep_sleep(dev);
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}
#endif /* CONFIG_PM_DEVICE */

static DEVICE_API(display, ac057tc1_api) = {
	.blanking_on = ac057tc1_blanking_on,
	.blanking_off = ac057tc1_blanking_off,
	.write = ac057tc1_write,
	.get_capabilities = ac057tc1_get_capabilities,
	.set_pixel_format = ac057tc1_set_pixel_format,
};

#define AC057TC1_DEFINE(inst)                                                                      \
	static const struct ac057tc1_config ac057tc1_cfg_##inst = {                                \
		.mipi_dev = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                   \
		.dbi_config =                                                                      \
			{                                                                          \
				.mode = MIPI_DBI_MODE_SPI_4WIRE,                                   \
				.config = MIPI_DBI_SPI_CONFIG_DT_INST(                             \
					inst, SPI_OP_MODE_MASTER | SPI_WORD_SET(8), 0),            \
			},                                                                         \
		.busy_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, busy_gpios, {0}),                      \
		.width = DT_INST_PROP(inst, width),                                                \
		.height = DT_INST_PROP(inst, height),                                              \
	};                                                                                         \
	static struct ac057tc1_data ac057tc1_data_##inst;                                          \
	PM_DEVICE_DT_INST_DEFINE(inst, ac057tc1_pm_action);                                        \
	DEVICE_DT_INST_DEFINE(inst, ac057tc1_init, PM_DEVICE_DT_INST_GET(inst),                    \
			      &ac057tc1_data_##inst, &ac057tc1_cfg_##inst, POST_KERNEL,            \
			      CONFIG_DISPLAY_INIT_PRIORITY, &ac057tc1_api);

DT_INST_FOREACH_STATUS_OKAY(AC057TC1_DEFINE)
