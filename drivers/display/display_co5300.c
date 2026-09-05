/*
 * Copyright 2024 Protocentral Electronics
 * Copyright 2025 NXP
 * Copyright (c) 2025 Pavel Maloletkov.
 * Copyright (c) 2026 Muhammad Waleed Badar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT chipone_co5300

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(co5300, CONFIG_DISPLAY_LOG_LEVEL);

#include <zephyr/display/mipi_display.h>
#include <zephyr/drivers/display.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/gpio.h>

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(mipi_dsi)
#include <zephyr/drivers/mipi_dsi.h>
#include <zephyr/drivers/mipi_dsi/mipi_dsi_mcux_2l.h>
#include <fsl_lcdif.h>
#include <fsl_mipi_dsi.h>

#define CO5300_PIXFMT_RGB565 MIPI_DSI_PIXFMT_RGB565
#define CO5300_PIXFMT_RGB888 MIPI_DSI_PIXFMT_RGB888
#endif

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(mspi) || \
	DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#define CO5300_PIXFMT_RGB565 PIXEL_FORMAT_RGB_565
#define CO5300_PIXFMT_RGB888 PIXEL_FORMAT_RGB_888

#define CO5300_SPI_MODE          0xC4
#define CO5300_SPI_CMD           0x02
#define CO5300_SPI_WRITE4_PXIEL  0x32
#define CO5300_SPI_WRITE_TIMEOUT 100
#endif

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(mspi)
#include <zephyr/drivers/mspi.h>
#endif

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#include <zephyr/drivers/spi.h>
#endif

#define CO5300_PAGESET                      0xFE
#define CO5300_PASSWORD_BYTE1               0xF4
#define CO5300_PASSWORD_BYTE2               0xF5
#define CO5300_WRITE_HBM_DISPLAY_BRIGHTNESS 0x63

#define CO5300_DELAY_MS 150

typedef int (*co5300_cmd_write_fn)(const struct device *dev, uint8_t cmd, const void *tx_buf,
				   uint32_t tx_len);
typedef int (*co5300_display_write_fn)(const struct device *dev,
				       const struct display_buffer_descriptor *desc,
				       const void *buf);

struct co5300_config {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(mipi_dsi)
	const struct device *mipi_dev;
	uint16_t channel;
	uint16_t num_of_lanes;
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(mspi)
	const struct device *mspi_bus;
	struct mspi_dev_id dev_id;
	uint16_t chunk_size;
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	struct spi_dt_spec spi_bus;
#endif
	co5300_cmd_write_fn cmd_write;
	co5300_display_write_fn display_write;
	const struct gpio_dt_spec reset_gpios;
	const struct gpio_dt_spec tear_effect_gpios;
	const struct gpio_dt_spec backlight_gpios;
	uint16_t panel_width;
	uint16_t panel_height;
	bool red_blue_swap;
};

struct co5300_data {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(mspi)
	struct mspi_dev_cfg mspi_dev_config;
#endif
	uint8_t pixel_format;
	uint8_t bytes_per_pixel;
	struct gpio_callback tear_effect_gpio_cb;
	struct k_sem tear_effect_sem;
};

static int co5300_set_window(const struct device *dev, const uint16_t x, const uint16_t y,
			     const struct display_buffer_descriptor *desc)
{
	const struct co5300_config *config = dev->config;
	uint16_t cmd[2];
	int ret;

	cmd[0] = sys_cpu_to_be16(x + 6U);
	cmd[1] = sys_cpu_to_be16(x + 6U + desc->width - 1U);

	ret = config->cmd_write(dev, MIPI_DCS_SET_COLUMN_ADDRESS, (uint8_t *)cmd, 4U);
	if (ret < 0) {
		return ret;
	}

	cmd[0] = sys_cpu_to_be16(y);
	cmd[1] = sys_cpu_to_be16(y + desc->height - 1U);

	ret = config->cmd_write(dev, MIPI_DCS_SET_PAGE_ADDRESS, (uint8_t *)cmd, 4U);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(mipi_dsi)
static int co5300_mipi_dsi_cmd_write(const struct device *dev, uint8_t cmd, const void *tx_buf,
				     uint32_t tx_len)
{
	const struct co5300_config *config = dev->config;

	return mipi_dsi_dcs_write(config->mipi_dev, config->channel, cmd, tx_buf, tx_len);
}

static int co5300_mipi_dsi_display_write(const struct device *dev,
					 const struct display_buffer_descriptor *desc,
					 const void *buf)
{
	const struct co5300_config *config = dev->config;
	struct co5300_data *data = dev->data;
	struct mipi_dsi_msg msg = {0};
	uint16_t line_each_sent = 0U;
	int bytes_written = 0;
	const uint8_t *src = (const uint8_t *)buf;
	uint32_t tx_size = desc->height * desc->width * data->bytes_per_pixel;

	/* Start memory write. */
	msg.type = MIPI_DSI_DCS_LONG_WRITE;
	msg.flags = MCUX_DSI_2L_FB_DATA;
	msg.user_data = (void *)desc;
	msg.cmd = MIPI_DCS_WRITE_MEMORY_START;

	while (tx_size > 0) {
		msg.tx_len = tx_size;
		msg.tx_buf = src;
		bytes_written = (int)mipi_dsi_transfer(config->mipi_dev, config->channel, &msg);
		if (bytes_written < 0) {
			return bytes_written;
		}

		tx_size -= bytes_written;

		if (tx_size == 0U) {
			break;
		}

		/* Advance source pointer and decrement remaining */
		if (desc->pitch > desc->width) {
			line_each_sent = bytes_written / (desc->width * data->bytes_per_pixel);
			src += line_each_sent * desc->pitch * data->bytes_per_pixel;
			src += bytes_written % (desc->width * data->bytes_per_pixel);
		} else {
			src += bytes_written;
		}

		/* All future commands should use WRITE_MEMORY_CONTINUE */
		msg.cmd = MIPI_DCS_WRITE_MEMORY_CONTINUE;
	}
	return 0;
}
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(mipi_dsi) */

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(mspi)
static int co5300_mspi_write_helper(const struct device *dev, enum mspi_io_mode io_mode,
				    uint8_t cmd, const void *tx_buf, uint32_t tx_len)
{
	const struct co5300_config *config = dev->config;
	struct co5300_data *data = dev->data;
	struct mspi_xfer_packet pkt = {0};
	struct mspi_xfer xfer = {0};
	const uint8_t *framebuf = (const uint8_t *)tx_buf;
	uint32_t chunk_size;
	bool cmd_present = true;
	int ret;

	data->mspi_dev_config.io_mode = io_mode;
	ret = mspi_dev_config(config->mspi_bus, &config->dev_id, MSPI_DEVICE_CONFIG_IO_MODE,
			       &data->mspi_dev_config);
	if (ret < 0) {
		return ret;
	}

	xfer.packets = &pkt;
	xfer.num_packet = 1;
	xfer.async = false;
	xfer.timeout = CO5300_SPI_WRITE_TIMEOUT;
	xfer.priority = 0;

	do {
		chunk_size = MIN(tx_len, config->chunk_size);

		pkt.dir = MSPI_TX;
		pkt.data_buf = (uint8_t *)framebuf;
		pkt.num_bytes = chunk_size;

		if (cmd_present) {
			if (io_mode == MSPI_IO_MODE_SINGLE) {
				pkt.cmd = CO5300_SPI_CMD;
			} else if (io_mode == MSPI_IO_MODE_QUAD_1_1_4) {
				pkt.cmd = CO5300_SPI_WRITE4_PXIEL;
			} else {
				LOG_ERR("Unsupported IO mode: %d", io_mode);
				return -ENOTSUP;
			}
			pkt.address = (uint32_t)cmd << 8;
			xfer.cmd_length = 1;
			xfer.addr_length = 3;
		} else {
			pkt.cmd = 0;
			pkt.address = 0;
			xfer.cmd_length = 0;
			xfer.addr_length = 0;
		}

		xfer.hold_ce = (chunk_size < tx_len);
		ret = mspi_transceive(config->mspi_bus, &config->dev_id, &xfer);
		if (ret < 0) {
			return ret;
		}

		framebuf += chunk_size;
		tx_len -= chunk_size;
		cmd_present = false;
	} while (tx_len > 0);

	return 0;
}

static int co5300_mspi_cmd_write(const struct device *dev, uint8_t cmd, const void *tx_buf,
				 uint32_t tx_len)
{
	return co5300_mspi_write_helper(dev, MSPI_IO_MODE_SINGLE, cmd, tx_buf, tx_len);
}

static int co5300_mspi_display_write(const struct device *dev,
				     const struct display_buffer_descriptor *desc,
				     const void *buf)
{
	struct co5300_data *data = dev->data;
	uint32_t framebuf_size = desc->height * desc->width * data->bytes_per_pixel;

	return co5300_mspi_write_helper(dev, MSPI_IO_MODE_QUAD_1_1_4, MIPI_DCS_WRITE_MEMORY_START,
					buf, framebuf_size);
}
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(mspi) */

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
static int co5300_spi_cmd_write(const struct device *dev, uint8_t cmd,
				const void *tx_buf, uint32_t tx_len)
{
	const struct co5300_config *config = dev->config;
	struct spi_buf tx_data[3];
	struct spi_buf_set tx_bufs = {.buffers = tx_data, .count = 2U};
	uint8_t opcode[2] = {CO5300_SPI_CMD, 00};
	uint8_t cmd_buf[2] = {cmd, 00};

	tx_data[0].buf = &opcode;
	tx_data[0].len = 2U;
	tx_data[1].buf = &cmd_buf;
	tx_data[1].len = 2U;

	if (tx_buf != NULL && tx_len > 0) {
		tx_data[2].buf = (void *)tx_buf;
		tx_data[2].len = tx_len;
		tx_bufs.count = 3U;
	}

	return spi_write_dt(&config->spi_bus, &tx_bufs);
}

static int co5300_spi_display_write(const struct device *dev,
				    const struct display_buffer_descriptor *desc,
				    const void *buf)
{
	struct co5300_data *data = dev->data;
	uint32_t framebuf_size = desc->height * desc->width * data->bytes_per_pixel;

	return co5300_spi_cmd_write(dev, MIPI_DCS_WRITE_MEMORY_START, buf, framebuf_size);
}
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */

static void co5300_tear_effect_isr_handler(const struct device *gpio_dev, struct gpio_callback *cb,
					   uint32_t pins)
{
	struct co5300_data *data = CONTAINER_OF(cb, struct co5300_data, tear_effect_gpio_cb);

	k_sem_give(&data->tear_effect_sem);
}

static int co5300_blanking_on(const struct device *dev)
{
	const struct co5300_config *config = dev->config;
	int ret;

	if (config->backlight_gpios.port != NULL) {
		ret = gpio_pin_set_dt(&config->backlight_gpios, 0);
		if (ret < 0) {
			return ret;
		}
	}

	return config->cmd_write(dev, MIPI_DCS_SET_DISPLAY_OFF, NULL, 0);
}

static int co5300_blanking_off(const struct device *dev)
{
	const struct co5300_config *config = dev->config;
	int ret;

	if (config->backlight_gpios.port != NULL) {
		ret = gpio_pin_set_dt(&config->backlight_gpios, 1);
		if (ret < 0) {
			return ret;
		}
	}

	return config->cmd_write(dev, MIPI_DCS_SET_DISPLAY_ON, NULL, 0);
}

static int co5300_write(const struct device *dev, const uint16_t x, const uint16_t y,
			const struct display_buffer_descriptor *desc, const void *buf)
{
	const struct co5300_config *config = dev->config;
	struct co5300_data *data = dev->data;
	int ret;

	/* Check whether the updated area is outside of the panel frame. */
	if ((x > config->panel_width) || (y > config->panel_height) ||
	    ((x + desc->width) > config->panel_width) ||
	    ((y + desc->height) > config->panel_height)) {
		LOG_ERR("Update area outside panel dimensions");
		return -EINVAL;
	}

	/* Check whether the updated area is valid */
	if (desc->width == 0 || desc->height == 0) {
		LOG_ERR("The height/width of the update area cannot be 0");
		return -EINVAL;
	}

	/*
	 * Set column address of target area. The circular panel actually starts
	 * to show from row 6, row 0~5 are cut off physically. The actual display
	 * area is row 6~472 and line 0~466. So adjust coordinates accordingly.
	 */
	ret = co5300_set_window(dev, x, y, desc);
	if (ret < 0) {
		LOG_ERR("Could not set window (%d)", ret);
		return ret;
	}

	/*
	 * When writing the to the framebuffer and the tearing effect GPIO is present,
	 * we need to wait for the tear_effect GPIO semaphore to be released.
	 */
	if (config->tear_effect_gpios.port != NULL) {
		k_sem_take(&data->tear_effect_sem, K_FOREVER);
	}

	return config->display_write(dev, desc, buf);
}

static int co5300_set_brightness(const struct device *dev, const uint8_t contrast)
{
	const struct co5300_config *config = dev->config;

	return config->cmd_write(dev, MIPI_DCS_SET_DISPLAY_BRIGHTNESS, &contrast, 1);
}

static void co5300_get_capabilities(const struct device *dev,
				    struct display_capabilities *capabilities)
{
	const struct co5300_config *config = dev->config;
	struct co5300_data *data = dev->data;

	memset(capabilities, 0, sizeof(struct display_capabilities));
	capabilities->x_resolution = config->panel_width;
	capabilities->y_resolution = config->panel_height;
	capabilities->supported_pixel_formats = PIXEL_FORMAT_RGB_565 | PIXEL_FORMAT_RGB_888;

	switch (data->pixel_format) {
	case CO5300_PIXFMT_RGB565:
		capabilities->current_pixel_format = PIXEL_FORMAT_RGB_565;
		break;
	case CO5300_PIXFMT_RGB888:
		capabilities->current_pixel_format = PIXEL_FORMAT_RGB_888;
		break;
	default:
		LOG_ERR("Unsupported display format");
		/* Other display formats not implemented */
		break;
	}
	capabilities->current_orientation = DISPLAY_ORIENTATION_NORMAL;
}

static int co5300_set_pixel_format(const struct device *dev,
				   const enum display_pixel_format pixel_format)
{
	const struct co5300_config *config = dev->config;
	struct co5300_data *data = dev->data;
	uint8_t cmd_params[2];
	int ret;
	uint8_t mipi_pixel_format, bytes_per_pixel;

	switch (pixel_format) {
	case PIXEL_FORMAT_RGB_565:
		mipi_pixel_format = CO5300_PIXFMT_RGB565;
		bytes_per_pixel = 2U;
		cmd_params[0] = config->red_blue_swap ? 0 : MIPI_DCS_ADDRESS_MODE_BGR;
		cmd_params[1] = MIPI_DCS_PIXEL_FORMAT_16BIT;
		break;
	case PIXEL_FORMAT_RGB_888:
		mipi_pixel_format = CO5300_PIXFMT_RGB888;
		bytes_per_pixel = 3U;
		cmd_params[0] = config->red_blue_swap ? MIPI_DCS_ADDRESS_MODE_BGR : 0;
		cmd_params[1] = MIPI_DCS_PIXEL_FORMAT_24BIT;
		break;
	default:
		/* Other display formats not implemented */
		return -ENOTSUP;
	}

	ret = config->cmd_write(dev, MIPI_DCS_SET_ADDRESS_MODE, &cmd_params[0], sizeof(uint8_t));
	if (ret < 0) {
		return ret;
	}

	ret = config->cmd_write(dev, MIPI_DCS_SET_PIXEL_FORMAT, &cmd_params[1], sizeof(uint8_t));
	if (ret < 0) {
		return ret;
	}

	/* Update the format in the device data after DCS command succeeds. */
	data->bytes_per_pixel = bytes_per_pixel;
	data->pixel_format = mipi_pixel_format;

	return 0;
}

static int co5300_set_orientation(const struct device *dev,
				  const enum display_orientation orientation)
{
	if (orientation == DISPLAY_ORIENTATION_NORMAL) {
		return 0;
	}
	LOG_ERR("Changing display orientation not implemented");
	return -ENOTSUP;
}

static int co5300_reset(const struct device *dev)
{
	const struct co5300_config *config = dev->config;
	int ret = 0;

	if (config->reset_gpios.port != NULL) {
		ret = gpio_pin_configure_dt(&config->reset_gpios, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_ERR("Could not configure reset GPIO (%d)", ret);
			return ret;
		}

		ret = gpio_pin_set_dt(&config->reset_gpios, 0);
		if (ret < 0) {
			LOG_ERR("Could not pull reset low (%d)", ret);
			return ret;
		}

		k_msleep(CO5300_DELAY_MS);
		ret = gpio_pin_set_dt(&config->reset_gpios, 1);
		if (ret < 0) {
			LOG_ERR("Could not pull reset high (%d)", ret);
			return ret;
		}
	} else {
		config->cmd_write(dev, MIPI_DCS_SOFT_RESET, NULL, 0);
	}

	k_msleep(CO5300_DELAY_MS);

	return 0;
}

static int co5300_setup_tear_effect(const struct device *dev)
{
	const struct co5300_config *config = dev->config;
	struct co5300_data *data = dev->data;
	int ret = 0;

	if (config->tear_effect_gpios.port != NULL) {
		ret = gpio_pin_configure_dt(&config->tear_effect_gpios, GPIO_INPUT);
		if (ret < 0) {
			LOG_ERR("Could not configure TE GPIO (%d)", ret);
			return ret;
		}

		ret = gpio_pin_interrupt_configure_dt(&config->tear_effect_gpios,
						      GPIO_INT_EDGE_TO_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Could not configure TE interrupt (%d)", ret);
			return ret;
		}

		gpio_init_callback(&data->tear_effect_gpio_cb, co5300_tear_effect_isr_handler,
				   BIT(config->tear_effect_gpios.pin));
		ret = gpio_add_callback(config->tear_effect_gpios.port, &data->tear_effect_gpio_cb);
		if (ret < 0) {
			LOG_ERR("Could not add TE gpio callback");
			return ret;
		}

		/* Setup semaphore for using the tear effect pin */
		k_sem_init(&data->tear_effect_sem, 0, 1);
	}

	return 0;
}

static int co5300_configure_panel(const struct device *dev)
{
	const struct co5300_config *config = dev->config;
	struct co5300_data *data = dev->data;
	uint8_t cmd;
	int ret = 0;

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(mipi_dsi)
	cmd = 0x20;
	ret = config->cmd_write(dev, CO5300_PAGESET, &cmd, sizeof(cmd));
	if (ret < 0) {
		return ret;
	}

	cmd = 0x5A;
	ret = config->cmd_write(dev, CO5300_PASSWORD_BYTE1, &cmd, sizeof(cmd));
	if (ret < 0) {
		return ret;
	}

	cmd = 0x59;
	ret = config->cmd_write(dev, CO5300_PASSWORD_BYTE2, &cmd, sizeof(cmd));
	if (ret < 0) {
		return ret;
	}

	cmd = 0x40;
	ret = config->cmd_write(dev, CO5300_PAGESET, &cmd, sizeof(cmd));
	if (ret < 0) {
		return ret;
	}

	ret = config->cmd_write(dev, 0x96, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	ret = config->cmd_write(dev, 0xC9, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	ret = config->cmd_write(dev, CO5300_PAGESET, NULL, 0);
	if (ret < 0) {
		return ret;
	}
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(mipi_dsi) */

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(mspi) || \
	DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	cmd = 0x80;
	ret = config->cmd_write(dev, CO5300_SPI_MODE, &cmd, sizeof(cmd));
	if (ret < 0) {
		return ret;
	}
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(mspi) */

	/* Set pixel format */
	switch (data->pixel_format) {
	case CO5300_PIXFMT_RGB888:
		ret = co5300_set_pixel_format(dev, PIXEL_FORMAT_RGB_888);
		break;
	case CO5300_PIXFMT_RGB565:
		ret = co5300_set_pixel_format(dev, PIXEL_FORMAT_RGB_565);
		break;
	default:
		return -ENOTSUP;
	}

	if (ret < 0) {
		return ret;
	}

	ret = config->cmd_write(dev, MIPI_DCS_SET_TEAR_ON, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	cmd = 0x20;
	ret = config->cmd_write(dev, MIPI_DCS_WRITE_CONTROL_DISPLAY, &cmd, sizeof(cmd));
	if (ret < 0) {
		return ret;
	}

	cmd = UINT8_MAX;
	ret = config->cmd_write(dev, MIPI_DCS_SET_DISPLAY_BRIGHTNESS, &cmd, sizeof(cmd));
	if (ret < 0) {
		return ret;
	}

	cmd = UINT8_MAX;
	ret = config->cmd_write(dev, CO5300_WRITE_HBM_DISPLAY_BRIGHTNESS, &cmd, sizeof(cmd));
	if (ret < 0) {
		return ret;
	}

	/* Delay 50 ms before exiting sleep mode */
	k_msleep(50);
	return config->cmd_write(dev, MIPI_DCS_EXIT_SLEEP_MODE, NULL, 0);
}

static int co5300_init(const struct device *dev)
{
	const struct co5300_config *config = dev->config;
	__maybe_unused struct co5300_data *data = dev->data;
	int ret = 0;

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(mipi_dsi)
	struct mipi_dsi_device mdev = {0};

	/* Attach to MIPI DSI host */
	mdev.data_lanes = config->num_of_lanes;
	mdev.pixfmt = data->pixel_format;
	ret = mipi_dsi_attach(config->mipi_dev, config->channel, &mdev);
	if (ret < 0) {
		LOG_ERR("Could not attach to MIPI-DSI host");
		return ret;
	}
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(mipi_dsi) */

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(mspi)
	ret = mspi_dev_config(config->mspi_bus, &config->dev_id,
			      MSPI_DEVICE_CONFIG_IO_MODE | MSPI_DEVICE_CONFIG_CPP |
				      MSPI_DEVICE_CONFIG_CE_NUM | MSPI_DEVICE_CONFIG_CE_POL |
				      MSPI_DEVICE_CONFIG_FREQUENCY,
			      &data->mspi_dev_config);
	if (ret < 0) {
		LOG_ERR("Failed to configure MSPI device: %d", ret);
		return ret;
	}
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(mspi) */

	/* Perform Reset */
	ret = co5300_reset(dev);
	if (ret < 0) {
		return ret;
	}

	ret = co5300_configure_panel(dev);
	if (ret < 0) {
		LOG_ERR("Could not configure panel (%d)", ret);
		return ret;
	}

	/* Setup backlight */
	if (config->backlight_gpios.port != NULL) {
		ret = gpio_pin_configure_dt(&config->backlight_gpios, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Could not configure bl GPIO (%d)", ret);
			return ret;
		}
	}

	/* Setup tear effect pin and callback */
	ret = co5300_setup_tear_effect(dev);
	if (ret < 0) {
		LOG_ERR("Could not configure tear effect (%d)", ret);
		return ret;
	}

	/* Enable display */
	return config->cmd_write(dev, MIPI_DCS_SET_DISPLAY_ON, NULL, 0);
}

static DEVICE_API(display, co5300_api) = {
	.blanking_on = co5300_blanking_on,
	.blanking_off = co5300_blanking_off,
	.write = co5300_write,
	.set_brightness = co5300_set_brightness,
	.get_capabilities = co5300_get_capabilities,
	.set_pixel_format = co5300_set_pixel_format,
	.set_orientation = co5300_set_orientation,
};

#define CO5300_CONFIG_COMMON(node_id, _cmd_write, _display_write)				\
	.cmd_write = _cmd_write,								\
	.display_write = _display_write,							\
	.reset_gpios = GPIO_DT_SPEC_INST_GET_OR(node_id, reset_gpios, {0}),			\
	.backlight_gpios = GPIO_DT_SPEC_INST_GET_OR(node_id, backlight_gpios, {0}),		\
	.tear_effect_gpios = GPIO_DT_SPEC_INST_GET_OR(node_id, tear_effect_gpios, {0}),		\
	.panel_width = DT_INST_PROP(node_id, width),						\
	.panel_height = DT_INST_PROP(node_id, height),						\
	.red_blue_swap = DT_INST_PROP(node_id, red_blue_swap)

#define CO5300_CONFIG_SPI(node_id)								\
	{											\
		CO5300_CONFIG_COMMON(node_id, co5300_spi_cmd_write,				\
				     co5300_spi_display_write),					\
		.spi_bus = SPI_DT_SPEC_INST_GET(node_id, SPI_OP_MODE_CONTROLLER |		\
							 SPI_WORD_SET(8))			\
	}

#define CO5300_CONFIG_MSPI(node_id)								\
	{											\
		CO5300_CONFIG_COMMON(node_id, co5300_mspi_cmd_write,				\
				     co5300_mspi_display_write),				\
		.mspi_bus = DEVICE_DT_GET(DT_INST_BUS(node_id)),				\
		.dev_id = {.dev_idx = DT_INST_REG_ADDR(node_id),},				\
		.chunk_size = DT_INST_PROP(node_id, chunk_size),				\
	}

#define CO5300_CONFIG_MIPI_DSI(node_id)								\
	{											\
		CO5300_CONFIG_COMMON(node_id, co5300_mipi_dsi_cmd_write,			\
				     co5300_mipi_dsi_display_write),				\
		.mipi_dev = DEVICE_DT_GET(DT_INST_BUS(node_id)),				\
		.channel = DT_INST_REG_ADDR(node_id),						\
		.num_of_lanes = DT_INST_PROP_BY_IDX(node_id, data_lanes, 0),			\
	}

#define CO5300_DATA_MSPI(node_id)								\
	.mspi_dev_config = MSPI_DEVICE_CONFIG_DT_INST(node_id)

#define CO5300_DEVICE_INIT(node_id)								\
	static const struct co5300_config co5300_config_##node_id =				\
	COND_CASE_1(DT_INST_ON_BUS(node_id, mipi_dsi), (CO5300_CONFIG_MIPI_DSI(node_id)),	\
		    DT_INST_ON_BUS(node_id, mspi), (CO5300_CONFIG_MSPI(node_id)),		\
		    DT_INST_ON_BUS(node_id, spi), (CO5300_CONFIG_SPI(node_id)), ());		\
												\
	static struct co5300_data co5300_data_##node_id = {					\
		.pixel_format = DT_INST_PROP(node_id, pixel_format),				\
		COND_CODE_1(DT_INST_ON_BUS(node_id, mspi), (CO5300_DATA_MSPI(node_id)),	())	\
	};											\
												\
	DEVICE_DT_INST_DEFINE(node_id,								\
			    &co5300_init,							\
			    0,									\
			    &co5300_data_##node_id,						\
			    &co5300_config_##node_id,						\
			    POST_KERNEL,							\
			    CONFIG_APPLICATION_INIT_PRIORITY,					\
			    &co5300_api);

DT_INST_FOREACH_STATUS_OKAY(CO5300_DEVICE_INIT)
