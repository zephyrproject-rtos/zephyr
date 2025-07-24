/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT himax_hm01b0

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/video-controls.h>
#include <zephyr/drivers/video.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <../drivers/video/video_ctrls.h>
#include <../drivers/video/video_device.h>

LOG_MODULE_REGISTER(hm01b0, /*CONFIG_VIDEO_LOG_LEVEL*/ LOG_LEVEL_DBG);
#define MAX_FRAME_RATE 10
#define MIN_FRAME_RATE 1
#define HM01B0_ID      0x01B0

enum hm01b0_resolution {
	RESOLUTION_160x120 = 0,
	RESOLUTION_320x240,
	RESOLUTION_320x320
};

enum hm01b0_reg {
	REG_ID = 0x0000,
	REG_STS = 0x0100,
	REG_RESET = 0x0103,
	REG_GRP_PARAM_HOLD = 0x0104,
	REG_INTEGRATION_H = 0x0202,
	REG_FRAME_LENGTH_LINES = 0x0340,
	REG_LINE_LENGTH_PCLK = 0x0342,
	REG_WIDTH = 0x0383,
	REG_HEIGHT = 0x0387,
	REG_BINNING_MODE = 0x0390,
	REG_QVGA_WIN_EN = 0x3010,
	REG_BIT_CONTROL = 0x3059,
	REG_OSC_CLOCK_DIV = 0x3060
};

struct hm01b0_reg_value {
	enum hm01b0_reg address;
	size_t value_size; // 1 or 2 bytes
	union {
		uint16_t word;
		uint8_t byte;
	} value;
};

const struct hm01b0_reg_value hm01b0_reg_values[RESOLUTION_320x320 + 1][6] = {
	[RESOLUTION_160x120] = {{REG_WIDTH, sizeof(uint8_t), .value.byte = 0x3},
				{REG_HEIGHT, sizeof(uint8_t), .value.byte = 0x3},
				{REG_BINNING_MODE, sizeof(uint8_t), .value.byte = 0x3},
				{REG_QVGA_WIN_EN, sizeof(uint8_t), .value.byte = 0x1},
				{REG_FRAME_LENGTH_LINES, sizeof(uint16_t), .value.word = 0x80},
				{REG_LINE_LENGTH_PCLK, sizeof(uint16_t), .value.word = 0xD7}},
	[RESOLUTION_320x240] = {{REG_WIDTH, sizeof(uint8_t), .value.byte = 0x1},
				{REG_HEIGHT, sizeof(uint8_t), .value.byte = 0x1},
				{REG_BINNING_MODE, sizeof(uint8_t), .value.byte = 0x0},
				{REG_QVGA_WIN_EN, sizeof(uint8_t), .value.byte = 0x1},
				{REG_FRAME_LENGTH_LINES, sizeof(uint16_t), .value.word = 0x104},
				{REG_LINE_LENGTH_PCLK, sizeof(uint16_t), .value.word = 0x178}

	},
	[RESOLUTION_320x320] = {{REG_WIDTH, sizeof(uint8_t), .value.byte = 0x1},
				{REG_HEIGHT, sizeof(uint8_t), .value.byte = 0x1},
				{REG_BINNING_MODE, sizeof(uint8_t), .value.byte = 0x0},
				{REG_QVGA_WIN_EN, sizeof(uint8_t), .value.byte = 0x0},
				{REG_FRAME_LENGTH_LINES, sizeof(uint16_t), .value.word = 0x158},
				{REG_LINE_LENGTH_PCLK, sizeof(uint16_t), .value.word = 0x178}}};

struct hm01b0_ctrls {
	struct video_ctrl integration;
};

struct hm01b0_data {
	struct hm01b0_ctrls ctrls;
	struct video_format fmt;
	enum hm01b0_resolution resolution;
	uint8_t ctrl_val;
};

struct hm01b0_config {
	struct i2c_dt_spec i2c_dev;
	const uint8_t data_bits;
};

#define HM01B0_VIDEO_FORMAT_CAP(width, height, format)                                             \
	{.pixelformat = (format),                                                                  \
	 .width_min = (width),                                                                     \
	 .width_max = (width),                                                                     \
	 .height_min = (height),                                                                   \
	 .height_max = (height),                                                                   \
	 .width_step = 0,                                                                          \
	 .height_step = 0}

static const struct video_format_cap hm01b0_fmts[] = {
	HM01B0_VIDEO_FORMAT_CAP(160, 120, VIDEO_PIX_FMT_GREY),
	HM01B0_VIDEO_FORMAT_CAP(320, 240, VIDEO_PIX_FMT_GREY),
	HM01B0_VIDEO_FORMAT_CAP(320, 320, VIDEO_PIX_FMT_GREY),
	{0}};

static int hm01b0_write_reg8(const struct device *dev, enum hm01b0_reg reg, uint8_t val)
{
	struct hm01b0_config *config = (struct hm01b0_config *)dev->config;
	uint8_t data[3];
	uint16_t address = (uint16_t)reg;
	address = sys_cpu_to_be16(address);
	memcpy(data, &address, sizeof(address));
	data[2] = val;
	return i2c_write_dt(&config->i2c_dev, (const uint8_t *)data, sizeof(data));
}

static int hm01b0_read_reg8(const struct device *dev, enum hm01b0_reg reg, uint8_t *val)
{
	struct hm01b0_config *config = (struct hm01b0_config *)dev->config;
	uint16_t address = (uint16_t)reg;
	address = sys_cpu_to_be16(reg);
	return i2c_write_read_dt(&config->i2c_dev, &address, sizeof(address), val, sizeof(*val));
}

static int hm01b0_write_reg16(const struct device *dev, enum hm01b0_reg reg, uint16_t val)
{
	struct hm01b0_config *config = (struct hm01b0_config *)dev->config;
	uint16_t address = (uint16_t)reg;
	const uint16_t data[2] = {sys_cpu_to_be16(address), sys_cpu_to_be16(val)};
	return i2c_write_dt(&config->i2c_dev, (const uint8_t *)data, sizeof(data));
}

static int hm01b0_read_reg16(const struct device *dev, enum hm01b0_reg reg, uint16_t *val)
{
	struct hm01b0_config *config = (struct hm01b0_config *)dev->config;
	uint16_t address = (uint16_t)reg;
	address = sys_cpu_to_be16(reg);
	int res = i2c_write_read_dt(&config->i2c_dev, &address, sizeof(address), val, sizeof(*val));
	*val = sys_be16_to_cpu(*val);
	return res;
}

static int hm01b0_apply_configuration(const struct device *dev, enum hm01b0_resolution resolution)
{
	struct hm01b0_data *data = (struct hm01b0_data *)dev->data;
	int ret;
	for (int i = 0; i < ARRAY_SIZE(hm01b0_reg_values[resolution]); i++) {
		const struct hm01b0_reg_value *reg_val = &hm01b0_reg_values[resolution][i];
		if (reg_val->value_size == sizeof(uint8_t)) {
			ret = hm01b0_write_reg8(dev, reg_val->address, reg_val->value.byte);
		} else {
			ret = hm01b0_write_reg16(dev, reg_val->address, reg_val->value.word);
		}
		if (ret < 0) {
			LOG_ERR("Failed to write config list register 0x%04x", reg_val->address);
			return ret;
		}
	}
	/* REG_BIT_CONTROL */
	ret = hm01b0_write_reg8(dev, REG_BIT_CONTROL, data->ctrl_val);
	if (ret == 0) {
		/* OSC_CLK_DIV */
		ret = hm01b0_write_reg8(dev, REG_OSC_CLOCK_DIV, 0x08);
		if (ret == 0) {
			/* INTEGRATION_H */
			ret = hm01b0_write_reg16(dev, REG_INTEGRATION_H,
						 hm01b0_reg_values[resolution][5].value.word / 2);
			if (ret == 0) {
				/* GRP_PARAM_HOLD */
				hm01b0_write_reg8(dev, REG_GRP_PARAM_HOLD, 0x01);
			}
		}
	}
	if (ret != 0) {
		LOG_ERR("Error applying new configuration (%d)", ret);
	}
	return ret;
}

static int hm01b0_get_caps(const struct device *dev, struct video_caps *caps)
{
	caps->min_vbuf_count = 0;
	caps->min_line_count = LINE_COUNT_HEIGHT;
	caps->max_line_count = LINE_COUNT_HEIGHT;
	caps->format_caps = hm01b0_fmts;
	return 0;
}

static int hm01b0_set_fmt(const struct device *dev, struct video_format *fmt)
{
	struct hm01b0_data *drv_data = (struct hm01b0_data *)dev->data;
	uint16_t width, height;
	int ret = 0;
	int i = 0;
	LOG_INF("HM01B0 set_fmt: %d x %d, fmt: %s", fmt->width, fmt->height,
		VIDEO_FOURCC_TO_STR(fmt->pixelformat));

	/* We only support GREY pixel formats */
	if (fmt->pixelformat != VIDEO_PIX_FMT_GREY) {
		LOG_ERR("HM01B0 camera only supports GREY pixel formats!");
		return -ENOTSUP;
	}

	width = fmt->width;
	height = fmt->height;

	if (!memcmp(&drv_data->fmt, fmt, sizeof(drv_data->fmt))) {
		return 0;
	}

	/* Check if camera is capable of handling given format */
	while (hm01b0_fmts[i].pixelformat) {
		if (hm01b0_fmts[i].width_min == width && hm01b0_fmts[i].height_min == height &&
		    hm01b0_fmts[i].pixelformat == fmt->pixelformat) {
			drv_data->resolution = (enum hm01b0_resolution)i;
			drv_data->fmt = *fmt;
			ret = hm01b0_apply_configuration(dev, drv_data->resolution);
			return ret;
		}
		++i;
	}
	/* Camera is not capable of handling given format */
	LOG_ERR("Image resolution not supported\n");
	return -ENOTSUP;
}

static int hm01b0_get_fmt(const struct device *dev, struct video_format *fmt)
{
	struct hm01b0_data *data = dev->data;
	*fmt = data->fmt;
	LOG_INF("HM01B0 get_fmt: %d x %d, fmt: %s", fmt->width, fmt->height,
		VIDEO_FOURCC_TO_STR(fmt->pixelformat));
	return 0;
}

static int hm01b0_set_ctrl(const struct device *dev, uint32_t id)
{
	return 0;
}

static int hm01b0_set_stream(const struct device *dev, bool enable, enum video_buf_type type)
{
	int ret;

	/* SET MODE_SELECT */
	if (enable) {
		ret = hm01b0_write_reg8(dev, REG_STS, 0x01);
	} else {
		ret = hm01b0_write_reg8(dev, REG_STS, 0x00);
	}
	return 0;
}

static int hm01b0_soft_reset(const struct device *dev)
{
	int ret = hm01b0_write_reg8(dev, REG_RESET, 0x01);
	uint8_t val = 0xff;
	if (ret == 0) {
		for (int retries = 0; retries < 10; retries++) {
			ret = hm01b0_read_reg8(dev, REG_STS, &val);
			if (ret != 0 || val == 0x0) {
				break;
			}
			k_msleep(100);
		}
	}
	if (ret != 0) {
		LOG_ERR("Soft reset error (%d)", ret);
	}
	return ret;
}

static DEVICE_API(video, hm01b0_driver_api) = {.set_format = hm01b0_set_fmt,
					       .get_format = hm01b0_get_fmt,
					       .set_stream = hm01b0_set_stream,
					       .get_caps = hm01b0_get_caps,
					       .set_ctrl = hm01b0_set_ctrl};

static int hm01b0_init_controls(const struct device *dev)
{
	return 0;
}

static bool hm01b0_check_connection(const struct device *dev)
{
	uint16_t model_id;
	int ret = hm01b0_read_reg16(dev, REG_ID, &model_id);
	bool is_connected = (ret == 0 && model_id == HM01B0_ID);
	if (!is_connected) {
		LOG_ERR("Model ID mismatch: expected 0x%04x, got 0x%04x, ret (%d)", HM01B0_ID,
			model_id, ret);
	}
	return is_connected;
}

static int hm01b0_init(const struct device *dev)
{
	struct hm01b0_data *data = (struct hm01b0_data *)dev->data;
	struct hm01b0_config *config = (struct hm01b0_config *)dev->config;

	if (config->data_bits == 8) {
		data->ctrl_val = 0x02;
	} else if (config->data_bits == 4) {
		data->ctrl_val = 0x42;
	} else if (config->data_bits == 1) {
		data->ctrl_val = 0x22;
	} else {
		LOG_ERR("Invalid data bits!");
		return -ENODEV;
	}

	if (!hm01b0_check_connection(dev)) {
		LOG_ERR("%s is not ready", dev->name);
		return -ENODEV;
	}

	if (hm01b0_soft_reset(dev)) {
		LOG_ERR("error sof reset");
		return -ENODEV;
	}

	struct video_format fmt = {.pixelformat = VIDEO_PIX_FMT_GREY,
				   .width = 160,
				   .height = 120,
				   .type = VIDEO_BUF_TYPE_OUTPUT,
				   .pitch = 160 * video_bits_per_pixel(VIDEO_PIX_FMT_GREY) /
					    BITS_PER_BYTE};

	if (hm01b0_set_fmt(dev, &fmt)) {
		LOG_ERR("error setting video format");
		return -ENODEV;
	}
	return hm01b0_init_controls(dev);
}

#define HM01B0_INIT(inst)                                                                          \
	static struct hm01b0_config hm01b0_config_##inst = {                                       \
		.i2c_dev = I2C_DT_SPEC_INST_GET(inst),                                             \
		.data_bits = 1 /* Use only 1 pin for data */                                       \
	};                                                                                         \
	static struct hm01b0_data hm01b0_data_##inst;                                              \
	DEVICE_DT_INST_DEFINE(inst, &hm01b0_init, NULL, &hm01b0_data_##inst,                       \
			      &hm01b0_config_##inst, POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY,      \
			      &hm01b0_driver_api);                                                 \
	VIDEO_DEVICE_DEFINE(hm01b0_##inst, DEVICE_DT_INST_GET(inst), NULL);

DT_INST_FOREACH_STATUS_OKAY(HM01B0_INIT)
