/*
 * Copyright 2025 ST Microelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ovti_ov9655

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/video.h>
#include <zephyr/logging/log.h>

#include "video_common.h"
#include "video_device.h"

LOG_MODULE_REGISTER(video_ov9655, CONFIG_VIDEO_LOG_LEVEL);

struct ov9655_config {
	struct i2c_dt_spec i2c;
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
	struct gpio_dt_spec reset;
#endif
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(pwdn_gpios)
	struct gpio_dt_spec pwdn;
#endif
};

struct ov9655_data {
	struct video_format fmt;
};

#define OV9655_REG8(addr)  ((uint32_t)(addr) | VIDEO_REG_ADDR8_DATA8)
#define OV9655_REG16(addr)  ((uint32_t)(addr) | VIDEO_REG_ADDR8_DATA16_BE)

#define OV9655_PID			OV9655_REG16(0x0aU)
#define OV9655_COMMON_CTRL7		0x12U
#define OV9655_HORIZONTAL_FRAME_START	0x17U
#define OV9655_HORIZONTAL_FRAME_STOP	0x18U
#define OV9655_VERTICAL_FRAME_START	0x19U
#define OV9655_VERTICAL_FRAME_STOP	0x1aU
#define OV9655_HORIZONTAL_FRAME_CTRL	0x32U
#define OV9655_TSLB			0x3aU
#define OV9655_COMMON_CTRL14		0x3eU
#define OV9655_COMMON_CTRL15		0x40U
#define OV9655_POIDX			0x72U
#define OV9655_PCKDV			0x73U

/* Initialization sequence for QQVGA & QVGA resolutions */
static const struct video_reg8 ov9655_init_common[] = {
	{0x03, 0x02},
	{0x04, 0x03},
	{0x0e, 0x61},
	{0x0f, 0x40},
	{0x11, 0x01},
	{OV9655_COMMON_CTRL7, 0x62},
	{0x13, 0xc7},
	{0x14, 0x3a},
	{0x16, 0x24},
	{OV9655_HORIZONTAL_FRAME_START, 0x18},
	{OV9655_HORIZONTAL_FRAME_STOP, 0x04},
	{OV9655_VERTICAL_FRAME_START, 0x01},
	{OV9655_VERTICAL_FRAME_STOP, 0x81},
	{0x24, 0x3c},
	{0x25, 0x36},
	{0x26, 0x72},
	{0x27, 0x08},
	{0x28, 0x08},
	{0x29, 0x15},
	{0x2c, 0x08},
	{0x35, 0x00},
	{0x36, 0x3a},
	{0x39, 0x57},
	{OV9655_TSLB, 0xcc},
	{0x3b, 0x04},
	{0x3f, 0xc1},
	{0x41, 0x41},
	{0x42, 0xc0},
	{0x43, 0x0a},
	{0x44, 0xf0},
	{0x45, 0x46},
	{0x46, 0x62},
	{0x47, 0x2a},
	{0x48, 0x3c},
	{0x4a, 0xfc},
	{0x4b, 0xfc},
	{0x4c, 0x7f},
	{0x4d, 0x7f},
	{0x4e, 0x7f},
	{0x4f, 0x98},
	{0x50, 0x98},
	{0x51, 0x00},
	{0x52, 0x28},
	{0x53, 0x70},
	{0x54, 0x98},
	{0x58, 0x1a},
	{0x59, 0x85},
	{0x5a, 0xa9},
	{0x5b, 0x64},
	{0x5c, 0x84},
	{0x5d, 0x53},
	{0x5e, 0x0e},
	{0x69, 0x0a},
	{0x6b, 0x5a},
	{0x6c, 0x04},
	{0x6d, 0x55},
	{0x6e, 0x00},
	{0x6f, 0x9d},
	{0x70, 0x21},
	{0x71, 0x78},
	{0x74, 0x10},
	{0x75, 0x10},
	{0x76, 0x01},
	{0x77, 0x02},
	{0x7A, 0x12},
	{0x7B, 0x08},
	{0x7C, 0x16},
	{0x7D, 0x30},
	{0x7E, 0x5e},
	{0x7F, 0x72},
	{0x80, 0x82},
	{0x81, 0x8e},
	{0x82, 0x9a},
	{0x83, 0xa4},
	{0x84, 0xac},
	{0x85, 0xb8},
	{0x86, 0xc3},
	{0x87, 0xd6},
	{0x88, 0xe6},
	{0x89, 0xf2},
	{0x8a, 0x24},
	{0x8c, 0x80},
	{0x90, 0x7d},
	{0x91, 0x7b},
	{0x9d, 0x02},
	{0x9e, 0x02},
	{0x9f, 0x7a},
	{0xa0, 0x79},
	{0xa4, 0x50},
	{0xa5, 0x68},
	{0xa6, 0x4a},
	{0xa8, 0xc1},
	{0xa9, 0xef},
	{0xaa, 0x92},
	{0xab, 0x04},
	{0xac, 0x80},
	{0xad, 0x80},
	{0xae, 0x80},
	{0xaf, 0x80},
	{0xb2, 0xf2},
	{0xb3, 0x20},
	{0xb4, 0x20},
	{0xb5, 0x00},
	{0xb6, 0xaf},
	{0xbb, 0xae},
	{0xbc, 0x7f},
	{0xbd, 0x7f},
	{0xbe, 0x7f},
	{0xbf, 0x7f},
	{0xc0, 0xaa},
	{0xc1, 0xc0},
	{0xc2, 0x01},
	{0xc3, 0x4e},
	{0xc6, 0x05},
	{0xc9, 0xe0},
	{0xca, 0xe8},
	{0xcb, 0xf0},
	{0xcc, 0xd8},
	{0xcd, 0x93},
};

#define OV9655_VIDEO_FORMAT_CAP(width, height, format)	\
	{						\
		.pixelformat = (format),		\
		.width_min = (width),			\
		.width_max = (width),			\
		.height_min = (height),			\
		.height_max = (height),			\
		.width_step = 0,			\
		.height_step = 0  \
	}

static const struct video_format_cap fmts[] = {
	OV9655_VIDEO_FORMAT_CAP(160, 120, VIDEO_PIX_FMT_RGB565),	/* QQVGA */
	OV9655_VIDEO_FORMAT_CAP(320, 240, VIDEO_PIX_FMT_RGB565),	/* QVGA */
	OV9655_VIDEO_FORMAT_CAP(160, 120, VIDEO_PIX_FMT_YUYV),		/* QQVGA */
	OV9655_VIDEO_FORMAT_CAP(320, 240, VIDEO_PIX_FMT_YUYV),		/* QVGA */
	{ 0 },
};

static int ov9655_get_caps(const struct device *dev, struct video_caps *caps)
{
	caps->format_caps = fmts;

	return 0;
}

static const struct video_reg8 ov9655_init_rgb565[] = {
	{OV9655_COMMON_CTRL7, 0x63},
	{OV9655_COMMON_CTRL15, 0x10},
};

static const struct video_reg8 ov9655_init_yuyv[] = {
	{OV9655_COMMON_CTRL7, 0x62},
	{OV9655_TSLB, 0xc0},
	{OV9655_COMMON_CTRL15, 0xc0},
};

static int ov9655_set_output_format(const struct device *dev, uint32_t pixelformat)
{
	const struct ov9655_config *config = dev->config;
	int ret;

	if (pixelformat == VIDEO_PIX_FMT_RGB565) {
		ret = video_write_cci_multiregs8(&config->i2c, ov9655_init_rgb565,
						 ARRAY_SIZE(ov9655_init_rgb565));
	} else if (pixelformat == VIDEO_PIX_FMT_YUYV) {
		ret = video_write_cci_multiregs8(&config->i2c, ov9655_init_yuyv,
						 ARRAY_SIZE(ov9655_init_yuyv));
	} else {
		return -ENOTSUP;
	}

	return ret;
}

/* Initialization sequence for QVGA resolution (320x240) */
static const struct video_reg8 ov9655_init_res_320x240[] = {
	{OV9655_HORIZONTAL_FRAME_CTRL, 0x12},
	{OV9655_COMMON_CTRL14, 0x02},
	{OV9655_POIDX, 0x11},
	{OV9655_PCKDV, 0x01},
	{0xc7, 0x81},
};

/* Initialization sequence for QQVGA resolution (160x120) */
static const struct video_reg8 ov9655_init_res_160x120[] = {
	{OV9655_HORIZONTAL_FRAME_CTRL, 0xa4},
	{OV9655_COMMON_CTRL14, 0x0e},
	{OV9655_POIDX, 0x22},
	{OV9655_PCKDV, 0x02},
	{0xc7, 0x82},
};

static int ov9655_set_output_resolution(const struct device *dev, uint32_t width, uint32_t height)
{
	const struct ov9655_config *config = dev->config;
	int ret;

	if (width == 160 && height == 120) {
		ret = video_write_cci_multiregs8(&config->i2c, ov9655_init_res_160x120,
						 ARRAY_SIZE(ov9655_init_res_160x120));
	} else if (width == 320 && height == 240) {
		ret = video_write_cci_multiregs8(&config->i2c, ov9655_init_res_320x240,
						 ARRAY_SIZE(ov9655_init_res_320x240));
	} else {
		return -ENOTSUP;
	}

	return ret;
}

static int ov9655_set_fmt(const struct device *dev, struct video_format *fmt)
{
	const struct ov9655_config *config = dev->config;
	struct ov9655_data *data = dev->data;
	int ret;

	if (fmt->pixelformat != VIDEO_PIX_FMT_RGB565 && fmt->pixelformat != VIDEO_PIX_FMT_YUYV) {
		LOG_ERR("Only RGB565 and YUYV supported!");
		return -ENOTSUP;
	}

	if (!memcmp(&data->fmt, fmt, sizeof(data->fmt))) {
		/* nothing to do */
		return 0;
	}

	memcpy(&data->fmt, fmt, sizeof(data->fmt));

	/* Reset the sensor registers */
	ret = video_write_cci_reg(&config->i2c, OV9655_REG8(OV9655_COMMON_CTRL7), 0x80);
	if (ret < 0) {
		LOG_ERR("Failed to reset the sensor");
		return ret;
	}
	k_msleep(200);

	/* Write initialization values to OV9655 */
	ret = video_write_cci_multiregs8(&config->i2c, ov9655_init_common,
					 ARRAY_SIZE(ov9655_init_common));
	if (ret < 0) {
		return ret;
	}

	/* Set output resolution */
	ret = ov9655_set_output_resolution(dev, fmt->width, fmt->height);
	if (ret < 0) {
		return ret;
	}

	/* Set pixel format */
	ret = ov9655_set_output_format(dev, fmt->pixelformat);
	if (ret < 0) {
		return ret;
	}

	/* COM10 - invert HRef signal */
	ret = video_write_cci_reg(&config->i2c, OV9655_REG8(0x15), 0x08);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int ov9655_get_fmt(const struct device *dev, struct video_format *fmt)
{
	struct ov9655_data *data = dev->data;

	memcpy(fmt, &data->fmt, sizeof(data->fmt));

	return 0;
}

static int ov9655_init(const struct device *dev)
{
	const struct ov9655_config *config = dev->config;
	struct video_format fmt;
	uint32_t pid;
	int ret;

	if (!i2c_is_ready_dt(&config->i2c)) {
		/* I2C device is not ready, return */
		return -ENODEV;
	}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(pwdn_gpios)
	/* Power up camera module */
	if (config->pwdn.port) {
		if (!gpio_is_ready_dt(&config->pwdn)) {
			return -ENODEV;
		}
		ret = gpio_pin_configure_dt(&config->pwdn, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_ERR("Could not clear power down pin: %d", ret);
			return ret;
		}
		k_msleep(3);
	}
#endif
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
	/* Reset camera module */
	if (config->reset.port) {
		if (!gpio_is_ready_dt(&config->reset)) {
			return -ENODEV;
		}
		ret = gpio_pin_configure_dt(&config->reset, GPIO_OUTPUT);
		if (ret < 0) {
			LOG_ERR("Could not set reset pin: %d", ret);
			return ret;
		}
		/* Reset is active low, has 1ms settling time */
		gpio_pin_set_dt(&config->reset, 0);
		k_msleep(1);
		gpio_pin_set_dt(&config->reset, 1);
		k_msleep(1);
	}
#endif

	/* Read Product ID & Version ID */
	ret = video_read_cci_reg(&config->i2c, OV9655_PID, &pid);
	if (ret < 0) {
		LOG_ERR("Could not request product ID: %d", ret);
		return ret;
	}

	if (pid != 0x9657) {
		LOG_ERR("Incorrect product ID: 0x%04X", pid);
		return -ENODEV;
	}

	/* Set default camera format (QQVGA, YUYV) */
	fmt.pixelformat = VIDEO_PIX_FMT_YUYV;
	fmt.width = 160;
	fmt.height = 120;

	return ov9655_set_fmt(dev, &fmt);
}

static int ov9655_set_stream(const struct device *dev, bool enable, enum video_buf_type type)
{
	return 0;
}

static int ov9655_get_frmival(const struct device *dev, struct video_frmival *frmival)
{
	frmival->numerator = 1;
	frmival->denominator = 30;

	return 0;
}

static int ov9655_enum_frmival(const struct device *dev, struct video_frmival_enum *fie)
{
	if (fie->index > 0) {
		return -EINVAL;
	}

	fie->type = VIDEO_FRMIVAL_TYPE_DISCRETE;
	fie->discrete.numerator = 1;
	fie->discrete.denominator = 30;

	return 0;
}

static const struct video_driver_api ov9655_api = {
	.set_format = ov9655_set_fmt,
	.get_format = ov9655_get_fmt,
	.get_caps = ov9655_get_caps,
	.set_stream = ov9655_set_stream,
	.set_frmival = ov9655_get_frmival,
	.get_frmival = ov9655_get_frmival,
	.enum_frmival = ov9655_enum_frmival,
};

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
#define OV9655_RESET_GPIO(inst) .reset = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {}),
#else
#define OV9655_RESET_GPIO(inst)
#endif

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(pwdn_gpios)
#define OV9655_PWDN_GPIO(inst) .pwdn = GPIO_DT_SPEC_INST_GET_OR(inst, pwdn_gpios, {}),
#else
#define OV9655_PWDN_GPIO(inst)
#endif

#define OV9655_INIT(inst)                                                                          \
	const struct ov9655_config ov9655_config_##inst = {.i2c = I2C_DT_SPEC_INST_GET(inst),      \
							   OV9655_RESET_GPIO(inst)                 \
							   OV9655_PWDN_GPIO(inst)};		   \
	struct ov9655_data ov9655_data_##inst;                                                     \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, ov9655_init, NULL, &ov9655_data_##inst, &ov9655_config_##inst, \
			      POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY, &ov9655_api);		   \
												   \
	VIDEO_DEVICE_DEFINE(ov9655_##n, DEVICE_DT_INST_GET(inst), NULL);

DT_INST_FOREACH_STATUS_OKAY(OV9655_INIT)
