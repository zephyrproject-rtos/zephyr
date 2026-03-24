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
#include <zephyr/logging/log.h>

#include "video_ctrls.h"
#include "video_device.h"

LOG_MODULE_REGISTER(video_emul_imager, CONFIG_VIDEO_LOG_LEVEL);

#define EMUL_IMAGER_REG_SENSOR_ID 0x0000
#define EMUL_IMAGER_SENSOR_ID     0x99
#define EMUL_IMAGER_REG_CTRL      0x0001
#define EMUL_IMAGER_REG_INIT1     0x0002
#define EMUL_IMAGER_REG_INIT2     0x0003
#define EMUL_IMAGER_REG_TIMING1   0x0004
#define EMUL_IMAGER_REG_TIMING2   0x0005
#define EMUL_IMAGER_REG_TIMING3   0x0006
#define EMUL_IMAGER_REG_CUSTOM    0x0007
#define EMUL_IMAGER_REG_FORMAT    0x000a
#define EMUL_IMAGER_PATTERN_OFF   0x00
#define EMUL_IMAGER_PATTERN_BARS1 0x01
#define EMUL_IMAGER_PATTERN_BARS2 0x02

/* Custom control that is just an I2C write for example and test purpose */
#define EMUL_IMAGER_CID_CUSTOM (VIDEO_CID_PRIVATE_BASE + 0x01)

/* Emulated register bank */
uint8_t emul_imager_fake_regs[10];

enum {
	EMUL_IMAGER_320x240_RGB565,
	EMUL_IMAGER_320x240_YUYV,
	EMUL_IMAGER_160x120_RGB565,
	EMUL_IMAGER_160x120_YUYV,
};

struct emul_imager_reg {
	uint16_t addr;
	uint8_t value;
};

struct emul_imager_config {
	struct i2c_dt_spec i2c;
};

struct emul_imager_ctrls {
	struct video_ctrl custom;
};

struct emul_imager_data {
	/* First field is a line buffer for I/O emulation purpose */
	uint8_t framebuffer[320 * sizeof(uint16_t)];
	/* Other fields are shared with real hardware drivers */
	uint32_t frame_rate;
	struct video_format fmt;
	struct emul_imager_ctrls ctrls;
};

/* All the I2C registers sent on various scenario */
static const struct emul_imager_reg emul_imager_init_regs[] = {
	{EMUL_IMAGER_REG_CTRL, 0x00},
	{EMUL_IMAGER_REG_INIT1, 0x10},
	{EMUL_IMAGER_REG_INIT2, 0x00},
	/* Undocumented registers from the vendor */
	{0x1200, 0x01},
	{0x1204, 0x01},
	{0x1205, 0x20},
	{0x1209, 0x7f},
	{0},
};
static const struct emul_imager_reg emul_imager_320x240_rgb565[] = {
	{EMUL_IMAGER_REG_FORMAT, 0x01},
	{EMUL_IMAGER_REG_TIMING1, 32},
	{EMUL_IMAGER_REG_TIMING2, 24},
};
static const struct emul_imager_reg emul_imager_320x240_yuyv[] = {
	{EMUL_IMAGER_REG_FORMAT, 0x02},
	{EMUL_IMAGER_REG_TIMING1, 32},
	{EMUL_IMAGER_REG_TIMING2, 24},
};
static const struct emul_imager_reg emul_imager_160x120_rgb565[] = {
	{EMUL_IMAGER_REG_FORMAT, 0x01},
	{EMUL_IMAGER_REG_TIMING1, 16},
	{EMUL_IMAGER_REG_TIMING2, 12},
};
static const struct emul_imager_reg emul_imager_160x120_yuyv[] = {
	{EMUL_IMAGER_REG_FORMAT, 0x02},
	{EMUL_IMAGER_REG_TIMING1, 16},
	{EMUL_IMAGER_REG_TIMING2, 12},
};
static const struct emul_imager_reg emul_imager_15fps[] = {
	{EMUL_IMAGER_REG_TIMING3, 15},
	{0},
};
static const struct emul_imager_reg emul_imager_30fps[] = {
	{EMUL_IMAGER_REG_TIMING3, 30},
	{0},
};
static const struct emul_imager_reg emul_imager_60fps[] = {
	{EMUL_IMAGER_REG_TIMING3, 60},
	{0},
};

#define EMUL_IMAGER_CAP(format, width, height)                                                     \
	{                                                                                          \
		/* For a real imager, the width and height would be macro parameters */            \
		.pixelformat = (format),                                                           \
		.width_min = (width),                                                              \
		.width_max = (width),                                                              \
		.width_step = 0,                                                                   \
		.height_min = (height),                                                            \
		.height_max = (height),                                                            \
		.height_step = 0,                                                                  \
	}
static const struct video_format_cap fmts[] = {
	[EMUL_IMAGER_320x240_RGB565]	= EMUL_IMAGER_CAP(VIDEO_PIX_FMT_RGB565, 320, 240),
	[EMUL_IMAGER_320x240_YUYV]	= EMUL_IMAGER_CAP(VIDEO_PIX_FMT_YUYV, 320, 240),
	[EMUL_IMAGER_160x120_RGB565]	= EMUL_IMAGER_CAP(VIDEO_PIX_FMT_RGB565, 160, 120),
	[EMUL_IMAGER_160x120_YUYV]	= EMUL_IMAGER_CAP(VIDEO_PIX_FMT_YUYV, 160, 120),
	{0},
};

/* Emulated I2C register interface, to replace with actual I2C calls for real hardware */
static int emul_imager_read_reg(const struct device *const dev, uint8_t reg_addr, uint8_t *value)
{
	LOG_DBG("Placeholder for I2C read from 0x%02x", reg_addr);
	switch (reg_addr) {
	case EMUL_IMAGER_REG_SENSOR_ID:
		*value = EMUL_IMAGER_SENSOR_ID;
		break;
	default:
		*value = emul_imager_fake_regs[reg_addr];
	}
	return 0;
}

/* Some sensors will need reg8 or reg16 variants. */
static int emul_imager_write_reg(const struct device *const dev, uint8_t reg_addr, uint8_t value)
{
	LOG_DBG("Placeholder for I2C write 0x%08x to 0x%02x", value, reg_addr);
	emul_imager_fake_regs[reg_addr] = value;
	return 0;
}

static int emul_imager_write_multi(const struct device *const dev,
				   const struct emul_imager_reg *regs)
{
	int ret;

	for (int i = 0; regs[i].addr != 0; i++) {
		ret = emul_imager_write_reg(dev, regs[i].addr, regs[i].value);
		if (ret < 0) {
			return ret;
		}
	}
	return 0;
}

static int emul_imager_get_caps(const struct device *dev, struct video_caps *caps)
{
	caps->format_caps = fmts;
	return 0;
}

static int emul_imager_set_ctrl(const struct device *dev, unsigned int cid)
{
	struct emul_imager_data *data = dev->data;

	switch (cid) {
	case EMUL_IMAGER_CID_CUSTOM:
		return emul_imager_write_reg(dev, EMUL_IMAGER_REG_CUSTOM, data->ctrls.custom.val);
	default:
		return -ENOTSUP;
	}
}

static int emul_imager_set_fmt(const struct device *dev, struct video_format *fmt)
{
	const struct emul_imager_config *cfg = dev->config;
	struct emul_imager_data *data = dev->data;
	size_t idx;
	int ret;

	ret = video_format_caps_index(fmts, fmt, &idx);
	if (ret < 0) {
		LOG_ERR("Format requested '%s' %ux%u not supported",
			VIDEO_FOURCC_TO_STR(fmt->pixelformat), fmt->width, fmt->height);
		return -ENOTSUP;
	}

	ret = video_estimate_fmt_size(fmt);
	if (ret < 0) {
		return ret;
	}

	switch (idx) {
	case EMUL_IMAGER_320x240_RGB565:
		ret = emul_imager_write_multi(dev, emul_imager_320x240_rgb565);
		break;
	case EMUL_IMAGER_320x240_YUYV:
		ret = emul_imager_write_multi(dev, emul_imager_320x240_yuyv);
		break;
	case EMUL_IMAGER_160x120_RGB565:
		ret = emul_imager_write_multi(dev, emul_imager_160x120_rgb565);
		break;
	case EMUL_IMAGER_160x120_YUYV:
		ret = emul_imager_write_multi(dev, emul_imager_160x120_yuyv);
		break;
	default:
		CODE_UNREACHABLE;
		return -EINVAL;
	}
	if (ret < 0) {
		return ret;
	}

	data->fmt = *fmt;

	/* For the purpose of simulation, fill the image line buffer with 50% gray, this data
	 * will be collected by the video_emul_rx driver.
	 */
	if (fmt->pixelformat == VIDEO_PIX_FMT_RGB565) {
		for (int i = 0; i < fmt->width; i++) {
			((uint16_t *)data->framebuffer)[i] = sys_cpu_to_le16(0x7bef);
		}
	} else {
		memset(data->framebuffer, 0x7f, fmt->pitch);
	}

	return 0;
}

static int emul_imager_get_fmt(const struct device *dev, struct video_format *fmt)
{
	struct emul_imager_data *data = dev->data;

	*fmt = data->fmt;

	return 0;
}

enum {
	EMUL_IMAGER_60FPS_IDX,
	EMUL_IMAGER_30FPS_IDX,
	EMUL_IMAGER_15FPS_IDX,
};

enum {
	EMUL_IMAGER_60FPS = 60,
	EMUL_IMAGER_30FPS = 30,
	EMUL_IMAGER_15FPS = 15,
};

static const uint32_t emul_imager_framerates[] = {
	[EMUL_IMAGER_60FPS_IDX] = EMUL_IMAGER_60FPS,
	[EMUL_IMAGER_30FPS_IDX] = EMUL_IMAGER_30FPS,
	[EMUL_IMAGER_15FPS_IDX] = EMUL_IMAGER_15FPS,
};

static int emul_imager_enum_frmival(const struct device *dev, struct video_frmival_enum *fie)
{
	if (fie->index >= ARRAY_SIZE(emul_imager_framerates)) {
		return -EINVAL;
	}

	fie->type = VIDEO_FRMIVAL_TYPE_DISCRETE;
	fie->discrete.numerator = 1;
	fie->discrete.denominator = emul_imager_framerates[fie->index];

	return 0;
}

static int emul_imager_set_frmival(const struct device *dev, struct video_frmival *frmival)
{
	struct emul_imager_data *data = dev->data;
	struct video_frmival_enum fie = {.format = &data->fmt, .discrete = *frmival};
	int ret;

	switch (fie.index) {
	case EMUL_IMAGER_60FPS_IDX:
		ret = emul_imager_write_multi(dev, emul_imager_60fps);
		break;
	case EMUL_IMAGER_30FPS_IDX:
		ret = emul_imager_write_multi(dev, emul_imager_30fps);
		break;
	case EMUL_IMAGER_15FPS_IDX:
		ret = emul_imager_write_multi(dev, emul_imager_15fps);
		break;
	default:
		CODE_UNREACHABLE;
		return -EINVAL;
	}

	if (ret < 0) {
		return ret;
	}

	frmival->numerator = 1;
	frmival->denominator = data->frame_rate = emul_imager_framerates[fie.index];

	return 0;
}

static int emul_imager_get_frmival(const struct device *dev, struct video_frmival *frmival)
{
	struct emul_imager_data *data = dev->data;

	frmival->numerator = 1;
	frmival->denominator = data->frame_rate;
	return 0;
}

static int emul_imager_set_stream(const struct device *dev, bool enable, enum video_buf_type type)
{
	return emul_imager_write_reg(dev, EMUL_IMAGER_REG_CTRL, enable ? 1 : 0);
}

static DEVICE_API(video, emul_imager_driver_api) = {
	.set_ctrl = emul_imager_set_ctrl,
	.set_frmival = emul_imager_set_frmival,
	.get_frmival = emul_imager_get_frmival,
	.enum_frmival = emul_imager_enum_frmival,
	.set_format = emul_imager_set_fmt,
	.get_format = emul_imager_get_fmt,
	.get_caps = emul_imager_get_caps,
	.set_stream = emul_imager_set_stream,
};

static int emul_imager_init_controls(const struct device *dev)
{
	struct emul_imager_data *drv_data = dev->data;

	return video_init_ctrl(
		&drv_data->ctrls.custom, dev, EMUL_IMAGER_CID_CUSTOM,
		(struct video_ctrl_range){.min = 0, .max = 255, .step = 1, .def = 128});
}

int emul_imager_init(const struct device *dev)
{
	struct video_format fmt = {
		.pixelformat = fmts[0].pixelformat,
		.width = fmts[0].width_min,
		.height = fmts[0].height_min,
	};
	uint8_t sensor_id;
	int ret;

	if (/* !i2c_is_ready_dt(&cfg->i2c) */ false) {
		/* LOG_ERR("Bus %s is not ready", cfg->i2c.bus->name); */
		return -ENODEV;
	}

	ret = emul_imager_read_reg(dev, EMUL_IMAGER_REG_SENSOR_ID, &sensor_id);
	if (ret < 0 || sensor_id != EMUL_IMAGER_SENSOR_ID) {
		LOG_ERR("Failed to get a correct sensor ID 0x%x",  sensor_id);
		return ret;
	}

	ret = emul_imager_write_multi(dev, emul_imager_init_regs);
	if (ret < 0) {
		LOG_ERR("Could not set initial registers");
		return ret;
	}

	ret = emul_imager_set_fmt(dev, &fmt);
	if (ret < 0) {
		LOG_ERR("Failed to set to default format %x %ux%u",
			fmt.pixelformat, fmt.width, fmt.height);
	}

	/* Initialize controls */
	return emul_imager_init_controls(dev);
}

#define EMUL_IMAGER_DEFINE(inst)                                                                   \
	static struct emul_imager_data emul_imager_data_##inst;                                    \
                                                                                                   \
	static const struct emul_imager_config emul_imager_cfg_##inst = {                          \
		.i2c = /* I2C_DT_SPEC_INST_GET(inst) */ {0},                                       \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &emul_imager_init, NULL, &emul_imager_data_##inst,             \
			      &emul_imager_cfg_##inst, POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY,    \
			      &emul_imager_driver_api);                                            \
                                                                                                   \
	VIDEO_DEVICE_DEFINE(emul_imager_##inst, DEVICE_DT_INST_GET(inst), NULL);

DT_INST_FOREACH_STATUS_OKAY(EMUL_IMAGER_DEFINE)
