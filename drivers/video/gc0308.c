/*
 * Copyright (c) 2025 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT galaxycore_gc0308

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zephyr/drivers/video.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>

#include "video_common.h"
#include "video_device.h"

LOG_MODULE_REGISTER(video_gc0308, CONFIG_VIDEO_LOG_LEVEL);

#define GC0308_REG8(addr) ((uint32_t)(addr) | VIDEO_REG_ADDR8_DATA8)

#define GC0308_REG_PAGE_SELECT 0xfe
#define GC0308_PAGE_0          0x00
#define GC0308_PAGE_1          0x01
#define GC0308_SW_RESET        0xf0

#define GC0308_REG_OUTPUT_FMT    0x24
#define GC0308_OUTPUT_FMT_MASK   0x0f
#define GC0308_OUTPUT_FMT_YUYV   0x02
#define GC0308_OUTPUT_FMT_RGB565 0x06

#define GC0308_SENSOR_WIDTH  640
#define GC0308_SENSOR_HEIGHT 480
#define GC0308_QVGA_WIDTH    320
#define GC0308_QVGA_HEIGHT   240

#define GC0308_REG_ROW_START_H  0x05
#define GC0308_REG_ROW_START_L  0x06
#define GC0308_REG_COL_START_H  0x07
#define GC0308_REG_COL_START_L  0x08
#define GC0308_REG_WIN_HEIGHT_H 0x09
#define GC0308_REG_WIN_HEIGHT_L 0x0a
#define GC0308_REG_WIN_WIDTH_H  0x0b
#define GC0308_REG_WIN_WIDTH_L  0x0c

#define GC0308_REG_SUBSAMPLE  0x54
#define GC0308_REG_SUBMODE    0x55
#define GC0308_REG_SUB_ROW_N1 0x56
#define GC0308_REG_SUB_ROW_N2 0x57
#define GC0308_REG_SUB_COL_N1 0x58
#define GC0308_REG_SUB_COL_N2 0x59

#define GC0308_REG_CROP_MODE     0x46
#define GC0308_REG_CROP_Y0       0x47
#define GC0308_REG_CROP_X0       0x48
#define GC0308_REG_CROP_HEIGHT_H 0x49
#define GC0308_REG_CROP_HEIGHT_L 0x4a
#define GC0308_REG_CROP_WIDTH_H  0x4b
#define GC0308_REG_CROP_WIDTH_L  0x4c
#define GC0308_CROP_ENABLE       0x80

struct gc0308_data {
	struct video_format fmt;
};

struct gc0308_config {
	struct i2c_dt_spec i2c;
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
	struct gpio_dt_spec reset_gpio;
#endif
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(pwdn_gpios)
	struct gpio_dt_spec pwdn_gpio;
#endif
};

#define GC0308_VIDEO_FORMAT_CAP(width, height, format)                                             \
	{                                                                                          \
		.pixelformat = (format),                                                           \
		.width_min = (width),                                                              \
		.width_max = (width),                                                              \
		.height_min = (height),                                                            \
		.height_max = (height),                                                            \
		.width_step = 0,                                                                   \
		.height_step = 0,                                                                  \
	}

static const struct video_format_cap gc0308_fmts[] = {
	GC0308_VIDEO_FORMAT_CAP(GC0308_QVGA_WIDTH, GC0308_QVGA_HEIGHT, VIDEO_PIX_FMT_YUYV),
	GC0308_VIDEO_FORMAT_CAP(GC0308_SENSOR_WIDTH, GC0308_SENSOR_HEIGHT, VIDEO_PIX_FMT_YUYV),
	GC0308_VIDEO_FORMAT_CAP(GC0308_QVGA_WIDTH, GC0308_QVGA_HEIGHT, VIDEO_PIX_FMT_RGB565),
	GC0308_VIDEO_FORMAT_CAP(GC0308_SENSOR_WIDTH, GC0308_SENSOR_HEIGHT, VIDEO_PIX_FMT_RGB565),
	{0},
};

/* Based on the initialization sequence from esp32-camera (Apache-2.0). */
static const struct video_reg8 gc0308_default_regs[] = {
	{0xfe, 0x00}, {0xec, 0x20}, {0x05, 0x00}, {0x06, 0x00}, {0x07, 0x00}, {0x08, 0x00},
	{0x09, 0x01}, {0x0a, 0xe8}, {0x0b, 0x02}, {0x0c, 0x88}, {0x0d, 0x02}, {0x0e, 0x02},
	{0x10, 0x26}, {0x11, 0x0d}, {0x12, 0x2a}, {0x13, 0x00}, {0x14, 0x10}, {0x15, 0x0a},
	{0x16, 0x05}, {0x17, 0x01}, {0x18, 0x44}, {0x19, 0x44}, {0x1a, 0x2a}, {0x1b, 0x00},
	{0x1c, 0x49}, {0x1d, 0x9a}, {0x1e, 0x61}, {0x1f, 0x00}, {0x20, 0x7f}, {0x21, 0xfa},
	{0x22, 0x57}, {0x24, 0xa2}, {0x25, 0x0f}, {0x26, 0x03}, {0x28, 0x00}, {0x2d, 0x0a},
	{0x2f, 0x01}, {0x30, 0xf7}, {0x31, 0x50}, {0x32, 0x00}, {0x33, 0x28}, {0x34, 0x2a},
	{0x35, 0x28}, {0x39, 0x04}, {0x3a, 0x20}, {0x3b, 0x20}, {0x3c, 0x00}, {0x3d, 0x00},
	{0x3e, 0x00}, {0x3f, 0x00}, {0x50, 0x14}, {0x52, 0x41}, {0x53, 0x80}, {0x54, 0x80},
	{0x55, 0x80}, {0x56, 0x80}, {0x5a, 0x56}, {0x5b, 0x40}, {0x5c, 0x4a}, {0x8b, 0x20},
	{0x8c, 0x20}, {0x8d, 0x20}, {0x8e, 0x14}, {0x8f, 0x10}, {0x90, 0x14}, {0x91, 0x3c},
	{0x92, 0x50}, {0x5d, 0x12}, {0x5e, 0x1a}, {0x5f, 0x24}, {0x60, 0x07}, {0x61, 0x15},
	{0x62, 0x08}, {0x64, 0x03}, {0x66, 0xe8}, {0x67, 0x86}, {0x68, 0x82}, {0x69, 0x18},
	{0x6a, 0x0f}, {0x6b, 0x00}, {0x6c, 0x5f}, {0x6d, 0x8f}, {0x6e, 0x55}, {0x6f, 0x38},
	{0x70, 0x15}, {0x71, 0x33}, {0x72, 0xdc}, {0x73, 0x00}, {0x74, 0x02}, {0x75, 0x3f},
	{0x76, 0x02}, {0x77, 0x38}, {0x78, 0x88}, {0x79, 0x81}, {0x7a, 0x81}, {0x7b, 0x22},
	{0x7c, 0xff}, {0x93, 0x48}, {0x94, 0x02}, {0x95, 0x07}, {0x96, 0xe0}, {0x97, 0x40},
	{0x98, 0xf0}, {0xb1, 0x40}, {0xb2, 0x40}, {0xb3, 0x40}, {0xb6, 0xe0}, {0xbd, 0x38},
	{0xbe, 0x36}, {0xd0, 0xcb}, {0xd1, 0x10}, {0xd2, 0x90}, {0xd3, 0x48}, {0xd5, 0xf2},
	{0xd6, 0x16}, {0xdb, 0x92}, {0xdc, 0xa5}, {0xdf, 0x23}, {0xd9, 0x00}, {0xda, 0x00},
	{0xe0, 0x09}, {0xed, 0x04}, {0xee, 0xa0}, {0xef, 0x40}, {0x80, 0x03}, {0x9f, 0x10},
	{0xa0, 0x20}, {0xa1, 0x38}, {0xa2, 0x4e}, {0xa3, 0x63}, {0xa4, 0x76}, {0xa5, 0x87},
	{0xa6, 0xa2}, {0xa7, 0xb8}, {0xa8, 0xca}, {0xa9, 0xd8}, {0xaa, 0xe3}, {0xab, 0xeb},
	{0xac, 0xf0}, {0xad, 0xf8}, {0xae, 0xfd}, {0xaf, 0xff}, {0xc0, 0x00}, {0xc1, 0x10},
	{0xc2, 0x1c}, {0xc3, 0x30}, {0xc4, 0x43}, {0xc5, 0x54}, {0xc6, 0x65}, {0xc7, 0x75},
	{0xc8, 0x93}, {0xc9, 0xb0}, {0xca, 0xcb}, {0xcb, 0xe6}, {0xcc, 0xff}, {0xf0, 0x02},
	{0xf1, 0x01}, {0xf2, 0x02}, {0xf3, 0x30}, {0xf7, 0x04}, {0xf8, 0x02}, {0xf9, 0x9f},
	{0xfa, 0x78}, {0xfe, 0x01}, {0x00, 0xf5}, {0x02, 0x20}, {0x04, 0x10}, {0x05, 0x08},
	{0x06, 0x20}, {0x08, 0x0a}, {0x0a, 0xa0}, {0x0b, 0x60}, {0x0c, 0x08}, {0x0e, 0x44},
	{0x0f, 0x32}, {0x10, 0x41}, {0x11, 0x37}, {0x12, 0x22}, {0x13, 0x19}, {0x14, 0x44},
	{0x15, 0x44}, {0x16, 0xc2}, {0x17, 0xa8}, {0x18, 0x18}, {0x19, 0x50}, {0x1a, 0xd8},
	{0x1b, 0xf5}, {0x70, 0x40}, {0x71, 0x58}, {0x72, 0x30}, {0x73, 0x48}, {0x74, 0x20},
	{0x75, 0x60}, {0x77, 0x20}, {0x78, 0x32}, {0x30, 0x03}, {0x31, 0x40}, {0x32, 0x10},
	{0x33, 0xe0}, {0x34, 0xe0}, {0x35, 0x00}, {0x36, 0x80}, {0x37, 0x00}, {0x38, 0x04},
	{0x39, 0x09}, {0x3a, 0x12}, {0x3b, 0x1c}, {0x3c, 0x28}, {0x3d, 0x31}, {0x3e, 0x44},
	{0x3f, 0x57}, {0x40, 0x6c}, {0x41, 0x81}, {0x42, 0x94}, {0x43, 0xa7}, {0x44, 0xb8},
	{0x45, 0xd6}, {0x46, 0xee}, {0x47, 0x0d}, {0x62, 0xf7}, {0x63, 0x68}, {0x64, 0xd3},
	{0x65, 0xd3}, {0x66, 0x60}, {0xfe, 0x00}, {0x01, 0x32}, {0x02, 0x0c}, {0x0f, 0x01},
	{0xe2, 0x00}, {0xe3, 0x78}, {0xe4, 0x00}, {0xe5, 0xfe}, {0xe6, 0x01}, {0xe7, 0xe0},
	{0xe8, 0x01}, {0xe9, 0xe0}, {0xea, 0x01}, {0xeb, 0xe0}, {0xfe, 0x00},
};

static int gc0308_write_reg(const struct device *dev, uint8_t reg, uint8_t value)
{
	const struct gc0308_config *cfg = dev->config;

	return video_write_cci_reg(&cfg->i2c, GC0308_REG8(reg), value);
}

static int gc0308_set_window(const struct device *dev, uint16_t width, uint16_t height)
{
	uint8_t subsample;
	const uint16_t win_height = GC0308_SENSOR_HEIGHT + 8;
	const uint16_t win_width = GC0308_SENSOR_WIDTH + 8;
	int ret;

	if (width == 0U || height == 0U || width > GC0308_SENSOR_WIDTH ||
	    height > GC0308_SENSOR_HEIGHT) {
		return -EINVAL;
	}

	if (width == GC0308_SENSOR_WIDTH && height == GC0308_SENSOR_HEIGHT) {
		subsample = 0x11;
	} else if (width == GC0308_QVGA_WIDTH && height == GC0308_QVGA_HEIGHT) {
		subsample = 0x22;
	} else {
		return -ENOTSUP;
	}

	ret = gc0308_write_reg(dev, GC0308_REG_PAGE_SELECT, GC0308_PAGE_1);
	if (ret < 0) {
		return ret;
	}
	ret = gc0308_write_reg(dev, GC0308_REG_SUBSAMPLE, subsample);
	if (ret < 0) {
		return ret;
	}
	ret = gc0308_write_reg(dev, GC0308_REG_SUBMODE, 0x03);
	if (ret < 0) {
		return ret;
	}
	ret = gc0308_write_reg(dev, GC0308_REG_SUB_ROW_N1, 0x00);
	if (ret < 0) {
		return ret;
	}
	ret = gc0308_write_reg(dev, GC0308_REG_SUB_ROW_N2, 0x00);
	if (ret < 0) {
		return ret;
	}
	ret = gc0308_write_reg(dev, GC0308_REG_SUB_COL_N1, 0x00);
	if (ret < 0) {
		return ret;
	}
	ret = gc0308_write_reg(dev, GC0308_REG_SUB_COL_N2, 0x00);
	if (ret < 0) {
		return ret;
	}

	ret = gc0308_write_reg(dev, GC0308_REG_PAGE_SELECT, GC0308_PAGE_0);
	if (ret < 0) {
		return ret;
	}

	ret = gc0308_write_reg(dev, GC0308_REG_ROW_START_H, 0x00);
	if (ret < 0) {
		return ret;
	}
	ret = gc0308_write_reg(dev, GC0308_REG_ROW_START_L, 0x00);
	if (ret < 0) {
		return ret;
	}
	ret = gc0308_write_reg(dev, GC0308_REG_COL_START_H, 0x00);
	if (ret < 0) {
		return ret;
	}
	ret = gc0308_write_reg(dev, GC0308_REG_COL_START_L, 0x00);
	if (ret < 0) {
		return ret;
	}

	ret = gc0308_write_reg(dev, GC0308_REG_WIN_HEIGHT_H, (uint8_t)(win_height >> 8));
	if (ret < 0) {
		return ret;
	}
	ret = gc0308_write_reg(dev, GC0308_REG_WIN_HEIGHT_L, (uint8_t)win_height);
	if (ret < 0) {
		return ret;
	}
	ret = gc0308_write_reg(dev, GC0308_REG_WIN_WIDTH_H, (uint8_t)(win_width >> 8));
	if (ret < 0) {
		return ret;
	}
	ret = gc0308_write_reg(dev, GC0308_REG_WIN_WIDTH_L, (uint8_t)win_width);
	if (ret < 0) {
		return ret;
	}

	ret = gc0308_write_reg(dev, GC0308_REG_CROP_MODE, GC0308_CROP_ENABLE);
	if (ret < 0) {
		return ret;
	}
	ret = gc0308_write_reg(dev, GC0308_REG_CROP_Y0, 0x00);
	if (ret < 0) {
		return ret;
	}
	ret = gc0308_write_reg(dev, GC0308_REG_CROP_X0, 0x00);
	if (ret < 0) {
		return ret;
	}
	ret = gc0308_write_reg(dev, GC0308_REG_CROP_HEIGHT_H, (uint8_t)((height >> 8) & 0x01));
	if (ret < 0) {
		return ret;
	}
	ret = gc0308_write_reg(dev, GC0308_REG_CROP_HEIGHT_L, (uint8_t)height);
	if (ret < 0) {
		return ret;
	}
	ret = gc0308_write_reg(dev, GC0308_REG_CROP_WIDTH_H, (uint8_t)((width >> 8) & 0x03));
	if (ret < 0) {
		return ret;
	}
	return gc0308_write_reg(dev, GC0308_REG_CROP_WIDTH_L, (uint8_t)width);
}

static int gc0308_set_output_format(const struct device *dev, uint32_t pixelformat)
{
	const struct gc0308_config *cfg = dev->config;
	uint8_t fmt;
	int ret;

	switch (pixelformat) {
	case VIDEO_PIX_FMT_YUYV:
		fmt = GC0308_OUTPUT_FMT_YUYV;
		break;
	case VIDEO_PIX_FMT_RGB565:
		fmt = GC0308_OUTPUT_FMT_RGB565;
		break;
	default:
		return -ENOTSUP;
	}

	ret = video_write_cci_reg(&cfg->i2c, GC0308_REG8(GC0308_REG_PAGE_SELECT), GC0308_PAGE_0);
	if (ret < 0) {
		return ret;
	}

	return video_modify_cci_reg(&cfg->i2c, GC0308_REG8(GC0308_REG_OUTPUT_FMT),
				    GC0308_OUTPUT_FMT_MASK, fmt);
}

static int gc0308_get_caps(const struct device *dev, struct video_caps *caps)
{
	ARG_UNUSED(dev);

	caps->format_caps = gc0308_fmts;
	return 0;
}

static int gc0308_set_fmt(const struct device *dev, struct video_format *fmt)
{
	struct gc0308_data *data = dev->data;
	int ret;
	size_t idx;

	ret = video_format_caps_index(gc0308_fmts, fmt, &idx);
	if (ret < 0) {
		LOG_ERR("Unsupported format");
		return ret;
	}

	ret = video_estimate_fmt_size(fmt);
	if (ret < 0) {
		return ret;
	}

	fmt->type = VIDEO_BUF_TYPE_OUTPUT;

	if (data->fmt.pixelformat == fmt->pixelformat && data->fmt.width == fmt->width &&
	    data->fmt.height == fmt->height) {
		data->fmt = *fmt;
		return 0;
	}

	ret = gc0308_set_window(dev, fmt->width, fmt->height);
	if (ret < 0) {
		LOG_ERR("Failed to set window");
		return ret;
	}

	ret = gc0308_set_output_format(dev, fmt->pixelformat);
	if (ret < 0) {
		LOG_ERR("Failed to set output format");
		return ret;
	}

	data->fmt = *fmt;
	return 0;
}

static int gc0308_get_fmt(const struct device *dev, struct video_format *fmt)
{
	struct gc0308_data *data = dev->data;

	*fmt = data->fmt;
	return 0;
}

static int gc0308_set_stream(const struct device *dev, bool enable, enum video_buf_type type)
{
	const struct gc0308_config *cfg = dev->config;

	ARG_UNUSED(type);

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(pwdn_gpios)
	if (cfg->pwdn_gpio.port != NULL) {
		gpio_pin_set_dt(&cfg->pwdn_gpio, enable ? 0 : 1);
	}
#endif

	return 0;
}

static DEVICE_API(video, gc0308_driver_api) = {
	.set_format = gc0308_set_fmt,
	.get_format = gc0308_get_fmt,
	.get_caps = gc0308_get_caps,
	.set_stream = gc0308_set_stream,
};

static int gc0308_init(const struct device *dev)
{
	const struct gc0308_config *cfg = dev->config;
	struct gc0308_data *data = dev->data;
	struct video_format fmt = {
		.type = VIDEO_BUF_TYPE_OUTPUT,
		.pixelformat = VIDEO_PIX_FMT_YUYV,
		.width = GC0308_QVGA_WIDTH,
		.height = GC0308_QVGA_HEIGHT,
	};
	int ret;

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(pwdn_gpios)
	if (cfg->pwdn_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->pwdn_gpio)) {
			LOG_ERR("PWDN GPIO not ready");
			return -ENODEV;
		}
		ret = gpio_pin_configure_dt(&cfg->pwdn_gpio, GPIO_OUTPUT_INACTIVE);
		if (ret) {
			return ret;
		}
		k_msleep(10);
	}
#endif

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
	if (cfg->reset_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->reset_gpio)) {
			LOG_ERR("RESET GPIO not ready");
			return -ENODEV;
		}
		ret = gpio_pin_configure_dt(&cfg->reset_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret) {
			return ret;
		}
		k_msleep(1);
		gpio_pin_set_dt(&cfg->reset_gpio, 0);
		k_msleep(1);
	}
#endif

	ret = video_write_cci_reg(&cfg->i2c, GC0308_REG8(GC0308_REG_PAGE_SELECT), GC0308_SW_RESET);
	if (ret < 0) {
		return ret;
	}
	k_msleep(80);

	ret = video_write_cci_multiregs8(&cfg->i2c, gc0308_default_regs,
					 ARRAY_SIZE(gc0308_default_regs));
	if (ret < 0) {
		return ret;
	}
	k_msleep(80);

	ret = gc0308_set_fmt(dev, &fmt);
	if (ret < 0) {
		return ret;
	}

	data->fmt = fmt;
	return 0;
}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
#define GC0308_RESET_GPIO(inst) .reset_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {}),
#else
#define GC0308_RESET_GPIO(inst)
#endif

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(pwdn_gpios)
#define GC0308_PWDN_GPIO(inst) .pwdn_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, pwdn_gpios, {}),
#else
#define GC0308_PWDN_GPIO(inst)
#endif

#define GC0308_INIT(inst)                                                                          \
	const struct gc0308_config gc0308_config_##inst = {.i2c = I2C_DT_SPEC_INST_GET(inst),      \
							   GC0308_RESET_GPIO(inst)                 \
								   GC0308_PWDN_GPIO(inst)};        \
	struct gc0308_data gc0308_data_##inst;                                                     \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, gc0308_init, NULL, &gc0308_data_##inst, &gc0308_config_##inst, \
			      POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY, &gc0308_driver_api);        \
                                                                                                   \
	VIDEO_DEVICE_DEFINE(gc0308_##inst, DEVICE_DT_INST_GET(inst), NULL);

DT_INST_FOREACH_STATUS_OKAY(GC0308_INIT)
