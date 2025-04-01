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

struct emul_imager_mode {
	uint8_t fps;
	/* List of registers lists to configure the various properties of the sensor.
	 * This permits to deduplicate the list of registers in case some lare sections
	 * are repeated across modes, such as the resolution for different FPS.
	 */
	const struct video_reg *regs[3];
	/* More fields can be added according to the needs of the sensor driver */
};

struct emul_imager_config {
	struct i2c_dt_spec i2c;
};

struct emul_imager_data {
	/* First field is a line buffer for I/O emulation purpose */
	uint8_t framebuffer[320 * sizeof(uint16_t)];
	/* Other fields are shared with real hardware drivers */
	const struct emul_imager_mode *mode;
	enum emul_imager_fmt_id fmt_id;
	struct video_format fmt;
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
struct emul_imager_mode emul_imager_rgb565_320x240_modes[] = {
	{.fps = 15, .regs = {emul_imager_320x240, emul_imager_rgb565, emul_imager_15fps}},
	{.fps = 30, .regs = {emul_imager_320x240, emul_imager_rgb565, emul_imager_30fps}},
	{.fps = 60, .regs = {emul_imager_320x240, emul_imager_rgb565, emul_imager_60fps}},
	{0},
};
struct emul_imager_mode emul_imager_yuyv_320x240_modes[] = {
	{.fps = 15, .regs = {emul_imager_320x240, emul_imager_yuyv, emul_imager_15fps}},
	{.fps = 30, .regs = {emul_imager_320x240, emul_imager_yuyv, emul_imager_30fps}},
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

static int emul_imager_set_ctrl(const struct device *dev, unsigned int cid, void *value)
{
	const struct emul_imager_config *cfg = dev->config;

	switch (cid) {
	case EMUL_IMAGER_CID_CUSTOM:
		return video_write_cci_reg(&cfg->i2c, EMUL_IMAGER_REG_CUSTOM, (int)value);
	default:
		return -ENOTSUP;
	}
}

static int emul_imager_get_ctrl(const struct device *dev, unsigned int cid, void *value)
{
	const struct emul_imager_config *cfg = dev->config;
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
		ret = video_write_cci_multi(&cfg->i2c, mode->regs[i]);
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

static int emul_imager_set_frmival(const struct device *dev, enum video_endpoint_id ep,
				   struct video_frmival *frmival)
{
	struct emul_imager_data *data = dev->data;
	struct video_frmival_enum fie = {.format = &data->fmt, .discrete = *frmival};

	if (ep != VIDEO_EP_OUT && ep != VIDEO_EP_ALL) {
		return -EINVAL;
	}

	video_closest_frmival(dev, ep, &fie);
	LOG_DBG("Applying frame interval number %u", fie.index);
	return emul_imager_set_mode(dev, &emul_imager_modes[data->fmt_id][fie.index]);
}

static int emul_imager_get_frmival(const struct device *dev, enum video_endpoint_id ep,
				   struct video_frmival *frmival)
{
	struct emul_imager_data *data = dev->data;

	if (ep != VIDEO_EP_OUT && ep != VIDEO_EP_ALL) {
		return -EINVAL;
	}

	frmival->numerator = 1;
	frmival->denominator = data->mode->fps;
	return 0;
}

static int emul_imager_enum_frmival(const struct device *dev, enum video_endpoint_id ep,
				    struct video_frmival_enum *fie)
{
	const struct emul_imager_mode *mode;
	size_t fmt_id;
	int ret;

	if (ep != VIDEO_EP_OUT && ep != VIDEO_EP_ALL) {
		return -EINVAL;
	}

	ret = video_format_caps_index(fmts, fie->format, &fmt_id);
	if (ret < 0) {
		return ret;
	}

	mode = &emul_imager_modes[fmt_id][fie->index];

	fie->type = VIDEO_FRMIVAL_TYPE_DISCRETE;
	fie->discrete.numerator = 1;
	fie->discrete.denominator = mode->fps;
	fie->index++;

	return mode->fps == 0;
}

static int emul_imager_set_fmt(const struct device *const dev, enum video_endpoint_id ep,
			       struct video_format *fmt)
{
	struct emul_imager_data *data = dev->data;
	size_t fmt_id;
	int ret;

	if (ep != VIDEO_EP_OUT && ep != VIDEO_EP_ALL) {
		return -EINVAL;
	}

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

static int emul_imager_get_fmt(const struct device *dev, enum video_endpoint_id ep,
			       struct video_format *fmt)
{
	struct emul_imager_data *data = dev->data;

	if (ep != VIDEO_EP_OUT && ep != VIDEO_EP_ALL) {
		return -EINVAL;
	}

	*fmt = data->fmt;
	return 0;
}

static int emul_imager_get_caps(const struct device *dev, enum video_endpoint_id ep,
				struct video_caps *caps)
{
	if (ep != VIDEO_EP_OUT && ep != VIDEO_EP_ALL) {
		return -EINVAL;
	}

	caps->format_caps = fmts;
	return 0;
}

static int emul_imager_set_stream(const struct device *dev, bool enable)
{
	const struct emul_imager_config *cfg = dev->config;

	return video_write_cci_reg(&cfg->i2c, EMUL_IMAGER_REG_CTRL, enable ? 1 : 0);
}

static DEVICE_API(video, emul_imager_driver_api) = {
	.set_ctrl = emul_imager_set_ctrl,
	.get_ctrl = emul_imager_get_ctrl,
	.set_frmival = emul_imager_set_frmival,
	.get_frmival = emul_imager_get_frmival,
	.enum_frmival = emul_imager_enum_frmival,
	.set_format = emul_imager_set_fmt,
	.get_format = emul_imager_get_fmt,
	.get_caps = emul_imager_get_caps,
	.set_stream = emul_imager_set_stream,
};

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

	ret = video_write_cci_multi(&cfg->i2c, emul_imager_init_regs);
	if (ret < 0) {
		LOG_ERR("Could not set initial registers");
		return ret;
	}

	fmt.pixelformat = fmts[0].pixelformat;
	fmt.width = fmts[0].width_min;
	fmt.height = fmts[0].height_min;
	fmt.pitch = fmt.width * 2;

	ret = emul_imager_set_fmt(dev, VIDEO_EP_OUT, &fmt);
	if (ret < 0) {
		LOG_ERR("Failed to set to default format %x %ux%u",
			fmt.pixelformat, fmt.width, fmt.height);
	}

	return 0;
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
			      &emul_imager_driver_api);

DT_INST_FOREACH_STATUS_OKAY(EMUL_IMAGER_DEFINE)

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
