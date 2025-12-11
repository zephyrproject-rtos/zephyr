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

#include "video_device.h"
#include "video_common.h"

LOG_MODULE_REGISTER(hm01b0, CONFIG_VIDEO_LOG_LEVEL);
#define MAX_FRAME_RATE 10
#define MIN_FRAME_RATE 1
#define HM01B0_ID      0x01B0
#define HM01B0_LINE_LEN_PCLK_IDX 5

#define HM01B0_REG8(addr)             ((addr) | VIDEO_REG_ADDR16_DATA8)
#define HM01B0_REG16(addr)            ((addr) | VIDEO_REG_ADDR16_DATA16_BE)
#define HM01B0_CCI_ID                 HM01B0_REG16(0x0000)
#define HM01B0_CCI_STS                HM01B0_REG8(0x0100)
#define HM01B0_CCI_RESET              HM01B0_REG8(0x0103)
#define HM01B0_CCI_GRP_PARAM_HOLD     HM01B0_REG8(0x0104)
#define HM01B0_CCI_INTEGRATION_H      HM01B0_REG16(0x0202)
#define HM01B0_CCI_FRAME_LENGTH_LINES HM01B0_REG16(0x0340)
#define HM01B0_CCI_LINE_LENGTH_PCLK   HM01B0_REG16(0x0342)
#define HM01B0_CCI_WIDTH              HM01B0_REG8(0x0383)
#define HM01B0_CCI_HEIGHT             HM01B0_REG8(0x0387)
#define HM01B0_CCI_BINNING_MODE       HM01B0_REG8(0x0390)
#define HM01B0_CCI_QVGA_WIN_EN        HM01B0_REG8(0x3010)
#define HM01B0_CCI_BIT_CONTROL        HM01B0_REG8(0x3059)
#define HM01B0_CCI_OSC_CLOCK_DIV      HM01B0_REG8(0x3060)

#define HM01B0_CTRL_VAL(data_bits) \
	((data_bits) == 8 ? 0x02 : \
	(data_bits) == 4 ? 0x42 : \
	(data_bits) == 1 ? 0x22 : 0x00)

enum hm01b0_resolution {
	RESOLUTION_160x120,
	RESOLUTION_320x240,
	RESOLUTION_320x320,
};

struct video_reg hm01b0_160x120_regs[] = {
	{HM01B0_CCI_WIDTH, 0x3},
	{HM01B0_CCI_HEIGHT, 0x3},
	{HM01B0_CCI_BINNING_MODE, 0x3},
	{HM01B0_CCI_QVGA_WIN_EN, 0x1},
	{HM01B0_CCI_FRAME_LENGTH_LINES, 0x80},
	{HM01B0_CCI_LINE_LENGTH_PCLK, 0xD7},
};

struct video_reg hm01b0_320x240_regs[] = {
	{HM01B0_CCI_WIDTH, 0x1},
	{HM01B0_CCI_HEIGHT, 0x1},
	{HM01B0_CCI_BINNING_MODE, 0x0},
	{HM01B0_CCI_QVGA_WIN_EN, 0x1},
	{HM01B0_CCI_FRAME_LENGTH_LINES, 0x104},
	{HM01B0_CCI_LINE_LENGTH_PCLK, 0x178},
};

struct video_reg hm01b0_320x320_regs[] = {
	{HM01B0_CCI_WIDTH, 0x1},
	{HM01B0_CCI_HEIGHT, 0x1},
	{HM01B0_CCI_BINNING_MODE, 0x0},
	{HM01B0_CCI_QVGA_WIN_EN, 0x0},
	{HM01B0_CCI_FRAME_LENGTH_LINES, 0x158},
	{HM01B0_CCI_LINE_LENGTH_PCLK, 0x178},
};

struct video_reg *hm01b0_init_regs[] = {
	[RESOLUTION_160x120] = hm01b0_160x120_regs,
	[RESOLUTION_320x240] = hm01b0_320x240_regs,
	[RESOLUTION_320x320] = hm01b0_320x320_regs,
};

struct hm01b0_data {
	struct video_format fmt;
};

struct hm01b0_config {
	const struct i2c_dt_spec i2c;
	const uint8_t data_bits;
	const uint8_t ctrl_val;
};

#define HM01B0_VIDEO_FORMAT_CAP(width, height, format)                                             \
	{                                                                                          \
		.pixelformat = (format),                                                           \
		.width_min = (width),                                                              \
		.width_max = (width),                                                              \
		.height_min = (height),                                                            \
		.height_max = (height),                                                            \
		.width_step = 0,                                                                   \
		.height_step = 0,                                                                  \
	}

static const struct video_format_cap hm01b0_fmts[] = {
	HM01B0_VIDEO_FORMAT_CAP(160, 120, VIDEO_PIX_FMT_GREY),
	HM01B0_VIDEO_FORMAT_CAP(320, 240, VIDEO_PIX_FMT_GREY),
	HM01B0_VIDEO_FORMAT_CAP(320, 320, VIDEO_PIX_FMT_GREY),
	{0},
};

static int hm01b0_apply_configuration(const struct device *dev, enum hm01b0_resolution resolution)
{
	const struct hm01b0_config *config = dev->config;
	int ret;

	/* Number of registers is the same for all configuration */
	ret = video_write_cci_multiregs(&config->i2c, hm01b0_init_regs[resolution],
					ARRAY_SIZE(hm01b0_160x120_regs));
	if (ret < 0) {
		LOG_ERR("Failed to write config list registers (%d)", ret);
		return ret;
	}

	/* REG_BIT_CONTROL */
	ret = video_write_cci_reg(&config->i2c, HM01B0_CCI_BIT_CONTROL, config->ctrl_val);
	if (ret < 0) {
		LOG_ERR("Failed to write BIT_CONTROL reg (%d)", ret);
		return ret;
	}

	/* OSC_CLK_DIV */
	ret = video_write_cci_reg(&config->i2c, HM01B0_CCI_OSC_CLOCK_DIV, 0x08);
	if (ret < 0) {
		LOG_ERR("Failed to write OSC_CLK_DIV reg (%d)", ret);
		return ret;
	}

	/* INTEGRATION_H */
	ret = video_write_cci_reg(&config->i2c, HM01B0_CCI_INTEGRATION_H,
				  hm01b0_init_regs[resolution][HM01B0_LINE_LEN_PCLK_IDX].data / 2);
	if (ret < 0) {
		LOG_ERR("Failed to write INTEGRATION_H reg (%d)", ret);
		return ret;
	}

	/* GRP_PARAM_HOLD */
	ret = video_write_cci_reg(&config->i2c, HM01B0_CCI_GRP_PARAM_HOLD, 0x01);
	if (ret < 0) {
		LOG_ERR("Failed to write GRP_PARAM_HOLD reg (%d)", ret);
		return ret;
	}

	return ret;
}

static int hm01b0_get_caps(const struct device *dev, struct video_caps *caps)
{
	caps->format_caps = hm01b0_fmts;

	return 0;
}

static int hm01b0_set_fmt(const struct device *dev, struct video_format *fmt)
{
	struct hm01b0_data *data = dev->data;
	size_t idx;
	int ret;

	LOG_DBG("HM01B0 set_fmt: %d x %d, fmt: %s", fmt->width, fmt->height,
		VIDEO_FOURCC_TO_STR(fmt->pixelformat));

	ret = video_format_caps_index(hm01b0_fmts, fmt, &idx);
	if (ret != 0) {
		LOG_ERR("Image resolution not supported\n");
		return ret;
	}

	if (!memcmp(&data->fmt, fmt, sizeof(data->fmt))) {
		return 0;
	}

	/* Check if camera is capable of handling given format */
	ret = hm01b0_apply_configuration(dev, (enum hm01b0_resolution)idx);
	if (ret != 0) {
		/* Camera is not capable of handling given format */
		LOG_ERR("Image resolution not supported");
		return ret;
	}
	data->fmt = *fmt;

	return ret;
}

static int hm01b0_get_fmt(const struct device *dev, struct video_format *fmt)
{
	struct hm01b0_data *data = dev->data;

	*fmt = data->fmt;
	LOG_DBG("HM01B0 get_fmt: %d x %d, fmt: %s", fmt->width, fmt->height,
		VIDEO_FOURCC_TO_STR(fmt->pixelformat));

	return 0;
}

static int hm01b0_set_stream(const struct device *dev, bool enable, enum video_buf_type type)
{
	const struct hm01b0_config *config = dev->config;

	/* SET MODE_SELECT */
	return video_write_cci_reg(&config->i2c, HM01B0_CCI_STS, enable ? 1 : 0);
}

static int hm01b0_soft_reset(const struct device *dev)
{
	const struct hm01b0_config *config = dev->config;
	uint32_t val = 0xff;
	int ret;

	ret = video_write_cci_reg(&config->i2c, HM01B0_CCI_RESET, 0x01);
	if (ret < 0) {
		LOG_ERR("Error writing HM01B0_CCI_RESET (%d)", ret);
		return ret;
	}

	for (int retries = 0; retries < 10; retries++) {
		ret = video_read_cci_reg(&config->i2c, HM01B0_CCI_STS, &val);
		if (ret != 0 || val == 0x0) {
			break;
		}
		k_msleep(100);
	}
	if (ret != 0) {
		LOG_ERR("Soft reset error (%d)", ret);
	}

	return ret;
}

static DEVICE_API(video, hm01b0_driver_api) = {
	.set_format = hm01b0_set_fmt,
	.get_format = hm01b0_get_fmt,
	.set_stream = hm01b0_set_stream,
	.get_caps = hm01b0_get_caps,
};

static bool hm01b0_check_connection(const struct device *dev)
{
	const struct hm01b0_config *config = dev->config;
	uint32_t model_id;
	int ret;

	ret = video_read_cci_reg(&config->i2c, HM01B0_CCI_ID, &model_id);
	if (ret < 0) {
		LOG_ERR("Error reading id reg (%d)", ret);
		return false;
	}

	return (model_id == HM01B0_ID);
}

static int hm01b0_init(const struct device *dev)
{
	int ret;

	if (!hm01b0_check_connection(dev)) {
		LOG_ERR("%s is not ready", dev->name);
		return -ENODEV;
	}

	ret = hm01b0_soft_reset(dev);
	if (ret != 0) {
		LOG_ERR("error soft reset (%d)", ret);
		return ret;
	}

	struct video_format fmt = {
		.pixelformat = VIDEO_PIX_FMT_GREY,
		.width = 160,
		.height = 120,
		.type = VIDEO_BUF_TYPE_OUTPUT,
	};

	ret = hm01b0_set_fmt(dev, &fmt);
	if (ret != 0) {
		LOG_ERR("Error setting video format (%d)", ret);
		return ret;
	}

	return 0;
}

#define HM01B0_INIT(inst)                                                                          \
	const struct hm01b0_config hm01b0_config_##inst = {                                        \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.data_bits = DT_INST_PROP(inst, data_bits),                                        \
		.ctrl_val = HM01B0_CTRL_VAL(DT_INST_PROP(inst, data_bits)),                        \
	};                                                                                         \
	struct hm01b0_data hm01b0_data_##inst;                                                     \
	DEVICE_DT_INST_DEFINE(inst, &hm01b0_init, NULL, &hm01b0_data_##inst,                       \
			      &hm01b0_config_##inst, POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY,      \
			      &hm01b0_driver_api);                                                 \
	VIDEO_DEVICE_DEFINE(hm01b0_##inst, DEVICE_DT_INST_GET(inst), NULL);

DT_INST_FOREACH_STATUS_OKAY(HM01B0_INIT)
