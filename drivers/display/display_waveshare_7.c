/*
 * Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT waveshare_7

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/mipi_dsi.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(waveshare_7, CONFIG_DISPLAY_LOG_LEVEL);

/* Waveshare 7-inch display panel register(s) and value(s) */
#define APPLY_ADDR                          (0xAAU)
#define APPLY_CMD                           (0x01U)

/* The brightness levels are inverted in this panel, value "0x00" means full
 * brightness, value "0xFF" means minimum brightness.
 */
#define BRIGHTNESS_MAX_VAL                  (255U)
#define BRIGHTNESS_LEVEL_ADDR               (0xABU)

#define INIT_REG_1                          (0xC0U)
#define INIT_REG_2                          (0xC2U)
#define INIT_REG_3                          (0xACU)

#define PANEL_CTRL_ADDR                     (0xADU)
#define PANEL_ENABLE_CMD                    (0x01U)
#define PANEL_DISABLE_CMD                   (0x00U)

/* Waveshare 7-inch display resolution */
#define DISP_WAVESHARE_7_HOR_RES            (1024U)
#define DISP_WAVESHARE_7_VER_RES            (600U)

struct waveshare_7_bus {
	struct i2c_dt_spec i2c;
};

typedef bool (*waveshare_7_bus_ready_fn)(const struct device *dev);
typedef int (*waveshare_7_write_bus_fn)(const struct device *dev, uint8_t reg, uint8_t val);
typedef const char *(*waveshare_7_bus_name_fn)(const struct device *dev);

struct waveshare_7_config {
	const struct device *mipi_dsi;
	uint8_t channel;
	uint8_t num_of_lanes;
	struct waveshare_7_bus bus;
	waveshare_7_bus_ready_fn bus_ready;
	waveshare_7_write_bus_fn write_bus;
	waveshare_7_bus_name_fn bus_name;
};

struct waveshare_7_data {
	uint8_t pixel_format;
	uint8_t brightness;
};

static bool waveshare_7_bus_ready_i2c(const struct device *dev)
{
	const struct waveshare_7_config *config = dev->config;

	return i2c_is_ready_dt(&config->bus.i2c);
}

static int waveshare_7_write_bus_i2c(const struct device *dev, uint8_t reg, uint8_t val)
{
	const struct waveshare_7_config *config = dev->config;
	uint8_t buf[2];
	int ret;

	buf[0] = reg;
	buf[1] = val;

	ret = i2c_write_dt(&config->bus.i2c, buf, 2);

	k_sleep(K_MSEC(4));
	return ret;
}

static const char *waveshare_7_bus_name_i2c(const struct device *dev)
{
	const struct waveshare_7_config *config = dev->config;

	return config->bus.i2c.bus->name;
}

static int waveshare_7_init(const struct device *dev)
{
	const struct waveshare_7_config *config = dev->config;
	struct waveshare_7_data *data = dev->data;

	if (!config->bus_ready(dev)) {
		LOG_ERR("Bus device %s not ready!", config->bus_name(dev));
		return -EINVAL;
	}

	/* wait for the display to power on and I2C devices to initialize */
	k_sleep(K_MSEC(CONFIG_WAVESHARE_7_POWER_ON_DELAY_MS));

	/* send initialization commands */
	if (0 != config->write_bus(dev, PANEL_CTRL_ADDR, PANEL_DISABLE_CMD)) {
		LOG_ERR("Failed to write disable command to bus %s", config->bus_name(dev));
		return -EIO;
	}
	if (0 != config->write_bus(dev, INIT_REG_1, 0x01U)) {
		LOG_ERR("Failed to write init reg 1 to bus %s", config->bus_name(dev));
		return -EIO;
	}
	if (0 != config->write_bus(dev, INIT_REG_2, 0x01U)) {
		LOG_ERR("Failed to write init reg 2 to bus %s", config->bus_name(dev));
		return -EIO;
	}
	if (0 != config->write_bus(dev, INIT_REG_3, 0x01U)) {
		LOG_ERR("Failed to write init reg 3 to bus %s", config->bus_name(dev));
		return -EIO;
	}
	if (0 != config->write_bus(dev, PANEL_CTRL_ADDR, PANEL_ENABLE_CMD)) {
		LOG_ERR("Failed to write enable command to bus %s", config->bus_name(dev));
		return -EIO;
	}
	if (0 != config->write_bus(dev, BRIGHTNESS_LEVEL_ADDR, BRIGHTNESS_MAX_VAL)) {
		LOG_ERR("Failed to write brightness command to bus %s", config->bus_name(dev));
		return -EIO;
	}
	if (0 != config->write_bus(dev, APPLY_ADDR, APPLY_CMD)) {
		LOG_ERR("Failed to write apply command to bus %s", config->bus_name(dev));
		return -EIO;
	}

	data->brightness = 255U;
	LOG_DBG("waveshare 7 driver controller init succeeded");

	return 0;
}

static void waveshare_7_get_capabilities(const struct device *dev,
					   struct display_capabilities *capabilities)
{
	const struct waveshare_7_data *data = dev->data;

	memset(capabilities, 0, sizeof(struct display_capabilities));

	capabilities->current_pixel_format = data->pixel_format;
	capabilities->x_resolution = DISP_WAVESHARE_7_HOR_RES;
	capabilities->y_resolution = DISP_WAVESHARE_7_VER_RES;
	capabilities->current_orientation = DISPLAY_ORIENTATION_NORMAL;
}

static int waveshare_7_set_brightness(const struct device *dev, const uint8_t brightness)
{
	const struct waveshare_7_config *config = dev->config;
	struct waveshare_7_data *data = dev->data;
	int ret;

	data->brightness = brightness;

	/* Brightness values are inverted in this panel */
	ret = config->write_bus(dev, BRIGHTNESS_LEVEL_ADDR,
			BRIGHTNESS_MAX_VAL - data->brightness);
	if (ret < 0) {
		return ret;
	}

	return config->write_bus(dev, APPLY_ADDR, APPLY_CMD);
}

static int waveshare_7_blanking_on(const struct device *dev)
{
	return waveshare_7_set_brightness(dev, 0);
}

static int waveshare_7_blanking_off(const struct device *dev)
{
	struct waveshare_7_data *data = dev->data;

	return waveshare_7_set_brightness(dev, data->brightness);
}

static DEVICE_API(display, waveshare_7_api) = {
	.blanking_on = waveshare_7_blanking_on,
	.blanking_off = waveshare_7_blanking_off,
	.set_brightness = waveshare_7_set_brightness,
	.get_capabilities = waveshare_7_get_capabilities,
};

#define WAVESHARE_7_DEFINE(id)                                                                     \
	static const struct waveshare_7_config config_##id = {                                     \
		.bus = {.i2c = I2C_DT_SPEC_INST_GET(id)},                                          \
		.bus_ready = waveshare_7_bus_ready_i2c,                                            \
		.write_bus = waveshare_7_write_bus_i2c,                                            \
		.bus_name = waveshare_7_bus_name_i2c,                                              \
	};                                                                                         \
	static struct waveshare_7_data data_##id = {                                               \
		.pixel_format = DT_INST_PROP(id, pixel_format),                                    \
		.brightness = 255,                                                                 \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(id, waveshare_7_init, NULL, &data_##id, &config_##id,                \
			      POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY, &waveshare_7_api);

DT_INST_FOREACH_STATUS_OKAY(WAVESHARE_7_DEFINE)
