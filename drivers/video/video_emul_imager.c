/*
 * Copyright (c) 2024-2025 tinyVision.ai Inc.
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
#include "video_ctrls.h"
#include "video_device.h"

LOG_MODULE_REGISTER(video_emul_imager, CONFIG_VIDEO_LOG_LEVEL);

#define EMUL_IMAGER_REG8(addr) ((addr) | VIDEO_REG_ADDR8_DATA8)
#define EMUL_IMAGER_REG16(addr) ((addr) | VIDEO_REG_ADDR8_DATA16_BE)
#define EMUL_IMAGER_REG24(addr) ((addr) | VIDEO_REG_ADDR8_DATA24_BE)

#define EMUL_IMAGER_REG_SENSOR_ID EMUL_IMAGER_REG16(0x00)
#define EMUL_IMAGER_SENSOR_ID     0x2025
#define EMUL_IMAGER_REG_CTRL      EMUL_IMAGER_REG8(0x02)
#define EMUL_IMAGER_REG_TIMING1   EMUL_IMAGER_REG16(0x04)
#define EMUL_IMAGER_REG_TIMING2   EMUL_IMAGER_REG16(0x06)
#define EMUL_IMAGER_REG_TIMING3   EMUL_IMAGER_REG16(0x08)
#define EMUL_IMAGER_REG_CUSTOM    EMUL_IMAGER_REG24(0x10)
#define EMUL_IMAGER_REG_FORMAT    EMUL_IMAGER_REG8(0x20)

/* Custom control that is just an I2C write for example and test purpose */
#define EMUL_IMAGER_CID_CUSTOM (VIDEO_CID_PRIVATE_BASE + 0x01)

/* Helper to avoid repetition */
#define EMUL_IMAGER_REG_LIST(array) {.regs = (array), .size = ARRAY_SIZE(array)}

enum emul_imager_fmt_id {
	RGB565_320x240,
	YUYV_320x240,
};

struct emul_imager_reg_list {
	const struct video_reg *regs;
	size_t size;
};

struct emul_imager_mode {
	uint8_t fps;
	/* List of registers lists to configure the various properties of the sensor.
	 * This permits to deduplicate the list of registers in case some lare sections
	 * are repeated across modes, such as the resolution for different FPS.
	 */
	const struct emul_imager_reg_list lists[3];
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
	const struct emul_imager_mode *mode;
	enum emul_imager_fmt_id fmt_id;
	struct video_format fmt;
	struct emul_imager_ctrls ctrls;
};

/* All the I2C registers sent on various scenario */
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
static const struct video_reg emul_imager_rgb565[] = {
	{EMUL_IMAGER_REG_FORMAT, 0x01},
};
static const struct video_reg emul_imager_yuyv[] = {
	{EMUL_IMAGER_REG_FORMAT, 0x02},
};
static const struct video_reg emul_imager_320x240[] = {
	{EMUL_IMAGER_REG_TIMING1, 320},
	{EMUL_IMAGER_REG_TIMING2, 240},
};
static const struct video_reg emul_imager_160x120[] = {
	{EMUL_IMAGER_REG_TIMING1, 160},
	{EMUL_IMAGER_REG_TIMING2, 120},
};
static const struct video_reg emul_imager_15fps[] = {
	{EMUL_IMAGER_REG_TIMING3, 15},
};
static const struct video_reg emul_imager_30fps[] = {
	{EMUL_IMAGER_REG_TIMING3, 30},
};
static const struct video_reg emul_imager_60fps[] = {
	{EMUL_IMAGER_REG_TIMING3, 60},
};

/* Description of "modes", that pick lists of registesr that will be all sentto the imager */
struct emul_imager_mode emul_imager_rgb565_320x240_modes[] = {
	{
		.fps = 15,
		.lists = {
			EMUL_IMAGER_REG_LIST(emul_imager_320x240),
			EMUL_IMAGER_REG_LIST(emul_imager_rgb565),
			EMUL_IMAGER_REG_LIST(emul_imager_15fps),
		},
	},
	{
		.fps = 30,
		.lists = {
			EMUL_IMAGER_REG_LIST(emul_imager_320x240),
			EMUL_IMAGER_REG_LIST(emul_imager_rgb565),
			EMUL_IMAGER_REG_LIST(emul_imager_30fps),
		},
	},
	{
		.fps = 60,
		.lists = {
			EMUL_IMAGER_REG_LIST(emul_imager_320x240),
			EMUL_IMAGER_REG_LIST(emul_imager_rgb565),
			EMUL_IMAGER_REG_LIST(emul_imager_60fps),
		},
	},
	{0},
};

struct emul_imager_mode emul_imager_yuyv_320x240_modes[] = {
	{
		.fps = 15,
		.lists = {
			EMUL_IMAGER_REG_LIST(emul_imager_320x240),
			EMUL_IMAGER_REG_LIST(emul_imager_yuyv),
			EMUL_IMAGER_REG_LIST(emul_imager_15fps),
		}
	},
	{
		.fps = 30,
		.lists = {
			EMUL_IMAGER_REG_LIST(emul_imager_320x240),
			EMUL_IMAGER_REG_LIST(emul_imager_yuyv),
			EMUL_IMAGER_REG_LIST(emul_imager_30fps),
		}
	},
	{0},
};

/* Summary of all the modes of all the frame formats, with indexes matching those of fmts[]. */
static const struct emul_imager_mode *emul_imager_modes[] = {
	[RGB565_320x240] = emul_imager_rgb565_320x240_modes,
	[YUYV_320x240] = emul_imager_yuyv_320x240_modes,
};

/* Video device capabilities where the supported resolutions and pixel formats are listed.
 * The format ID is used as index to fetch the matching mode from the list above.
 */
#define EMUL_IMAGER_VIDEO_FORMAT_CAP(format, width, height)                                        \
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
	[RGB565_320x240] = EMUL_IMAGER_VIDEO_FORMAT_CAP(VIDEO_PIX_FMT_RGB565, 320, 240),
	[YUYV_320x240] = EMUL_IMAGER_VIDEO_FORMAT_CAP(VIDEO_PIX_FMT_YUYV, 320, 240),
	{0},
};

static int emul_imager_set_ctrl(const struct device *dev, uint32_t id)
{
	const struct emul_imager_config *cfg = dev->config;
	struct emul_imager_data *data = dev->data;

	return video_write_cci_reg(&cfg->i2c, EMUL_IMAGER_REG_CUSTOM, data->ctrls.custom.val);
}

/* Customize this function according to your "struct emul_imager_mode". */
static int emul_imager_set_mode(const struct device *dev, const struct emul_imager_mode *mode)
{
	const struct emul_imager_config *cfg = dev->config;
	struct emul_imager_data *data = dev->data;
	int ret;

	if (data->mode == mode) {
		return 0;
	}

	LOG_DBG("Applying mode %p at %d FPS", mode, mode->fps);

	/* Apply all the configuration registers for that mode */
	for (int i = 0; i < 2; i++) {
		const struct emul_imager_reg_list *list = &mode->lists[i];

		ret = video_write_cci_multiregs(&cfg->i2c, list->regs, list->size);
		if (ret < 0) {
			goto err;
		}
	}

	data->mode = mode;
	return 0;
err:
	LOG_ERR("Could not apply mode %p (%u FPS)", mode, mode->fps);
	return ret;
}

static int emul_imager_set_frmival(const struct device *dev, struct video_frmival *frmival)
{
	struct emul_imager_data *data = dev->data;
	struct video_frmival_enum fie = {.format = &data->fmt, .discrete = *frmival};

	video_closest_frmival(dev, &fie);
	LOG_DBG("Applying frame interval number %u", fie.index);
	return emul_imager_set_mode(dev, &emul_imager_modes[data->fmt_id][fie.index]);
}

static int emul_imager_get_frmival(const struct device *dev, struct video_frmival *frmival)
{
	struct emul_imager_data *data = dev->data;

	frmival->numerator = 1;
	frmival->denominator = data->mode->fps;
	return 0;
}

static int emul_imager_enum_frmival(const struct device *dev, struct video_frmival_enum *fie)
{
	const struct emul_imager_mode *mode;
	size_t fmt_id;
	int ret;

	ret = video_format_caps_index(fmts, fie->format, &fmt_id);
	if (ret < 0) {
		return ret;
	}

	mode = &emul_imager_modes[fmt_id][fie->index];

	fie->type = VIDEO_FRMIVAL_TYPE_DISCRETE;
	fie->discrete.numerator = 1;
	fie->discrete.denominator = mode->fps;

	return mode->fps == 0;
}

static int emul_imager_set_fmt(const struct device *const dev, struct video_format *fmt)
{
	struct emul_imager_data *data = dev->data;
	size_t fmt_id;
	int ret;

	if (memcmp(&data->fmt, fmt, sizeof(data->fmt)) == 0) {
		return 0;
	}

	ret = video_format_caps_index(fmts, fmt, &fmt_id);
	if (ret < 0) {
		LOG_ERR("Format %x %ux%u not found", fmt->pixelformat, fmt->width, fmt->height);
		return ret;
	}

	ret = emul_imager_set_mode(dev, &emul_imager_modes[fmt_id][0]);
	if (ret < 0) {
		return ret;
	}

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

	data->fmt_id = fmt_id;
	data->fmt = *fmt;
	return 0;
}

static int emul_imager_get_fmt(const struct device *dev, struct video_format *fmt)
{
	struct emul_imager_data *data = dev->data;

	*fmt = data->fmt;
	return 0;
}

static int emul_imager_get_caps(const struct device *dev, struct video_caps *caps)
{
	caps->format_caps = fmts;
	return 0;
}

static int emul_imager_set_stream(const struct device *dev, bool enable, enum video_buf_type type)
{
	const struct emul_imager_config *cfg = dev->config;

	return video_write_cci_reg(&cfg->i2c, EMUL_IMAGER_REG_CTRL, enable ? 1 : 0);
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
	const struct emul_imager_config *cfg = dev->config;
	struct video_format fmt;
	uint32_t sensor_id;
	int ret;

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR("Bus %s is not ready", cfg->i2c.bus->name);
		return -ENODEV;
	}

	ret = video_read_cci_reg(&cfg->i2c, EMUL_IMAGER_REG_SENSOR_ID, &sensor_id);
	if (ret < 0 || sensor_id != EMUL_IMAGER_SENSOR_ID) {
		LOG_ERR("Failed to get a correct sensor ID 0x%x",  sensor_id);
		return ret;
	}

	ret = video_write_cci_multiregs(&cfg->i2c, emul_imager_init_regs,
					ARRAY_SIZE(emul_imager_init_regs));
	if (ret < 0) {
		LOG_ERR("Could not set initial registers");
		return ret;
	}

	fmt.pixelformat = fmts[0].pixelformat;
	fmt.width = fmts[0].width_min;
	fmt.height = fmts[0].height_min;
	fmt.pitch = fmt.width * 2;

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
		/* SENSOR_ID_LO */
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
