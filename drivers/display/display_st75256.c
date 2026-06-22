/*
 * Copyright (c) 2025 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/sys/util.h"
#define DT_DRV_COMPAT sitronix_st75256

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(st75256, CONFIG_DISPLAY_LOG_LEVEL);

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mipi_dbi.h>
#include <zephyr/kernel.h>

#define ST75256_EXTCOM   0x30
#define ST75256_EXTCOM_1 ST75256_EXTCOM
#define ST75256_EXTCOM_2 ST75256_EXTCOM + 1
#define ST75256_EXTCOM_3 ST75256_EXTCOM + 8
#define ST75256_EXTCOM_4 ST75256_EXTCOM + 9

/* EXTCOM_1 Commands */
#define ST75256_DISPLAY_NORMAL 0xa6
#define ST75256_DISPLAY_INVERT 0xa7
#define ST75256_DISPLAY_ON     0xaf
#define ST75256_DISPLAY_OFF    0xae
#define ST75256_ALL_OFF        0x22
#define ST75256_ALL_ON         0x23
#define ST75256_SLEEP_IN       0x95
#define ST75256_SLEEP_OUT      0x94
#define ST75256_SET_VOP        0x81
#define ST75256_COL_RANGE      0x15
#define ST75256_PAGE_RANGE     0x75
#define ST75256_LSB_BOTTOM     0x8
#define ST75256_LSB_TOP        0xc
#define ST75256_FLIP_CONFIG    0xbc
#define ST75256_DISP_CONTROL   0xca
#define ST75256_MULTI_MASTER   0x6E
#define ST75256_MULTI_SLAVE    0x6F
#define ST75256_START_WRITE    0x5C

#define ST75256_COLOR_MODE 0xf0
#define ST75256_GREYSCALE  0x11
#define ST75256_MONO       0x10

#define ST75256_POWER_CONTROL 0x20

/* EXTCOM_2 Commands */
#define ST75256_AUTOREAD         0xd7
#define ST75256_AUTOREAD_ENABLE  0x8f
#define ST75256_AUTOREAD_DISABLE 0x9f
#define ST75256_ANALOG_SETTINGS  0x32
#define ST75256_OTP_READ         0xE3
#define ST75256_OTP_WRITE        0xE2
#define ST75256_OTP_OUT          0xE1
#define ST75256_SET_GREY         0x20
#define ST75256_POWER_INTERNAL   0x40
#define ST75256_POWER_EXTERNAL   0x41

#define ST75256_OTP_RW       0xE0
#define ST75256_OTP_RW_READ  0x00
#define ST75256_OTP_RW_WRITE 0x20

#define ST75256_BOOSTER_LEVEL    0x51
#define ST75256_BOOSTER_LEVEL_10 0xFB
#define ST75256_BOOSTER_LEVEL_8  0xFA

struct st75256_config {
	const struct device *mipi_dev;
	struct mipi_dbi_config dbi_config;
	uint16_t height;
	uint16_t width;
	uint8_t booster_frequency;
	uint8_t bias_ratio;
	bool lsb_invdir;
	uint8_t flip_configuration;
	uint8_t duty;
	uint8_t fi_settings;
	uint8_t power_control;
	uint8_t light_grey;
	uint8_t dark_grey;
	bool external_power;
	bool inversion_on;
	uint8_t *conversion_buf;
	size_t conversion_buf_size;
};

struct st75256_data {
	int current_pixel_format;
	int current_screen_info;
};

static inline int st75256_write_command(const struct device *dev, uint8_t cmd, const uint8_t *buf,
					size_t len)
{
	const struct st75256_config *config = dev->config;

	return mipi_dbi_command_write(config->mipi_dev, &config->dbi_config, cmd, buf, len);
}

static int st75256_blanking_on(const struct device *dev)
{
	int err;

	err = st75256_write_command(dev, ST75256_EXTCOM_1, NULL, 0);
	if (err < 0) {
		return err;
	}
	err = st75256_write_command(dev, ST75256_DISPLAY_OFF, NULL, 0);
	if (err < 0) {
		return err;
	}
	return st75256_write_command(dev, ST75256_SLEEP_IN, NULL, 0);
}

static int st75256_blanking_off(const struct device *dev)
{
	int err;

	err = st75256_write_command(dev, ST75256_EXTCOM_1, NULL, 0);
	if (err < 0) {
		return err;
	}
	err = st75256_write_command(dev, ST75256_SLEEP_OUT, NULL, 0);
	if (err < 0) {
		return err;
	}
	/* Wait 10 msec to allow display out of sleep */
	k_msleep(10);
	return st75256_write_command(dev, ST75256_DISPLAY_ON, NULL, 0);
}

static int st75256_set_window(const struct device *dev, int x, int y, int width, int height)
{
	struct st75256_data *data = dev->data;
	int ret;
	uint8_t x_position[] = {x, x + width - 1};
	uint8_t y_position[2];

	if (data->current_pixel_format == PIXEL_FORMAT_L_8) {
		y_position[0] = y / 4;
		y_position[1] = ((y + height) / 4) - 1;
	} else {
		y_position[0] = y / 8;
		y_position[1] = ((y + height) / 8) - 1;
	}

	ret = st75256_write_command(dev, ST75256_EXTCOM_1, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	ret = st75256_write_command(dev, ST75256_PAGE_RANGE, y_position, 2);
	if (ret < 0) {
		return ret;
	}

	return st75256_write_command(dev, ST75256_COL_RANGE, x_position, 2);
}

static int st75256_start_write(const struct device *dev)
{
	int ret;

	ret = st75256_write_command(dev, ST75256_EXTCOM_1, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	return st75256_write_command(dev, ST75256_START_WRITE, NULL, 0);
}

static int st75256_write_pixels_MONO01(const struct device *dev, const uint16_t x, const uint16_t y,
				       const uint8_t *buf,
				       const struct display_buffer_descriptor *desc)
{
	const struct st75256_config *config = dev->config;
	struct display_buffer_descriptor mipi_desc;
	int ret;

	for (int i = 0; i < desc->height / 8; i++) {
		st75256_set_window(dev, x, y + i * 8, desc->width, desc->height);
		st75256_start_write(dev);
		mipi_desc.buf_size = desc->width;
		mipi_desc.width = desc->width;
		mipi_desc.height = 8;
		ret = mipi_dbi_write_display(config->mipi_dev, &config->dbi_config, buf, &mipi_desc,
					     PIXEL_FORMAT_MONO01);
		if (ret < 0) {
			return ret;
		}
	}
	return mipi_dbi_release(config->mipi_dev, &config->dbi_config);
}

/* ST75256 4-level is greyscale is 4 pixels per byte vtiled.
 * It also does not have windowing capability so we do one line per one line.
 */
static int st75256_write_pixels_L_8(const struct device *dev, const uint16_t x, const uint16_t y,
				    const uint8_t *buf,
				    const struct display_buffer_descriptor *desc)
{
	const struct st75256_config *config = dev->config;
	struct display_buffer_descriptor mipi_desc;
	int32_t line_count = desc->height;
	int ret, l;
	int line_total = 0;

	mipi_desc.pitch = desc->pitch;

	st75256_set_window(dev, x, y, desc->width, desc->height);
	st75256_start_write(dev);
	while (line_count > line_total) {
		l = 0;

		/* Convert what the conversion buffer can hold to
		 * pixelx[7:6], pixelx+pitch[5:4], pixelx+2pitch[3:2], pixelx+3pitch[1:0]
		 */
		for (; l * desc->width / 4 < config->conversion_buf_size &&
		       line_count > line_total + l;
		     l += 4) {
			int i_lt = line_total + l;

			for (int i = 0; i < desc->width &&
					i + l * desc->width / 4 < config->conversion_buf_size;
			     i++) {
				int i_l = i + l * desc->width / 4;

				config->conversion_buf[i_l] =
					((buf[i + (i_lt + 0) * desc->pitch] >> 6) << 0) |
					((buf[i + (i_lt + 1) * desc->pitch] >> 6) << 2) |
					((buf[i + (i_lt + 2) * desc->pitch] >> 6) << 4) |
					((buf[i + (i_lt + 3) * desc->pitch] >> 6) << 6);
			}
		}
		mipi_desc.buf_size = l * desc->width / 4;
		mipi_desc.width = desc->width;
		mipi_desc.height = l;

		/* This is the wrong format, but it doesn't matter to almost all mipi drivers */
		ret = mipi_dbi_write_display(config->mipi_dev, &config->dbi_config,
					     config->conversion_buf, &mipi_desc, PIXEL_FORMAT_L_8);
		if (ret < 0) {
			return ret;
		}
		line_total += l;
	}
	mipi_dbi_release(config->mipi_dev, &config->dbi_config);
	return 0;
}

static int st75256_write(const struct device *dev, const uint16_t x, const uint16_t y,
			 const struct display_buffer_descriptor *desc, const void *buf)
{
	struct st75256_data *data = dev->data;
	size_t buf_len;

	/* pitch is always width because of vtiled monochrome at 8 pixels per byte
	 * or greyscale at one pixel per byte converted to vtiled 4 pixels per byte
	 */
	if (desc->pitch != desc->width) {
		LOG_ERR("Pitch is not width");
		return -EINVAL;
	}

	if (data->current_pixel_format == PIXEL_FORMAT_MONO01) {
		buf_len = MIN(desc->buf_size, desc->height * desc->width / 8);
		if ((y % 8) != 0 || (desc->height % 8) != 0) {
			LOG_ERR("Y and height must be aligned on 8 boundary");
			return -EINVAL;
		}
	} else if (data->current_pixel_format == PIXEL_FORMAT_L_8) {
		buf_len = MIN(desc->buf_size, desc->height * desc->width / 4);
		if ((y % 4) != 0 || (desc->height % 4) != 0) {
			LOG_ERR("Y and height must be aligned on 4 boundary");
			return -EINVAL;
		}
	} else {
		return -EINVAL;
	}

	if (buf == NULL || buf_len == 0U) {
		LOG_ERR("Display buffer is not available");
		return -EINVAL;
	}

	LOG_DBG("x %u, y %u, pitch %u, width %u, height %u, buf_len %u", x, y, desc->pitch,
		desc->width, desc->height, buf_len);

	if (data->current_pixel_format == PIXEL_FORMAT_L_8) {
		return st75256_write_pixels_L_8(dev, x, y, buf, desc);
	} else {
		return st75256_write_pixels_MONO01(dev, x, y, buf, desc);
	}
}

static int st75256_set_contrast(const struct device *dev, const uint8_t contrast)
{
	int err;
	uint8_t contrast_out[2];

	err = st75256_write_command(dev, ST75256_EXTCOM_1, NULL, 0);
	if (err < 0) {
		return err;
	}
	contrast_out[0] = (contrast & 0x1f) << 1;
	contrast_out[1] = contrast >> 5;
	return st75256_write_command(dev, ST75256_SET_VOP, contrast_out, sizeof(contrast_out));
}

static void st75256_get_capabilities(const struct device *dev, struct display_capabilities *caps)
{
	const struct st75256_config *config = dev->config;
	struct st75256_data *data = dev->data;

	memset(caps, 0, sizeof(struct display_capabilities));
	caps->x_resolution = config->width;
	caps->y_resolution = config->height;
	caps->supported_pixel_formats = PIXEL_FORMAT_MONO01 | PIXEL_FORMAT_L_8;
	caps->current_pixel_format = data->current_pixel_format;
	caps->screen_info = data->current_screen_info;
}

static int st75256_set_pixel_format(const struct device *dev,
				    const enum display_pixel_format pixel_format)
{
	struct st75256_data *data = dev->data;
	uint8_t tmp;
	int ret;

	if (pixel_format == PIXEL_FORMAT_MONO01) {
		tmp = ST75256_MONO;
		ret = st75256_write_command(dev, ST75256_COLOR_MODE, &tmp, 1);
		if (ret < 0) {
			return ret;
		}
		data->current_screen_info = SCREEN_INFO_MONO_VTILED;
		data->current_pixel_format = PIXEL_FORMAT_MONO01;
	} else if (pixel_format == PIXEL_FORMAT_L_8) {
		tmp = ST75256_GREYSCALE;
		ret = st75256_write_command(dev, ST75256_COLOR_MODE, &tmp, 1);
		if (ret < 0) {
			return ret;
		}
		data->current_screen_info = 0;
		data->current_pixel_format = PIXEL_FORMAT_L_8;
	} else {
		LOG_ERR("Unsupported Pixel format");
		return -EINVAL;
	}
	return 0;
}

static int st75256_init_device(const struct device *dev)
{
	int ret;
	uint8_t data[16];
	const struct st75256_config *config = dev->config;

	ret = mipi_dbi_reset(config->mipi_dev, 1);
	if (ret < 0) {
		return ret;
	}
	k_msleep(10);

	ret = st75256_blanking_off(dev);
	if (ret < 0) {
		return ret;
	}

	ret = st75256_write_command(dev, ST75256_EXTCOM_1, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	/* Enable Master mode (multidisplay) */
	ret = st75256_write_command(dev, ST75256_MULTI_MASTER, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	ret = st75256_write_command(dev, ST75256_EXTCOM_2, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	data[0] = ST75256_AUTOREAD_DISABLE;
	ret = st75256_write_command(dev, ST75256_AUTOREAD, data, 1);
	if (ret < 0) {
		return ret;
	}

	data[0] = ST75256_OTP_RW_READ;
	ret = st75256_write_command(dev, ST75256_OTP_RW, data, 1);
	if (ret < 0) {
		return ret;
	}

	k_msleep(10);
	/* Load OTPs */
	ret = st75256_write_command(dev, ST75256_OTP_READ, NULL, 0);
	if (ret < 0) {
		return ret;
	}
	k_msleep(20);

	ret = st75256_write_command(dev, ST75256_OTP_OUT, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	ret = st75256_blanking_off(dev);
	if (ret < 0) {
		return ret;
	}
	k_msleep(20);

	ret = st75256_write_command(dev, ST75256_EXTCOM_1, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	ret = st75256_write_command(dev, ST75256_POWER_CONTROL, &config->power_control, 1);
	if (ret < 0) {
		return ret;
	}

	ret = st75256_set_contrast(dev, CONFIG_ST75256_DEFAULT_CONTRAST);
	if (ret < 0) {
		return ret;
	}

	ret = st75256_write_command(dev, ST75256_EXTCOM_2, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	data[0] = 0;
	data[1] = 0;
	data[2] = 0;
	data[3] = config->light_grey;
	data[4] = config->light_grey;
	data[5] = config->light_grey;
	data[6] = 0;
	data[7] = 0;
	data[8] = config->dark_grey;
	data[9] = 0;
	data[10] = 0;
	data[11] = config->dark_grey;
	data[12] = config->dark_grey;
	data[13] = config->dark_grey;
	data[14] = 0;
	data[15] = 0;
	ret = st75256_write_command(dev, ST75256_SET_GREY, data, 16);
	if (ret < 0) {
		return ret;
	}

	data[0] = 0;
	data[1] = config->booster_frequency;
	data[2] = config->bias_ratio;
	ret = st75256_write_command(dev, ST75256_ANALOG_SETTINGS, data, 3);
	if (ret < 0) {
		return ret;
	}

	data[0] = ST75256_BOOSTER_LEVEL_10;
	ret = st75256_write_command(dev, ST75256_BOOSTER_LEVEL, data, 1);
	if (ret < 0) {
		return ret;
	}

	ret = st75256_write_command(
		dev, config->external_power ? ST75256_POWER_EXTERNAL : ST75256_POWER_INTERNAL, NULL,
		0);
	if (ret < 0) {
		return ret;
	}

	ret = st75256_write_command(dev, ST75256_EXTCOM_1, NULL, 0);
	if (ret < 0) {
		return ret;
	}

#if CONFIG_ST75256_DEFAULT_GREYSCALE
	data[0] = ST75256_GREYSCALE;
	ret = st75256_write_command(dev, ST75256_COLOR_MODE, data, 1);
	if (ret < 0) {
		return ret;
	}
#else
	data[0] = ST75256_MONO;
	ret = st75256_write_command(dev, ST75256_COLOR_MODE, data, 1);
	if (ret < 0) {
		return ret;
	}
#endif

	ret = st75256_write_command(dev, config->lsb_invdir ? ST75256_LSB_BOTTOM : ST75256_LSB_TOP,
				    NULL, 0);
	if (ret < 0) {
		return ret;
	}

	data[0] = 0;
	data[1] = config->duty;
	data[2] = config->fi_settings;
	ret = st75256_write_command(dev, ST75256_DISP_CONTROL, data, 3);
	if (ret < 0) {
		return ret;
	}

	data[0] = config->flip_configuration;
	ret = st75256_write_command(dev, ST75256_FLIP_CONFIG, data, 1);
	if (ret < 0) {
		return ret;
	}

	ret = st75256_write_command(
		dev, config->inversion_on ? ST75256_DISPLAY_INVERT : ST75256_DISPLAY_NORMAL, NULL,
		0);
	if (ret < 0) {
		return ret;
	}
	return st75256_blanking_off(dev);
}

static int st75256_init(const struct device *dev)
{
	const struct st75256_config *config = dev->config;
	int ret;

	if (!device_is_ready(config->mipi_dev)) {
		LOG_ERR("MIPI not ready!");
		return -ENODEV;
	}

	ret = st75256_init_device(dev);
	if (ret < 0) {
		LOG_ERR("Failed to initialize device, err = %d", ret);
	}

	return ret;
}

static DEVICE_API(display, st75256_driver_api) = {
	.blanking_on = st75256_blanking_on,
	.blanking_off = st75256_blanking_off,
	.write = st75256_write,
	.set_contrast = st75256_set_contrast,
	.get_capabilities = st75256_get_capabilities,
	.set_pixel_format = st75256_set_pixel_format,
};

#define ST75256_WORD_SIZE(inst)                                                                    \
	((DT_STRING_UPPER_TOKEN(inst, mipi_mode) == MIPI_DBI_MODE_SPI_4WIRE) ? SPI_WORD_SET(8)     \
									     : SPI_WORD_SET(9))

#define ST75256_CONV_BUFFER_SIZE(node_id)                                                          \
	DIV_ROUND_UP(DT_PROP(node_id, width) * CONFIG_ST75256_CONV_BUFFER_LINES, 4)

#if CONFIG_ST75256_DEFAULT_GREYSCALE
#define ST75256_DATA(node_id)                                                                      \
	static struct st75256_data data##node_id = {                                               \
		.current_pixel_format = PIXEL_FORMAT_L_8,                                          \
		.current_screen_info = 0,                                                          \
	};
#else
#define ST75256_DATA(node_id)                                                                      \
	static struct st75256_data data##node_id = {                                               \
		.current_pixel_format = PIXEL_FORMAT_MONO01,                                       \
		.current_screen_info = SCREEN_INFO_MONO_VTILED,                                    \
	};
#endif

#define ST75256_DEFINE(node_id)                                                                    \
	ST75256_DATA(node_id)                                                                      \
	static uint8_t conversion_buf##node_id[ST75256_CONV_BUFFER_SIZE(node_id)];                 \
	static const struct st75256_config config##node_id = {                                     \
		.height = DT_PROP(node_id, height),                                                \
		.width = DT_PROP(node_id, width),                                                  \
		.booster_frequency = DT_PROP(node_id, booster_frequency),                          \
		.bias_ratio = DT_PROP(node_id, bias_ratio),                                        \
		.lsb_invdir = DT_PROP(node_id, lsb_invdir),                                        \
		.flip_configuration = DT_PROP(node_id, flip_configuration),                        \
		.duty = DT_PROP(node_id, duty),                                                    \
		.power_control = DT_PROP(node_id, power_control),                                  \
		.light_grey = DT_PROP(node_id, light_grey),                                        \
		.dark_grey = DT_PROP(node_id, dark_grey),                                          \
		.external_power = DT_PROP(node_id, external_power),                                \
		.fi_settings = DT_PROP(node_id, fi_settings),                                      \
		.inversion_on = DT_PROP(node_id, inversion_on),                                    \
		.mipi_dev = DEVICE_DT_GET(DT_PARENT(node_id)),                                     \
		.dbi_config = MIPI_DBI_CONFIG_DT(                                                  \
			node_id, ST75256_WORD_SIZE(node_id) | SPI_OP_MODE_MASTER, 0),              \
		.conversion_buf = conversion_buf##node_id,                                         \
		.conversion_buf_size = sizeof(conversion_buf##node_id),                            \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_DEFINE(node_id, st75256_init, NULL, &data##node_id, &config##node_id,            \
			 POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY, &st75256_driver_api);

DT_FOREACH_STATUS_OKAY(sitronix_st75256, ST75256_DEFINE)
