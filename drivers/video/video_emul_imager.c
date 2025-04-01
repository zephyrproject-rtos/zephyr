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

#include "video_common.h"

LOG_MODULE_REGISTER(video_emul_imager, CONFIG_VIDEO_LOG_LEVEL);

#define EMUL_IMAGER_REG8(addr) ((addr) | VIDEO_REG_ADDR8_DATA8)

#define EMUL_IMAGER_REG_SENSOR_ID EMUL_IMAGER_REG8(0x00)
#define EMUL_IMAGER_SENSOR_ID     0x99
#define EMUL_IMAGER_REG_CTRL      EMUL_IMAGER_REG8(0x01)
#define EMUL_IMAGER_REG_INIT1     EMUL_IMAGER_REG8(0x02)
#define EMUL_IMAGER_REG_INIT2     EMUL_IMAGER_REG8(0x03)
#define EMUL_IMAGER_REG_TIMING1   EMUL_IMAGER_REG8(0x04)
#define EMUL_IMAGER_REG_TIMING2   EMUL_IMAGER_REG8(0x05)
#define EMUL_IMAGER_REG_TIMING3   EMUL_IMAGER_REG8(0x06)
#define EMUL_IMAGER_REG_CUSTOM    EMUL_IMAGER_REG8(0x07)
#define EMUL_IMAGER_REG_FORMAT    EMUL_IMAGER_REG8(0x0a)
#define EMUL_IMAGER_PATTERN_OFF   EMUL_IMAGER_REG8(0x00)
#define EMUL_IMAGER_PATTERN_BARS1 EMUL_IMAGER_REG8(0x01)
#define EMUL_IMAGER_PATTERN_BARS2 EMUL_IMAGER_REG8(0x02)

/* Custom control that is just an I2C write for example and test purpose */
#define EMUL_IMAGER_CID_CUSTOM (VIDEO_CID_PRIVATE_BASE + 0x01)

enum emul_imager_fmt_id {
	RGB565_320x240,
	YUYV_320x240,
};

/* Wrapper over the struct video_imager_data that extends it with other fields */
struct emul_imager_data {
	/* The first field is the default data used by all image sensors */
	struct video_imager_data imager;
	/* A line buffer for I/O emulation purpose */
	uint8_t linebuffer[320 * sizeof(uint16_t)];
};

/* All the I2C registers sent on various scenario */
static const struct video_reg emul_imager_init_regs[] = {
	{EMUL_IMAGER_REG_CTRL, 0x00},
	{EMUL_IMAGER_REG_INIT1, 0x10},
	{EMUL_IMAGER_REG_INIT2, 0x00},
	/* Undocumented registers from the vendor */
	{EMUL_IMAGER_REG8(0x80), 0x01},
	{EMUL_IMAGER_REG8(0x84), 0x01},
	{EMUL_IMAGER_REG8(0x85), 0x20},
	{EMUL_IMAGER_REG8(0x89), 0x7f},
	{0},
};
static const struct video_reg emul_imager_rgb565[] = {
	{EMUL_IMAGER_REG_FORMAT, 0x01},
	{0},
};
static const struct video_reg emul_imager_yuyv[] = {
	{EMUL_IMAGER_REG_FORMAT, 0x02},
	{0},
};
static const struct video_reg emul_imager_320x240[] = {
	{EMUL_IMAGER_REG_TIMING1, 0x32},
	{EMUL_IMAGER_REG_TIMING2, 0x24},
	{0},
};
static const struct video_reg emul_imager_15fps[] = {
	{EMUL_IMAGER_REG_TIMING3, 15},
	{0},
};
static const struct video_reg emul_imager_30fps[] = {
	{EMUL_IMAGER_REG_TIMING3, 30},
	{0},
};
static const struct video_reg emul_imager_60fps[] = {
	{EMUL_IMAGER_REG_TIMING3, 60},
	{0},
};

/* Description of "modes", that pick lists of registesr that will be all sentto the imager */
static struct video_imager_mode emul_imager_rgb565_320x240_modes[] = {
	{.fps = 15, .regs = {emul_imager_320x240, emul_imager_rgb565, emul_imager_15fps}},
	{.fps = 30, .regs = {emul_imager_320x240, emul_imager_rgb565, emul_imager_30fps}},
	{.fps = 60, .regs = {emul_imager_320x240, emul_imager_rgb565, emul_imager_60fps}},
	{0},
};
static struct video_imager_mode emul_imager_yuyv_320x240_modes[] = {
	{.fps = 15, .regs = {emul_imager_320x240, emul_imager_yuyv, emul_imager_15fps}},
	{.fps = 30, .regs = {emul_imager_320x240, emul_imager_yuyv, emul_imager_30fps}},
	{0},
};

static const struct video_imager_mode *emul_imager_modes[] = {
	[RGB565_320x240] = emul_imager_rgb565_320x240_modes,
	[YUYV_320x240] = emul_imager_yuyv_320x240_modes,
};
static const struct video_format_cap emul_imager_fmts[] = {
	[RGB565_320x240] = VIDEO_IMAGER_FORMAT_CAP(VIDEO_PIX_FMT_RGB565, 320, 240),
	[YUYV_320x240] = VIDEO_IMAGER_FORMAT_CAP(VIDEO_PIX_FMT_YUYV, 320, 240),
	{0},
};

static int emul_imager_set_fmt(const struct device *const dev, enum video_endpoint_id ep,
			       struct video_format *fmt)
{
	struct emul_imager_data *data = dev->data;

	/* For the purpose of simulation, fill the image line buffer with 50% gray, this data
	 * will be collected by the video_emul_rx driver.
	 */
	if (fmt->pixelformat == VIDEO_PIX_FMT_RGB565) {
		for (int i = 0; i < sizeof(data->linebuffer) / sizeof(uint16_t); i++) {
			((uint16_t *)data->linebuffer)[i] = sys_cpu_to_le16(0x7bef);
		}
	} else {
		memset(data->linebuffer, 0x7f, sizeof(data->linebuffer));
	}

	return video_imager_set_fmt(dev, ep, fmt);
}

static int emul_imager_set_ctrl(const struct device *dev, unsigned int cid, void *value)
{
	const struct video_imager_config *cfg = dev->config;

	switch (cid) {
	case EMUL_IMAGER_CID_CUSTOM:
		return video_write_cci_reg(&cfg->i2c, EMUL_IMAGER_REG_CUSTOM, (int)value);
	default:
		return -ENOTSUP;
	}
}

static int emul_imager_get_ctrl(const struct device *dev, unsigned int cid, void *value)
{
	const struct video_imager_config *cfg = dev->config;
	uint32_t reg = 0;
	int ret;

	switch (cid) {
	case EMUL_IMAGER_CID_CUSTOM:
		ret = video_read_cci_reg(&cfg->i2c, EMUL_IMAGER_REG_CUSTOM, &reg);
		break;
	default:
		return -ENOTSUP;
	}

	*(uint32_t *)value = reg;
	return ret;
}

static int emul_imager_set_stream(const struct device *dev, bool enable)
{
	const struct video_imager_config *cfg = dev->config;

	return video_write_cci_reg(&cfg->i2c, EMUL_IMAGER_REG_CTRL, enable ? 1 : 0);
}

static DEVICE_API(video, emul_imager_driver_api) = {
	/* Local implementation */
	.set_ctrl = emul_imager_set_ctrl,
	.get_ctrl = emul_imager_get_ctrl,
	.set_stream = emul_imager_set_stream,
	.set_format = emul_imager_set_fmt,
	/* Default implementation */
	.set_frmival = video_imager_set_frmival,
	.get_frmival = video_imager_get_frmival,
	.enum_frmival = video_imager_enum_frmival,
	.get_format = video_imager_get_fmt,
	.get_caps = video_imager_get_caps,
};

int emul_imager_init(const struct device *dev)
{
	const struct video_imager_config *cfg = dev->config;
	uint32_t sensor_id;
	int ret;

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR("Bus %s is not ready", cfg->i2c.bus->name);
		return -ENODEV;
	}

	ret = video_read_cci_reg(&cfg->i2c, EMUL_IMAGER_REG_SENSOR_ID, &sensor_id);
	if (ret < 0 || sensor_id != EMUL_IMAGER_SENSOR_ID) {
		LOG_ERR("Failed to get a correct sensor ID 0x%x", sensor_id);
		return ret;
	}

	return video_imager_init(dev, emul_imager_init_regs, 0);
}

#define EMUL_IMAGER_DEFINE(inst)                                                                   \
	static struct emul_imager_data emul_imager_data_##inst;                                    \
                                                                                                   \
	static struct video_imager_data video_imager_data_##inst;                                  \
                                                                                                   \
	static const struct video_imager_config video_imager_cfg_##inst = {                        \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.modes = emul_imager_modes,                                                        \
		.fmts = emul_imager_fmts,                                                          \
		.data = &video_imager_data_##inst,                                                 \
		.write_multi = video_write_cci_multi,                                              \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &emul_imager_init, NULL, &emul_imager_data_##inst,             \
			      &video_imager_cfg_##inst, POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY,   \
			      &emul_imager_driver_api);

DT_INST_FOREACH_STATUS_OKAY(EMUL_IMAGER_DEFINE)

/* Simulated MIPI bus */

const uint8_t *video_emul_imager_get_linebuffer(const struct device *dev, size_t *pitch)
{
	struct emul_imager_data *data = dev->data;

	*pitch = sizeof(data->linebuffer);
	return data->linebuffer;
}

/* Simulated I2C bus */

static int emul_imager_transfer_i2c(const struct emul *target, struct i2c_msg msgs[], int num_msgs,
				    int addr)
{
	static uint8_t fake_regs[UINT8_MAX] = {
		[EMUL_IMAGER_REG_SENSOR_ID & VIDEO_REG_ADDR_MASK] = EMUL_IMAGER_SENSOR_ID,
	};

	if (num_msgs == 0) {
		CODE_UNREACHABLE;
	} else if (num_msgs == 1 &&
		   msgs[0].len == 2 && (msgs[0].flags & I2C_MSG_READ) == 0) {
		/* Register write */
		fake_regs[msgs[0].buf[0]] =  msgs[0].buf[1];
	} else if (num_msgs == 2 &&
		   msgs[0].len == 1 && (msgs[0].flags & I2C_MSG_READ) == 0 &&
		   msgs[1].len == 1 && (msgs[1].flags & I2C_MSG_READ) == 0) {
		/* Register write */
		fake_regs[msgs[0].buf[0]] =  msgs[1].buf[0];
	} else if (num_msgs == 2 &&
		   msgs[0].len == 1 && (msgs[0].flags & I2C_MSG_READ) == 0 &&
		   msgs[1].len == 1 && (msgs[1].flags & I2C_MSG_READ) != 0) {
		/* Register read */
		msgs[1].buf[0] = fake_regs[msgs[0].buf[0]];
	} else {
		LOG_ERR("Unsupported I2C operation of %u messages", num_msgs);
		return -EIO;
	}

	return 0;
}

static const struct i2c_emul_api emul_imager_i2c_api = {
	.transfer = emul_imager_transfer_i2c,
};

static int emul_imager_init_i2c(const struct emul *target, const struct device *dev)
{
	return 0;
}

#define EMUL_I2C_DEFINE(inst)                                                                      \
	EMUL_DT_INST_DEFINE(inst, &emul_imager_init_i2c, NULL, NULL, &emul_imager_i2c_api, NULL);

DT_INST_FOREACH_STATUS_OKAY(EMUL_I2C_DEFINE)
