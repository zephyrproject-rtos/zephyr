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
#define EMUL_IMAGER_REG_EXPOSURE  0x0007
#define EMUL_IMAGER_REG_GAIN      0x0008
#define EMUL_IMAGER_REG_PATTERN   0x0009
#define EMUL_IMAGER_PATTERN_OFF   0x00
#define EMUL_IMAGER_PATTERN_BARS1 0x01
#define EMUL_IMAGER_PATTERN_BARS2 0x02

/* Emulated register bank */
uint8_t emul_imager_fake_regs[10];

enum emul_imager_fmt_id {
	RGB565_64x20,
	YUYV_64x20,
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
	const struct emul_imager_reg *regs[2];
	/* More fields can be added according to the needs of the sensor driver */
};

struct emul_imager_config {
	struct i2c_dt_spec i2c;
};

struct emul_imager_data {
	/* First field is a framebuffer for I/O emulation purpose */
	uint8_t framebuffer[CONFIG_VIDEO_EMUL_IMAGER_FRAMEBUFFER_SIZE];
	/* Other fields are shared with real hardware drivers */
	const struct emul_imager_mode *mode;
	enum emul_imager_fmt_id fmt_id;
	struct video_format fmt;
};

/* Initial parameters of the sensors common to all modes. */
static const struct emul_imager_reg emul_imager_init_regs[] = {
	{EMUL_IMAGER_REG_CTRL, 0x00},
	/* Example comment about REG_INIT1 */
	{EMUL_IMAGER_REG_INIT1, 0x10},
	{EMUL_IMAGER_REG_INIT2, 0x00},
	{0},
};

/* List of registers aggregated together in "modes" that can be applied
 * to set the timing parameters and other mode-dependent configuration.
 */

static const struct emul_imager_reg emul_imager_rgb565_64x20[] = {
	{EMUL_IMAGER_REG_TIMING1, 0x64},
	{EMUL_IMAGER_REG_TIMING2, 0x20},
	{0},
};
static const struct emul_imager_reg emul_imager_rgb565_64x20_15fps[] = {
	{EMUL_IMAGER_REG_TIMING3, 15},
	{0},
};
static const struct emul_imager_reg emul_imager_rgb565_64x20_30fps[] = {
	{EMUL_IMAGER_REG_TIMING3, 30},
	{0},
};
static const struct emul_imager_reg emul_imager_rgb565_64x20_60fps[] = {
	{EMUL_IMAGER_REG_TIMING3, 60},
	{0},
};
struct emul_imager_mode emul_imager_rgb565_64x20_modes[] = {
	{.fps = 15, .regs = {emul_imager_rgb565_64x20, emul_imager_rgb565_64x20_15fps}},
	{.fps = 30, .regs = {emul_imager_rgb565_64x20, emul_imager_rgb565_64x20_30fps}},
	{.fps = 60, .regs = {emul_imager_rgb565_64x20, emul_imager_rgb565_64x20_60fps}},
	{0},
};

static const struct emul_imager_reg emul_imager_yuyv_64x20[] = {
	{EMUL_IMAGER_REG_TIMING1, 0x64},
	{EMUL_IMAGER_REG_TIMING2, 0x20},
	{0},
};
static const struct emul_imager_reg emul_imager_yuyv_64x20_15fps[] = {
	{EMUL_IMAGER_REG_TIMING3, 15},
	{0},
};
static const struct emul_imager_reg emul_imager_yuyv_64x20_30fps[] = {
	{EMUL_IMAGER_REG_TIMING3, 30},
	{0},
};
struct emul_imager_mode emul_imager_yuyv_64x20_modes[] = {
	{.fps = 15, .regs = {emul_imager_yuyv_64x20, emul_imager_yuyv_64x20_15fps}},
	{.fps = 30, .regs = {emul_imager_yuyv_64x20, emul_imager_yuyv_64x20_30fps}},
	{0},
};

/* Summary of all the modes of all the frame formats, with the format ID as
 * index, matching fmts[].
 */
static const struct emul_imager_mode *emul_imager_modes[] = {
	[RGB565_64x20] = emul_imager_rgb565_64x20_modes,
	[YUYV_64x20] = emul_imager_yuyv_64x20_modes,
};

/* Video device capabilities where the supported resolutions and pixel formats are listed.
 * The format ID is used as index to fetch the matching mode from the list above.
 */
#define EMUL_IMAGER_VIDEO_FORMAT_CAP(width, height, format)                                        \
	{                                                                                          \
		.pixelformat = (format),                                                           \
		.width_min = (width),                                                              \
		.width_max = (width),                                                              \
		.height_min = (height),                                                            \
		.height_max = (height),                                                            \
		.width_step = 0,                                                                   \
		.height_step = 0,                                                                  \
	}
static const struct video_format_cap fmts[] = {
	[RGB565_64x20] = EMUL_IMAGER_VIDEO_FORMAT_CAP(64, 20, VIDEO_PIX_FMT_RGB565),
	[YUYV_64x20] = EMUL_IMAGER_VIDEO_FORMAT_CAP(64, 20, VIDEO_PIX_FMT_YUYV),
	{0},
};

/* Emulated I2C register interface, to replace with actual I2C calls for real hardware */
static int emul_imager_read_reg(const struct device *const dev, uint8_t reg_addr, uint8_t *value)
{
	LOG_DBG("%s placeholder for I2C read from 0x%02x", dev->name, reg_addr);
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
	LOG_DBG("%s placeholder for I2C write 0x%08x to 0x%02x", dev->name, value, reg_addr);
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
	case VIDEO_CID_EXPOSURE:
		return emul_imager_write_reg(dev, EMUL_IMAGER_REG_EXPOSURE, (int)value);
	case VIDEO_CID_GAIN:
		return emul_imager_write_reg(dev, EMUL_IMAGER_REG_GAIN, (int)value);
	case VIDEO_CID_TEST_PATTERN:
		return emul_imager_write_reg(dev, EMUL_IMAGER_REG_PATTERN, (int)value);
	default:
		return -ENOTSUP;
	}
}

static int emul_imager_get_ctrl(const struct device *dev, unsigned int cid, void *value)
{
	struct emul_imager_data *data = dev->data;

	switch (cid) {
	case VIDEO_CID_EXPOSURE:
		return emul_imager_read_int(dev, EMUL_IMAGER_REG_EXPOSURE, value);
	case VIDEO_CID_GAIN:
		return emul_imager_read_int(dev, EMUL_IMAGER_REG_GAIN, value);
	case VIDEO_CID_TEST_PATTERN:
		return emul_imager_read_int(dev, EMUL_IMAGER_REG_PATTERN, value);
	case VIDEO_CID_PIXEL_RATE:
		*(int64_t *)value = (int64_t)data->fmt.width * data->fmt.pitch * data->mode->fps;
		return 0;
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
	LOG_ERR("Could not apply %s mode %p (%u FPS)", dev->name, mode, mode->fps);
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

/* White, Yellow, Cyan, Green, Magenta, Red, Blue, Black */
static const uint16_t pattern_8bars_yuv[8][3] = {
	{0xFF, 0x7F, 0x7F}, {0xFF, 0x00, 0xFF}, {0xFF, 0xFF, 0x00}, {0x7F, 0x00, 0x00},
	{0x00, 0xFF, 0xFF}, {0x00, 0x00, 0xFF}, {0x00, 0xFF, 0x00}, {0x00, 0x7F, 0x7F}};
static const uint16_t pattern_8bars_rgb[8][3] = {
	{0xFF, 0xFF, 0xFF}, {0xFF, 0xFF, 0x00}, {0x00, 0xFF, 0xFF}, {0x00, 0xFF, 0x00},
	{0xFF, 0x00, 0xFF}, {0xFF, 0x00, 0x00}, {0x00, 0x00, 0xFF}, {0x00, 0x00, 0x00}};
static void emul_imager_fill_framebuffer(const struct device *const dev, struct video_format *fmt)
{
	struct emul_imager_data *data = dev->data;
	uint16_t *fb16 = (uint16_t *)data->framebuffer;
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
		memcpy(data->framebuffer + fmt->pitch * i, data->framebuffer, fmt->pitch);
	}
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

	if (fmt->pitch * fmt->height > CONFIG_VIDEO_EMUL_IMAGER_FRAMEBUFFER_SIZE) {
		LOG_ERR("%s has %u bytes of memory, unable to support %x %ux%u (%u bytes)",
			dev->name, CONFIG_VIDEO_EMUL_IMAGER_FRAMEBUFFER_SIZE, fmt->pixelformat,
			fmt->width, fmt->height, fmt->pitch * fmt->height);
		return -ENOMEM;
	}

	if (memcmp(&data->fmt, fmt, sizeof(data->fmt)) == 0) {
		return 0;
	}

	ret = video_format_caps_index(fmts, fmt, &fmt_id);
	if (ret < 0) {
		LOG_ERR("Format %x %ux%u not found for %s", fmt->pixelformat, fmt->width,
			fmt->height, dev->name);
		return ret;
	}

	ret = emul_imager_set_mode(dev, &emul_imager_modes[fmt_id][0]);
	if (ret < 0) {
		return ret;
	}

	/* Change the image pattern on the framebuffer */
	emul_imager_fill_framebuffer(dev, fmt);

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
		LOG_ERR("Failed to get %s correct sensor ID (0x%x", dev->name, sensor_id);
		return ret;
	}

	ret = emul_imager_write_multi(dev, emul_imager_init_regs);
	if (ret < 0) {
		LOG_ERR("Could not set %s initial registers", dev->name);
		return ret;
	}

	fmt.pixelformat = fmts[0].pixelformat;
	fmt.width = fmts[0].width_min;
	fmt.height = fmts[0].height_min;
	fmt.pitch = fmt.width * 2;

	ret = emul_imager_set_fmt(dev, VIDEO_EP_OUT, &fmt);
	if (ret < 0) {
		LOG_ERR("Failed to set %s to default format %x %ux%u", dev->name, fmt.pixelformat,
			fmt.width, fmt.height);
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
