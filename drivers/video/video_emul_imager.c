/*
 * Copyright (c) 2024 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_video_emul_imager

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video-controls.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/logging/log.h>

#include "video_imager.h"

LOG_MODULE_REGISTER(video_emul_imager, CONFIG_VIDEO_LOG_LEVEL);

#define EMUL_IMAGER_REG_CTRL      VIDEO_CCI_REG(0x0001, 2, 1)
#define EMUL_IMAGER_REG_INIT1     VIDEO_CCI_REG(0x0002, 2, 1)
#define EMUL_IMAGER_REG_INIT2     VIDEO_CCI_REG(0x0003, 2, 1)
#define EMUL_IMAGER_REG_TIMING1   VIDEO_CCI_REG(0x0004, 2, 1)
#define EMUL_IMAGER_REG_TIMING2   VIDEO_CCI_REG(0x0005, 2, 1)
#define EMUL_IMAGER_REG_TIMING3   VIDEO_CCI_REG(0x0006, 2, 1)
#define EMUL_IMAGER_REG_EXPOSURE  VIDEO_CCI_REG(0x0007, 2, 2)
#define EMUL_IMAGER_REG_GAIN      VIDEO_CCI_REG(0x0008, 2, 1)
#define EMUL_IMAGER_REG_PATTERN   VIDEO_CCI_REG(0x0009, 2, 1)
#define EMUL_IMAGER_PATTERN_OFF   0x00
#define EMUL_IMAGER_PATTERN_BARS1 0x01
#define EMUL_IMAGER_PATTERN_BARS2 0x02

/* Emulated register bank */
uint8_t emul_imager_fake_regs[10];

struct emul_imager_config {
	/* A framebuffer for I/O emulation purpose */
	uint8_t *framebuffer;
};

/* Initial parameters of the sensors common to all modes. */
static const struct video_cci_reg emul_imager_init_regs[] = {
	{EMUL_IMAGER_REG_CTRL, 0x00},
	/* Example comment about REG_INIT1 */
	{EMUL_IMAGER_REG_INIT1, 0x10},
	{EMUL_IMAGER_REG_INIT2, 0x00},
	{0},
};

/* List of registers aggregated together in "modes" that can be applied
 * to set the timing parameters and other mode-dependent configuration.
 */

static const struct video_cci_reg emul_imager_rgb565_64x20[] = {
	{EMUL_IMAGER_REG_TIMING1, 0x64},
	{EMUL_IMAGER_REG_TIMING2, 0x20},
	{0},
};
static const struct video_cci_reg emul_imager_rgb565_64x20_15fps[] = {
	{EMUL_IMAGER_REG_TIMING3, 15},
	{0},
};
static const struct video_cci_reg emul_imager_rgb565_64x20_30fps[] = {
	{EMUL_IMAGER_REG_TIMING3, 30},
	{0},
};
static const struct video_cci_reg emul_imager_rgb565_64x20_60fps[] = {
	{EMUL_IMAGER_REG_TIMING3, 60},
	{0},
};
static const struct video_imager_mode emul_imager_rgb565_64x20_modes[] = {
	{.fps = 15, .regs = {emul_imager_rgb565_64x20, emul_imager_rgb565_64x20_15fps}},
	{.fps = 30, .regs = {emul_imager_rgb565_64x20, emul_imager_rgb565_64x20_30fps}},
	{.fps = 60, .regs = {emul_imager_rgb565_64x20, emul_imager_rgb565_64x20_60fps}},
	{0},
};

static const struct video_cci_reg emul_imager_yuyv_64x20[] = {
	{EMUL_IMAGER_REG_TIMING1, 0x64},
	{EMUL_IMAGER_REG_TIMING2, 0x20},
	{0},
};
static const struct video_cci_reg emul_imager_yuyv_64x20_15fps[] = {
	{EMUL_IMAGER_REG_TIMING3, 15},
	{0},
};
static const struct video_cci_reg emul_imager_yuyv_64x20_30fps[] = {
	{EMUL_IMAGER_REG_TIMING3, 30},
	{0},
};
static const struct video_imager_mode emul_imager_yuyv_64x20_modes[] = {
	{.fps = 15, .regs = {emul_imager_yuyv_64x20, emul_imager_yuyv_64x20_15fps}},
	{.fps = 30, .regs = {emul_imager_yuyv_64x20, emul_imager_yuyv_64x20_30fps}},
	{0},
};

enum emul_imager_fmt_id {
	RGB565_64x20,
	YUYV_64x20,
};

/* Summary of all the modes of all the frame formats, with the format ID as
 * index, matching fmts[].
 */
static const struct video_imager_mode *modes[] = {
	[RGB565_64x20] = emul_imager_rgb565_64x20_modes,
	[YUYV_64x20] = emul_imager_yuyv_64x20_modes,
};

/* Video device capabilities where the supported resolutions and pixel formats are listed.
 * The format ID is used as index to fetch the matching mode from the list above.
 */
static const struct video_format_cap fmts[] = {
	[RGB565_64x20] = VIDEO_IMAGER_FORMAT_CAP(64, 20, VIDEO_PIX_FMT_RGB565),
	[YUYV_64x20] = VIDEO_IMAGER_FORMAT_CAP(64, 20, VIDEO_PIX_FMT_YUYV),
	{0},
};

static int emul_imager_set_ctrl(const struct device *dev, unsigned int cid, void *value)
{
	struct video_imager_data *data = dev->data;

	switch (cid) {
	case VIDEO_CID_EXPOSURE:
		return video_cci_write_reg(&data->i2c, EMUL_IMAGER_REG_EXPOSURE, (int)value);
	case VIDEO_CID_GAIN:
		return video_cci_write_reg(&data->i2c, EMUL_IMAGER_REG_GAIN, (int)value);
	case VIDEO_CID_TEST_PATTERN:
		return video_cci_write_reg(&data->i2c, EMUL_IMAGER_REG_PATTERN, (int)value);
	default:
		return -ENOTSUP;
	}
}

static int emul_imager_get_ctrl(const struct device *dev, unsigned int cid, void *value)
{
	struct video_imager_data *data = dev->data;
	uint32_t reg;

	switch (cid) {
	case VIDEO_CID_EXPOSURE:
		video_cci_read_reg(&data->i2c, EMUL_IMAGER_REG_EXPOSURE, &reg);
	case VIDEO_CID_GAIN:
		video_cci_read_reg(&data->i2c, EMUL_IMAGER_REG_GAIN, &reg);
	case VIDEO_CID_TEST_PATTERN:
		video_cci_read_reg(&data->i2c, EMUL_IMAGER_REG_PATTERN, &reg);
	case VIDEO_CID_PIXEL_RATE:
		*(int64_t *)value = (int64_t)data->fmt.width * data->fmt.pitch * data->mode->fps;
		return 0;
	default:
		return -ENOTSUP;
	}

	*(int *)value = reg;
}

/* White, Yellow, Cyan, Green, Magenta, Red, Blue, Black */
static const uint16_t pattern_8bars_yuv[8][3] = {
	{0xFF, 0x7F, 0x7F}, {0xFF, 0x00, 0xFF}, {0xFF, 0xFF, 0x00}, {0x7F, 0x00, 0x00},
	{0x00, 0xFF, 0xFF}, {0x00, 0x00, 0xFF}, {0x00, 0xFF, 0x00}, {0x00, 0x7F, 0x7F}};
static const uint16_t pattern_8bars_rgb[8][3] = {
	{0xFF, 0xFF, 0xFF}, {0xFF, 0xFF, 0x00}, {0x00, 0xFF, 0xFF}, {0x00, 0xFF, 0x00},
	{0xFF, 0x00, 0xFF}, {0xFF, 0x00, 0x00}, {0x00, 0x00, 0xFF}, {0x00, 0x00, 0x00}};
static void emul_imager_fill_framebuffer(const struct device *const dev, struct video_format *fmt)
{
	const struct emul_imager_config *cfg = dev->config;
	uint16_t *fb16 = (uint16_t *)cfg->framebuffer;
	uint16_t r, g, b, y, uv;

	/* Fill the first row of the emulated framebuffer */
	switch (fmt->pixelformat) {
	case VIDEO_PIX_FMT_YUYV:
		for (size_t i = 0; i < fmt->width; i++) {
			y = pattern_8bars_yuv[i * 8 / fmt->width][0];
			uv = pattern_8bars_yuv[i * 8 / fmt->width][1 + i % 2];
			fb16[i] = sys_cpu_to_be16(y << 8 | uv << 0);
		}
		break;
	case VIDEO_PIX_FMT_RGB565:
		for (size_t i = 0; i < fmt->width; i++) {
			r = pattern_8bars_rgb[i * 8 / fmt->width][0] >> (8 - 5);
			g = pattern_8bars_rgb[i * 8 / fmt->width][1] >> (8 - 6);
			b = pattern_8bars_rgb[i * 8 / fmt->width][2] >> (8 - 5);
			fb16[i] = sys_cpu_to_le16((r << 11) | (g << 6) | (b << 0));
		}
		break;
	default:
		LOG_WRN("Unsupported pixel format %x, supported: %x, %x", fmt->pixelformat,
			VIDEO_PIX_FMT_YUYV, VIDEO_PIX_FMT_RGB565);
		memset(fb16, 0, fmt->pitch);
	}

	/* Duplicate the first row over the whole frame */
	for (size_t i = 1; i < fmt->height; i++) {
		memcpy(cfg->framebuffer + fmt->pitch * i, cfg->framebuffer, fmt->pitch);
	}
}

static int emul_imager_set_fmt(const struct device *dev, enum video_endpoint_id ep,
			       struct video_format *fmt)
{
	emul_imager_fill_framebuffer(dev, fmt);

	return video_imager_set_fmt(dev, ep, fmt);
}

static int emul_imager_set_stream(const struct device *dev, bool enable)
{
	struct video_imager_data *data = dev->data;

	return video_cci_write_reg(&data->i2c, EMUL_IMAGER_REG_CTRL, enable ? 1 : 0);
}

static DEVICE_API(video, emul_imager_api) = {
	.set_stream = emul_imager_set_stream,
	.set_ctrl = emul_imager_set_ctrl,
	.get_ctrl = emul_imager_get_ctrl,
	.set_frmival = video_imager_set_frmival,
	.get_frmival = video_imager_get_frmival,
	.enum_frmival = video_imager_enum_frmival,
	.set_format = emul_imager_set_fmt,
	.get_format = video_imager_get_fmt,
	.get_caps = video_imager_get_caps,
};

int emul_imager_init(const struct device *dev)
{
	return video_imager_init(dev, emul_imager_init_regs, RGB565_64x20);
}

#define EMUL_IMAGER_DEFINE(inst)                                                                   \
	uint8_t emul_imager_framebuffer_##inst[CONFIG_VIDEO_EMUL_IMAGER_FRAMEBUFFER_SIZE];         \
                                                                                                   \
	static const struct emul_imager_config emul_imager_cfg_##inst = {                          \
		.framebuffer = emul_imager_framebuffer_##inst,                                     \
	};                                                                                         \
                                                                                                   \
	static struct video_imager_data emul_imager_data_##inst = {                                \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.fmts = fmts,                                                                      \
		.modes = modes,                                                                    \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &emul_imager_init, NULL, &emul_imager_data_##inst,             \
			      &emul_imager_cfg_##inst, POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY,    \
			      &emul_imager_api);

DT_INST_FOREACH_STATUS_OKAY(EMUL_IMAGER_DEFINE)

/* Emulation API, absent from real drvice drivers */

static int emul_imager_transfer_i2c(const struct emul *target, struct i2c_msg msgs[], int num_msgs,
				    int addr)
{
	return 0;
}

static const struct i2c_emul_api emul_imager_api_i2c = {
	.transfer = emul_imager_transfer_i2c,
};

static int emul_imager_init_i2c(const struct emul *target, const struct device *dev)
{
	return 0;
}

#define EMUL_IMAGER_DEFINE(inst)                                                                   \
	EMUL_DT_INST_DEFINE(inst, emul_imager_init_i2c, NULL, NULL, &emul_imager_api_i2c,          \
			    &emul_imager_api);

DT_INST_FOREACH_STATUS_OKAY(EMUL_IMAGER_DEFINE)
