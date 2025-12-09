/*
 * Copyright 2025 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT waveshare_dsi2dpi

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/mipi_dsi.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(waveshare_dsi2dpi, CONFIG_DISPLAY_LOG_LEVEL);

struct waveshare_dsi2dpi_bus {
	struct i2c_dt_spec i2c;
};

typedef bool (*waveshare_dsi2dpi_bus_ready_fn)(const struct device *dev);
typedef int (*waveshare_dsi2dpi_write_bus_fn)(const struct device *dev, uint8_t reg, uint8_t val);
typedef const char *(*waveshare_dsi2dpi_bus_name_fn)(const struct device *dev);

struct waveshare_dsi2dpi_config {
	const struct device *mipi_dsi;
	uint8_t channel;
	uint8_t num_of_lanes;
	struct waveshare_dsi2dpi_bus bus;
	waveshare_dsi2dpi_bus_ready_fn bus_ready;
	waveshare_dsi2dpi_write_bus_fn write_bus;
	waveshare_dsi2dpi_bus_name_fn bus_name;
};

struct waveshare_dsi2dpi_data {
	uint8_t pixel_format;
};

static bool waveshare_dsi2dpi_bus_ready_i2c(const struct device *dev)
{
	const struct waveshare_dsi2dpi_config *config = dev->config;

	return i2c_is_ready_dt(&config->bus.i2c);
}

static int waveshare_dsi2dpi_write_bus_i2c(const struct device *dev, uint8_t reg, uint8_t val)
{
	const struct waveshare_dsi2dpi_config *config = dev->config;
	uint8_t buf[2];

	buf[0] = reg;
	buf[1] = val;

	return i2c_write_dt(&config->bus.i2c, buf, 2);
}

static const char *waveshare_dsi2dpi_bus_name_i2c(const struct device *dev)
{
	const struct waveshare_dsi2dpi_config *config = dev->config;

	return config->bus.i2c.bus->name;
}

static int waveshare_dsi2dpi_enable(const struct device *dev, bool enable)
{
	const struct waveshare_dsi2dpi_config *config = dev->config;

	config->write_bus(dev, 0xad, enable ? 0x01 : 0x00);

	return 0;
}

static int waveshare_dsi2dpi_bl_update_status(const struct device *dev, uint8_t brightness)
{
	const struct waveshare_dsi2dpi_config *config = dev->config;

	config->write_bus(dev, 0xab, 0xff - brightness);
	config->write_bus(dev, 0xaa, 0x01);

	return 0;
}

static int waveshare_dsi2dpi_init(const struct device *dev)
{
	const struct waveshare_dsi2dpi_config *config = dev->config;
	struct waveshare_dsi2dpi_data *data = dev->data;
	int ret;
	struct mipi_dsi_device mdev = {0};

	if (!config->bus_ready(dev)) {
		LOG_ERR("Bus device %s not ready!", config->bus_name(dev));
		return -EINVAL;
	}

	config->write_bus(dev, 0xc0, 0x01);
	config->write_bus(dev, 0xc2, 0x01);
	config->write_bus(dev, 0xac, 0x01);

	waveshare_dsi2dpi_bl_update_status(dev, 0xff);
	waveshare_dsi2dpi_enable(dev, true);

	/* Attach to MIPI DSI host */
	mdev.data_lanes = config->num_of_lanes;
	mdev.pixfmt = data->pixel_format;
	mdev.mode_flags =
		MIPI_DSI_MODE_VIDEO_HSE | MIPI_DSI_MODE_VIDEO | MIPI_DSI_CLOCK_NON_CONTINUOUS;

	ret = mipi_dsi_attach(config->mipi_dsi, config->channel, &mdev);
	if (ret < 0) {
		LOG_ERR("Could not attach to MIPI-DSI host");
		return ret;
	}

	LOG_DBG("waveshare dsi2dpi driver controller init succeeded");

	return 0;
}

#define WAVESHARE_DSI2DPI_DEFINE(id)                                                               \
	static const struct waveshare_dsi2dpi_config config_##id = {                               \
		.mipi_dsi = DEVICE_DT_GET(DT_INST_PHANDLE(id, mipi_dsi)),                          \
		.channel = DT_INST_REG_ADDR(id),                                                   \
		.num_of_lanes = DT_INST_PROP_BY_IDX(id, data_lanes, 0),                            \
		.bus = {.i2c = I2C_DT_SPEC_INST_GET(id)},                                          \
		.bus_ready = waveshare_dsi2dpi_bus_ready_i2c,                                      \
		.write_bus = waveshare_dsi2dpi_write_bus_i2c,                                      \
		.bus_name = waveshare_dsi2dpi_bus_name_i2c,                                        \
	};                                                                                         \
	static struct waveshare_dsi2dpi_data data_##id = {                                         \
		.pixel_format = DT_INST_PROP(id, pixel_format),                                    \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(id, waveshare_dsi2dpi_init, NULL, &data_##id, &config_##id,          \
			      POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(WAVESHARE_DSI2DPI_DEFINE)
