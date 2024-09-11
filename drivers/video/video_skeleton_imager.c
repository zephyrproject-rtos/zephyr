/*
 * Copyright (c) 2024 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* To use this template efficiently:
 * - replace the copyright information with your own
 * - replace "vendorname", "sensorname", "SENSORNAME" by names matching your sensor
 * - replace the registers (..._REG..., sensor ID) by those matching your sensor
 * - replace the mode and format capability tables by those matching your sensor
 * - adjust the struct definitions, and the functions accordingly
 * - delete the irrelevant comments
 */

#define DT_DRV_COMPAT vendorname_sensorname

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(video_sensorname, CONFIG_VIDEO_LOG_LEVEL);

/* Best to use the exact same name as in the datasheets */
#define SENSORNAME_REG0 0x0000
#define SENSORNAME_REG1 0x0001
#define SENSORNAME_REG2 0x0002
#define SENSORNAME_REG3 0x0003
#define SENSORNAME_REG4 0x0004
#define SENSORNAME_REG5 0x0005
#define SENSORNAME_REG6 0x0006
#define SENSORNAME_REG7 0x0007
#define SENSORNAME_REG8 0x0008
#define SENSORNAME_REG9 0x0008

#define SENSORNAME_REG_SENSOR_ID 0x99
#define SENSORNAME_SENSOR_ID 0x99

enum sensorname_fmt_id {
	BGGR8_640x480,
	RGB565_640x480,
	RGB565_1280x720,
};

struct sensorname_reg {
	uint16_t addr;
	uint8_t value;
};

struct sensorname_mode {
	uint8_t fps;
	/* List of registers lists to configure the various properties of the sensor.
	 * This permits to deduplicate the list of registers in case some lare sections
	 * are repeated across modes, such as the resolution for different FPS.
	 */
	struct sensorname_reg regs[2];
	/* More fields can be added according to the needs of the sensor driver */
};

struct sensorname_config {
	struct i2c_dt_spec i2c;
};

struct sensorname_data {
	struct sensorname_mode *mode;
	enum sensorname_fmt_id fmt_id;
	struct video_format fmt;
};

/* Initial parameters of the sensors common to all modes. */
static const struct sensorname_reg sensorname_init_regs[] = {
	{SENSORNAME_REG0, 0x00},
	{SENSORNAME_REG1, 0x01},
	/* Example comment about REG2 */
	{SENSORNAME_REG2, 0x10},
	{SENSORNAME_REG3, 0x00},
	{0},
};

/* List of registers aggregated together in "modes" that can be applied
 * to set the timing parameters and other mode-dependent configuration.
 */

static const struct sensorname_reg sensorname_bggr8_640x480[] = {
	{SENSORNAME_REG4, 0x64},
	{SENSORNAME_REG5, 0x48},
	{0},
};
static const struct sensorname_reg sensorname_bggr8_640x480_15fps[] = {
	{SENSORNAME_REG6, 15},
	{0},
};
static const struct sensorname_reg sensorname_bggr8_640x480_30fps[] = {
	{SENSORNAME_REG6, 30},
	{0},
};
struct sensorname_mode sensorname_bggr8_640x480_modes[] = {
	{.fps = 15, .regs = {sensorname_bggr8_640x480, sensorname_bggr8_640x480_15fps}},
	{.fps = 30, .regs = {sensorname_bggr8_640x480, sensorname_bggr8_640x480_30fps}},
	{0},
};

static const struct sensorname_reg sensorname_rgb565_640x480[] = {
	{SENSORNAME_REG4, 0x64},
	{SENSORNAME_REG5, 0x48},
	{0},
};
static const struct sensorname_reg sensorname_rgb565_640x480_15fps[] = {
	{SENSORNAME_REG6, 15},
	{0},
};
static const struct sensorname_reg sensorname_rgb565_640x480_30fps[] = {
	{SENSORNAME_REG6, 30},
	{0},
};
static const struct sensorname_reg sensorname_rgb565_640x480_60fps[] = {
	{SENSORNAME_REG6, 60},
	{0},
};
struct sensorname_mode sensorname_rgb565_640x480_modes[] = {
	{.fps = 15, .regs = {sensorname_rgb565_640x480, sensorname_rgb565_640x480_15fps}},
	{.fps = 30, .regs = {sensorname_rgb565_640x480, sensorname_rgb565_640x480_30fps}},
	{.fps = 60, .regs = {sensorname_rgb565_640x480, sensorname_rgb565_640x480_60fps}},
	{0},
};

static const struct sensorname_reg sensorname_rgb565_1280x720[] = {
	{SENSORNAME_REG4, 0x12},
	{SENSORNAME_REG5, 0x72},
	{0},
};
static const struct sensorname_reg sensorname_rgb565_1280x720_15fps[] = {
	{SENSORNAME_REG6, 15},
	{0},
};
static const struct sensorname_reg sensorname_rgb565_1280x720_30fps[] = {
	{SENSORNAME_REG6, 30},
	{0},
};
struct sensorname_mode sensorname_rgb565_1280x720_modes[] = {
	{.fps = 15, .regs = {sensorname_rgb565_1280x720, sensorname_rgb565_1280x720_15fps}},
	{.fps = 30, .regs = {sensorname_rgb565_1280x720, sensorname_rgb565_1280x720_30fps}},
	{0},
};

/* Summary of all the modes of all the frame formats, with the format ID as
 * index, matching fmts[].
 */
static const struct sensorname_mode *sensorname_modes[] = {
	[BGGR8_640x480] = sensorname_mode sensorname_bggr8_640x480_modes,
	[RGB565_640x480] = sensorname_mode sensorname_rgb565_640x480_modes,
	[RGB565_1280x720] = sensorname_reg sensorname_rgb565_1280x720_modes,
};

/* Video device capabilities where the supported resolutions and pixel formats are listed.
 * The format ID is used as index to fetch the matching mode from the list above.
 */
#define SENSORNAME_VIDEO_FORMAT_CAP(width, height, format)                                         \
	{                                                                                          \
		.pixelformat = (format), .width_min = (width), .width_max = (width),               \
		.height_min = (height), .height_max = (height), .width_step = 0, .height_step = 0, \
	}
static const struct video_format_cap fmts[] = {
	[BGGR8_640x480] = SENSORNAME_VIDEO_FORMAT_CAP(640, 480, VIDEO_PIX_FMT_BGGR8),
	[RGB565_640x480] = SENSORNAME_VIDEO_FORMAT_CAP(640, 480, VIDEO_PIX_FMT_RGB565),
	[RGB565_1280x720] = SENSORNAME_VIDEO_FORMAT_CAP(1280, 720, VIDEO_PIX_FMT_RGB565),
	{0},
};

/* Some sensors will need reg8 or reg16 variants. */
static int sensorname_read_reg(const struct device *const dev, uint8_t reg_addr, uint8_t *value)
{
	const struct sensorname_config *conf = dev->config;
	int ret;

	ret = i2c_reg_read_byte_dt(conf->i2c, reg_addr, value);
	if (ret < 0) {
		LOG_ERR("Failed to read from %s register 0x%x", dev->name, reg_addr);
		return ret;
	}
	return 0;
}

/* Some sensors will need reg8 or reg16 variants. */
static int sensorname_write_reg(const struct device *const dev, uint8_t reg_addr, uint8_t value)
{
	const struct sensorname_config *conf = dev->config;
	int ret;

	ret = i2c_reg_write_byte_dt(conf->i2c, reg_addr, value);
	if (ret < 0) {
		LOG_ERR("Failed to write 0x%x to %s register 0x%x", value, dev->name, reg_addr);
		return ret;
	}
	return 0;
}

static int sensorname_write_multi(const struct device *const dev, struct sensorname_reg *regs)
{
	int ret;

	for (int i = 0; regs[i].addr != 0; i++) {
		ret = sensorname_write_reg(dev, regs[i].addr, regs[i].value);
		if (ret < 0) {
			return ret;
		}
	}
	return 0;
}

static int sensorname_set_ctrl(const struct device *dev, unsigned int cid, void *value)
{
	switch (cid) {
	case VIDEO_CID_EXPOSURE:
		return sensorname_write_reg(dev, SENSORNAME_REG7, (uint32_t)value);
	case VIDEO_CID_GAIN:
		return sensorname_write_reg(dev, SENSORNAME_REG8, (uint32_t)value);
	case VIDEO_CID_TEST_PATTERN:
		return sensorname_write_reg(dev, SENSORNAME_REG9, (uint32_t)value);
	default:
		return -ENOTSUP;
	}
}

static int sensorname_get_ctrl(const struct device *dev, unsigned int cid, void *value)
{
	struct sensorname_data *data = dev->data;
	struct sensorname_mode *mode = data->mode;
	struct video_format *fmt = &data->fmt;

	switch (cid) {
	case VIDEO_CID_EXPOSURE:
		return sensorname_read_reg(dev, SENSORNAME_REG7, value);
	case VIDEO_CID_GAIN:
		return sensorname_read_reg(dev, SENSORNAME_REG8, value);
	case VIDEO_CID_TEST_PATTERN:
		return sensorname_read_reg(dev, SENSORNAME_REG9, value);
	case VIDEO_CID_PIXEL_RATE:
		*(uint64_t *)value = (uint64_t)mode->fps * fmt->height * fmt->width;
		return 0;
	default:
		return -ENOTSUP;
	}
}

/* Customize this function according to your "struct sensorname_mode". */
static int sensorname_set_mode(const struct device *dev, struct sensorname_mode *mode)
{
	struct sensorname_data *data = dev->data;
	int ret;

	if (data->mode == mode) {
		return 0;
	}

	/* Apply all the configuration registers for that mode */
	for (int i = 0; i < 2; i++) {
		ret = sensorname_write_multi(dev, mode->regs[i]);
		if (ret < 0) {
			goto err;
		}
	}

	data->mode = mode;
	return 0;
err:
	LOG_ERR("Could not apply %s mode %p (%u FPS)", dev->name, mode, mode->fps);
	return ret;
}

static int sensorname_set_frmival(const struct device *dev, enum video_endpoint_id ep,
				  struct video_frmival *frmival)
{
	struct sensorname_data *data = dev->data;
	struct video_frmival_enum fie = {.format = &data->fmt};
	int ret;

	if (ep != VIDEO_EP_OUT && ep != VIDEO_EP_ALL) {
		return -EINVAL;
	}

	video_get_closest_frmival(dev, ep, frmival, &fie);
	return sensorname_set_mode(dev, &sensorname_modes[data->fmt_id][fie.index]);
}

static int sensorname_get_frmival(const struct device *dev, enum video_endpoint_id ep,
				  struct video_frmival *frmival)
{
	struct sensorname_data *data = dev->data;

	if (ep != VIDEO_EP_OUT && ep != VIDEO_EP_ALL) {
		return -EINVAL;
	}

	frmival->numerator = 1;
	frmival->denominator = data->mode->fps;
	return 0;
}

static int sensorname_enum_frmival(const struct device *dev, enum video_endpoint_id ep,
				   struct video_frmival_enum *fie)
{
	struct sensorname_mode *mode;
	enum sensorname_fmt_id fmt_id;
	int ret;

	if (ep != VIDEO_EP_OUT && ep != VIDEO_EP_ALL) {
		return -EINVAL;
	}

	ret = video_get_format_index(fmts, ep, fie->fmt, &fmt_id);
	if (ret < 0) {
		return ret;
	}

	mode = &sensorname_modes[fmt_id][fie->index];

	fie->type = VIDEO_FRMIVAL_TYPE_DISCRETE;
	fie->discrete.numerator = 1;
	fie->discrete.denominator = mode->fps;
	fie->index++;

	return mode->fps == 0;
}

static int sensorname_set_fmt(const struct device *const dev, enum video_endpoint_id ep,
			      struct video_format *fmt)
{
	struct sensorname_data *data = dev->data;
	enum sensorname_fmt_id fmt_id;
	int ret;

	if (ep != VIDEO_EP_OUT && ep != VIDEO_EP_ALL) {
		return -EINVAL;
	}

	if (memcmp(&data->fmt, fmt, sizeof(data->fmt)) == 0) {
		return 0;
	}

	ret = video_get_format_index(fmts, fmt, &fmt_id);
	if (ret < 0) {
		LOG_ERR("Format " PRIvfmt " not found for %s", PRIvfmt_arg(fmt), dev->name);
		return ret;
	}

	ret = sensorname_set_mode(dev, &sensorname_modes[fmt_id][0]);
	if (ret < 0) {
		return ret;
	}

	data->fmt_id = fmt_id;
	data->fmt = *fmt;
	return 0;
}

static int sensorname_get_fmt(const struct device *dev, enum video_endpoint_id ep,
			      struct video_format *fmt)
{
	struct sensorname_data *data = dev->data;

	if (ep != VIDEO_EP_OUT && ep != VIDEO_EP_ALL) {
		return -EINVAL;
	}

	*fmt = data->fmt;
	return 0;
}

static int sensorname_get_caps(const struct device *dev, enum video_endpoint_id ep,
			       struct video_caps *caps)
{
	if (ep != VIDEO_EP_OUT && ep != VIDEO_EP_ALL) {
		return -EINVAL;
	}

	caps->format_caps = fmts;
	return 0;
}

static int sensorname_stream_start(const struct device *dev)
{
	return sensorname_write_reg(dev, SENSORNAME_REG0, 1);
}

static int sensorname_stream_stop(const struct device *dev)
{
	return sensorname_write_reg(dev, SENSORNAME_REG0, 0);
}

static const struct video_driver_api sensorname_driver_api = {
	.set_ctrl = sensorname_set_ctrl,
	.get_ctrl = sensorname_get_ctrl,
	.set_frmival = sensorname_set_frmival,
	.get_frmival = sensorname_get_frmival,
	.enum_frmival = sensorname_enum_frmival,
	.set_format = sensorname_set_fmt,
	.get_format = sensorname_get_fmt,
	.get_caps = sensorname_get_caps,
	.stream_start = sensorname_stream_start,
	.stream_stop = sensorname_stream_stop,
};

int sensorname_init(const struct device *dev)
{
	struct sensorname_data *data = dev->data;
	struct video_format fmt;
	uint8_t sensor_id;
	int ret;

	if (!i2c_is_ready_dt(&conf->i2c)) {
		LOG_ERR("Bus %s is not ready", conf->i2c.bus->name);
		return -ENODEV;
	}

	ret = sensorname_read_reg(dev, SENSORNAME_REG_SENSOR_ID);
	if (ret < 0 || sensor_id != SENSORNAME_SENSOR_ID) {
		LOG_ERR("Failed to get %s correct sensor ID (0x%x", dev->name, sensor_id);
		return ret;
	}

	ret = sensorname_write_multi(dev, &sensorname_init_regs);
	if (ret < 0) {
		LOG_ERR("Could not set %s initial registers", dev->name);
		return ret;
	}

	fmt.pixelformat = VIDEO_PIX_FMT_RGB565;
	fmt.width = 1280;
	fmt.height = 720;
	fmt.pitch = fmt.width * 2;

	return sensorname_set_fmt(dev, &fmt);
}

#define SENSORNAME_DEFINE(inst)                                                                    \
	static struct sensorname_data sensorname_data_##inst;                                      \
                                                                                                   \
	static const struct sensorname_config sensorname_conf_##inst = {                           \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &sensorname_init, NULL, &sensorname_data_##inst,               \
			      &sensorname_conf_##inst, POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY,    \
			      &sensorname_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SENSORNAME_DEFINE)
