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

enum emul_imager_fmt_id {
	RGB565_320x240,
	YUYV_320x240,
};

struct emul_imager_reg {
	uint16_t addr;
	uint8_t value;
};

struct emul_imager_mode {
	uint8_t fps;
	/* List of registers lists to configure the various properties of the sensor.
	 * This permits to deduplicate the list of registers in case some lare sections
	 * are repeated across modes, such as the resolution for different FPS.
	 */
	const struct emul_imager_reg *regs[3];
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
static const struct emul_imager_reg emul_imager_rgb565[] = {
	{EMUL_IMAGER_REG_FORMAT, 0x01},
	{0},
};
static const struct emul_imager_reg emul_imager_yuyv[] = {
	{EMUL_IMAGER_REG_FORMAT, 0x02},
	{0},
};
static const struct emul_imager_reg emul_imager_320x240[] = {
	{EMUL_IMAGER_REG_TIMING1, 0x32},
	{EMUL_IMAGER_REG_TIMING2, 0x24},
	{0},
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

/* Helper to read a full integer directly from a register */
static int emul_imager_read_int(const struct device *const dev, uint8_t reg_addr, int *value)
{
	uint8_t val8;
	int ret;

	ret = emul_imager_read_reg(dev, reg_addr, &val8);
	*value = val8;
	return ret;
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

static int emul_imager_set_ctrl(const struct device *dev, unsigned int cid, void *value)
{
	switch (cid) {
	case EMUL_IMAGER_CID_CUSTOM:
		return emul_imager_write_reg(dev, EMUL_IMAGER_REG_CUSTOM, (int)value);
	default:
		return -ENOTSUP;
	}
}

static int emul_imager_get_ctrl(const struct device *dev, unsigned int cid, void *value)
{
	switch (cid) {
	case EMUL_IMAGER_CID_CUSTOM:
		return emul_imager_read_int(dev, EMUL_IMAGER_REG_CUSTOM, value);
	default:
		return -ENOTSUP;
	}
}

/* Customize this function according to your "struct emul_imager_mode". */
static int emul_imager_set_mode(const struct device *dev, const struct emul_imager_mode *mode)
{
	struct emul_imager_data *data = dev->data;
	int ret;

	if (data->mode == mode) {
		return 0;
	}

	LOG_DBG("Applying mode %p at %d FPS", mode, mode->fps);

	/* Apply all the configuration registers for that mode */
	for (int i = 0; i < 2; i++) {
		ret = emul_imager_write_multi(dev, mode->regs[i]);
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
	return emul_imager_write_reg(dev, EMUL_IMAGER_REG_CTRL, enable ? 1 : 0);
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
	struct video_format fmt;
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
		.i2c = /* I2C_DT_SPEC_INST_GET(inst) */ {0},                                       \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &emul_imager_init, NULL, &emul_imager_data_##inst,             \
			      &emul_imager_cfg_##inst, POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY,    \
			      &emul_imager_driver_api);

DT_INST_FOREACH_STATUS_OKAY(EMUL_IMAGER_DEFINE)
