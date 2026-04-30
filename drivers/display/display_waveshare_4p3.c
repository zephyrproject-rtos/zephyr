/*
 * Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT waveshare_4p3

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/mipi_dsi.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(waveshare_4p3, CONFIG_DISPLAY_LOG_LEVEL);

/* Waveshare 4.3-inch display panel register(s) and value(s) */
#define DISP_WAVESHARE_4P3_ID_REG                 (0x80U)
#define DISP_WAVESHARE_4P3_CTRL_REG               (0x85U)
#define DISP_WAVESHARE_4P3_POWERON_REG            (0x81U)
#define DISP_WAVESHARE_4P3_BRIGHTNESS_CTRL_REG    (0x86U)

#define DISP_WAVESHARE_4P3_ID_1                   (0xC3U)
#define DISP_WAVESHARE_4P3_ID_2                   (0xDEU)
#define DISP_WAVESHARE_4P3_NUM_ID_BYTES           (0x01U)

#define DISP_WAVESHARE_4P3_DISABLE_CMD            (0x00U)
#define DISP_WAVESHARE_4P3_ENABLE_CMD             (0x01U)
#define DISP_WAVESHARE_4P3_POWERON_CMPLT_CMD      (0x04U)

/* Waveshare 4.3-inch display resolution */
#define DISP_WAVESHARE_4P3_HOR_RES                (800U)
#define DISP_WAVESHARE_4P3_VER_RES                (480U)

struct waveshare_4p3_bus {
	struct i2c_dt_spec i2c;
};

typedef bool (*waveshare_4p3_bus_ready_fn)(const struct device *dev);
typedef int (*waveshare_4p3_write_bus_fn)(const struct device *dev, uint8_t reg, uint8_t val);
typedef const char *(*waveshare_4p3_bus_name_fn)(const struct device *dev);

struct waveshare_4p3_config {
	const struct device *mipi_dsi;
	uint8_t channel;
	uint8_t num_of_lanes;
	struct waveshare_4p3_bus bus;
	waveshare_4p3_bus_ready_fn bus_ready;
	waveshare_4p3_write_bus_fn write_bus;
	waveshare_4p3_bus_name_fn bus_name;
};

struct waveshare_4p3_data {
	uint8_t pixel_format;
	uint8_t brightness;
};

static bool waveshare_4p3_bus_ready_i2c(const struct device *dev)
{
	const struct waveshare_4p3_config *config = dev->config;

	return i2c_is_ready_dt(&config->bus.i2c);
}

static int waveshare_4p3_write_bus_i2c(const struct device *dev, uint8_t reg, uint8_t val)
{
	const struct waveshare_4p3_config *config = dev->config;
	uint8_t buf[2];

	buf[0] = reg;
	buf[1] = val;

	return i2c_write_dt(&config->bus.i2c, buf, 2);
}

static const char *waveshare_4p3_bus_name_i2c(const struct device *dev)
{
	const struct waveshare_4p3_config *config = dev->config;

	return config->bus.i2c.bus->name;
}

static int waveshare_4p3_init(const struct device *dev)
{
	const struct waveshare_4p3_config *config = dev->config;
	struct waveshare_4p3_data *data = dev->data;
	int ret;

	if (!config->bus_ready(dev)) {
		LOG_ERR("Bus device %s not ready!", config->bus_name(dev));
		return -EINVAL;
	}

	/* wait for the display to power on and I2C devices to initialize */
	k_sleep(K_MSEC(1000));

	/* verify the device ID of the controller */
	uint8_t cmd = DISP_WAVESHARE_4P3_ID_REG;
	uint8_t id = 0;

	ret = i2c_write_read_dt(&config->bus.i2c, &cmd, 1, &id, 1);
	if (ret < 0) {
		LOG_ERR("Failed to read device ID from bus %s", config->bus_name(dev));
		return ret;
	}

	if (DISP_WAVESHARE_4P3_ID_1 != id && DISP_WAVESHARE_4P3_ID_2 != id) {
		LOG_ERR("Unexpected device ID 0x%02x read from bus %s", id, config->bus_name(dev));
		return -EIO;
	}

	/* send initialization commands */
	if (0 !=
	    config->write_bus(dev, DISP_WAVESHARE_4P3_CTRL_REG, DISP_WAVESHARE_4P3_DISABLE_CMD)) {
		LOG_ERR("Failed to write disable command to bus %s", config->bus_name(dev));
		return -EIO;
	}
	if (0 !=
	    config->write_bus(dev, DISP_WAVESHARE_4P3_CTRL_REG, DISP_WAVESHARE_4P3_ENABLE_CMD)) {
		LOG_ERR("Failed to write enable command to bus %s", config->bus_name(dev));
		return -EIO;
	}
	if (0 != config->write_bus(dev, DISP_WAVESHARE_4P3_POWERON_REG,
				   DISP_WAVESHARE_4P3_POWERON_CMPLT_CMD)) {
		LOG_ERR("Failed to write power on complete command to bus %s",
			config->bus_name(dev));
		return -EIO;
	}
	if (0 != config->write_bus(dev, DISP_WAVESHARE_4P3_BRIGHTNESS_CTRL_REG, data->brightness)) {
		LOG_ERR("Failed to write brightness command to bus %s", config->bus_name(dev));
		return -EIO;
	}

	LOG_DBG("waveshare 4p3 driver controller init succeeded");

	return 0;
}

static void waveshare_4p3_get_capabilities(const struct device *dev,
					   struct display_capabilities *capabilities)
{
	const struct waveshare_4p3_data *data = dev->data;

	memset(capabilities, 0, sizeof(struct display_capabilities));

	capabilities->current_pixel_format = data->pixel_format;
	capabilities->x_resolution = DISP_WAVESHARE_4P3_HOR_RES;
	capabilities->y_resolution = DISP_WAVESHARE_4P3_VER_RES;
	capabilities->current_orientation = DISPLAY_ORIENTATION_NORMAL;
}

static int waveshare_4p3_set_brightness(const struct device *dev, const uint8_t brightness)
{
	const struct waveshare_4p3_config *config = dev->config;
	struct waveshare_4p3_data *data = dev->data;

	data->brightness = brightness;

	return config->write_bus(dev, DISP_WAVESHARE_4P3_BRIGHTNESS_CTRL_REG, brightness);
}

static int waveshare_4p3_blanking_on(const struct device *dev)
{
	return waveshare_4p3_set_brightness(dev, 0);
}

static int waveshare_4p3_blanking_off(const struct device *dev)
{
	struct waveshare_4p3_data *data = dev->data;

	return waveshare_4p3_set_brightness(dev, data->brightness);
}

static DEVICE_API(display, waveshare_4p3_api) = {
	.blanking_on = waveshare_4p3_blanking_on,
	.blanking_off = waveshare_4p3_blanking_off,
	.set_brightness = waveshare_4p3_set_brightness,
	.get_capabilities = waveshare_4p3_get_capabilities,
};

#define WAVESHARE_4P3_DEFINE(id)                                                                   \
	static const struct waveshare_4p3_config config_##id = {                                   \
		.bus = {.i2c = I2C_DT_SPEC_INST_GET(id)},                                          \
		.bus_ready = waveshare_4p3_bus_ready_i2c,                                          \
		.write_bus = waveshare_4p3_write_bus_i2c,                                          \
		.bus_name = waveshare_4p3_bus_name_i2c,                                            \
	};                                                                                         \
	static struct waveshare_4p3_data data_##id = {                                             \
		.pixel_format = DT_INST_PROP(id, pixel_format),                                    \
		.brightness = 255,                                                                 \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(id, waveshare_4p3_init, NULL, &data_##id, &config_##id,              \
			      POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY, &waveshare_4p3_api);

DT_INST_FOREACH_STATUS_OKAY(WAVESHARE_4P3_DEFINE)
