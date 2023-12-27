/**
 * Copyright (c) 2023 Mr Beam Lasers GmbH.
 * Copyright (c) 2023 Amrith Venkat Kesavamoorthi <amrith@mr-beam.org>
 * Copyright (c) 2023 Martin Kiepfer <mrmarteng@teleschirm.org>
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT galaxycore_gc9x01x

#include "display_gc9x01x.h"

#include <zephyr/dt-bindings/display/gc9x01x.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(display_gc9x01x, CONFIG_DISPLAY_LOG_LEVEL);

/* Command/data GPIO level for commands. */
#define GC9X01X_GPIO_LEVEL_CMD 0U

/* Command/data GPIO level for data. */
#define GC9X01X_GPIO_LEVEL_DATA 1U

/* Maximum number of default init registers  */
#define GC9X01X_NUM_DEFAULT_INIT_REGS 12U

/* Display data struct */
struct gc9x01x_data {
	uint8_t bytes_per_pixel;
	enum display_pixel_format pixel_format;
	enum display_orientation orientation;
};

/* Configuration data struct.*/
struct gc9x01x_config {
	struct spi_dt_spec spi;
	struct gpio_dt_spec cmd_data;
	struct gpio_dt_spec reset;
	uint8_t pixel_format;
	uint16_t orientation;
	uint16_t x_resolution;
	uint16_t y_resolution;
	bool inversion;
	const void *regs;
};

/* Initialization command data struct  */
struct gc9x01x_default_init_regs {
	uint8_t cmd;
	uint8_t len;
	uint8_t data[GC9X01X_NUM_DEFAULT_INIT_REGS];
};

/*
 * Default initialization commands. There are a lot of undocumented commands
 * within the manufacturer sample code, that are essential for proper operation of
 * the display controller
 */
static const struct gc9x01x_default_init_regs default_init_regs[] = {
	{
		.cmd = 0xEBU,
		.len = 1U,
		.data = {0x14U},
	},
	{
		.cmd = 0x84U,
		.len = 1U,
		.data = {0x40U},
	},
	{
		.cmd = 0x85U,
		.len = 1U,
		.data = {0xFFU},
	},
	{
		.cmd = 0x86U,
		.len = 1U,
		.data = {0xFFU},
	},
	{
		.cmd = 0x87U,
		.len = 1U,
		.data = {0xFFU},
	},
	{
		.cmd = 0x88U,
		.len = 1U,
		.data = {0x0AU},
	},
	{
		.cmd = 0x89U,
		.len = 1U,
		.data = {0x21U},
	},
	{
		.cmd = 0x8AU,
		.len = 1U,
		.data = {0x00U},
	},
	{
		.cmd = 0x8BU,
		.len = 1U,
		.data = {0x80U},
	},
	{
		.cmd = 0x8CU,
		.len = 1U,
		.data = {0x01U},
	},
	{
		.cmd = 0x8DU,
		.len = 1U,
		.data = {0x01U},
	},
	{
		.cmd = 0x8EU,
		.len = 1U,
		.data = {0xFFU},
	},
	{
		.cmd = 0x8FU,
		.len = 1U,
		.data = {0xFFU},
	},
	{
		.cmd = 0xB6U,
		.len = 2U,
		.data = {0x00U, 0x20U},
	},
	{
		.cmd = 0x90U,
		.len = 4U,
		.data = {0x08U, 0x08U, 0x08U, 0x08U},
	},
	{
		.cmd = 0xBDU,
		.len = 1U,
		.data = {0x06U},
	},
	{
		.cmd = 0xBCU,
		.len = 1U,
		.data = {0x00U},
	},
	{
		.cmd = 0xFFU,
		.len = 3U,
		.data = {0x60U, 0x01U, 0x04U},
	},
	{
		.cmd = 0xBEU,
		.len = 1U,
		.data = {0x11U},
	},
	{
		.cmd = 0xE1U,
		.len = 2U,
		.data = {0x10U, 0x0EU},
	},
	{
		.cmd = 0xDFU,
		.len = 3U,
		.data = {0x21U, 0x0CU, 0x02U},
	},
	{
		.cmd = 0xEDU,
		.len = 2U,
		.data = {0x1BU, 0x0BU},
	},
	{
		.cmd = 0xAEU,
		.len = 1U,
		.data = {0x77U},
	},
	{
		.cmd = 0xCDU,
		.len = 1U,
		.data = {0x63U},
	},
	{
		.cmd = 0x70U,
		.len = 9U,
		.data = {0x07U, 0x07U, 0x04U, 0x0EU, 0x0FU, 0x09U, 0x07U, 0x08U, 0x03U},
	},
	{
		.cmd = 0x62U,
		.len = 12U,
		.data = {0x18U, 0x0DU, 0x71U, 0xEDU, 0x70U, 0x70U, 0x18U, 0x0FU, 0x71U, 0xEFU,
			 0x70U, 0x70U},
	},
	{
		.cmd = 0x63U,
		.len = 12U,
		.data = {0x18U, 0x11U, 0x71U, 0xF1U, 0x70U, 0x70U, 0x18U, 0x13U, 0x71U, 0xF3U,
			 0x70U, 0x70U},
	},
	{
		.cmd = 0x64U,
		.len = 7U,
		.data = {0x28U, 0x29U, 0xF1U, 0x01U, 0xF1U, 0x00U, 0x07U},
	},
	{
		.cmd = 0x66U,
		.len = 10U,
		.data = {0x3CU, 0x00U, 0xCDU, 0x67U, 0x45U, 0x45U, 0x10U, 0x00U, 0x00U, 0x00U},
	},
	{
		.cmd = 0x67U,
		.len = 10U,
		.data = {0x00U, 0x3CU, 0x00U, 0x00U, 0x00U, 0x01U, 0x54U, 0x10U, 0x32U, 0x98U},
	},
	{
		.cmd = 0x74U,
		.len = 7U,
		.data = {0x10U, 0x85U, 0x80U, 0x00U, 0x00U, 0x4EU, 0x00U},
	},
	{
		.cmd = 0x98U,
		.len = 2U,
		.data = {0x3EU, 0x07U},
	},
};

static int gc9x01x_transmit(const struct device *dev, uint8_t cmd, const void *tx_data,
			    size_t tx_len)
{
	const struct gc9x01x_config *config = dev->config;
	int ret;
	struct spi_buf tx_buf = {.buf = &cmd, .len = 1U};
	struct spi_buf_set tx_bufs = {.buffers = &tx_buf, .count = 1U};

	ret = gpio_pin_set_dt(&config->cmd_data, GC9X01X_GPIO_LEVEL_CMD);
	if (ret < 0) {
		return ret;
	}
	ret = spi_write_dt(&config->spi, &tx_bufs);
	if (ret < 0) {
		return ret;
	}

	/* send data (if any) */
	if (tx_data != NULL) {
		tx_buf.buf = (void *)tx_data;
		tx_buf.len = tx_len;

		ret = gpio_pin_set_dt(&config->cmd_data, GC9X01X_GPIO_LEVEL_DATA);
		if (ret < 0) {
			return ret;
		}
		ret = spi_write_dt(&config->spi, &tx_bufs);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int gc9x01x_regs_init(const struct device *dev)
{
	const struct gc9x01x_config *config = dev->config;
	const struct gc9x01x_regs *regs = config->regs;
	int ret;

	/* Enable inter-command mode */
	ret = gc9x01x_transmit(dev, GC9X01X_CMD_INREGEN1, NULL, 0);
	if (ret < 0) {
		return ret;
	}
	ret = gc9x01x_transmit(dev, GC9X01X_CMD_INREGEN2, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	/* Apply default init sequence */
	for (int i = 0; (i < ARRAY_SIZE(default_init_regs)) && (ret == 0); i++) {
		ret = gc9x01x_transmit(dev, default_init_regs[i].cmd, default_init_regs[i].data,
				       default_init_regs[i].len);
		if (ret < 0) {
			return ret;
		}
	}

	/* Apply generic configuration */
	ret = gc9x01x_transmit(dev, GC9X01X_CMD_PWRCTRL2, regs->pwrctrl2, sizeof(regs->pwrctrl2));
	if (ret < 0) {
		return ret;
	}
	ret = gc9x01x_transmit(dev, GC9X01X_CMD_PWRCTRL3, regs->pwrctrl3, sizeof(regs->pwrctrl3));
	if (ret < 0) {
		return ret;
	}
	ret = gc9x01x_transmit(dev, GC9X01X_CMD_PWRCTRL4, regs->pwrctrl4, sizeof(regs->pwrctrl4));
	if (ret < 0) {
		return ret;
	}
	ret = gc9x01x_transmit(dev, GC9X01X_CMD_GAMMA1, regs->gamma1, sizeof(regs->gamma1));
	if (ret < 0) {
		return ret;
	}
	ret = gc9x01x_transmit(dev, GC9X01X_CMD_GAMMA2, regs->gamma2, sizeof(regs->gamma2));
	if (ret < 0) {
		return ret;
	}
	ret = gc9x01x_transmit(dev, GC9X01X_CMD_GAMMA3, regs->gamma3, sizeof(regs->gamma3));
	if (ret < 0) {
		return ret;
	}
	ret = gc9x01x_transmit(dev, GC9X01X_CMD_GAMMA4, regs->gamma4, sizeof(regs->gamma4));
	if (ret < 0) {
		return ret;
	}
	ret = gc9x01x_transmit(dev, GC9X01X_CMD_FRAMERATE, regs->framerate,
			       sizeof(regs->framerate));
	if (ret < 0) {
		return ret;
	}

	/* Enable Tearing line */
	ret = gc9x01x_transmit(dev, GC9X01X_CMD_TEON, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int gc9x01x_exit_sleep(const struct device *dev)
{
	int ret;

	ret = gc9x01x_transmit(dev, GC9X01X_CMD_SLPOUT, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	/*
	 * Exit sleepmode and enable display. 30ms on top of the sleepout time to account for
	 * any manufacturing defects.
	 * This is to allow time for the supply voltages and clock circuits stabilize
	 */
	k_msleep(GC9X01X_SLEEP_IN_OUT_DURATION_MS + 30);

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int gc9x01x_enter_sleep(const struct device *dev)
{
	int ret;

	ret = gc9x01x_transmit(dev, GC9X01X_CMD_SLPIN, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	/*
	 * Exit sleepmode and enable display. 30ms on top of the sleepout time to account for
	 * any manufacturing defects.
	 */
	k_msleep(GC9X01X_SLEEP_IN_OUT_DURATION_MS + 30);

	return 0;
}
#endif

static int gc9x01x_hw_reset(const struct device *dev)
{
	const struct gc9x01x_config *config = dev->config;

	if (config->reset.port == NULL) {
		return -ENODEV;
	}

	gpio_pin_set_dt(&config->reset, 1U);
	k_msleep(100);
	gpio_pin_set_dt(&config->reset, 0U);
	k_msleep(10);

	return 0;
}

static int gc9x01x_display_blanking_off(const struct device *dev)
{
	LOG_DBG("Turning display blanking off");
	return gc9x01x_transmit(dev, GC9X01X_CMD_DISPON, NULL, 0);
}

static int gc9x01x_display_blanking_on(const struct device *dev)
{
	LOG_DBG("Turning display blanking on");
	return gc9x01x_transmit(dev, GC9X01X_CMD_DISPOFF, NULL, 0);
}

static int gc9x01x_set_pixel_format(const struct device *dev,
				    const enum display_pixel_format pixel_format)
{
	struct gc9x01x_data *data = dev->data;
	int ret;
	uint8_t tx_data;
	uint8_t bytes_per_pixel;

	if (pixel_format == PIXEL_FORMAT_RGB_565) {
		bytes_per_pixel = 2U;
		tx_data = GC9X01X_PIXFMT_VAL_MCU_16_BIT | GC9X01X_PIXFMT_VAL_RGB_16_BIT;
	} else if (pixel_format == PIXEL_FORMAT_RGB_888) {
		bytes_per_pixel = 3U;
		tx_data = GC9X01X_PIXFMT_VAL_MCU_18_BIT | GC9X01X_PIXFMT_VAL_RGB_18_BIT;
	} else {
		LOG_ERR("Unsupported pixel format");
		return -ENOTSUP;
	}

	ret = gc9x01x_transmit(dev, GC9X01X_CMD_PIXFMT, &tx_data, 1U);
	if (ret < 0) {
		return ret;
	}

	data->pixel_format = pixel_format;
	data->bytes_per_pixel = bytes_per_pixel;

	return 0;
}

static int gc9x01x_set_orientation(const struct device *dev,
				   const enum display_orientation orientation)
{
	struct gc9x01x_data *data = dev->data;
	int ret;
	uint8_t tx_data = GC9X01X_MADCTL_VAL_BGR;

	if (orientation == DISPLAY_ORIENTATION_NORMAL) {
		/* works 0° - default */
	} else if (orientation == DISPLAY_ORIENTATION_ROTATED_90) {
		/* works CW 90° */
		tx_data |= GC9X01X_MADCTL_VAL_MV | GC9X01X_MADCTL_VAL_MY;
	} else if (orientation == DISPLAY_ORIENTATION_ROTATED_180) {
		/* works CW 180° */
		tx_data |= GC9X01X_MADCTL_VAL_MY | GC9X01X_MADCTL_VAL_MX | GC9X01X_MADCTL_VAL_MH;
	} else if (orientation == DISPLAY_ORIENTATION_ROTATED_270) {
		/* works CW 270° */
		tx_data |= GC9X01X_MADCTL_VAL_MV | GC9X01X_MADCTL_VAL_MX;
	}

	ret = gc9x01x_transmit(dev, GC9X01X_CMD_MADCTL, &tx_data, 1U);
	if (ret < 0) {
		return ret;
	}

	data->orientation = orientation;

	return 0;
}

static int gc9x01x_configure(const struct device *dev)
{
	const struct gc9x01x_config *config = dev->config;
	int ret;

	/* Set all the required registers. */
	ret = gc9x01x_regs_init(dev);
	if (ret < 0) {
		return ret;
	}

	/* Pixel format */
	ret = gc9x01x_set_pixel_format(dev, config->pixel_format);
	if (ret < 0) {
		return ret;
	}

	/* Orientation */
	ret = gc9x01x_set_orientation(dev, config->orientation);
	if (ret < 0) {
		return ret;
	}

	/* Display inversion mode. */
	if (config->inversion) {
		ret = gc9x01x_transmit(dev, GC9X01X_CMD_INVON, NULL, 0);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int gc9x01x_init(const struct device *dev)
{
	const struct gc9x01x_config *config = dev->config;
	int ret;

	if (!spi_is_ready_dt(&config->spi)) {
		LOG_ERR("SPI device is not ready");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&config->cmd_data)) {
		LOG_ERR("Command/Data GPIO device not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->cmd_data, GPIO_OUTPUT);
	if (ret < 0) {
		LOG_ERR("Could not configure command/data GPIO (%d)", ret);
		return ret;
	}

	if (config->reset.port != NULL) {
		if (!device_is_ready(config->reset.port)) {
			LOG_ERR("Reset GPIO device not ready");
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&config->reset, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_ERR("Could not configure reset GPIO (%d)", ret);
			return ret;
		}
	}

	gc9x01x_hw_reset(dev);

	gc9x01x_display_blanking_on(dev);

	ret = gc9x01x_configure(dev);
	if (ret < 0) {
		LOG_ERR("Could not configure display (%d)", ret);
		return ret;
	}

	ret = gc9x01x_exit_sleep(dev);
	if (ret < 0) {
		LOG_ERR("Could not exit sleep mode (%d)", ret);
		return ret;
	}

	return 0;
}

static int gc9x01x_set_mem_area(const struct device *dev, const uint16_t x, const uint16_t y,
				const uint16_t w, const uint16_t h)
{
	int ret;
	uint16_t spi_data[2];

	spi_data[0] = sys_cpu_to_be16(x);
	spi_data[1] = sys_cpu_to_be16(x + w - 1U);
	ret = gc9x01x_transmit(dev, GC9X01X_CMD_COLSET, &spi_data[0], 4U);
	if (ret < 0) {
		return ret;
	}

	spi_data[0] = sys_cpu_to_be16(y);
	spi_data[1] = sys_cpu_to_be16(y + h - 1U);
	ret = gc9x01x_transmit(dev, GC9X01X_CMD_ROWSET, &spi_data[0], 4U);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int gc9x01x_write(const struct device *dev, const uint16_t x, const uint16_t y,
			 const struct display_buffer_descriptor *desc, const void *buf)
{
	const struct gc9x01x_config *config = dev->config;
	struct gc9x01x_data *data = dev->data;
	int ret;
	const uint8_t *write_data_start = (const uint8_t *)buf;
	struct spi_buf tx_buf;
	struct spi_buf_set tx_bufs;
	uint16_t write_cnt;
	uint16_t nbr_of_writes;
	uint16_t write_h;

	__ASSERT(desc->width <= desc->pitch, "Pitch is smaller than width");
	__ASSERT((desc->pitch * data->bytes_per_pixel * desc->height) <= desc->buf_size,
		 "Input buffer to small");

	LOG_DBG("Writing %dx%d (w,h) @ %dx%d (x,y)", desc->width, desc->height, x, y);
	ret = gc9x01x_set_mem_area(dev, x, y, desc->width, desc->height);
	if (ret < 0) {
		return ret;
	}

	if (desc->pitch > desc->width) {
		write_h = 1U;
		nbr_of_writes = desc->height;
	} else {
		write_h = desc->height;
		nbr_of_writes = 1U;
	}

	ret = gc9x01x_transmit(dev, GC9X01X_CMD_MEMWR, write_data_start,
				  desc->width * data->bytes_per_pixel * write_h);
	if (ret < 0) {
		return ret;
	}

	tx_bufs.buffers = &tx_buf;
	tx_bufs.count = 1U;

	write_data_start += desc->pitch * data->bytes_per_pixel;
	for (write_cnt = 1U; write_cnt < nbr_of_writes; ++write_cnt) {
		tx_buf.buf = (void *)write_data_start;
		tx_buf.len = desc->width * data->bytes_per_pixel * write_h;

		ret = spi_write_dt(&config->spi, &tx_bufs);
		if (ret < 0) {
			return ret;
		}

		write_data_start += desc->pitch * data->bytes_per_pixel;
	}

	return 0;
}

static void gc9x01x_get_capabilities(const struct device *dev,
				     struct display_capabilities *capabilities)
{
	struct gc9x01x_data *data = dev->data;
	const struct gc9x01x_config *config = dev->config;

	memset(capabilities, 0, sizeof(struct display_capabilities));

	capabilities->supported_pixel_formats = PIXEL_FORMAT_RGB_565 | PIXEL_FORMAT_RGB_888;
	capabilities->current_pixel_format = data->pixel_format;

	if (data->orientation == DISPLAY_ORIENTATION_NORMAL ||
	    data->orientation == DISPLAY_ORIENTATION_ROTATED_180) {
		capabilities->x_resolution = config->x_resolution;
		capabilities->y_resolution = config->y_resolution;
	} else {
		capabilities->x_resolution = config->y_resolution;
		capabilities->y_resolution = config->x_resolution;
	}

	capabilities->current_orientation = data->orientation;
}

#ifdef CONFIG_PM_DEVICE
static int gc9x01x_pm_action(const struct device *dev, enum pm_device_action action)
{
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = gc9x01x_exit_sleep(dev);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		ret = gc9x01x_enter_sleep(dev);
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}
#endif /* CONFIG_PM_DEVICE */

/* Device driver API*/
static const struct display_driver_api gc9x01x_api = {
	.blanking_on = gc9x01x_display_blanking_on,
	.blanking_off = gc9x01x_display_blanking_off,
	.write = gc9x01x_write,
	.get_capabilities = gc9x01x_get_capabilities,
	.set_pixel_format = gc9x01x_set_pixel_format,
	.set_orientation = gc9x01x_set_orientation,
};

#define GC9X01X_INIT(inst)                                                                         \
	GC9X01X_REGS_INIT(inst);                                                                   \
	static const struct gc9x01x_config gc9x01x_config_##inst = {                               \
		.spi = SPI_DT_SPEC_INST_GET(inst, SPI_OP_MODE_MASTER | SPI_WORD_SET(8), 0),        \
		.cmd_data = GPIO_DT_SPEC_INST_GET(inst, cmd_data_gpios),                           \
		.reset = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),                         \
		.pixel_format = DT_INST_PROP(inst, pixel_format),                                  \
		.orientation = DT_INST_ENUM_IDX(inst, orientation),                                \
		.x_resolution = DT_INST_PROP(inst, width),                                         \
		.y_resolution = DT_INST_PROP(inst, height),                                        \
		.inversion = DT_INST_PROP(inst, display_inversion),                                \
		.regs = &gc9x01x_regs_##inst,                                                      \
	};                                                                                         \
	static struct gc9x01x_data gc9x01x_data_##inst;                                            \
	PM_DEVICE_DT_INST_DEFINE(inst, gc9x01x_pm_action);                                         \
	DEVICE_DT_INST_DEFINE(inst, &gc9x01x_init, PM_DEVICE_DT_INST_GET(inst),                    \
			      &gc9x01x_data_##inst, &gc9x01x_config_##inst, POST_KERNEL,           \
			      CONFIG_DISPLAY_INIT_PRIORITY, &gc9x01x_api);

DT_INST_FOREACH_STATUS_OKAY(GC9X01X_INIT)
