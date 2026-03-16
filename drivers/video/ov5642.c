/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2025 Silicon Signals Pvt. Ltd.
 *
 * Author: Hardevsinh Palaniya <hardevsinh.palaniya@siliconsignals.io>
 * Author: Rutvij Trivedi <rutvij.trivedi@siliconsignals.io>
 */

#define DT_DRV_COMPAT ovti_ov5642

#include <math.h>
#include <stdlib.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/video-controls.h>
#include <zephyr/drivers/video.h>
#include <zephyr/dt-bindings/video/video-interfaces.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include "video_common.h"
#include "video_ctrls.h"
#include "video_device.h"

LOG_MODULE_REGISTER(video_ov5642, CONFIG_VIDEO_LOG_LEVEL);

#define OV5642_REG8(addr)  ((uint32_t)(addr) | VIDEO_REG_ADDR16_DATA8)
#define OV5642_REG16(addr) ((uint32_t)(addr) | VIDEO_REG_ADDR16_DATA16_BE)

#define CHIP_ID_REG		0x300a
#define CHIP_ID_CCI		OV5642_REG16(CHIP_ID_REG)
#define CHIP_ID_VAL		0x5642

#define SYS_CTRL0_REG		0x3008
#define SYS_CTRL0_CCI		OV5642_REG8(SYS_CTRL0_REG)
#define SYS_CTRL0_SW_PWDN	0x80
#define SYS_CTRL0_SW_PWUP	0x02
#define SYS_CTRL0_SW_RST	0x82

#define IO_MIPI_CTRL00_REG	0x300e
#define IO_MIPI_CTRL00_CCI	OV5642_REG8(IO_MIPI_CTRL00_REG)
#define IO_MIPI_CTRL00_DVP_MODE 0x08
#define PCLK_CLOCK_SEL_REG	0x3103

#define TIMING_TC_REG18_REG	0x3818
#define TIMING_TC_REG18_CCI	OV5642_REG8(TIMING_TC_REG18_REG)

#define AEC_PK_MANUAL		0x3503
#define AEC_PK_CCI		OV5642_REG8(AEC_PK_MANUAL)
#define REAL_GAIN		0x52a6
#define ISP_CTRL00_REG		0x5000
#define ISP_CTRL00_CCI		OV5642_REG8(ISP_CTRL00_REG)
#define ISP_CTRL01_REG		0x5001
#define ISP_CTRL01_CCI		OV5642_REG8(ISP_CTRL01_REG)

#define SDE_CTRL0_REG		0x5580
#define SDE_CTRL0_CCI		OV5642_REG8(SDE_CTRL0_REG)
#define SDE_CTRL1_REG		0x5581
#define SDE_CTRL1_CCI		OV5642_REG8(SDE_CTRL1_REG)
#define SDE_CTRL2_REG		0x5582
#define SDE_CTRL2_CCI		OV5642_REG8(SDE_CTRL2_REG)
#define SDE_CTRL3_REG		0x5583
#define SDE_CTRL3_CCI		OV5642_REG8(SDE_CTRL3_REG)
#define SDE_CTRL4_REG		0x5584
#define SDE_CTRL4_CCI		OV5642_REG8(SDE_CTRL4_REG)
#define SDE_CTRL5_REG		0x5585
#define SDE_CTRL5_CCI		OV5642_REG8(SDE_CTRL5_REG)
#define SDE_CTRL6_REG		0x5586
#define SDE_CTRL6_CCI		OV5642_REG8(SDE_CTRL6_REG)
#define SDE_CTRL7_REG		0x5587
#define SDE_CTRL7_CCI		OV5642_REG8(SDE_CTRL7_REG)
#define SDE_CTRL8_REG		0x5588
#define SDE_CTRL8_CCI		OV5642_REG8(SDE_CTRL8_REG)
#define SDE_CTRL9_REG		0x5589
#define SDE_CTRL9_CCI		OV5642_REG8(SDE_CTRL9_REG)
#define SDE_CTRL10_REG		0x558a
#define SDE_CTRL10_CCI		OV5642_REG8(SDE_CTRL10_REG)

#define SYSTEM_RESET00_REG	0x3000
#define SYSTEM_RESET01_REG	0x3001
#define SYSTEM_RESET02_REG	0x3002
#define SYSTEM_RESET03_REG	0x3003

#define CLOCK_EN00_REG		0x3004
#define CLOCK_EN01_REG		0x3005
#define CLOCK_EN02_REG		0x3006
#define CLOCK_EN03_REG		0x3007

#define PLL_CTRL00_REG		0x300f
#define PLL_CTRL01_REG		0x3010
#define PLL_CTRL02_REG		0x3011
#define PLL_CTRL03_REG		0x3012

#define VFIFO_CTRL0C_REG	0x460c
#define DVP_PCLK_DIV_REG	0X3815

#define BLC_CTRL00_REG		0x4000
#define BLC_FRAME_CTRL_REG	0x401d
#define AWB_CTRL_23_REG		0x5197

#define UVADJUST_CTRL0_REG	0x5500
#define EVEN_CTRL00_REG		0X5080

#define FORMAT_MUX_CTRL_REG	0x501f
#define FORMAT_CTRL00_REG	0X4300
#define DVP_CTRL_1D_REG		0x471d

#define ARRAY_CTRL01_REG	0X3621
#define AEC_CTRL_REG		0x3A1A
#define AEC_CTRL13_REG		0x3A13
#define AEC_CTRL0F_REG		0x3A0F
#define AEC_CTRL10_REG		0x3A10
#define AEC_CTRL11_REG		0X3A11
#define AEC_CTRL1B_REG		0X3A1B
#define AEC_CTRL1E_REG		0X3A1E
#define AEC_CTRL1F_REG		0X3A1F
#define ISP_CONTROL_37_REG	0x5025

#define PI 3.141592654

enum ov5642_res {
	OV5642_RES_320x240,
	OV5642_RES_640x480,
	OV5642_RES_1024x768,
	OV5642_RES_1280x960,
};

struct ov5642_config {
	struct i2c_dt_spec i2c;
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(powerdown_gpios)
	struct gpio_dt_spec powerdown_gpio;
#endif
	int bus_type;
};

struct ov5642_mode_config {
	uint16_t width;
	uint16_t height;
	uint16_t array_size_res_params;
	const struct video_reg16 *res_params;
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
};

static const struct video_reg16 init_params_common[] = {
	/* Power-up sequence */
	{SYS_CTRL0_REG, SYS_CTRL0_SW_PWDN},
	{PCLK_CLOCK_SEL_REG, 0x93},
	{SYS_CTRL0_REG, SYS_CTRL0_SW_PWUP},

	{0x3017, 0x7f},
	{0x3018, 0xf0},

	/* System reset registers */
	{SYSTEM_RESET00_REG, 0xf8},
	{SYSTEM_RESET01_REG, 0x48},
	{SYSTEM_RESET02_REG, 0x5c},
	{SYSTEM_RESET03_REG, 0x02},

	/* Clock enable registers */
	{CLOCK_EN01_REG, 0xb7},
	{CLOCK_EN02_REG, 0x43},
	{CLOCK_EN03_REG, 0x37},
	{PLL_CTRL00_REG, 0x06},
	{PLL_CTRL02_REG, 0x08},
	{PLL_CTRL01_REG, 0x20},

	{VFIFO_CTRL0C_REG, 0x22},
	{DVP_PCLK_DIV_REG, 0x04},

	/* Analog Control Registers */
	{0x370c, 0xa0},
	{0x3602, 0xfc},
	{0x3612, 0xff},
	{0x3634, 0xc0},
	{0x3613, 0x00},
	{0x3622, 0x00},
	{0x3603, 0x27},
	{0x3600, 0x54},
	{0x3605, 0x04},
	{0x3606, 0x3f},
	{0x3710, 0x10},
	{0x3632, 0x41},
	{0x3631, 0x01},
	{0x3604, 0x40},
	{0x3705, 0xdb},
	{0x370a, 0x81},
	{0x3633, 0x07},
	{0x3702, 0x10},
	{0x3703, 0xb2},
	{0x3704, 0x18},
	{0x370b, 0x40},
	{0x370d, 0x02},
	{0x3620, 0x52},

	{BLC_CTRL00_REG, 0x21},
	{BLC_FRAME_CTRL_REG, 0x02},

	/* ISP reserved */
	{0x5020, 0x04},
	{AWB_CTRL_23_REG, 0x01},

	{ISP_CTRL01_REG, 0x00},
	{UVADJUST_CTRL0_REG, 0x10},

	/* UV ADJ TH1 */
	{0x5502, 0x00},
	{0x5503, 0x04},

	/* UV ADJ TH2 */
	{0x5504, 0x00},
	{0x5505, 0x7f},

	{EVEN_CTRL00_REG, 0x08},
	{IO_MIPI_CTRL00_REG, 0x18},
	{DVP_CTRL_1D_REG, 0x05},

	/* DVP Control registers */
	{0x4708, 0x06},

	{FORMAT_MUX_CTRL_REG, 0x03},
	{FORMAT_CTRL00_REG, 0xf8},
	{0x3824, 0x11},
	{ISP_CTRL00_REG, 0x00},
	{TIMING_TC_REG18_REG, 0xc1},
	{ARRAY_CTRL01_REG, 0xc7},
	{AEC_CTRL_REG, 0x04},
	{AEC_CTRL13_REG, 0x30},
	{CLOCK_EN00_REG, 0xff},

	/* AEC PK VTS */
	{0x350c, 0x07},
	{0x350d, 0xd0},

	/* AEC PK LONG EXPO */
	{0x34ff, 0x00},
	{0x3501, 0x20},
	{0x3502, 0x00},

	/* AEC PK AGC ADJ */
	{0x350a, 0x00},
	{0x350b, 0x10},

	{AEC_PK_MANUAL,  0x07},
	{AEC_CTRL0F_REG, 0x78},
	{AEC_CTRL11_REG, 0xd0},
	{AEC_CTRL1B_REG, 0x7a},
	{AEC_CTRL1E_REG, 0x66},
	{AEC_CTRL1F_REG, 0x40},
	{AEC_CTRL10_REG, 0x68},

	/* AEC MAX EXPO */
	{0x3A01, 0x04},
	{0x3A02, 0x00},
	{0x3A03, 0x78},
	{0x3a04, 0x00},
	{0x3a05, 0x30},
	{0x3a14, 0x00},
	{0x3a15, 0x64},
	{0x3a16, 0x00},
	{0x3a17, 0x89},
	{0x3a18, 0x00},
	{0x3a19, 0x70},
	{0x3a00, 0x78},
	{0x3a08, 0x12},
	{0x3a09, 0xc0},
	{0x3a0a, 0x0f},
	{0x3a0b, 0xa0},
	{0x3a0d, 0x04},
	{0x3a0e, 0x03},

	/* light frequency registers */
	{0x3c00, 0x04},
	{0x3c01, 0xb4},

	/* Scale/average registers */
	{0x5682, 0x05},
	{0x5688, 0xfd},
	{0x5689, 0xdf},
	{0x568a, 0xfe},
	{0x568b, 0xef},
	{0x568c, 0xfe},
	{0x568d, 0xef},
	{0x568e, 0xaa},
	{0x568f, 0xaa},

	/* DNS control registers */
	{0x528a, 0x00},
	{0x528b, 0x02},
	{0x528c, 0x08},
	{0x528d, 0x10},
	{0x528e, 0x20},
	{0x528f, 0x28},
	{0x5290, 0x30},
	{0x5292, 0x00},
	{0x5293, 0x00},
	{0x5294, 0x00},
	{0x5295, 0x02},
	{0x5296, 0x00},
	{0x5297, 0x08},
	{0x5298, 0x00},
	{0x5299, 0x10},
	{0x529a, 0x00},
	{0x529b, 0x20},
	{0x529c, 0x00},
	{0x529d, 0x28},
	{0x529e, 0x00},
	{0x5282, 0x00},
	{0x529f, 0x30},

	/* CIP control registers */
	{0x5300, 0x00},
	{0x5302, 0x00},
	{0x5303, 0x7c},
	{0x530c, 0x00},
	{0x530d, 0x0c},
	{0x530e, 0x20},
	{0x530f, 0x80},
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
	{0x5383, 0x4e},
	{0x5384, 0x00},
	{0x5385, 0x0f},
	{0x5386, 0x00},
	{0x5387, 0x00},
	{0x5388, 0x01},
	{0x5389, 0x15},
	{0x538a, 0x00},
	{0x538b, 0x31},
	{0x538c, 0x00},
	{0x538d, 0x00},
	{0x538e, 0x00},
	{0x538f, 0x0f},
	{0x5390, 0x00},
	{0x5391, 0xab},
	{0x5392, 0x00},
	{0x5393, 0xa2},
	{0x5394, 0x08},
	{0x5301, 0x20},

	/* YUV gamma control registers */
	{0x5480, 0x14},
	{0x5482, 0x03},
	{0x5483, 0x57},
	{0x5484, 0x65},
	{0x5485, 0x71},
	{0x5481, 0x21},
	{0x5486, 0x7d},
	{0x5487, 0x87},
	{0x5488, 0x91},
	{0x5489, 0x9a},
	{0x548a, 0xaa},
	{0x548b, 0xb8},
	{0x548c, 0xcd},
	{0x548d, 0xdd},
	{0x548e, 0xea},
	{0x548f, 0x10},
	{0x5490, 0x05},
	{0x5491, 0x00},
	{0x5492, 0x04},
	{0x5493, 0x20},
	{0x5494, 0x03},
	{0x5495, 0x60},
	{0x5496, 0x02},
	{0x5497, 0xb8},
	{0x5498, 0x02},
	{0x5499, 0x86},
	{0x549a, 0x02},
	{0x549b, 0x5b},
	{0x549c, 0x02},
	{0x549d, 0x3b},
	{0x549e, 0x02},
	{0x549f, 0x1c},
	{0x54a0, 0x02},
	{0x54a1, 0x04},
	{0x54a2, 0x01},
	{0x54a3, 0xed},
	{0x54a4, 0x01},
	{0x54a5, 0xc5},
	{0x54a6, 0x01},
	{0x54a7, 0xa5},
	{0x54a8, 0x01},
	{0x54a9, 0x6c},
	{0x54aa, 0x01},
	{0x54ab, 0x41},
	{0x54ac, 0x01},
	{0x54ad, 0x20},
	{0x54ae, 0x00},
	{0x54af, 0x16},

	/* AWB/Gamma adjustments */
	{0x589b, 0x04},
	{0x589a, 0xc5},
	{0x3406, 0x00},
	{0x5192, 0x04},
	{0x5191, 0xf8},
	{0x5193, 0x70},
	{0x5194, 0xf0},
	{0x5195, 0xf0},
	{0x518d, 0x3d},
	{0x518f, 0x54},
	{0x518e, 0x3d},
	{0x5190, 0x54},
	{0x518b, 0xc0},
	{0x518c, 0xbd},
	{0x5187, 0x18},
	{0x5188, 0x18},
	{0x5189, 0x6e},
	{0x518a, 0x68},
	{0x5186, 0x1c},
	{0x5181, 0x50},
	{0x5182, 0x11},
	{0x5183, 0x14},
	{0x5184, 0x25},
	{0x5185, 0x24},

	{ISP_CONTROL_37_REG, 0x82},
	{SDE_CTRL3_REG, 0x40},
	{SDE_CTRL4_REG, 0x40},
	{SDE_CTRL0_REG, 0x02},
	{0xffff, 0xff},
};

/* Initialization sequence for QVGA resolution (320x240) */
static const struct video_reg16 dvp_320x240_res_params[] = {
	{0x3800, 0x01}, {0x3801, 0xa8}, {0x3802, 0x00}, {0x3803, 0x0A}, {0x3804, 0x0A},
	{0x3805, 0x20}, {0x3806, 0x07}, {0x3807, 0x98}, {0x3808, 0x01}, {0x3809, 0x40},
	{0x380a, 0x00}, {0x380b, 0xF0}, {0x380c, 0x0c}, {0x380d, 0x80}, {0x380e, 0x07},
	{0x380f, 0xd0}, {0x5001, 0x7f}, {0x5680, 0x00}, {0x5681, 0x00}, {0x5682, 0x0A},
	{0x5683, 0x20}, {0x5684, 0x00}, {0x5685, 0x00}, {0x5686, 0x07}, {0x5687, 0x98},
	{0x3801, 0xb0}, {0xffff, 0xff},
};

/* Initialization sequence for VGA resolution (640x480) */
static const struct video_reg16 dvp_640x480_res_params[] = {
	{0x3800, 0x00}, {0x3801, 0x10}, {0x3802, 0x00}, {0x3803, 0x1C}, {0x3804, 0x0A},
	{0x3805, 0x20}, {0x3806, 0x07}, {0x3807, 0x98}, {0x3808, 0x02}, {0x3809, 0x80},
	{0x380a, 0x01}, {0x380b, 0xe0}, {0x380c, 0x0c}, {0x380d, 0x80}, {0x380e, 0x07},
	{0x380f, 0xd0}, {0x5001, 0x7f}, {0x5680, 0x00}, {0x5681, 0x00}, {0x5682, 0x0A},
	{0x5683, 0x20}, {0x5684, 0x00}, {0x5685, 0x00}, {0x5686, 0x07}, {0x5687, 0x98},
};

/* Initialization sequence for XGA resolution (1024x768) */
static const struct video_reg16 dvp_1024x768_res_params[] = {
	{0x3800, 0x01}, {0x3801, 0xB0}, {0x3802, 0x00}, {0x3803, 0x0A}, {0x3804, 0x0A},
	{0x3805, 0x20}, {0x3806, 0x07}, {0x3807, 0x98}, {0x3808, 0x04}, {0x3809, 0x00},
	{0x380a, 0x03}, {0x380b, 0x00}, {0x380c, 0x0c}, {0x380d, 0x80}, {0x380e, 0x07},
	{0x380f, 0xd0}, {0x5001, 0x7f}, {0x5680, 0x00}, {0x5681, 0x00}, {0x5682, 0x0A},
	{0x5683, 0x20}, {0x5684, 0x00}, {0x5685, 0x00}, {0x5686, 0x07}, {0x5687, 0x98},
	{0xffff, 0xff},
};

/* Initialization sequence for SXGA resolution (1280x960) */
static const struct video_reg16 dvp_1280x960_res_params[] = {
	{0x3800, 0x03}, {0x3801, 0xE8}, {0x3802, 0x03}, {0x3803, 0xE8}, {0x3804, 0x38},
	{0x3805, 0x00}, {0x3806, 0x03}, {0x3807, 0xC0}, {0x3808, 0x05}, {0x3809, 0x00},
	{0x380A, 0x03}, {0x380B, 0xC0}, {0x380C, 0x0A}, {0x380D, 0xF0}, {0x380E, 0x03},
	{0x380F, 0xE8}, {0x3827, 0x08}, {0x3810, 0xC0}, {0x5683, 0x00}, {0x5686, 0x03},
	{0x5687, 0xC0},
};

static const struct ov5642_mode_config dvp_modes[] = {
	[OV5642_RES_320x240] = {
		.width = 320,
		.height = 240,
		.array_size_res_params = ARRAY_SIZE(dvp_320x240_res_params),
		.res_params = dvp_320x240_res_params,
	},
	[OV5642_RES_640x480] = {
		.width = 640,
		.height = 480,
		.array_size_res_params = ARRAY_SIZE(dvp_640x480_res_params),
		.res_params = dvp_640x480_res_params,
	},
	[OV5642_RES_1024x768] = {
		.width = 1024,
		.height = 768,
		.array_size_res_params = ARRAY_SIZE(dvp_1024x768_res_params),
		.res_params = dvp_1024x768_res_params,
	},
	[OV5642_RES_1280x960] = {
		.width = 1280,
		.height = 960,
		.array_size_res_params = ARRAY_SIZE(dvp_1280x960_res_params),
		.res_params = dvp_1280x960_res_params,
	},
};

#define OV5642_VIDEO_FORMAT_CAP(width, height, format)				\
	{.pixelformat = (format),						\
	.width_min = (width),							\
	.width_max = (width),							\
	.height_min = (height),							\
	.height_max = (height),							\
	.width_step = 0,							\
	.height_step = 0}

static const struct video_format_cap dvp_fmts[] = {
	[OV5642_RES_320x240] = OV5642_VIDEO_FORMAT_CAP(320, 240, VIDEO_PIX_FMT_GREY),
	[OV5642_RES_640x480] = OV5642_VIDEO_FORMAT_CAP(640, 480, VIDEO_PIX_FMT_GREY),
	[OV5642_RES_1024x768] = OV5642_VIDEO_FORMAT_CAP(1024, 768, VIDEO_PIX_FMT_GREY),
	[OV5642_RES_1280x960] = OV5642_VIDEO_FORMAT_CAP(1280, 960, VIDEO_PIX_FMT_GREY),
	{0}};

static int ov5642_set_fmt(const struct device *dev, struct video_format *fmt)
{
	struct ov5642_data *drv_data = dev->data;
	const struct ov5642_config *cfg = dev->config;
	const struct video_format_cap *fmts = dvp_fmts;
	const struct ov5642_mode_config *modes = dvp_modes;
	int ret = 0;
	size_t i;

	ret = video_format_caps_index(fmts, fmt, &i);
	if (ret < 0) {
		LOG_ERR("Unsupported pixel format or resolution");
		return ret;
	}

	if (memcmp(&drv_data->fmt, fmt, sizeof(drv_data->fmt)) == 0) {
		return 0;
	}

	ret = video_write_cci_multiregs16(&cfg->i2c, modes[i].res_params,
					  modes[i].array_size_res_params);
	if (ret < 0) {
		LOG_ERR("Unable to set resolution parameters");
		return ret;
	}

	drv_data->fmt = *fmt;

	return 0;
}

static int ov5642_set_stream(const struct device *dev, bool enable,
			     enum video_buf_type type)
{
	const struct ov5642_config *cfg = dev->config;
	uint8_t val = enable ? SYS_CTRL0_SW_PWUP : SYS_CTRL0_SW_PWDN;
	int ret;

	ret = video_write_cci_reg(&cfg->i2c, SYS_CTRL0_CCI, val);
	if (ret < 0) {
		LOG_ERR("Failed to %s streaming: %d", enable ? "start" : "stop", ret);
		return ret;
	}

	LOG_DBG("Streaming %s", enable ? "started" : "stopped");

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
	double rad_val = value * PI / 180.0;
	int ret;

	ret = video_modify_cci_reg(&cfg->i2c, SDE_CTRL0_CCI, BIT(0), BIT(0));
	if (ret < 0) {
		return ret;
	}

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
	} else {
		return -EINVAL;
	}

	struct video_reg16 hue_params[] = {
		{SDE_CTRL1_REG, abs(cos_coef)},
		{SDE_CTRL2_REG, abs(sin_coef)}
	};

	ret = video_modify_cci_reg(&cfg->i2c, SDE_CTRL10_CCI, 0x7F, sign);
	if (ret < 0) {
		return ret;
	}

	return video_write_cci_multiregs16(&cfg->i2c, hue_params, ARRAY_SIZE(hue_params));
}

static int ov5642_set_ctrl_saturation(const struct device *dev, int value)
{
	const struct ov5642_config *cfg = dev->config;

	struct video_reg16 saturation_params[] = {
		{SDE_CTRL3_REG, value},
		{SDE_CTRL4_REG, value}
	};

	return video_write_cci_multiregs16(&cfg->i2c, saturation_params,
					   ARRAY_SIZE(saturation_params));
}

static int ov5642_set_ctrl_contrast(const struct device *dev, int value)
{
	const struct ov5642_config *cfg = dev->config;
	int ret;

	ret = video_modify_cci_reg(&cfg->i2c, SDE_CTRL0_CCI, BIT(2), BIT(2));
	if (ret < 0) {
		return ret;
	}

	ret = video_modify_cci_reg(&cfg->i2c, SDE_CTRL8_CCI, BIT(2), BIT(2));
	if (ret < 0) {
		return ret;
	}

	return video_write_cci_reg(&cfg->i2c, SDE_CTRL8_CCI, value);
}

static int ov5642_set_ctrl_brightness(const struct device *dev, int value)
{
	const struct ov5642_config *cfg = dev->config;
	int ret;

	ret = video_modify_cci_reg(&cfg->i2c, SDE_CTRL0_CCI, BIT(2), BIT(2));

	if (ret < 0) {
		return ret;
	}

	ret = video_modify_cci_reg(&cfg->i2c, SDE_CTRL10_CCI, BIT(2),
				   value >= 0 ? 0 : BIT(2));
	if (ret < 0) {
		return ret;
	}

	return video_write_cci_reg(&cfg->i2c, SDE_CTRL9_CCI, (abs(value) << 4) & 0xf0);
}

static int ov5642_set_ctrl_exposure(const struct device *dev, int value)
{
	const struct ov5642_config *cfg = dev->config;
	int ret;

	switch (value) {
	case VIDEO_EXPOSURE_AUTO:
	case VIDEO_EXPOSURE_MANUAL:
		/* TODO: Add support for manual exposure mode */
		ret = video_modify_cci_reg(&cfg->i2c, AEC_PK_CCI, BIT(0), value);
		break;

	case VIDEO_EXPOSURE_SHUTTER_PRIORITY:
	case VIDEO_EXPOSURE_APERTURE_PRIORITY:
		return -ENOTSUP;

	default:
		CODE_UNREACHABLE;
	}

	return ret;
}

static int ov5642_set_ctrl(const struct device *dev, uint32_t id)
{
	struct ov5642_data *drv_data = dev->data;
	struct ov5642_ctrls *ctrls = &drv_data->ctrls;
	const struct ov5642_config *cfg = dev->config;

	switch (id) {
	case VIDEO_CID_HFLIP:
		return video_modify_cci_reg(&cfg->i2c, TIMING_TC_REG18_CCI,
					    BIT(6), ctrls->hflip.val ? BIT(6) : 0);
	case VIDEO_CID_VFLIP:
		return video_modify_cci_reg(&cfg->i2c, TIMING_TC_REG18_CCI, BIT(5),
					    ctrls->vflip.val ? BIT(5) : 0);
	case VIDEO_CID_EXPOSURE_AUTO:
		return ov5642_set_ctrl_exposure(dev, ctrls->ae.val);
	case VIDEO_CID_AUTO_WHITE_BALANCE:
		return video_modify_cci_reg(&cfg->i2c, ISP_CTRL01_CCI, BIT(0),
					    ctrls->awb.val ? BIT(0) : 0);
	case VIDEO_CID_AUTOGAIN:
		return video_modify_cci_reg(&cfg->i2c, AEC_PK_CCI, BIT(1),
					    ctrls->auto_gain.val ? 0 : BIT(1));
	case VIDEO_CID_BRIGHTNESS:
		return ov5642_set_ctrl_brightness(dev, ctrls->brightness.val);
	case VIDEO_CID_CONTRAST:
		return ov5642_set_ctrl_contrast(dev, ctrls->contrast.val);
	case VIDEO_CID_SATURATION:
		return ov5642_set_ctrl_saturation(dev, ctrls->saturation.val);
	case VIDEO_CID_HUE:
		return ov5642_set_ctrl_hue(dev, ctrls->hue.val);
	default:
		CODE_UNREACHABLE;
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
			      .min = 0,
			      .max = 1,
			      .step = 1,
			      .def = 0
			      });
	if (ret < 0) {
		return ret;
	}

	ret = video_init_ctrl(&ctrls->vflip, dev, VIDEO_CID_VFLIP,
			      (struct video_ctrl_range){
			      .min = 0,
			      .max = 1,
			      .step = 1,
			      .def = 0
			      });
	if (ret < 0) {
		return ret;
	}

	ret = video_init_menu_ctrl(&ctrls->ae, dev, VIDEO_CID_EXPOSURE_AUTO,
				   VIDEO_EXPOSURE_AUTO, NULL);
	if (ret < 0) {
		return ret;
	}

	ret = video_init_ctrl(&ctrls->awb, dev, VIDEO_CID_AUTO_WHITE_BALANCE,
			      (struct video_ctrl_range){
			      .min = 0,
			      .max = 1,
			      .step = 1,
			      .def = 1
			      });
	if (ret < 0) {
		return ret;
	}

	ret = video_init_ctrl(&ctrls->auto_gain, dev, VIDEO_CID_AUTOGAIN,
			      (struct video_ctrl_range){
			      .min = 0,
			      .max = 1,
			      .step = 1,
			      .def = 1
			      });
	if (ret < 0) {
		return ret;
	}

	ret = video_init_ctrl(&ctrls->brightness, dev, VIDEO_CID_BRIGHTNESS,
			      (struct video_ctrl_range){
			      .min = -15,
			      .max = 15,
			      .step = 1,
			      .def = 0
			      });
	if (ret < 0) {
		return ret;
	}

	ret = video_init_ctrl(&ctrls->contrast, dev, VIDEO_CID_CONTRAST,
			      (struct video_ctrl_range){
			      .min = 0,
			      .max = 255,
			      .step = 1,
			      .def = 0
			      });
	if (ret < 0) {
		return ret;
	}

	ret = video_init_ctrl(&ctrls->saturation, dev, VIDEO_CID_SATURATION,
			      (struct video_ctrl_range){
			      .min = 0,
			      .max = 255,
			      .step = 1,
			      .def = 64
			      });
	if (ret < 0) {
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
	uint32_t tmp;
	uint16_t chip_id;
	int ret;

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

/* Power up sequence */
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(powerdown_gpios)
	if (cfg->powerdown_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->powerdown_gpio)) {
			LOG_ERR("%s: device %s is not ready", dev->name,
				cfg->powerdown_gpio.port->name);
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&cfg->powerdown_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			return ret;
		}

		k_sleep(K_MSEC(5));

		gpio_pin_set_dt(&cfg->powerdown_gpio, 0);
	}
#endif

	k_sleep(K_MSEC(20));

	/* Check sensor chip id */
	ret = video_read_cci_reg(&cfg->i2c, CHIP_ID_CCI, &tmp);
	if (ret < 0) {
		LOG_ERR("Unable to read sensor chip ID, ret = %d", ret);
		return ret;
	}

	chip_id = tmp;

	if (chip_id != CHIP_ID_VAL) {
		LOG_ERR("Wrong chip ID: %04x (expected %04x)", chip_id, CHIP_ID_VAL);
		return -ENODEV;
	}

	k_sleep(K_MSEC(5));

	/* Initialize register values */
	ret = video_write_cci_multiregs16(&cfg->i2c, init_params_common,
					  ARRAY_SIZE(init_params_common));
	if (ret < 0) {
		LOG_ERR("Unable to initialize the sensor");
		return ret;
	}

	/* Explicitly set DVP mode if bus_type is not CSI2 */
	if (cfg->bus_type == VIDEO_BUS_TYPE_PARALLEL) {
		ret = video_write_cci_reg(&cfg->i2c, IO_MIPI_CTRL00_CCI,
					  IO_MIPI_CTRL00_DVP_MODE);
		if (ret < 0) {
			LOG_ERR("Failed to configure DVP mode");
			return ret;
		}
	} else {
		LOG_ERR("Bus type %d not supported", cfg->bus_type);
		return -ENOTSUP;
	}

	/* Initialize controls */
	return ov5642_init_controls(dev);
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
					VIDEO_BUS_TYPE_PARALLEL),		\
	};									\
										\
	DEVICE_DT_INST_DEFINE(n, &ov5642_init, NULL, &ov5642_data_##n,		\
			&ov5642_cfg_##n,					\
			POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY,		\
			&ov5642_driver_api);					\
										\
			VIDEO_DEVICE_DEFINE(ov5642_##n, DEVICE_DT_INST_GET(n), NULL);

DT_INST_FOREACH_STATUS_OKAY(OV5642_INIT)
