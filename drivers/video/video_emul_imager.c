/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 tinyVision.ai Inc.
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
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
#include "video_ctrls.h"
#include "video_device.h"

LOG_MODULE_REGISTER(video_emul_imager, CONFIG_VIDEO_LOG_LEVEL);

#define EMUL_IMAGER_REG8(addr) ((addr) | VIDEO_REG_ADDR8_DATA8)
#define EMUL_IMAGER_REG16(addr) ((addr) | VIDEO_REG_ADDR8_DATA16_LE)

#define EMUL_IMAGER_CCI_SENSOR_ID EMUL_IMAGER_REG16(0x00)
#define EMUL_IMAGER_SENSOR_ID     0x2025
#define EMUL_IMAGER_CCI_CTRL      EMUL_IMAGER_REG8(0x02)
#define EMUL_IMAGER_CCI_TIMING1   EMUL_IMAGER_REG16(0x04)
#define EMUL_IMAGER_CCI_TIMING2   EMUL_IMAGER_REG16(0x06)
#define EMUL_IMAGER_CCI_TIMING3   EMUL_IMAGER_REG16(0x08)
#define EMUL_IMAGER_CCI_CUSTOM    EMUL_IMAGER_REG16(0x10)
#define EMUL_IMAGER_CCI_FORMAT    EMUL_IMAGER_REG8(0x20)

/* Custom control that is just an I2C write for example and test purpose */
#define EMUL_IMAGER_CID_CUSTOM (VIDEO_CID_PRIVATE_BASE + 0x01)

enum {
	EMUL_IMAGER_320x240_RGB565,
	EMUL_IMAGER_320x240_YUYV,
	EMUL_IMAGER_160x120_RGB565,
	EMUL_IMAGER_160x120_YUYV,
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

static const struct video_reg8 emul_imager_init_regs[] = {
	/* Undocumented registers from the vendor */
	{0xe3, 0x60},
	{0x9e, 0xd3},
	{0x05, 0x5d},
	{0x39, 0xf7},
	{0xf0, 0xef},
	{0xe8, 0x40},
	{0x6d, 0x16},
	{0x16, 0x33},
	{0xeb, 0xa7},
	{0xb8, 0x1f},
	{0x45, 0xb7},
	{0x26, 0x3a},
	{0x6e, 0x32},
	{0x1b, 0x9e},
	{0xd3, 0xf7},
	{0xd3, 0xb3},
	{0x7b, 0x64},
	{0xa3, 0xaf},
	{0x3c, 0x6e},
	{0x11, 0x2d},
	{0x15, 0x67},
	{0xb9, 0xc8},
	{0x12, 0xc8},
	{0xa6, 0x31},
	{0x0e, 0x7c},
	{0x7b, 0x64},
	{0xf8, 0x5f},
	{0x44, 0x27},
	{0xc5, 0x9a},
	{0x8d, 0x54},
};

static const struct video_reg emul_imager_320x240_rgb565[] = {
	{EMUL_IMAGER_CCI_FORMAT, 0x01},
	{EMUL_IMAGER_CCI_TIMING1, 320},
	{EMUL_IMAGER_CCI_TIMING2, 240},
};

static const struct video_reg emul_imager_320x240_yuyv[] = {
	{EMUL_IMAGER_CCI_FORMAT, 0x02},
	{EMUL_IMAGER_CCI_TIMING1, 320},
	{EMUL_IMAGER_CCI_TIMING2, 240},
};

static const struct video_reg emul_imager_160x120_rgb565[] = {
	{EMUL_IMAGER_CCI_FORMAT, 0x01},
	{EMUL_IMAGER_CCI_TIMING1, 160},
	{EMUL_IMAGER_CCI_TIMING2, 120},
};

static const struct video_reg emul_imager_160x120_yuyv[] = {
	{EMUL_IMAGER_CCI_FORMAT, 0x02},
	{EMUL_IMAGER_CCI_TIMING1, 160},
	{EMUL_IMAGER_CCI_TIMING2, 120},
};

static const struct video_reg emul_imager_60fps[] = {
	{EMUL_IMAGER_CCI_TIMING3, 60},
};

static const struct video_reg emul_imager_30fps[] = {
	{EMUL_IMAGER_CCI_TIMING3, 30},
};

static const struct video_reg emul_imager_15fps[] = {
	{EMUL_IMAGER_CCI_TIMING3, 15},
};

#define EMUL_IMAGER_CAP(format, width, height)                                                     \
	{                                                                                          \
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

static int emul_imager_get_caps(const struct device *dev, struct video_caps *caps)
{
	caps->format_caps = fmts;
	return 0;
}

static int emul_imager_set_ctrl(const struct device *dev, unsigned int cid)
{
	const struct emul_imager_config *cfg = dev->config;
	struct emul_imager_data *data = dev->data;

	switch (cid) {
	case EMUL_IMAGER_CID_CUSTOM:
		return video_write_cci_reg(&cfg->i2c, EMUL_IMAGER_CCI_CUSTOM,
					   data->ctrls.custom.val);
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
		ret = video_write_cci_multiregs(&cfg->i2c, emul_imager_320x240_rgb565,
						ARRAY_SIZE(emul_imager_320x240_rgb565));
		break;
	case EMUL_IMAGER_320x240_YUYV:
		ret = video_write_cci_multiregs(&cfg->i2c, emul_imager_320x240_yuyv,
						ARRAY_SIZE(emul_imager_320x240_yuyv));
		break;
	case EMUL_IMAGER_160x120_RGB565:
		ret = video_write_cci_multiregs(&cfg->i2c, emul_imager_160x120_rgb565,
						ARRAY_SIZE(emul_imager_160x120_rgb565));
		break;
	case EMUL_IMAGER_160x120_YUYV:
		ret = video_write_cci_multiregs(&cfg->i2c, emul_imager_160x120_yuyv,
						ARRAY_SIZE(emul_imager_160x120_yuyv));
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
	const struct emul_imager_config *cfg = dev->config;
	struct video_frmival_enum fie = {.format = &data->fmt, .discrete = *frmival};
	int ret;

	switch (fie.index) {
	case EMUL_IMAGER_60FPS_IDX:
		ret = video_write_cci_multiregs(&cfg->i2c, emul_imager_60fps,
						ARRAY_SIZE(emul_imager_60fps));
		break;
	case EMUL_IMAGER_30FPS_IDX:
		ret = video_write_cci_multiregs(&cfg->i2c, emul_imager_30fps,
						ARRAY_SIZE(emul_imager_30fps));
		break;
	case EMUL_IMAGER_15FPS_IDX:
		ret = video_write_cci_multiregs(&cfg->i2c, emul_imager_15fps,
						ARRAY_SIZE(emul_imager_15fps));
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
	const struct emul_imager_config *cfg = dev->config;

	return video_write_cci_reg(&cfg->i2c, EMUL_IMAGER_CCI_CTRL, enable ? 1 : 0);
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
	struct emul_imager_data *data = dev->data;
	struct emul_imager_ctrls *ctrls = &data->ctrls;

	return video_init_ctrl(
		&ctrls->custom, dev, EMUL_IMAGER_CID_CUSTOM,
		(struct video_ctrl_range){.min = 0, .max = 255, .step = 1, .def = 128});
}

int emul_imager_init(const struct device *dev)
{
	const struct emul_imager_config *cfg = dev->config;
	struct video_format fmt = {
		.pixelformat = fmts[0].pixelformat,
		.width = fmts[0].width_min,
		.height = fmts[0].height_min,
		.pitch = fmt.width * 2,
	};
	uint32_t sensor_id;
	int ret;

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR("Bus %s is not ready", cfg->i2c.bus->name);
		return -ENODEV;
	}

	ret = video_read_cci_reg(&cfg->i2c, EMUL_IMAGER_CCI_SENSOR_ID, &sensor_id);
	if (ret < 0 || sensor_id != EMUL_IMAGER_SENSOR_ID) {
		LOG_ERR("Failed to get a correct sensor ID: 0x%x",  sensor_id);
		return ret;
	}

	ret = video_write_cci_multiregs8(&cfg->i2c, emul_imager_init_regs,
					 ARRAY_SIZE(emul_imager_init_regs));
	if (ret < 0) {
		LOG_ERR("Could not set initial registers");
		return ret;
	}

	ret = emul_imager_set_fmt(dev, &fmt);
	if (ret < 0) {
		LOG_ERR("Failed to set to default format %x %ux%u",
			fmt.pixelformat, fmt.width, fmt.height);
	}

	return emul_imager_init_controls(dev);
}

#define EMUL_IMAGER_DEFINE(inst)                                                                   \
	static struct emul_imager_data emul_imager_data_##inst;                                    \
                                                                                                   \
	static const struct emul_imager_config emul_imager_cfg_##inst = {                          \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &emul_imager_init, NULL, &emul_imager_data_##inst,             \
			      &emul_imager_cfg_##inst, POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY,    \
			      &emul_imager_driver_api);                                            \
                                                                                                   \
	VIDEO_DEVICE_DEFINE(emul_imager_##inst, DEVICE_DT_INST_GET(inst), NULL);

DT_INST_FOREACH_STATUS_OKAY(EMUL_IMAGER_DEFINE)

/* Simulated I2C bus */

static int emul_imager_transfer_i2c(const struct emul *target, struct i2c_msg msgs[], int num_msgs,
				    int addr)
{
	static uint8_t fake_regs[UINT8_MAX] = {
		/* SENSOR_ID */
		[0x00] = EMUL_IMAGER_SENSOR_ID & 0xff,
		[0x01] = EMUL_IMAGER_SENSOR_ID >> 8,
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
