/*
 * Copyright (c) 2026 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sitronix_st7586s

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(st7586s, CONFIG_DISPLAY_LOG_LEVEL);

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mipi_dbi.h>
#include <zephyr/kernel.h>

/* Constants */
#define ST7586S_RESET_MS		120
#define ST7586S_AUTOREAD_ENABLE		0x8f
#define ST7586S_AUTOREAD_DISABLE	0x9f
#define ST7586S_OTP_RW_READ		0x00
#define ST7586S_OTP_RW_WRITE		0x20
#define ST7586S_ANALOG_EN_1		0x1d
#define ST7586S_N_LINE_INV_FRAME	0x0
#define ST7586S_DDRAM_L2		0x2
#define ST7586S_DDRAM_L4		0x3
#define ST7586S_PPB_MONO		3
#define ST7586S_PPB_GRAY		2
#define ST7586S_PPC			3
#define ST7586S_PPA_MONO		(ST7586S_PPB_MONO * ST7586S_PPC)
#define ST7586S_PPA_GRAY		(ST7586S_PPB_GRAY * ST7586S_PPC)

/* Commands */
#define ST7586S_GRAYSCALE		0x38
#define ST7586S_MONO			0x39
#define ST7586S_SLEEP_IN		0x10
#define ST7586S_SLEEP_OUT		0x11
#define ST7586S_DISPLAY_ON		0x29
#define ST7586S_DISPLAY_OFF		0x28
#define ST7586S_AUTOREAD		0xd7
#define ST7586S_OTP_RW			0xe0
#define ST7586S_OTP_READ		0xe3
#define ST7586S_OTP_WRITE		0xe2
#define ST7586S_OTP_OUT			0xe1
#define ST7586S_SET_VOP			0xc0
#define ST7586S_SET_BIAS		0xc3
#define ST7586S_SET_BOOSTER_LEVEL	0xc4
#define ST7586S_ANALOG_EN_0		0xd0
#define ST7586S_SET_N_LINE_INV		0xb5
#define ST7586S_SET_DDRAM_MODE		0x3a
#define ST7586S_SET_FLIP_CONFIG		0x36
#define ST7586S_DISPLAY_NORMAL		0x20
#define ST7586S_DISPLAY_INVERT		0x21
#define ST7586S_SET_DUTY		0xb0
#define ST7586S_ALL_OFF			0x22
#define ST7586S_ALL_ON			0x23
#define ST7586S_SET_FRAMERATE_MONO	0xf1
#define ST7586S_SET_FRAMERATE_GRAY	0xf0
#define ST7586S_SET_START_LINE		0x37
#define ST7586S_SET_FIRST_COM		0xb1
#define ST7586S_SET_ROW_RANGE		0x2b
#define ST7586S_SET_COL_RANGE		0x2a
#define ST7586S_START_WRITE		0x2c

#define GET_MONO_PX(_buf, _i) ((_buf[(_i) / 8] & (1U << ((_i) % 8))) >> ((_i) % 8))

struct st7586s_config {
	const struct device *mipi_dev;
	struct mipi_dbi_config dbi_config;
	uint16_t height;
	uint16_t width;
	uint8_t start_line;
	uint8_t display_offset;
	uint8_t booster_level;
	uint8_t bias_ratio;
	uint8_t flip_configuration;
	uint8_t duty;
	uint8_t framerate;
	bool inversion_on;
	uint8_t *conversion_buf;
	size_t conversion_buf_size;
};

struct st7586s_data {
	int current_pixel_format;
};

static inline int st7586s_write_command(const struct device *dev, uint8_t cmd, const uint8_t *buf,
					size_t len)
{
	const struct st7586s_config *config = dev->config;

	return mipi_dbi_command_write(config->mipi_dev, &config->dbi_config, cmd, buf, len);
}

static int st7586s_blanking_on(const struct device *dev)
{
	int err;

	err = st7586s_write_command(dev, ST7586S_DISPLAY_OFF, NULL, 0);
	if (err < 0) {
		return err;
	}
	return st7586s_write_command(dev, ST7586S_SLEEP_IN, NULL, 0);
}

static int st7586s_blanking_off(const struct device *dev)
{
	int err;

	err = st7586s_write_command(dev, ST7586S_SLEEP_OUT, NULL, 0);
	if (err < 0) {
		return err;
	}
	/* Wait 10 msec to allow display out of sleep */
	k_msleep(10);
	return st7586s_write_command(dev, ST7586S_DISPLAY_ON, NULL, 0);
}

static int st7586s_set_window(const struct device *dev, int x, int y, int width, int height)
{
	int ret;
	const uint8_t y_position[] = { 0, y, 0, y + height - 1 };
	const uint8_t x_position[] = { 0, x / ST7586S_PPC, 0, ((x + width) / ST7586S_PPC) - 1 };

	ret = st7586s_write_command(dev, ST7586S_SET_ROW_RANGE, y_position, 4);
	if (ret < 0) {
		return ret;
	}

	return st7586s_write_command(dev, ST7586S_SET_COL_RANGE, x_position, 4);
}

static int st7586s_start_write(const struct device *dev)
{
	return st7586s_write_command(dev, ST7586S_START_WRITE, NULL, 0);
}

/* ST7586S Mono is htiled 3 bit 3 bit 2 bit for 3 pixels */
static int st7586s_convert_MONO(const struct device *dev, const uint8_t *buf, int cur_offset,
				uint32_t pixel_count, bool mono01)
{
	const struct st7586s_config *config = dev->config;
	int i = 0;
	size_t i_d;

	for (; i / ST7586S_PPB_MONO < config->conversion_buf_size && pixel_count > cur_offset + i;
	     i += ST7586S_PPB_MONO) {
		i_d = cur_offset + i;
		if (mono01) {
			config->conversion_buf[i / ST7586S_PPB_MONO] =
				GET_MONO_PX(buf, i_d) << 7
				| GET_MONO_PX(buf, i_d) << 6
				| GET_MONO_PX(buf, i_d) << 5
				| GET_MONO_PX(buf, i_d + 1) << 4
				| GET_MONO_PX(buf, i_d + 1) << 3
				| GET_MONO_PX(buf, i_d + 1) << 2
				| GET_MONO_PX(buf, i_d + 2) << 1
				| GET_MONO_PX(buf, i_d + 2);
		} else {
			config->conversion_buf[i / ST7586S_PPB_MONO] =
				~(GET_MONO_PX(buf, i_d) << 7
				| GET_MONO_PX(buf, i_d) << 6
				| GET_MONO_PX(buf, i_d) << 5
				| GET_MONO_PX(buf, i_d + 1) << 4
				| GET_MONO_PX(buf, i_d + 1) << 3
				| GET_MONO_PX(buf, i_d + 1) << 2
				| GET_MONO_PX(buf, i_d + 2) << 1
				| GET_MONO_PX(buf, i_d + 2));
		}
	}
	return i;
}

/* Convert what the conversion buffer can hold to pixelx+1 (3:0) and pixelx (7:4) */
static int st7586s_convert_L_8(const struct device *dev, const uint8_t *buf, int cur_offset,
			       uint32_t pixel_count)
{
	const struct st7586s_config *config = dev->config;
	int i = 0;

	for (; i / ST7586S_PPB_GRAY < config->conversion_buf_size && pixel_count > cur_offset + i;
	     i += ST7586S_PPB_GRAY) {
		config->conversion_buf[i / ST7586S_PPB_GRAY] = buf[cur_offset + i + 1] >> 4
							| (buf[cur_offset + i] >> 4) << 4;

	}
	return i;
}

static int st7586s_write(const struct device *dev, const uint16_t x, const uint16_t y,
			 const struct display_buffer_descriptor *desc, const void *buf)
{
	const struct st7586s_config *config = dev->config;
	const uint32_t pixel_count = desc->height * desc->width;
	struct st7586s_data *data = dev->data;
	size_t expected_len;
	struct display_buffer_descriptor mipi_desc;
	int ret, i;
	int total = 0;

	if (desc->pitch != desc->width) {
		LOG_ERR("Pitch is different from width");
		return -EINVAL;
	}

	if (data->current_pixel_format == PIXEL_FORMAT_MONO01
	    || data->current_pixel_format == PIXEL_FORMAT_MONO10) {
		expected_len = desc->height * desc->width / 8;
		if ((x % ST7586S_PPC) != 0 || (desc->width % ST7586S_PPC) != 0) {
			LOG_ERR("X and width must be aligned on %d boundary", ST7586S_PPC);
			return -EINVAL;
		}
	} else if (data->current_pixel_format == PIXEL_FORMAT_L_8) {
		expected_len = desc->height * desc->width / ST7586S_PPB_GRAY;
		if ((x % ST7586S_PPA_GRAY) != 0 || (desc->width % ST7586S_PPA_GRAY) != 0) {
			LOG_ERR("X and width must be aligned on %d boundary", ST7586S_PPA_GRAY);
			return -EINVAL;
		}
	} else {
		return -EINVAL;
	}

	if (buf == NULL || desc->buf_size < expected_len) {
		LOG_ERR("Display buffer is invalid");
		return -EINVAL;
	}

	LOG_DBG("x %u, y %u, pitch %u, width %u, height %u, len %u", x, y, desc->pitch,
		desc->width, desc->height, expected_len);

	st7586s_set_window(dev, x, y, desc->width, desc->height);
	st7586s_start_write(dev);

	mipi_desc.pitch = desc->pitch;

	while (pixel_count > total) {
		if (data->current_pixel_format == PIXEL_FORMAT_L_8) {
			i = st7586s_convert_L_8(dev, buf, total, pixel_count);
			mipi_desc.buf_size = i / ST7586S_PPB_GRAY;
		} else {
			i = st7586s_convert_MONO(dev, buf, total, pixel_count,
						 data->current_pixel_format == PIXEL_FORMAT_MONO01);
			mipi_desc.buf_size = i / ST7586S_PPB_MONO;
		}

		mipi_desc.width = mipi_desc.buf_size / desc->height;
		mipi_desc.height = mipi_desc.buf_size / desc->width;

		ret = mipi_dbi_write_display(config->mipi_dev, &config->dbi_config,
					     config->conversion_buf, &mipi_desc,
					     data->current_pixel_format);
		if (ret < 0) {
			return ret;
		}
		total += i;
	}
	return mipi_dbi_release(config->mipi_dev, &config->dbi_config);
}

static int st7586s_set_contrast(const struct device *dev, const uint8_t contrast)
{
	uint8_t contrast_out[2];

	contrast_out[0] = (contrast & 0x7f) << 1;
	contrast_out[1] = contrast >> 7;
	return st7586s_write_command(dev, ST7586S_SET_VOP, contrast_out, sizeof(contrast_out));
}

static void st7586s_get_capabilities(const struct device *dev, struct display_capabilities *caps)
{
	const struct st7586s_config *config = dev->config;
	struct st7586s_data *data = dev->data;

	memset(caps, 0, sizeof(struct display_capabilities));
	caps->x_resolution = config->width;
	caps->y_resolution = config->height;
	caps->supported_pixel_formats = PIXEL_FORMAT_MONO10
		| PIXEL_FORMAT_MONO01 | PIXEL_FORMAT_L_8;
	caps->current_pixel_format = data->current_pixel_format;
	caps->screen_info = 0;
}

static int st7586s_set_pixel_format(const struct device *dev,
				    const enum display_pixel_format pixel_format)
{
	struct st7586s_data *data = dev->data;
	uint8_t buf;
	int ret;

	if (pixel_format == PIXEL_FORMAT_MONO01 || pixel_format == PIXEL_FORMAT_MONO10) {
		ret = st7586s_write_command(dev, ST7586S_MONO, NULL, 0);
		if (ret < 0) {
			return ret;
		}
		buf = ST7586S_DDRAM_L2;
		ret = st7586s_write_command(dev, ST7586S_SET_DDRAM_MODE, &buf, 1);
		if (ret < 0) {
			return ret;
		}
		data->current_pixel_format = pixel_format;
	} else if (pixel_format == PIXEL_FORMAT_L_8) {
		ret = st7586s_write_command(dev, ST7586S_GRAYSCALE, NULL, 0);
		if (ret < 0) {
			return ret;
		}
		buf = ST7586S_DDRAM_L4;
		ret = st7586s_write_command(dev, ST7586S_SET_DDRAM_MODE, &buf, 1);
		if (ret < 0) {
			return ret;
		}
		data->current_pixel_format = PIXEL_FORMAT_L_8;
	} else {
		LOG_ERR("Unsupported Pixel format");
		return -EINVAL;
	}
	return 0;
}

static int st7586s_init_device(const struct device *dev)
{
	int ret;
	uint8_t data[4];
	const struct st7586s_config *config = dev->config;

	ret = mipi_dbi_reset(config->mipi_dev, 5);
	if (ret < 0) {
		return ret;
	}
	k_msleep(ST7586S_RESET_MS);

	data[0] = ST7586S_AUTOREAD_DISABLE;
	ret = st7586s_write_command(dev, ST7586S_AUTOREAD, data, 1);
	if (ret < 0) {
		return ret;
	}

	data[0] = ST7586S_OTP_RW_READ;
	ret = st7586s_write_command(dev, ST7586S_OTP_RW, data, 1);
	if (ret < 0) {
		return ret;
	}
	k_msleep(10);

	/* Load OTPs */
	ret = st7586s_write_command(dev, ST7586S_OTP_READ, NULL, 0);
	if (ret < 0) {
		return ret;
	}
	k_msleep(20);

	ret = st7586s_write_command(dev, ST7586S_OTP_OUT, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	ret = st7586s_blanking_off(dev);
	if (ret < 0) {
		return ret;
	}
	k_msleep(40);

	ret = st7586s_set_contrast(dev, CONFIG_ST7586S_DEFAULT_CONTRAST);
	if (ret < 0) {
		return ret;
	}

	ret = st7586s_write_command(dev, ST7586S_SET_BIAS, &config->bias_ratio, 1);
	if (ret < 0) {
		return ret;
	}

	ret = st7586s_write_command(dev, ST7586S_SET_BOOSTER_LEVEL, &config->booster_level, 1);
	if (ret < 0) {
		return ret;
	}

	data[0] = ST7586S_ANALOG_EN_1;
	ret = st7586s_write_command(dev, ST7586S_ANALOG_EN_0, data, 1);
	if (ret < 0) {
		return ret;
	}

	data[0] = ST7586S_N_LINE_INV_FRAME;
	ret = st7586s_write_command(dev, ST7586S_SET_N_LINE_INV, data, 1);
	if (ret < 0) {
		return ret;
	}

#if CONFIG_ST7586S_DEFAULT_GRAYSCALE
	ret = st7586s_write_command(dev, ST7586S_GRAYSCALE, NULL, 0);
	if (ret < 0) {
		return ret;
	}
	data[0] = ST7586S_DDRAM_L4;
	ret = st7586s_write_command(dev, ST7586S_SET_DDRAM_MODE, data, 1);
	if (ret < 0) {
		return ret;
	}
#else
	ret = st7586s_write_command(dev, ST7586S_MONO, NULL, 0);
	if (ret < 0) {
		return ret;
	}
	data[0] = ST7586S_DDRAM_L2;
	ret = st7586s_write_command(dev, ST7586S_SET_DDRAM_MODE, data, 1);
	if (ret < 0) {
		return ret;
	}
#endif

	ret = st7586s_write_command(dev, ST7586S_SET_FLIP_CONFIG, &config->flip_configuration, 1);
	if (ret < 0) {
		return ret;
	}

	ret = st7586s_write_command(dev, ST7586S_SET_DUTY, &config->duty, 1);
	if (ret < 0) {
		return ret;
	}

	data[0] = config->framerate;
	data[1] = config->framerate;
	data[2] = config->framerate;
	data[3] = config->framerate;
	ret = st7586s_write_command(dev, ST7586S_SET_FRAMERATE_GRAY, data, 4);
	if (ret < 0) {
		return ret;
	}

	data[0] = config->framerate;
	data[1] = config->framerate;
	data[2] = config->framerate;
	data[3] = config->framerate;
	ret = st7586s_write_command(dev, ST7586S_SET_FRAMERATE_MONO, data, 4);
	if (ret < 0) {
		return ret;
	}

	ret = st7586s_write_command(dev, ST7586S_SET_START_LINE, &config->start_line, 1);
	if (ret < 0) {
		return ret;
	}

	ret = st7586s_write_command(dev, ST7586S_SET_FIRST_COM, &config->display_offset, 1);
	if (ret < 0) {
		return ret;
	}

	ret = st7586s_write_command(
		dev, config->inversion_on ? ST7586S_DISPLAY_INVERT : ST7586S_DISPLAY_NORMAL, NULL,
		0);
	if (ret < 0) {
		return ret;
	}
	return st7586s_blanking_off(dev);
}

static int st7586s_init(const struct device *dev)
{
	const struct st7586s_config *config = dev->config;
	int ret;

	if (!device_is_ready(config->mipi_dev)) {
		LOG_ERR("MIPI not ready!");
		return -ENODEV;
	}

	ret = st7586s_init_device(dev);
	if (ret < 0) {
		LOG_ERR("Failed to initialize device, err = %d", ret);
	}

	return ret;
}

static DEVICE_API(display, st7586s_driver_api) = {
	.blanking_on = st7586s_blanking_on,
	.blanking_off = st7586s_blanking_off,
	.write = st7586s_write,
	.set_contrast = st7586s_set_contrast,
	.get_capabilities = st7586s_get_capabilities,
	.set_pixel_format = st7586s_set_pixel_format,
};

#define ST7586S_WORD_SIZE(inst)                                                                    \
	((DT_STRING_UPPER_TOKEN(inst, mipi_mode) == MIPI_DBI_MODE_SPI_4WIRE) ? SPI_WORD_SET(8)     \
									     : SPI_WORD_SET(9))

#define ST7586S_CONV_BUFFER_SIZE(node_id)                                                          \
	DIV_ROUND_UP(DT_PROP(node_id, width) * CONFIG_ST7586S_CONV_BUFFER_LINES, 2)

#if CONFIG_ST7586S_DEFAULT_GRAYSCALE
#define ST7586S_DATA(node_id)                                                                      \
	static struct st7586s_data data##node_id = {                                               \
		.current_pixel_format = PIXEL_FORMAT_L_8,                                          \
	};
#else
#define ST7586S_DATA(node_id)                                                                      \
	static struct st7586s_data data##node_id = {                                               \
		.current_pixel_format = PIXEL_FORMAT_MONO10,                                       \
	};
#endif

#define ST7586S_DEFINE(node_id)                                                                    \
	ST7586S_DATA(node_id)                                                                      \
	static uint8_t conversion_buf##node_id[ST7586S_CONV_BUFFER_SIZE(node_id)];                 \
	static const struct st7586s_config config##node_id = {                                     \
		.height = DT_PROP(node_id, height),                                                \
		.width = DT_PROP(node_id, width),                                                  \
		.bias_ratio = DT_PROP(node_id, bias_ratio),                                        \
		.flip_configuration = DT_PROP(node_id, flip_configuration),                        \
		.duty = DT_PROP(node_id, duty),                                                    \
		.framerate = DT_PROP(node_id, framerate),                                          \
		.booster_level = DT_PROP(node_id, booster_level),                                  \
		.inversion_on = DT_PROP(node_id, inversion_on),                                    \
		.start_line = DT_PROP(node_id, start_line),                                        \
		.display_offset = DT_PROP(node_id, display_offset),                                \
		.mipi_dev = DEVICE_DT_GET(DT_PARENT(node_id)),                                     \
		.dbi_config = MIPI_DBI_CONFIG_DT(                                                  \
			node_id, ST7586S_WORD_SIZE(node_id) | SPI_OP_MODE_MASTER, 0),              \
		.conversion_buf = conversion_buf##node_id,                                         \
		.conversion_buf_size = sizeof(conversion_buf##node_id),                            \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_DEFINE(node_id, st7586s_init, NULL, &data##node_id, &config##node_id,            \
			 POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY, &st7586s_driver_api);

DT_FOREACH_STATUS_OKAY(sitronix_st7586s, ST7586S_DEFINE)
