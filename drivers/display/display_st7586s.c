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
#define ST7586S_FLIP_X_EN		0x48

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

typedef int (*st7586s_convert)(const struct device *dev, const uint8_t *buf, int cur_offset,
			       uint32_t pixel_count, bool arg);

/* These cannot be pure enums due to the preprocessor evaluations */
#define PIXMAP_NORMAL	0
#define PIXMAP_2_1_1	1

enum st7586s_pixel_mapping {
	PIXMAP_NORMAL_e = PIXMAP_NORMAL,
	PIXMAP_2_1_1_e = PIXMAP_2_1_1,
	PIXMAP_MAX,
};

#define IS_PIXMAP_2_1_1(inst) \
	+ IS_EQ(DT_INST_ENUM_IDX_OR(inst, pixel_mapping, PIXMAP_NORMAL), PIXMAP_2_1_1)

#define IS_PIXMAP_NORMAL(inst) \
	+ IS_EQ(DT_INST_ENUM_IDX_OR(inst, pixel_mapping, PIXMAP_NORMAL), PIXMAP_NORMAL)

#define ANY_PIXMAP_NORMAL() \
	0 DT_INST_FOREACH_STATUS_OKAY(IS_PIXMAP_NORMAL)

#define ANY_PIXMAP_2_1_1() \
	0 DT_INST_FOREACH_STATUS_OKAY(IS_PIXMAP_2_1_1)

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
	enum st7586s_pixel_mapping pixel_mapping;
	bool inversion_on;
	uint8_t *conversion_buf;
	size_t conversion_buf_size;
};

struct st7586s_data {
	int current_pixel_format;
};

static inline uint8_t get_mono_px(const uint8_t *buf, size_t i)
{
	return (buf[i / 8] >> (i % 8)) & 0x1U;
}

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
	const struct st7586s_config *config = dev->config;
	int ret;
	const uint8_t y_position[] = { 0, y, 0, y + height - 1 };
	uint8_t x_position[4] = {0};

#if ANY_PIXMAP_2_1_1()
	if (config->pixel_mapping == PIXMAP_2_1_1) {
		x_position[1] = (x * 3U) / (2U * ST7586S_PPC);
		x_position[3] = (((x + width) * 3U) / (2U * ST7586S_PPC)) - 1U;
	}
#endif
#if ANY_PIXMAP_NORMAL()
	if (config->pixel_mapping == PIXMAP_NORMAL) {
		x_position[1] = x / ST7586S_PPC;
		x_position[3] = ((x + width) / ST7586S_PPC) - 1U;
	}
#endif

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

#if ANY_PIXMAP_NORMAL()

/* ST7586S Mono is htiled 3 bit 3 bit 2 bit for 3 pixels */
static int st7586s_convert_mono_normal(const struct device *dev, const uint8_t *buf, int cur_offset,
				       uint32_t pixel_count, bool mono01)
{
	const struct st7586s_config *config = dev->config;
	int i = 0;
	uint8_t byte;
	size_t i_d;

	for (; i / ST7586S_PPB_MONO < config->conversion_buf_size && pixel_count > cur_offset + i;
	     i += ST7586S_PPB_MONO) {
		i_d = cur_offset + i;

		byte = get_mono_px(buf, i_d) << 7
			| get_mono_px(buf, i_d) << 6
			| get_mono_px(buf, i_d) << 5
			| get_mono_px(buf, i_d + 1) << 4
			| get_mono_px(buf, i_d + 1) << 3
			| get_mono_px(buf, i_d + 1) << 2
			| get_mono_px(buf, i_d + 2) << 1
			| get_mono_px(buf, i_d + 2);

		config->conversion_buf[i / ST7586S_PPB_MONO] = mono01 ? byte : ~byte;
	}
	return i;
}

/* Convert what the conversion buffer can hold to pixelx+1 (3:0) and pixelx (7:4) */
static int st7586s_convert_l_8_normal(const struct device *dev, const uint8_t *buf, int cur_offset,
				      uint32_t pixel_count, bool unused)
{
	const struct st7586s_config *config = dev->config;
	int i = 0;

	ARG_UNUSED(unused);

	for (; i / ST7586S_PPB_GRAY < config->conversion_buf_size && pixel_count > cur_offset + i;
	     i += ST7586S_PPB_GRAY) {
		config->conversion_buf[i / ST7586S_PPB_GRAY] = buf[cur_offset + i + 1] >> 4
							| (buf[cur_offset + i] >> 4) << 4;

	}
	return i;
}

#endif

#if ANY_PIXMAP_2_1_1()

static int st7586s_convert_mono_2_1_1(const struct device *dev, const uint8_t *buf, int cur_offset,
				      uint32_t pixel_count, bool mono01)
{
	const struct st7586s_config *config = dev->config;
	int i = 0;
	uint8_t byte;
	size_t i_d, i_cb;
	bool x_flipped = (config->flip_configuration & ST7586S_FLIP_X_EN) == 0;

	for (; (i * 3U) / (2U * ST7586S_PPB_MONO) + 1 < config->conversion_buf_size
	       && pixel_count > cur_offset + i;
	     i += 4U) {
		i_d = cur_offset + i;
		i_cb = (i * 3U) / (2U * ST7586S_PPB_MONO);

		/* The data is handled by 2 cells so the x order must be taken into account */
		if (x_flipped) {
			byte = get_mono_px(buf, i_d) << 7
				| get_mono_px(buf, i_d) << 6
				| get_mono_px(buf, i_d) << 5
				| get_mono_px(buf, i_d + 1) << 4
				| get_mono_px(buf, i_d + 1) << 3
				| get_mono_px(buf, i_d + 1) << 2
				| get_mono_px(buf, i_d + 2) << 1
				| get_mono_px(buf, i_d + 2);

			config->conversion_buf[i_cb] = mono01 ? byte : ~byte;

			byte = get_mono_px(buf, i_d + 2) << 7
				| get_mono_px(buf, i_d + 2) << 6
				| get_mono_px(buf, i_d + 2) << 5
				| get_mono_px(buf, i_d + 3) << 4
				| get_mono_px(buf, i_d + 3) << 3
				| get_mono_px(buf, i_d + 3) << 2
				| get_mono_px(buf, i_d + 3) << 1
				| get_mono_px(buf, i_d + 3);

			config->conversion_buf[i_cb + 1] = mono01 ? byte : ~byte;
		} else {
			byte = get_mono_px(buf, i_d) << 7
				| get_mono_px(buf, i_d) << 6
				| get_mono_px(buf, i_d) << 5
				| get_mono_px(buf, i_d) << 4
				| get_mono_px(buf, i_d) << 3
				| get_mono_px(buf, i_d) << 2
				| get_mono_px(buf, i_d + 1) << 1
				| get_mono_px(buf, i_d + 1);

			config->conversion_buf[i_cb] = mono01 ? byte : ~byte;

			byte = get_mono_px(buf, i_d + 1) << 7
				| get_mono_px(buf, i_d + 1) << 6
				| get_mono_px(buf, i_d + 1) << 5
				| get_mono_px(buf, i_d + 2) << 4
				| get_mono_px(buf, i_d + 2) << 3
				| get_mono_px(buf, i_d + 2) << 2
				| get_mono_px(buf, i_d + 3) << 1
				| get_mono_px(buf, i_d + 3);

			config->conversion_buf[i_cb + 1] = mono01 ? byte : ~byte;
		}
	}
	return i;
}

static int st7586s_convert_l_8_2_1_1(const struct device *dev, const uint8_t *buf, int cur_offset,
				     uint32_t pixel_count, bool unused)
{
	const struct st7586s_config *config = dev->config;
	int i = 0;
	size_t i_d, i_cb;
	bool x_flipped = (config->flip_configuration & ST7586S_FLIP_X_EN) == 0;

	ARG_UNUSED(unused);

	for (; (i * 3U) / (2U * ST7586S_PPB_GRAY) + 2 < config->conversion_buf_size
	       && pixel_count > cur_offset + i;
	     i += 4U) {
		i_d = cur_offset + i;
		i_cb = (i * 3U) / (2U * ST7586S_PPB_GRAY);
		if (x_flipped) {
			config->conversion_buf[i_cb] = buf[i_d + 1] >> 4
								| (buf[i_d] >> 4) << 4;
			config->conversion_buf[i_cb + 1] = buf[i_d + 2] >> 4
								| (buf[i_d + 2] >> 4) << 4;
			config->conversion_buf[i_cb + 2] = buf[i_d + 3] >> 4
								| (buf[i_d + 3] >> 4) << 4;
		} else {
			config->conversion_buf[i_cb] = buf[i_d] >> 4
								| (buf[i_d] >> 4) << 4;
			config->conversion_buf[i_cb + 1] = buf[i_d + 1] >> 4
								| (buf[i_d + 1] >> 4) << 4;
			config->conversion_buf[i_cb + 2] = buf[i_d + 3] >> 4
								| (buf[i_d + 2] >> 4) << 4;
		}
	}
	return i;
}

#endif

static int st7586s_write(const struct device *dev, const uint16_t x, const uint16_t y,
			 const struct display_buffer_descriptor *desc, const void *buf)
{
	const struct st7586s_config *config = dev->config;
	const uint32_t pixel_count = desc->height * desc->width;
	struct st7586s_data *data = dev->data;
	size_t expected_len;
	struct display_buffer_descriptor mipi_desc;
	st7586s_convert convert_func;
	uint32_t div_buf_size;
	uint32_t mul_buf_size = 1U;
	uint16_t align_x = 0xffU;
	int ret, i;
	int total = 0;

	if (desc->pitch != desc->width) {
		LOG_ERR("Pitch is different from width");
		return -EINVAL;
	}

	if (data->current_pixel_format == PIXEL_FORMAT_MONO01
	    || data->current_pixel_format == PIXEL_FORMAT_MONO10) {
		expected_len = desc->height * desc->width / 8;

#if ANY_PIXMAP_2_1_1()
		if (config->pixel_mapping == PIXMAP_2_1_1) {
			convert_func = st7586s_convert_mono_2_1_1;
			/* Pixels are spread over 2 addresses */
			align_x = 4U;
			mul_buf_size = 3U;
			div_buf_size = 2U * ST7586S_PPB_MONO;
		}
#endif
#if ANY_PIXMAP_NORMAL()
		if (config->pixel_mapping == PIXMAP_NORMAL) {
			convert_func = st7586s_convert_mono_normal;
			align_x = ST7586S_PPC;
			div_buf_size = ST7586S_PPB_MONO;
		}
#endif

		if ((x % align_x) != 0 || (desc->width % align_x) != 0) {
			LOG_ERR("X and width must be aligned on %d boundary", align_x);
			return -EINVAL;
		}

	} else if (data->current_pixel_format == PIXEL_FORMAT_L_8) {
		expected_len = desc->height * desc->width / ST7586S_PPB_GRAY;

#if ANY_PIXMAP_2_1_1()
		if (config->pixel_mapping == PIXMAP_2_1_1) {
			convert_func = st7586s_convert_l_8_2_1_1;
			align_x = 4U;
			mul_buf_size = 3U;
			div_buf_size = 2U * ST7586S_PPB_GRAY;
		}
#endif
#if ANY_PIXMAP_NORMAL()
		if (config->pixel_mapping == PIXMAP_NORMAL) {
			convert_func = st7586s_convert_l_8_normal;
			/* Minimal alignment must both be a multiple of
			 * pixels per bytes and pixels per addresses
			 */
			align_x = ST7586S_PPC * ST7586S_PPB_GRAY;
			div_buf_size = ST7586S_PPB_GRAY;
		}
#endif

		if ((x % align_x) != 0 || (desc->width % align_x) != 0) {
			LOG_ERR("X and width must be aligned on %d boundary", align_x);
			return -EINVAL;
		}

	} else {
		return -EINVAL;
	}

	if (buf == NULL || desc->buf_size < expected_len) {
		LOG_ERR("Display buffer is invalid");
		return -EINVAL;
	}

	LOG_DBG("x %u, y %u, pitch %u, width %u, height %u, len %zu", x, y, desc->pitch,
		desc->width, desc->height, expected_len);

	ret = st7586s_set_window(dev, x, y, desc->width, desc->height);
	if (ret < 0) {
		LOG_ERR("Could not set write window");
		return ret;
	}

	ret = st7586s_start_write(dev);
	if (ret < 0) {
		LOG_ERR("Could not start write");
		return ret;
	}

	mipi_desc.pitch = desc->pitch;

	while (pixel_count > total) {
		i = convert_func(dev, buf, total, pixel_count,
				 data->current_pixel_format == PIXEL_FORMAT_MONO01);
		mipi_desc.buf_size =  (mul_buf_size * i) / div_buf_size;

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
	const struct st7586s_config *config = dev->config;
	int ret;
	uint8_t data[4];

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

#define ST7586S_WORD_SIZE(node_id)                                                                 \
	((DT_STRING_UPPER_TOKEN(node_id, mipi_mode) == MIPI_DBI_MODE_SPI_4WIRE) ? SPI_WORD_SET(8)  \
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
		.pixel_mapping = DT_ENUM_IDX(node_id, pixel_mapping),                              \
		.mipi_dev = DEVICE_DT_GET(DT_PARENT(node_id)),                                     \
		.dbi_config = MIPI_DBI_CONFIG_DT(                                                  \
			node_id, ST7586S_WORD_SIZE(node_id) | SPI_OP_MODE_CONTROLLER, 0),          \
		.conversion_buf = conversion_buf##node_id,                                         \
		.conversion_buf_size = sizeof(conversion_buf##node_id),                            \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_DEFINE(node_id, st7586s_init, NULL, &data##node_id, &config##node_id,            \
			 POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY, &st7586s_driver_api);

DT_FOREACH_STATUS_OKAY(sitronix_st7586s, ST7586S_DEFINE)
