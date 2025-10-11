/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2025 Silicon Signals Pvt. Ltd.
 *
 * Author: Hardevsinh Palaniya <hardevsinh.palaniya@siliconsignals.io>
 * Author: Rutvij Trivedi <rutvij.trivedi@siliconsignals.io>
 */

#define DT_DRV_COMPAT ovti_ov5642

#include <zephyr/kernel.h>
#include <math.h>
#include <stdlib.h>
#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video-controls.h>
#include <zephyr/dt-bindings/video/video-interfaces.h>

#include "video_ctrls.h"
#include "video_device.h"

LOG_MODULE_REGISTER(video_ov5642, CONFIG_VIDEO_LOG_LEVEL);

#define CHIP_ID_REG 0x300a
#define CHIP_ID_VAL 0x5642

#define SYS_CTRL0_REG     0x3008
#define SYS_CTRL0_SW_PWDN 0x80
#define SYS_CTRL0_SW_PWUP 0x02
#define SYS_CTRL0_SW_RST  0x82

#define IO_MIPI_CTRL00_REG   0x300e
#define PCLK_CLOCK_SEL_REG   0x3103
#define ISP_CTRL00_REG       0x5000
#define ISP_CTRL01_REG       0x5001
#define TIMING_TC_REG18_REG  0x3818

#define AEC_PK_MANUAL    0x3503
#define REAL_GAIN	 0x52a6

#define SDE_CTRL0_REG  0x5580
#define SDE_CTRL1_REG  0x5581
#define SDE_CTRL2_REG  0x5582
#define SDE_CTRL3_REG  0x5583
#define SDE_CTRL4_REG  0x5584
#define SDE_CTRL5_REG  0x5585
#define SDE_CTRL6_REG  0x5586
#define SDE_CTRL7_REG  0x5587
#define SDE_CTRL8_REG  0x5588
#define SDE_CTRL9_REG  0x5589
#define SDE_CTRL10_REG 0x558a

#define PI 3.141592654

#define ABS(a, b) (a > b ? a - b : b - a)

enum ov5642_frame_rate {
	OV5642_15_FPS = 15,
	OV5642_30_FPS = 30,
};

struct ov5642_config {
	struct i2c_dt_spec i2c;
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(powerdown_gpios)
	struct gpio_dt_spec powerdown_gpio;
#endif
	int bus_type;
};

struct ov5642_reg {
	uint16_t addr;
	uint8_t val;
};

struct ov5642_mode_config {
	uint16_t width;
	uint16_t height;
	uint16_t array_size_res_params;
	const struct ov5642_reg *res_params;
	uint16_t max_frmrate;
	uint16_t def_frmrate;
};

struct ov5642_ctrls {
	struct video_ctrl auto_gain;
	struct video_ctrl brightness;
	struct video_ctrl contrast;
	struct video_ctrl hue;
	struct video_ctrl saturation;
	struct video_ctrl hflip;
	struct video_ctrl vflip;
	struct video_ctrl ae;
	struct video_ctrl awb;
};

struct ov5642_data {
	struct ov5642_ctrls ctrls;
	struct video_format fmt;
	uint16_t cur_frmrate;
	const struct ov5642_mode_config *cur_mode;
};

static const struct ov5642_reg init_params_common[] = {
	{SYS_CTRL0_REG, SYS_CTRL0_SW_PWDN},
	{PCLK_CLOCK_SEL_REG, 0x93},
	{SYS_CTRL0_REG, SYS_CTRL0_SW_PWUP},
	{0x3017, 0x7f},
	{0x3018, 0xf0},
	{0x3615, 0xf0},
	{0x3000, 0xF8},
	{0x3001, 0x48},
	{0x3002, 0x5c},
	{0x3003, 0x02},
	{0x3005, 0xB7},
	{0x3006, 0x43},
	{0x3007, 0x37},
	{0x300f, 0x06},
	{0x3011, 0x08},
	{0x3010, 0x20},
	{0x3012, 0x00},
	{0x460c, 0x22},
	{0x3815, 0x04},
	{0x370c, 0xA0},
	{0x3602, 0xFC},
	{0x3612, 0xFF},
	{0x3634, 0xC0},
	{0x3613, 0x00},
	{0x3622, 0x00},
	{0x3603, 0x27},
	{0x4000, 0x21},
	{0x401D, 0x02},
	{0x3600, 0x54},
	{0x3605, 0x04},
	{0x3606, 0x3F},
	{0x5020, 0x04},
	{0x5197, 0x01},
	{ISP_CTRL01_REG, 0x00},
	{0x5500, 0x10},
	{0x5502, 0x00},
	{0x5503, 0x04},
	{0x5504, 0x00},
	{0x5505, 0x7F},
	{0x5080, 0x08},
	{0x300E, 0x18},
	{0x4610, 0x00},
	{0x471D, 0x05},
	{0x4708, 0x06},
	{0x3710, 0x10},
	{0x3632, 0x41},
	{0x3631, 0x01},
	{0x501F, 0x03},
	{0x3604, 0x40},
	{0x4300, 0xF8},
	{0x3824, 0x11},
	{ISP_CTRL00_REG, 0x00},
	{TIMING_TC_REG18_REG, 0xC1},
	{0x3705, 0xDB},
	{0x370A, 0x81},
	{0x3621, 0xC7},
	{0x3A1A, 0x04},
	{0x3A13, 0x30},
	{0x3004, 0xFF},
	{0x350C, 0x07},
	{0x350D, 0xD0},
	{0x3500, 0x00},
	{0x3501, 0x20},
	{0x3502, 0x00},
	{0x350A, 0x00},
	{0x350B, 0x10},
	{AEC_PK_MANUAL, 0x07},
	{0x5682, 0x05},
	{0x3A0F, 0x78},
	{0x3A11, 0xD0},
	{0x3A1B, 0x7A},
	{0x3A1E, 0x66},
	{0x3A1F, 0x40},
	{0x3A10, 0x68},
	{0x3030, 0x0B},
	{0x3A01, 0x04},
	{0x3A02, 0x00},
	{0x3A03, 0x78},
	{0x3A04, 0x00},
	{0x3A05, 0x30},
	{0x3A14, 0x00},
	{0x3A15, 0x64},
	{0x3A16, 0x00},
	{0x3A17, 0x89},
	{0x3A18, 0x00},
	{0x3A19, 0x70},
	{0x3A00, 0x78},
	{0x3A08, 0x12},
	{0x3A09, 0xC0},
	{0x3A0A, 0x0F},
	{0x3A0B, 0xA0},
	{0x3A0D, 0x04},
	{0x3A0E, 0x03},
	{0x3C00, 0x04},
	{0x3C01, 0xB4},
	{0x5688, 0xFD},
	{0x5689, 0xDF},
	{0x568A, 0xFE},
	{0x568B, 0xEF},
	{0x568C, 0xFE},
	{0x568D, 0xEF},
	{0x568E, 0xAA},
	{0x568F, 0xAA},
	{0x589B, 0x04},
	{0x589A, 0xC5},
	{0x528A, 0x00},
	{0x528B, 0x02},
	{0x528C, 0x08},
	{0x528D, 0x10},
	{0x528E, 0x20},
	{0x528F, 0x28},
	{0x5290, 0x30},
	{0x5292, 0x00},
	{0x5293, 0x00},
	{0x5294, 0x00},
	{0x5295, 0x02},
	{0x5296, 0x00},
	{0x5297, 0x08},
	{0x5298, 0x00},
	{0x5299, 0x10},
	{0x529A, 0x00},
	{0x529B, 0x20},
	{0x529C, 0x00},
	{0x529D, 0x28},
	{0x529E, 0x00},
	{0x5282, 0x00},
	{0x529F, 0x30},
	{0x5300, 0x00},
	{0x5302, 0x00},
	{0x5303, 0x7C},
	{0x530C, 0x00},
	{0x530D, 0x0C},
	{0x530E, 0x20},
	{0x530F, 0x80},
	{0x5310, 0x20},
	{0x5311, 0x80},
	{0x5308, 0x20},
	{0x5309, 0x40},
	{0x5304, 0x00},
	{0x5305, 0x30},
	{0x5306, 0x00},
	{0x5307, 0x80},
	{0x5314, 0x08},
	{0x5315, 0x20},
	{0x5319, 0x30},
	{0x5316, 0x10},
	{0x5317, 0x08},
	{0x5318, 0x02},
	{0x5380, 0x01},
	{0x5381, 0x20},
	{0x5382, 0x00},
	{0x5383, 0x4E},
	{0x5384, 0x00},
	{0x5385, 0x0F},
	{0x5386, 0x00},
	{0x5387, 0x00},
	{0x5388, 0x01},
	{0x5389, 0x15},
	{0x538A, 0x00},
	{0x538B, 0x31},
	{0x538C, 0x00},
	{0x538D, 0x00},
	{0x538E, 0x00},
	{0x538F, 0x0F},
	{0x5390, 0x00},
	{0x5391, 0xAB},
	{0x5392, 0x00},
	{0x5393, 0xA2},
	{0x5394, 0x08},
	{0x5301, 0x20},
	{0x5480, 0x14},
	{0x5482, 0x03},
	{0x5483, 0x57},
	{0x5484, 0x65},
	{0x5485, 0x71},
	{0x5481, 0x21},
	{0x5486, 0x7D},
	{0x5487, 0x87},
	{0x5488, 0x91},
	{0x5489, 0x9A},
	{0x548A, 0xAA},
	{0x548B, 0xB8},
	{0x548C, 0xCD},
	{0x548D, 0xDD},
	{0x548E, 0xEA},
	{0x548F, 0x10},
	{0x5490, 0x05},
	{0x5491, 0x00},
	{0x5492, 0x04},
	{0x5493, 0x20},
	{0x5494, 0x03},
	{0x5495, 0x60},
	{0x5496, 0x02},
	{0x5497, 0xB8},
	{0x5498, 0x02},
	{0x5499, 0x86},
	{0x549A, 0x02},
	{0x549B, 0x5B},
	{0x549C, 0x02},
	{0x549D, 0x3B},
	{0x549E, 0x02},
	{0x549F, 0x1C},
	{0x54A0, 0x02},
	{0x54A1, 0x04},
	{0x54A2, 0x01},
	{0x54A3, 0xED},
	{0x54A4, 0x01},
	{0x54A5, 0xC5},
	{0x54A6, 0x01},
	{0x54A7, 0xA5},
	{0x54A8, 0x01},
	{0x54A9, 0x6C},
	{0x54AA, 0x01},
	{0x54AB, 0x41},
	{0x54AC, 0x01},
	{0x54AD, 0x20},
	{0x54AE, 0x00},
	{0x54AF, 0x16},
	{0x3406, 0x00},
	{0x5192, 0x04},
	{0x5191, 0xF8},
	{0x5193, 0x70},
	{0x5194, 0xF0},
	{0x5195, 0xF0},
	{0x518D, 0x3D},
	{0x518F, 0x54},
	{0x518E, 0x3D},
	{0x5190, 0x54},
	{0x518B, 0xC0},
	{0x518C, 0xBD},
	{0x5187, 0x18},
	{0x5188, 0x18},
	{0x5189, 0x6E},
	{0x518A, 0x68},
	{0x5186, 0x1C},
	{0x5181, 0x50},
	{0x5182, 0x11},
	{0x5183, 0x14},
	{0x5184, 0x25},
	{0x5185, 0x24},
	{0x5025, 0x82},
	{SDE_CTRL3_REG, 0x40},
	{SDE_CTRL4_REG, 0x40},
	{SDE_CTRL0_REG, 0x02},
	{0x3633, 0x07},
	{0x3702, 0x10},
	{0x3703, 0xB2},
	{0x3704, 0x18},
	{0x370B, 0x40},
	{0x370D, 0x02},
	{0x3620, 0x52},
	{0x501E, 0x20},
	{0xffff, 0xff},

};

/* Initialization sequence for QVGA resolution (320x240) */
static const struct ov5642_reg dvp_320x240_res_params[] = {
	{0x3800, 0x01}, {0x3801, 0xa8}, {0x3802, 0x00}, {0x3803, 0x0A}, {0x3804, 0x0A},
	{0x3805, 0x20}, {0x3806, 0x07}, {0x3807, 0x98}, {0x3808, 0x01}, {0x3809, 0x40},
	{0x380a, 0x00}, {0x380b, 0xF0}, {0x380c, 0x0c}, {0x380d, 0x80}, {0x380e, 0x07},
	{0x380f, 0xd0}, {0x5001, 0x7f}, {0x5680, 0x00}, {0x5681, 0x00}, {0x5682, 0x0A},
	{0x5683, 0x20}, {0x5684, 0x00}, {0x5685, 0x00}, {0x5686, 0x07}, {0x5687, 0x98},
	{0x3801, 0xb0}, {0xffff, 0xff},
};

/* Initialization sequence for VGA resolution (640x480) */
static const struct ov5642_reg dvp_640x480_res_params[] = {
	{0x3800, 0x00}, {0x3801, 0x10}, {0x3802, 0x00}, {0x3803, 0x1C}, {0x3804, 0x0A},
	{0x3805, 0x20}, {0x3806, 0x07}, {0x3807, 0x98}, {0x3808, 0x02}, {0x3809, 0x80},
	{0x380a, 0x01}, {0x380b, 0xe0}, {0x380c, 0x0c}, {0x380d, 0x80}, {0x380e, 0x07},
	{0x380f, 0xd0}, {0x5001, 0x7f}, {0x5680, 0x00}, {0x5681, 0x00}, {0x5682, 0x0A},
	{0x5683, 0x20}, {0x5684, 0x00}, {0x5685, 0x00}, {0x5686, 0x07}, {0x5687, 0x98},
};

/* Initialization sequence for XGA resolution (1024x768) */
static const struct ov5642_reg dvp_1024x768_res_params[] = {
	{0x3800, 0x01}, {0x3801, 0xB0}, {0x3802, 0x00}, {0x3803, 0x0A}, {0x3804, 0x0A},
	{0x3805, 0x20}, {0x3806, 0x07}, {0x3807, 0x98}, {0x3808, 0x04}, {0x3809, 0x00},
	{0x380a, 0x03}, {0x380b, 0x00}, {0x380c, 0x0c}, {0x380d, 0x80}, {0x380e, 0x07},
	{0x380f, 0xd0}, {0x5001, 0x7f}, {0x5680, 0x00}, {0x5681, 0x00}, {0x5682, 0x0A},
	{0x5683, 0x20}, {0x5684, 0x00}, {0x5685, 0x00}, {0x5686, 0x07}, {0x5687, 0x98},
	{0xffff, 0xff},
};

/* Initialization sequence for SXGA resolution (1280x960) */
static const struct ov5642_reg dvp_1280x960_res_params[] = {
	{0x3800, 0x03}, {0x3801, 0xE8}, {0x3802, 0x03}, {0x3803, 0xE8}, {0x3804, 0x38},
	{0x3805, 0x00}, {0x3806, 0x03}, {0x3807, 0xC0}, {0x3808, 0x05}, {0x3809, 0x00},
	{0x380A, 0x03}, {0x380B, 0xC0}, {0x380C, 0x0A}, {0x380D, 0xF0}, {0x380E, 0x03},
	{0x380F, 0xE8}, {0x3827, 0x08}, {0x3810, 0xC0}, {0x5683, 0x00}, {0x5686, 0x03},
	{0x5687, 0xC0},
};

static const struct ov5642_mode_config dvp_modes[] = {
	{
		.width = 320,
		.height = 240,
		.array_size_res_params = ARRAY_SIZE(dvp_320x240_res_params),
		.res_params = dvp_320x240_res_params,
		.max_frmrate = OV5642_30_FPS,
		.def_frmrate = OV5642_30_FPS,
	},
	{
		.width = 640,
		.height = 480,
		.array_size_res_params = ARRAY_SIZE(dvp_640x480_res_params),
		.res_params = dvp_640x480_res_params,
		.max_frmrate = OV5642_30_FPS,
		.def_frmrate = OV5642_30_FPS,
	},
	{
		.width = 1024,
		.height = 768,
		.array_size_res_params = ARRAY_SIZE(dvp_1024x768_res_params),
		.res_params = dvp_1024x768_res_params,
		.max_frmrate = OV5642_30_FPS,
		.def_frmrate = OV5642_30_FPS,
	},
	{
		.width = 1280,
		.height = 960,
		.array_size_res_params = ARRAY_SIZE(dvp_1280x960_res_params),
		.res_params = dvp_1280x960_res_params,
		.max_frmrate = OV5642_30_FPS,
		.def_frmrate = OV5642_30_FPS,
	},
};

#define OV5642_VIDEO_FORMAT_CAP(width, height, format)		\
{.pixelformat = (format),					\
	.width_min = (width),					\
	.width_max = (width),					\
	.height_min = (height),					\
	.height_max = (height),					\
	.width_step = 0,					\
	.height_step = 0}

static const struct video_format_cap dvp_fmts[] = {
	OV5642_VIDEO_FORMAT_CAP(320, 240, VIDEO_PIX_FMT_GREY),
	OV5642_VIDEO_FORMAT_CAP(640, 480, VIDEO_PIX_FMT_GREY),
	OV5642_VIDEO_FORMAT_CAP(1024, 768, VIDEO_PIX_FMT_GREY),
	OV5642_VIDEO_FORMAT_CAP(1280, 960, VIDEO_PIX_FMT_GREY),
	{0}};

static int ov5642_read_reg(const struct i2c_dt_spec *spec, const uint16_t addr,
			   void *val, const uint8_t val_size)
{
	int ret;
	struct i2c_msg msg[2];
	uint8_t addr_buf[2];

	if (val_size > 4) {
		return -ENOTSUP;
	}

	addr_buf[1] = addr & 0xFF;
	addr_buf[0] = addr >> 8;
	msg[0].buf = addr_buf;
	msg[0].len = 2U;
	msg[0].flags = I2C_MSG_WRITE;

	msg[1].buf = (uint8_t *)val;
	msg[1].len = val_size;
	msg[1].flags = I2C_MSG_READ | I2C_MSG_STOP | I2C_MSG_RESTART;

	ret = i2c_transfer_dt(spec, msg, 2);
	if (ret) {
		return ret;
	}

	switch (val_size) {
	case 4:
		*(uint32_t *)val = sys_be32_to_cpu(*(uint32_t *)val);
		break;
	case 2:
		*(uint16_t *)val = sys_be16_to_cpu(*(uint16_t *)val);
		break;
	case 1:
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int ov5642_write_reg(const struct i2c_dt_spec *spec, const uint16_t addr,
			    const uint8_t val)
{
	uint8_t buf[3];

	buf[0] = addr >> 8;
	buf[1] = addr & 0xFF;
	buf[2] = val;

	return i2c_write_dt(spec, buf, sizeof(buf));
}

static int ov5642_modify_reg(const struct i2c_dt_spec *spec, const uint16_t addr,
			     const uint8_t mask, const uint8_t val)
{
	uint8_t reg_val = 0;
	int ret = ov5642_read_reg(spec, addr, &reg_val, sizeof(reg_val));

	if (ret) {
		return ret;
	}

	return ov5642_write_reg(spec, addr, (reg_val & ~mask) | (val & mask));
}

static int ov5642_write_multi_regs(const struct i2c_dt_spec *spec,
				   const struct ov5642_reg *regs,
				   const uint32_t num_regs)
{
	int ret;

	for (int i = 0; i < num_regs; i++) {
		ret = ov5642_write_reg(spec, regs[i].addr, regs[i].val);
		if (ret) {
			return ret;
		}
	}

	return 0;
}

static int ov5642_set_fmt(const struct device *dev, struct video_format *fmt)
{
	struct ov5642_data *drv_data = dev->data;
	const struct ov5642_config *cfg = dev->config;
	int ret = 0;
	int i;
	const struct video_format_cap *fmts;
	const struct ov5642_mode_config *modes;
	size_t num_fmts;
	size_t array_size_modes;

	if (fmt->pixelformat != VIDEO_PIX_FMT_GREY) {
		LOG_ERR("ov5642 camera supports GREY pixel formats!");
		return -ENOTSUP;
	}

	fmts = dvp_fmts;
	modes = dvp_modes;
	num_fmts = ARRAY_SIZE(dvp_fmts);
	array_size_modes = ARRAY_SIZE(dvp_modes);

	for (i = 0; i < num_fmts; ++i) {
		if (fmt->pixelformat == fmts[i].pixelformat &&
		    fmt->width >= fmts[i].width_min &&
		    fmt->width <= fmts[i].width_max &&
		    fmt->height >= fmts[i].height_min &&
		    fmt->height <= fmts[i].height_max) {
			break;
		}
	}

	if (i == num_fmts) {
		LOG_ERR("Unsupported pixel format or resolution");
		return -ENOTSUP;
	}

	if (!memcmp(&drv_data->fmt, fmt, sizeof(drv_data->fmt))) {
		return 0;
	}

	drv_data->fmt = *fmt;

	for (i = 0; i < array_size_modes; i++) {
		if (fmt->width == modes[i].width && fmt->height == modes[i].height) {
			ret = ov5642_write_multi_regs(&cfg->i2c, modes[i].res_params,
						      modes[i].array_size_res_params);
			if (ret) {
				LOG_ERR("Unable to set resolution parameters");
				return ret;
			}
			drv_data->cur_mode = &modes[i];
			break;
		}
	}

	if (i == array_size_modes) {
		LOG_ERR("Unsupported pixel resolution");
		return -ENOTSUP;
	}

	return 0;
}

static int ov5642_set_stream(const struct device *dev, bool enable,
			     enum video_buf_type type)
{
	const struct ov5642_config *cfg = dev->config;
	uint8_t val = enable ? SYS_CTRL0_SW_PWUP : SYS_CTRL0_SW_PWDN;
	int ret = ov5642_write_reg(&cfg->i2c, SYS_CTRL0_REG, val);

	if (ret) {
		LOG_ERR("Failed to %s streaming: %d", enable ? "start" : "stop", ret);
		return ret;
	}
	LOG_INF("Streaming %s", enable ? "started" : "stopped");
	return 0;
}

static int ov5642_get_fmt(const struct device *dev, struct video_format *fmt)
{
	struct ov5642_data *drv_data = dev->data;

	*fmt = drv_data->fmt;

	return 0;
}

static int ov5642_get_caps(const struct device *dev, struct video_caps *caps)
{
	caps->format_caps = dvp_fmts;
	return 0;
}

static int ov5642_set_ctrl_hue(const struct device *dev, int value)
{
	const struct ov5642_config *cfg = dev->config;
	int cos_coef, sin_coef, sign = 0;

	double rad_val = value;
	int ret = ov5642_modify_reg(&cfg->i2c, SDE_CTRL0_REG, BIT(0), BIT(0));

	if (ret) {
		return ret;
	}

	rad_val = value * PI / 180.0;
	cos_coef = round(cos(rad_val) * 128);
	sin_coef = round(sin(rad_val) * 128);

	if (0 <= value && value < 90) {
		sign = 0x01;
	} else if (90 <= value && value < 180) {
		sign = 0x31;
	} else if (180 <= value && value < 270) {
		sign = 0x32;
	} else if (270 <= value && value < 360) {
		sign = 0x02;
	}

	struct ov5642_reg hue_params[] = {{SDE_CTRL1_REG, abs(cos_coef) & 0xFF},
					  {SDE_CTRL2_REG, abs(sin_coef) & 0xFF}};

	ret = ov5642_modify_reg(&cfg->i2c, SDE_CTRL10_REG, 0x7F, sign);
	if (ret < 0) {
		return ret;
	}

	return ov5642_write_multi_regs(&cfg->i2c, hue_params, ARRAY_SIZE(hue_params));
}

static int ov5642_set_ctrl_saturation(const struct device *dev, int value)
{
	const struct ov5642_config *cfg = dev->config;

	struct ov5642_reg saturation_params[] = {{SDE_CTRL3_REG, value},
						{SDE_CTRL4_REG, value}};

	return ov5642_write_multi_regs(&cfg->i2c, saturation_params,
				       ARRAY_SIZE(saturation_params));
}

static int ov5642_set_ctrl_contrast(const struct device *dev, int value)
{
	const struct ov5642_config *cfg = dev->config;

	int ret = ov5642_modify_reg(&cfg->i2c, SDE_CTRL0_REG, BIT(2), BIT(2));

	if (ret) {
		return ret;
	}

	ret = ov5642_modify_reg(&cfg->i2c, SDE_CTRL8_REG, BIT(2),
				value >= 0 ? 0 : BIT(2));
	if (ret < 0) {
		return ret;
	}

	return ov5642_write_reg(&cfg->i2c, SDE_CTRL8_REG, value & 0xff);
}

static int ov5642_set_ctrl_brightness(const struct device *dev, int value)
{
	const struct ov5642_config *cfg = dev->config;

	int ret = ov5642_modify_reg(&cfg->i2c, SDE_CTRL0_REG, BIT(2), BIT(2));

	if (ret) {
		return ret;
	}

	ret = ov5642_modify_reg(&cfg->i2c, SDE_CTRL10_REG, BIT(2),
				value >= 0 ? 0 : BIT(2));
	if (ret < 0) {
		return ret;
	}

	return ov5642_write_reg(&cfg->i2c, SDE_CTRL9_REG, (abs(value) << 4) & 0xf0);
}

static int ov5642_set_ctrl_auto_gain(const struct device *dev, int value)
{
	const struct ov5642_config *cfg = dev->config;

	return ov5642_modify_reg(&cfg->i2c, AEC_PK_MANUAL, BIT(1),
				 value ? 0 : BIT(1));
}

static int ov5642_set_ctrl_white_bal(const struct device *dev, int value)
{
	const struct ov5642_config *cfg = dev->config;

	return ov5642_modify_reg(&cfg->i2c, ISP_CTRL01_REG, BIT(0),
				 value ? BIT(0) : 0);
}

static int ov5642_set_ctrl_exposure(const struct device *dev, int value)
{
	const struct ov5642_config *cfg = dev->config;

	return ov5642_modify_reg(&cfg->i2c, AEC_PK_MANUAL, BIT(0),
				 value ? BIT(0) : 0);
}

static int ov5642_set_ctrl_vflip(const struct device *dev, int value)
{
	const struct ov5642_config *cfg = dev->config;

	return ov5642_modify_reg(&cfg->i2c, TIMING_TC_REG18_REG, BIT(5),
				 value ? BIT(5) : 0);
}

static int ov5642_set_ctrl_hflip(const struct device *dev, int value)
{
	const struct ov5642_config *cfg = dev->config;

	return ov5642_modify_reg(&cfg->i2c, TIMING_TC_REG18_REG, BIT(6),
				 value ? BIT(6) : 0);
}

static int ov5642_set_ctrl(const struct device *dev, uint32_t id)
{
	struct ov5642_data *drv_data = dev->data;
	struct ov5642_ctrls *ctrls = &drv_data->ctrls;

	switch (id) {
	case VIDEO_CID_HFLIP:
		return ov5642_set_ctrl_hflip(dev, ctrls->hflip.val);
	case VIDEO_CID_VFLIP:
		return ov5642_set_ctrl_vflip(dev, ctrls->vflip.val);
	case VIDEO_CID_EXPOSURE:
		return ov5642_set_ctrl_exposure(dev, ctrls->ae.val);
	case VIDEO_CID_WHITE_BALANCE_TEMPERATURE:
		return ov5642_set_ctrl_white_bal(dev, ctrls->awb.val);
	case VIDEO_CID_AUTOGAIN:
		return ov5642_set_ctrl_auto_gain(dev, ctrls->auto_gain.val);
	case VIDEO_CID_BRIGHTNESS:
		return ov5642_set_ctrl_brightness(dev, ctrls->brightness.val);
	case VIDEO_CID_CONTRAST:
		return ov5642_set_ctrl_contrast(dev, ctrls->contrast.val);
	case VIDEO_CID_SATURATION:
		return ov5642_set_ctrl_saturation(dev, ctrls->saturation.val);
	case VIDEO_CID_HUE:
		return ov5642_set_ctrl_hue(dev, ctrls->hue.val);
	default:
			return -ENOTSUP;
	}
}

static DEVICE_API(video, ov5642_driver_api) = {
	.set_format = ov5642_set_fmt,
	.get_format = ov5642_get_fmt,
	.get_caps = ov5642_get_caps,
	.set_stream = ov5642_set_stream,
	.set_ctrl = ov5642_set_ctrl,
};

static int ov5642_init_controls(const struct device *dev)
{
	int ret;
	struct ov5642_data *drv_data = dev->data;
	struct ov5642_ctrls *ctrls = &drv_data->ctrls;

	ret = video_init_ctrl(&ctrls->hflip, dev, VIDEO_CID_HFLIP,
			      (struct video_ctrl_range){
			      .min = 0, .max = 1, .step = 1, .def = 0 });
	if (ret) {
		return ret;
	}

	ret = video_init_ctrl(&ctrls->vflip, dev, VIDEO_CID_VFLIP,
			      (struct video_ctrl_range){
			      .min = 0,
			      .max = 1,
			      .step = 1,
			      .def = 0
			      });
	if (ret) {
		return ret;
	}

	ret = video_init_ctrl(&ctrls->ae, dev, VIDEO_CID_EXPOSURE,
			      (struct video_ctrl_range){
			      .min = 0,
			      .max = 1,
			      .step = 1,
			      .def = 1
			      });
	if (ret) {
		return ret;
	}

	ret = video_init_ctrl(&ctrls->awb, dev, VIDEO_CID_WHITE_BALANCE_TEMPERATURE,
			      (struct video_ctrl_range){
			      .min = 0,
			      .max = 1,
			      .step = 1,
			      .def = 1
			      });
	if (ret) {
		return ret;
	}

	ret = video_init_ctrl(&ctrls->auto_gain, dev, VIDEO_CID_AUTOGAIN,
			      (struct video_ctrl_range){
			      .min = 0,
			      .max = 1,
			      .step = 1,
			      .def = 1
			      });
	if (ret) {
		return ret;
	}

	ret = video_init_ctrl(&ctrls->brightness, dev, VIDEO_CID_BRIGHTNESS,
			      (struct video_ctrl_range){
			      .min = -15,
			      .max = 15,
			      .step = 1,
			      .def = 0
			      });
	if (ret) {
		return ret;
	}

	ret = video_init_ctrl(&ctrls->contrast, dev, VIDEO_CID_CONTRAST,
			      (struct video_ctrl_range){
			      .min = 0,
			      .max = 255,
			      .step = 1,
			      .def = 0
			      });
	if (ret) {
		return ret;
	}

	ret = video_init_ctrl(&ctrls->saturation, dev, VIDEO_CID_SATURATION,
			      (struct video_ctrl_range){
			      .min = 0,
			      .max = 255,
			      .step = 1,
			      .def = 64
			      });
	if (ret) {
		return ret;
	}

	return video_init_ctrl(&ctrls->hue, dev, VIDEO_CID_HUE,
			       (struct video_ctrl_range){
			       .min = 0,
			       .max = 359,
			       .step = 1,
			       .def = 0
			       });
}

static int ov5642_init(const struct device *dev)
{
	const struct ov5642_config *cfg = dev->config;
	uint16_t chip_id;
	int ret;

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(powerdown_gpios)
	if (cfg->powerdown_gpio.port != NULL && !gpio_is_ready_dt(&cfg->powerdown_gpio)) {
		LOG_ERR("%s: device %s is not ready", dev->name,
			cfg->powerdown_gpio.port->name);
		return -ENODEV;
	}
#endif

	/* Power up sequence */
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(powerdown_gpios)
	if (cfg->powerdown_gpio.port != NULL) {
		ret = gpio_pin_configure_dt(&cfg->powerdown_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret) {
			return ret;
		}
	}
#endif

	k_sleep(K_MSEC(5));

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(powerdown_gpios)
	if (cfg->powerdown_gpio.port != NULL) {
		gpio_pin_set_dt(&cfg->powerdown_gpio, 0);
	}
#endif

	k_sleep(K_MSEC(20));

	/* Check sensor chip id */
	ret = ov5642_read_reg(&cfg->i2c, CHIP_ID_REG, &chip_id, sizeof(chip_id));
	if (ret) {
		LOG_ERR("Unable to read sensor chip ID, ret = %d", ret);
		return -ENODEV;
	}

	if (chip_id != CHIP_ID_VAL) {
		LOG_ERR("Wrong chip ID: %04x (expected %04x)", chip_id, CHIP_ID_VAL);
		return -ENODEV;
	}

	k_sleep(K_MSEC(5));

	/* Initialize register values */
	ret = ov5642_write_multi_regs(&cfg->i2c, init_params_common,
				      ARRAY_SIZE(init_params_common));
	if (ret) {
		LOG_ERR("Unable to initialize the sensor");
		return -EIO;
	}

	/* Explicitly set DVP mode if bus_type is not CSI2 */
	if (cfg->bus_type != VIDEO_BUS_TYPE_CSI2_DPHY) {
		ret = ov5642_write_reg(&cfg->i2c, 0x300e, 0x08);
		if (ret) {
			LOG_ERR("Failed to force DVP mode");
			return ret;
		}
	}

	/* Initialize controls */
	return ov5642_init_controls(dev);
	return 0;
}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(powerdown_gpios)
#define OV5642_GET_POWERDOWN_GPIO(n)						\
	.powerdown_gpio = GPIO_DT_SPEC_INST_GET_OR(n, powerdown_gpios, {0}),
#else
#define OV5642_GET_POWERDOWN_GPIO(n)
#endif

#define OV5642_INIT(n)								\
	static struct ov5642_data ov5642_data_##n;				\
										\
	static const struct ov5642_config ov5642_cfg_##n = {			\
		.i2c = I2C_DT_SPEC_INST_GET(n),					\
		.bus_type = DT_PROP_OR(DT_CHILD(DT_INST_CHILD(n, port),		\
					endpoint), bus_type,			\
					VIDEO_BUS_TYPE_CSI2_DPHY),		\
	};									\
										\
	DEVICE_DT_INST_DEFINE(n, &ov5642_init, NULL, &ov5642_data_##n,		\
			      &ov5642_cfg_##n,					\
			      POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY,		\
			      &ov5642_driver_api);				\
										\
	VIDEO_DEVICE_DEFINE(ov5642_##n, DEVICE_DT_INST_GET(n), NULL);

DT_INST_FOREACH_STATUS_OKAY(OV5642_INIT)
