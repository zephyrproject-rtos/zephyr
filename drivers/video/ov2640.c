/*
 * SPDX-FileCopyrightText: Copyright (c) 2021 Antmicro <www.antmicro.com>
 * SPDX-FileCopyrightText: Copyright (c) 2026 Panoramix Labs
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ovti_ov2640

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/video.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/video/video.h>

#include "video_common.h"

LOG_MODULE_REGISTER(video_ov2640, CONFIG_VIDEO_LOG_LEVEL);

/* DSP register bank FF=0x00*/
#define QS     0x44
#define HSIZE  0x51
#define VSIZE  0x52
#define XOFFL  0x53
#define YOFFL  0x54
#define VHYX   0x55
#define TEST   0x57
#define ZMOW   0x5A
#define ZMOH   0x5B
#define ZMHH   0x5C
#define BPADDR 0x7C
#define BPDATA 0x7D
#define SIZEL  0x8C
#define HSIZE8 0xC0
#define VSIZE8 0xC1
#define CTRL1  0xC3

#define CTRLI       0x50
#define CTRLI_LP_DP 0x80

#define CTRL0        0xC2
#define CTRL0_YUV422 0x08
#define CTRL0_YUV_EN 0x04
#define CTRL0_RGB_EN 0x02

#define CTRL2           0x86
#define CTRL2_DCW_EN    0x20
#define CTRL2_SDE_EN    0x10
#define CTRL2_UV_ADJ_EN 0x08
#define CTRL2_UV_AVG_EN 0x04
#define CTRL2_CMX_EN    0x01

#define CTRL3              0x87
#define CTRL3_BPC_EN       0x80
#define CTRL3_WPC_EN       0x40
#define R_DVP_SP           0xD3
#define R_DVP_SP_AUTO_MODE 0x80

#define R_BYPASS           0x05
#define R_BYPASS_DSP_EN    0x00
#define R_BYPASS_DSP_BYPAS 0x01

#define IMAGE_MODE         0xDA
#define IMAGE_MODE_JPEG_EN (1U << 4)
#define IMAGE_MODE_DVP_MASK  GENMASK(3, 2)
#define IMAGE_MODE_DVP_RGB565 (0x2 << 2)
#define IMAGE_MODE_DVP_YUV422 (0x0 << 2)
#define IMAGE_MODE_DVP_RAW10  (0x0 << 2)
#define IMAGE_MODE_HREF_IS_VSYNC (1U << 1)
#define IMAGE_MODE_BYTE_SWAP  (1U << 0)

#define RESET      0xE0
#define RESET_JPEG 0x10
#define RESET_DVP  0x04

#define MC_BIST              0xF9
#define MC_BIST_RESET        0x80
#define MC_BIST_BOOT_ROM_SEL 0x40

#define BANK_SEL        0xFF
#define BANK_SEL_DSP    0x00
#define BANK_SEL_SENSOR 0x01

/* Sensor register bank FF=0x01*/
#define COM1        0x03
#define REG_PID     0x0A
#define REG_PID_VAL 0x26
#define REG_VER     0x0B
#define REG_VER_VAL 0x42
#define AEC         0x10
#define CLKRC       0x11
#define COM10       0x15
#define HREFST      0x17
#define HREFEND     0x18
#define VSTART      0x19
#define VSTOP       0x1A
#define AEW         0x24
#define AEB         0x25
#define ARCOM2      0x34
#define FLL         0x46
#define FLH         0x47
#define ADDVSL      0x2d
#define ADDVSH      0x2e
#define COM19       0x48
#define ZOOMS       0x49
#define BD50        0x4F
#define BD60        0x50
#define REG5D       0x5D
#define REG5E       0x5E
#define REG5F       0x5F
#define REG60       0x60
#define HISTO_LOW   0x61
#define HISTO_HIGH  0x62

#define REG04           0x04
#define REG04_DEFAULT   0x28
#define REG04_HFLIP_IMG 0x80
#define REG04_VFLIP_IMG 0x40
#define REG04_VREF_EN   0x10
#define REG04_HREF_EN   0x08
#define REG04_SET(x)    (REG04_DEFAULT | x)

#define COM2              0x09
#define COM2_OUTPUT_DRIVE_MASK GENMASK(1, 0)

#define COM3             0x0C
#define COM3_DEFAULT     0x38
#define COM3_BAND_AUTO   0x02
#define COM3_BAND_SET(x) (COM3_DEFAULT | x)

#define COM7           0x12
#define COM7_SRST      0x80
#define COM7_RES_UXGA  0x00 /* UXGA */
#define COM7_ZOOM_EN   0x04 /* Enable Zoom */
#define COM7_COLOR_BAR 0x02 /* Enable Color Bar Test */

#define COM8         0x13
#define COM8_DEFAULT 0xC0
#define COM8_BNDF_EN 0x20 /* Enable Banding filter */
#define COM8_AGC_EN  0x04 /* AGC Auto/Manual control selection */
#define COM8_AEC_EN  0x01 /* Auto/Manual Exposure control */
#define COM8_SET(x)  (COM8_DEFAULT | x)

#define COM9             0x14 /* AGC gain ceiling */
#define COM9_DEFAULT     0x08
#define COM9_AGC_GAIN_8x 0x02 /* AGC:    8x */
#define COM9_AGC_SET(x)  (COM9_DEFAULT | (x << 5))

#define COM10 0x15

#define CTRL1_AWB 0x08 /* Enable AWB */

#define VV                  0x26
#define VV_AGC_TH_SET(h, l) ((h << 4) | (l & 0x0F))

#define REG32               0x32

#define OV2640_BASE_FPS 15

/* Configuration arrays */
#define SVGA_HSIZE (800)
#define SVGA_VSIZE (600)

#define UXGA_HSIZE (1600)
#define UXGA_VSIZE (1200)

struct ov2640_config {
	struct i2c_dt_spec i2c;
#if DT_INST_NODE_HAS_PROP(0, reset_gpios)
	struct gpio_dt_spec reset_gpio;
#endif
#if DT_INST_NODE_HAS_PROP(0, pwdn_gpios)
	struct gpio_dt_spec pwdn_gpio;
#endif
	uint8_t clock_multiplier;
	uint8_t drive_strength;
	bool jpeg_hsync;
};

struct ov2640_ctrls {
	struct video_ctrl hflip;
	struct video_ctrl vflip;
	struct video_ctrl ae;
	struct video_ctrl awb;
	struct video_ctrl gain;
	struct video_ctrl contrast;
	struct video_ctrl brightness;
	struct video_ctrl saturation;
	struct video_ctrl jpeg;
	struct video_ctrl test_pattern;
	struct video_ctrl link_freq;
};

struct ov2640_data {
	struct ov2640_ctrls ctrls;
	struct video_format fmt;
	uint64_t frmival_msec;
	uint8_t clock_divider;
};

struct ov2640_reg {
	uint8_t addr;
	uint8_t value;
};

const int64_t ov2640_link_freq[] = {
	MHZ(168), MHZ(72), MHZ(36)
};

static const uint8_t ov2640_clock_dividers[] = { 1, 2, 4 };

static const struct ov2640_reg default_regs[] = {
	/* Minimal init sequence */
	{ BANK_SEL, BANK_SEL_DSP },
	{ 0x2c, 0xff },
	{ 0x2e, 0xdf },
	{ BANK_SEL, BANK_SEL_SENSOR },
	{ 0xff, 0x01 },
	{ 0x3c, 0x32 },
	{ 0x11, 0x80 },

	{COM2, 0x00}, /* Output drive x2 */

	{0x2c, 0x0c},
	{0x33, 0x78},
	{0x3a, 0x33},
	{0x3b, 0xfb},
	{0x3e, 0x00},
	{0x43, 0x11},
	{0x16, 0x10},
	{0x39, 0x02},
	{0x35, 0x88},
	{0x22, 0x0a},
	{0x37, 0x40},
	{0x23, 0x00},
	{ARCOM2, 0xa0},
	{0x06, 0x02},
	{0x06, 0x88},
	{0x07, 0xc0},
	{0x0d, 0xb7},
	{0x0e, 0x01},
	{0x4c, 0x00},
	{0x4a, 0x81},
	{0x21, 0x99},
	{AEW, 0x40},
	{AEB, 0x38},
	/* AGC/AEC fast mode operating region */
	{VV, VV_AGC_TH_SET(0x08, 0x02)},
	{0x5c, 0x00},
	{0x63, 0x00},

	/* Set banding filter */
	{COM3, COM3_BAND_SET(COM3_BAND_AUTO)},
	{REG5D, 0x55},
	{REG5E, 0x7d},
	{REG5F, 0x7d},
	{REG60, 0x55},
	{HISTO_LOW, 0x70},
	{HISTO_HIGH, 0x80},
	{0x7c, 0x05},
	{0x20, 0x80},
	{0x28, 0x30},
	{0x6c, 0x00},
	{0x6d, 0x80},
	{0x6e, 0x00},
	{0x70, 0x02},
	{0x71, 0x94},
	{0x73, 0xc1},
	{0x3d, 0x34},
	{0x5a, 0x57},
	{BD50, 0xbb},
	{BD60, 0x9c},

	{BANK_SEL, BANK_SEL_DSP},
	{0xe5, 0x7f},
	{MC_BIST, MC_BIST_RESET | MC_BIST_BOOT_ROM_SEL},
	{0x41, 0x24},
	{RESET, RESET_JPEG | RESET_DVP},

	{0x76, 0xff},
	{0x33, 0xa0},
	{0x42, 0x20},
	{0x43, 0x18},
	{0x4c, 0x00},
	{CTRL3, CTRL3_BPC_EN | CTRL3_WPC_EN | 0x10},
	{0x88, 0x3f},
	{0xd7, 0x03},
	{0xd9, 0x10},
	{R_DVP_SP, R_DVP_SP_AUTO_MODE | 0x2},
	{0xc8, 0x08},
	{0xc9, 0x80},
	{BPADDR, 0x00},
	{BPDATA, 0x00},
	{BPADDR, 0x03},
	{BPDATA, 0x48},
	{BPDATA, 0x48},
	{BPADDR, 0x08},
	{BPDATA, 0x20},
	{BPDATA, 0x10},
	{BPDATA, 0x0e},
	{0x90, 0x00},
	{0x91, 0x0e},
	{0x91, 0x1a},
	{0x91, 0x31},
	{0x91, 0x5a},
	{0x91, 0x69},
	{0x91, 0x75},
	{0x91, 0x7e},
	{0x91, 0x88},
	{0x91, 0x8f},
	{0x91, 0x96},
	{0x91, 0xa3},
	{0x91, 0xaf},
	{0x91, 0xc4},
	{0x91, 0xd7},
	{0x91, 0xe8},
	{0x91, 0x20},
	{0x92, 0x00},
	{0x93, 0x06},
	{0x93, 0xe3},
	{0x93, 0x03},
	{0x93, 0x03},
	{0x93, 0x00},
	{0x93, 0x02},
	{0x93, 0x00},
	{0x93, 0x00},
	{0x93, 0x00},
	{0x93, 0x00},
	{0x93, 0x00},
	{0x93, 0x00},
	{0x93, 0x00},
	{0x96, 0x00},
	{0x97, 0x08},
	{0x97, 0x19},
	{0x97, 0x02},
	{0x97, 0x0c},
	{0x97, 0x24},
	{0x97, 0x30},
	{0x97, 0x28},
	{0x97, 0x26},
	{0x97, 0x02},
	{0x97, 0x98},
	{0x97, 0x80},
	{0x97, 0x00},
	{0x97, 0x00},
	{0xa4, 0x00},
	{0xa8, 0x00},
	{0xc5, 0x11},
	{0xc6, 0x51},
	{0xbf, 0x80},
	{0xc7, 0x10},
	{0xb6, 0x66},
	{0xb8, 0xA5},
	{0xb7, 0x64},
	{0xb9, 0x7C},
	{0xb3, 0xaf},
	{0xb4, 0x97},
	{0xb5, 0xFF},
	{0xb0, 0xC5},
	{0xb1, 0x94},
	{0xb2, 0x0f},
	{0xc4, 0x5c},
	{0xa6, 0x00},
	{0xa7, 0x20},
	{0xa7, 0xd8},
	{0xa7, 0x1b},
	{0xa7, 0x31},
	{0xa7, 0x00},
	{0xa7, 0x18},
	{0xa7, 0x20},
	{0xa7, 0xd8},
	{0xa7, 0x19},
	{0xa7, 0x31},
	{0xa7, 0x00},
	{0xa7, 0x18},
	{0xa7, 0x20},
	{0xa7, 0xd8},
	{0xa7, 0x19},
	{0xa7, 0x31},
	{0xa7, 0x00},
	{0xa7, 0x18},
	{0x7f, 0x00},
	{0xe5, 0x1f},
	{0xe1, 0x77},
	{0xdd, 0x7f},
	{CTRL0, CTRL0_YUV422 | CTRL0_YUV_EN | CTRL0_RGB_EN},
};

static const struct ov2640_reg uxga_regs[] = {
	{BANK_SEL, BANK_SEL_SENSOR},
	/* DSP input image resolution and window size control */
	{COM7, COM7_RES_UXGA},
	{COM1, 0x0F},        /* UXGA=0x0F, SVGA=0x0A, CIF=0x06 */

	{HREFST, 0x11}, /* UXGA=0x11, SVGA/CIF=0x11 */
	{HREFEND, 0x75},  /* UXGA=0x75, SVGA/CIF=0x43 */

	{VSTART, 0x01}, /* UXGA=0x01, SVGA/CIF=0x00 */
	{VSTOP, 0x97},  /* UXGA=0x97, SVGA/CIF=0x4b */
	{0x3d, 0x34},   /* UXGA=0x34, SVGA/CIF=0x38 */

	{0x35, 0x88},
	{0x22, 0x0a},
	{0x37, 0x40},
	{0x34, 0xa0},
	{0x06, 0x02},
	{0x0d, 0xb7},
	{0x0e, 0x01},
	{0x42, 0x83},

	/*
	 * Set DSP input image size and offset.
	 * The sensor output image can be scaled with OUTW/OUTH
	 */
	{BANK_SEL, BANK_SEL_DSP},
	{R_BYPASS, R_BYPASS_DSP_BYPAS},

	{RESET, RESET_DVP},
	{HSIZE8, (UXGA_HSIZE >> 3)}, /* Image Horizontal Size HSIZE[10:3] */
	{VSIZE8, (UXGA_VSIZE >> 3)}, /* Image Vertical Size VSIZE[10:3] */

	/* {HSIZE[11], HSIZE[2:0], VSIZE[2:0]} */
	{SIZEL, ((UXGA_HSIZE >> 6) & 0x40) | ((UXGA_HSIZE & 0x7) << 3) | (UXGA_VSIZE & 0x7)},

	{XOFFL, 0x00},                       /* OFFSET_X[7:0] */
	{YOFFL, 0x00},                       /* OFFSET_Y[7:0] */
	{HSIZE, ((UXGA_HSIZE >> 2) & 0xFF)}, /* H_SIZE[7:0] real/4 */
	{VSIZE, ((UXGA_VSIZE >> 2) & 0xFF)}, /* V_SIZE[7:0] real/4 */

	/* V_SIZE[8]/OFFSET_Y[10:8]/H_SIZE[8]/OFFSET_X[10:8] */
	{VHYX, ((UXGA_VSIZE >> 3) & 0x80) | ((UXGA_HSIZE >> 7) & 0x08)},
	{TEST, (UXGA_HSIZE >> 4) & 0x80}, /* H_SIZE[9] */

	{CTRL2, CTRL2_DCW_EN | CTRL2_SDE_EN | CTRL2_UV_AVG_EN | CTRL2_CMX_EN | CTRL2_UV_ADJ_EN},

	/* H_DIVIDER/V_DIVIDER */
	{CTRLI, CTRLI_LP_DP | 0x00},
	/* DVP prescaler */
	{R_DVP_SP, R_DVP_SP_AUTO_MODE | 0x04},


	{BANK_SEL, BANK_SEL_SENSOR},
	/* Ungated PCLK, VSYNC active-low, HSYNC active-high */
	{COM10, 0U << 1},
	{BANK_SEL, BANK_SEL_DSP},

	{R_BYPASS, R_BYPASS_DSP_EN},
	{RESET, 0x00},
};

#define NUM_BRIGHTNESS_LEVELS (5)
static const uint8_t brightness_regs[NUM_BRIGHTNESS_LEVELS + 1][5] = {
	{BPADDR, BPDATA, BPADDR, BPDATA, BPDATA}, /* reg addr */
	{0x00, 0x04, 0x09, 0x00, 0x00},           /* -2 */
	{0x00, 0x04, 0x09, 0x10, 0x00},           /* -1 */
	{0x00, 0x04, 0x09, 0x20, 0x00},           /*  0 */
	{0x00, 0x04, 0x09, 0x30, 0x00},           /* +1 */
	{0x00, 0x04, 0x09, 0x40, 0x00},           /* +2 */
};

#define NUM_CONTRAST_LEVELS (5)
static const uint8_t contrast_regs[NUM_CONTRAST_LEVELS + 1][7] = {
	{BPADDR, BPDATA, BPADDR, BPDATA, BPDATA, BPDATA, BPDATA}, /* reg addr */
	{0x00, 0x04, 0x07, 0x20, 0x18, 0x34, 0x06},               /* -2 */
	{0x00, 0x04, 0x07, 0x20, 0x1c, 0x2a, 0x06},               /* -1 */
	{0x00, 0x04, 0x07, 0x20, 0x20, 0x20, 0x06},               /*  0 */
	{0x00, 0x04, 0x07, 0x20, 0x24, 0x16, 0x06},               /* +1 */
	{0x00, 0x04, 0x07, 0x20, 0x28, 0x0c, 0x06},               /* +2 */
};

#define NUM_SATURATION_LEVELS (5)
static const uint8_t saturation_regs[NUM_SATURATION_LEVELS + 1][5] = {
	{BPADDR, BPDATA, BPADDR, BPDATA, BPDATA}, /* reg addr */
	{0x00, 0x02, 0x03, 0x28, 0x28},           /* -2 */
	{0x00, 0x02, 0x03, 0x38, 0x38},           /* -1 */
	{0x00, 0x02, 0x03, 0x48, 0x48},           /*  0 */
	{0x00, 0x02, 0x03, 0x58, 0x58},           /* +1 */
	{0x00, 0x02, 0x03, 0x58, 0x58},           /* +2 */
};

#define OV2640_VIDEO_FORMAT_CAP(pixfmt)                                                            \
	{                                                                                          \
		.pixelformat = (pixfmt),                                                           \
		.width_min = 16, .width_max = 1600, .width_step = 4,                               \
		.height_min = 12, .height_max = 1200, .height_step = 1,                            \
	}

static const struct video_format_cap fmts[] = {
	OV2640_VIDEO_FORMAT_CAP(VIDEO_PIX_FMT_RGB565),
	OV2640_VIDEO_FORMAT_CAP(VIDEO_PIX_FMT_RGB565X),
	OV2640_VIDEO_FORMAT_CAP(VIDEO_PIX_FMT_YUYV),
	OV2640_VIDEO_FORMAT_CAP(VIDEO_PIX_FMT_UYVY),
	OV2640_VIDEO_FORMAT_CAP(VIDEO_PIX_FMT_JPEG),
	{0},
};

static int ov2640_write_reg(const struct i2c_dt_spec *spec, uint8_t reg_addr, uint8_t value)
{
	uint8_t tries = 3;

	/**
	 * It rarely happens that the camera does not respond with ACK signal.
	 * In that case it usually responds on 2nd try but there is a 3rd one
	 * just to be sure that the connection error is not caused by driver
	 * itself.
	 */
	while (tries-- > 0) {
		if (!i2c_reg_write_byte_dt(spec, reg_addr, value)) {
			return 0;
		}
		/* If writing failed wait 5ms before next attempt */
		k_msleep(5);
	}
	LOG_ERR("failed to write 0x%x to 0x%x", value, reg_addr);

	return -1;
}

static int ov2640_read_reg(const struct i2c_dt_spec *spec, uint8_t reg_addr)
{
	uint8_t tries = 3;
	uint8_t value;

	/**
	 * It rarely happens that the camera does not respond with ACK signal.
	 * In that case it usually responds on 2nd try but there is a 3rd one
	 * just to be sure that the connection error is not caused by driver
	 * itself.
	 */
	while (tries-- > 0) {
		if (!i2c_reg_read_byte_dt(spec, reg_addr, &value)) {
			return value;
		}
		/* If reading failed wait 5ms before next attempt */
		k_msleep(5);
	}
	LOG_ERR("failed to read 0x%x register", reg_addr);

	return -1;
}

static int ov2640_write_all(const struct device *dev, const struct ov2640_reg *regs,
			    uint16_t reg_num)
{
	uint16_t i = 0;
	const struct ov2640_config *cfg = dev->config;

	for (i = 0; i < reg_num; i++) {
		int err;

		err = ov2640_write_reg(&cfg->i2c, regs[i].addr, regs[i].value);
		if (err) {
			return err;
		}
	}

	return 0;
}

static int ov2640_soft_reset(const struct device *dev)
{
	int ret = 0;
	const struct ov2640_config *cfg = dev->config;

	/* Switch to DSP register bank */
	ret |= ov2640_write_reg(&cfg->i2c, BANK_SEL, BANK_SEL_SENSOR);

	/* Initiate system reset */
	ret |= ov2640_write_reg(&cfg->i2c, COM7, COM7_SRST);

	return ret;
}

static int ov2640_set_level(const struct device *dev, int level, int max_level, int cols,
			    const uint8_t regs[][cols])
{
	int ret = 0;
	const struct ov2640_config *cfg = dev->config;

	level += max_level / 2 + 1;

	/* Switch to DSP register bank */
	ret |= ov2640_write_reg(&cfg->i2c, BANK_SEL, BANK_SEL_DSP);

	for (int i = 0; i < (ARRAY_SIZE(regs[0]) / sizeof(regs[0][0])); i++) {
		ret |= ov2640_write_reg(&cfg->i2c, regs[0][i], regs[level][i]);
	}

	return ret;
}

static int ov2640_set_quality(const struct device *dev, int qs)
{
	int ret = 0;
	const struct ov2640_config *cfg = dev->config;

	/* Switch to DSP register bank */
	ret |= ov2640_write_reg(&cfg->i2c, BANK_SEL, BANK_SEL_DSP);

	/* Write QS register */
	ret |= ov2640_write_reg(&cfg->i2c, QS, qs);

	return ret;
}

static int ov2640_set_colorbar(const struct device *dev, uint8_t enable)
{
	int ret = 0;
	const struct ov2640_config *cfg = dev->config;

	uint8_t reg;

	/* Switch to SENSOR register bank */
	ret |= ov2640_write_reg(&cfg->i2c, BANK_SEL, BANK_SEL_SENSOR);

	/* Update COM7 to enable/disable color bar test pattern */

	reg = ov2640_read_reg(&cfg->i2c, COM7);

	if (enable) {
		reg |= COM7_COLOR_BAR;
	} else {
		reg &= ~COM7_COLOR_BAR;
	}

	ret |= ov2640_write_reg(&cfg->i2c, COM7, reg);

	return ret;
}

static int ov2640_set_white_bal(const struct device *dev, int enable)
{
	int ret = 0;
	const struct ov2640_config *cfg = dev->config;

	uint8_t reg;

	/* Switch to SENSOR register bank */
	ret |= ov2640_write_reg(&cfg->i2c, BANK_SEL, BANK_SEL_SENSOR);

	/* Update CTRL1 to enable/disable automatic white balance*/
	reg = ov2640_read_reg(&cfg->i2c, CTRL1);

	if (enable) {
		reg |= CTRL1_AWB;
	} else {
		reg &= ~CTRL1_AWB;
	}

	ret |= ov2640_write_reg(&cfg->i2c, CTRL1, reg);

	return ret;
}

static int ov2640_set_gain_ctrl(const struct device *dev, int enable)
{
	int ret = 0;
	const struct ov2640_config *cfg = dev->config;

	uint8_t reg;

	/* Switch to SENSOR register bank */
	ret |= ov2640_write_reg(&cfg->i2c, BANK_SEL, BANK_SEL_SENSOR);

	/* Update COM8 to enable/disable automatic gain control */
	reg = ov2640_read_reg(&cfg->i2c, COM8);

	if (enable) {
		reg |= COM8_AGC_EN;
	} else {
		reg &= ~COM8_AGC_EN;
	}

	ret |= ov2640_write_reg(&cfg->i2c, COM8, reg);

	return ret;
}

static int ov2640_set_exposure_ctrl(const struct device *dev, int enable)
{
	int ret = 0;
	const struct ov2640_config *cfg = dev->config;

	uint8_t reg;

	/* Switch to SENSOR register bank */
	ret |= ov2640_write_reg(&cfg->i2c, BANK_SEL, BANK_SEL_SENSOR);

	/* Update COM8  to enable/disable automatic exposure control */
	reg = ov2640_read_reg(&cfg->i2c, COM8);

	if (enable) {
		reg |= COM8_AEC_EN;
	} else {
		reg &= ~COM8_AEC_EN;
	}

	ret |= ov2640_write_reg(&cfg->i2c, COM8, reg);

	return ret;
}

static int ov2640_set_horizontal_mirror(const struct device *dev, int enable)
{
	int ret = 0;
	const struct ov2640_config *cfg = dev->config;

	uint8_t reg;

	/* Switch to SENSOR register bank */
	ret |= ov2640_write_reg(&cfg->i2c, BANK_SEL, BANK_SEL_SENSOR);

	/* Update REG04 to enable/disable horizontal mirror */
	reg = ov2640_read_reg(&cfg->i2c, REG04);

	if (enable) {
		reg |= REG04_HFLIP_IMG;
	} else {
		reg &= ~REG04_HFLIP_IMG;
	}

	ret |= ov2640_write_reg(&cfg->i2c, REG04, reg);

	return ret;
}

static int ov2640_set_vertical_flip(const struct device *dev, int enable)
{
	int ret = 0;
	const struct ov2640_config *cfg = dev->config;

	uint8_t reg;

	/* Switch to SENSOR register bank */
	ret |= ov2640_write_reg(&cfg->i2c, BANK_SEL, BANK_SEL_SENSOR);

	/* Update REG04 to enable/disable vertical flip */
	reg = ov2640_read_reg(&cfg->i2c, REG04);

	if (enable) {
		reg |= REG04_VFLIP_IMG | REG04_VREF_EN;
	} else {
		reg &= ~(REG04_VFLIP_IMG | REG04_VREF_EN);
	}

	ret |= ov2640_write_reg(&cfg->i2c, REG04, reg);

	return ret;
}

static int ov2640_apply_config(const struct device *dev)
{
	const struct ov2640_config *cfg = dev->config;
	struct ov2640_data *data = dev->data;
	int val;
	int ret = 0;

	/* Disable DSP */

	ret |= ov2640_write_reg(&cfg->i2c, BANK_SEL, BANK_SEL_DSP);
	if (ret < 0) {
		return ret;
	}

	ret = ov2640_write_reg(&cfg->i2c, R_BYPASS, R_BYPASS_DSP_BYPAS);
	if (ret < 0) {
		return ret;
	}

	ret = ov2640_write_reg(&cfg->i2c, RESET, RESET_JPEG | RESET_DVP);
	if (ret < 0) {
		return ret;
	}

	/* Pixel format */

	switch (data->fmt.pixelformat) {
	case VIDEO_PIX_FMT_JPEG:
		if (cfg->jpeg_hsync) {
			ret = ov2640_write_reg(&cfg->i2c, IMAGE_MODE,
					       IMAGE_MODE_JPEG_EN | IMAGE_MODE_HREF_IS_VSYNC);
		} else {
			ret = ov2640_write_reg(
				&cfg->i2c, IMAGE_MODE, IMAGE_MODE_JPEG_EN);
		}
		break;
	case VIDEO_PIX_FMT_RGB565:
		ret = ov2640_write_reg(
			&cfg->i2c, IMAGE_MODE, IMAGE_MODE_DVP_RGB565);
		break;
	case VIDEO_PIX_FMT_RGB565X:
		ret = ov2640_write_reg(
			&cfg->i2c, IMAGE_MODE, IMAGE_MODE_DVP_RGB565 | IMAGE_MODE_BYTE_SWAP);
		break;
	case VIDEO_PIX_FMT_YUYV:
		ret = ov2640_write_reg(
			&cfg->i2c, IMAGE_MODE, IMAGE_MODE_DVP_YUV422);
		break;
	case VIDEO_PIX_FMT_UYVY:
		ret = ov2640_write_reg(
			&cfg->i2c, IMAGE_MODE, IMAGE_MODE_DVP_YUV422 | IMAGE_MODE_BYTE_SWAP);
		break;
	default:
		CODE_UNREACHABLE;
	}
	if (ret < 0) {
		return ret;
	}

	/* Output width */

	ret = ov2640_write_reg(&cfg->i2c, ZMOW, (data->fmt.width / 4) & 0xFF);
	if (ret < 0) {
		return ret;
	}

	ret = ov2640_write_reg(&cfg->i2c, ZMOH, (data->fmt.height / 4) & 0xFF);
	if (ret < 0) {
		return ret;
	}

	ret = ov2640_write_reg(&cfg->i2c, ZMHH,
				(((data->fmt.height / 4) >> 8) & 0x1) << 2
				| (((data->fmt.width / 4) >> 8) & 0x3) << 0);
	if (ret < 0) {
		return ret;
	}

	ret = ov2640_write_reg(&cfg->i2c, BANK_SEL, BANK_SEL_SENSOR);
	if (ret < 0) {
		return ret;
	}

	/* Frame interval */

	LOG_INF("Applying clock multiplier %u, clock divider %u",
		cfg->clock_multiplier, data->clock_divider);

	ret = ov2640_write_reg(&cfg->i2c, CLKRC,
		(cfg->clock_multiplier - 1) << 7 | (data->clock_divider - 1));
	if (ret < 0) {
		return ret;
	}

	/* Wait the new clock to stabilize */
	k_sleep(K_MSEC(1));

	/* DSP configuration in UXGA */

	ret = ov2640_write_all(dev, uxga_regs, ARRAY_SIZE(uxga_regs));
	if (ret < 0) {
		return ret;
	}

	/* Enable DSP */
	ret = ov2640_write_reg(&cfg->i2c, BANK_SEL, BANK_SEL_DSP);
	if (ret < 0) {
		return ret;
	}

	ret = ov2640_write_reg(&cfg->i2c, R_BYPASS, R_BYPASS_DSP_EN);
	if (ret < 0) {
		return ret;
	}

	k_msleep(30);

	return ret;
}

uint8_t ov2640_check_connection(const struct device *dev)
{
	int ret = 0;
	const struct ov2640_config *cfg = dev->config;

	uint8_t reg_pid_val, reg_ver_val;

	ret |= ov2640_write_reg(&cfg->i2c, BANK_SEL, BANK_SEL_SENSOR);
	reg_pid_val = ov2640_read_reg(&cfg->i2c, REG_PID);
	reg_ver_val = ov2640_read_reg(&cfg->i2c, REG_VER);

	if (REG_PID_VAL != reg_pid_val || REG_VER_VAL != reg_ver_val) {
		LOG_ERR("OV2640 not detected\n");
		return -ENODEV;
	}

	return ret;
}

static int ov2640_set_format(const struct device *dev, struct video_format *fmt)
{
	struct ov2640_data *drv_data = dev->data;
	uint32_t index;
	int ret = 0;

	if (!memcmp(&drv_data->fmt, fmt, sizeof(drv_data->fmt))) {
		/* nothing to do */
		return 0;
	}

	ret = video_format_caps_index(fmts, fmt, &index);
	if (ret < 0) {
		LOG_ERR("Format %s %ux%u not supported",
			VIDEO_FOURCC_TO_STR(fmt->pixelformat), fmt->width, fmt->height);
		return ret;
	}

	drv_data->fmt = *fmt;

	return ov2640_apply_config(dev);
}

static int ov2640_get_format(const struct device *dev, struct video_format *fmt)
{
	struct ov2640_data *drv_data = dev->data;

	*fmt = drv_data->fmt;

	return 0;
}

static int ov2640_enum_frmival(const struct device *dev, struct video_frmival_enum *fie)
{
	const struct ov2640_config *cfg = dev->config;

	if (fie->index >= ARRAY_SIZE(ov2640_clock_dividers)) {
		return -EINVAL;
	}

	fie->type = VIDEO_FRMIVAL_TYPE_DISCRETE;
	fie->discrete.numerator = (uint64_t)(USEC_PER_SEC / OV2640_BASE_FPS)
		/ cfg->clock_multiplier * ov2640_clock_dividers[fie->index];
	fie->discrete.denominator = USEC_PER_SEC;

	return 0;
}

static int ov2640_set_frmival(const struct device *dev, struct video_frmival *frmival)
{
	struct ov2640_data *data = dev->data;
	struct video_frmival_enum fie = {
		.discrete = *frmival,
		.type = VIDEO_FRMIVAL_TYPE_DISCRETE,
		.format = &data->fmt,
	};
	int ret;

	ret = video_closest_frmival(dev, &fie);
	if (ret < 0) {
		return ret;
	}

	data->frmival_msec = video_frmival_nsec(&fie.discrete) / NSEC_PER_MSEC;
	data->clock_divider = ov2640_clock_dividers[fie.index];

	return ov2640_apply_config(dev);
}

static int ov2640_get_frmival(const struct device *dev, struct video_frmival *frmival)
{
	struct ov2640_data *data = dev->data;

	frmival->numerator = data->frmival_msec;
	frmival->denominator = MSEC_PER_SEC;

	return 0;
}

static int ov2640_set_stream(const struct device *dev, bool enable, enum video_buf_type type)
{
	return 0;
}

static int ov2640_get_caps(const struct device *dev, struct video_caps *caps)
{
	caps->format_caps = fmts;
	return 0;
}

static int ov2640_set_ctrl(const struct device *dev, uint32_t id)
{
	struct ov2640_data *drv_data = dev->data;
	struct ov2640_ctrls *ctrls = &drv_data->ctrls;

	switch (id) {
	case VIDEO_CID_HFLIP:
		return ov2640_set_horizontal_mirror(dev, ctrls->hflip.val);
	case VIDEO_CID_VFLIP:
		return ov2640_set_vertical_flip(dev, ctrls->vflip.val);
	case VIDEO_CID_EXPOSURE:
		return ov2640_set_exposure_ctrl(dev, ctrls->ae.val);
	case VIDEO_CID_WHITE_BALANCE_TEMPERATURE:
		return ov2640_set_white_bal(dev, ctrls->awb.val);
	case VIDEO_CID_GAIN:
		return ov2640_set_gain_ctrl(dev, ctrls->gain.val);
	case VIDEO_CID_BRIGHTNESS:
		return ov2640_set_level(dev, ctrls->brightness.val, NUM_BRIGHTNESS_LEVELS,
					ARRAY_SIZE(brightness_regs[0]), brightness_regs);
	case VIDEO_CID_CONTRAST:
		return ov2640_set_level(dev, ctrls->contrast.val, NUM_CONTRAST_LEVELS,
					ARRAY_SIZE(contrast_regs[0]), contrast_regs);
	case VIDEO_CID_SATURATION:
		return ov2640_set_level(dev, ctrls->saturation.val, NUM_SATURATION_LEVELS,
					ARRAY_SIZE(saturation_regs[0]), saturation_regs);
	case VIDEO_CID_JPEG_COMPRESSION_QUALITY:
		return ov2640_set_quality(dev, ctrls->jpeg.val);
	case VIDEO_CID_TEST_PATTERN:
		return ov2640_set_colorbar(dev, ctrls->test_pattern.val);
	default:
		return -ENOTSUP;
	}
}

static DEVICE_API(video, ov2640_driver_api) = {
	.get_caps = ov2640_get_caps,
	.set_format = ov2640_set_format,
	.get_format = ov2640_get_format,
	.enum_frmival = ov2640_enum_frmival,
	.set_frmival = ov2640_set_frmival,
	.get_frmival = ov2640_get_frmival,
	.set_stream = ov2640_set_stream,
	.set_ctrl = ov2640_set_ctrl,
};

static int ov2640_init_controls(const struct device *dev)
{
	int ret;
	struct ov2640_data *drv_data = dev->data;
	struct ov2640_ctrls *ctrls = &drv_data->ctrls;

	ret = video_init_ctrl(&ctrls->hflip, dev, VIDEO_CID_HFLIP,
			      (struct video_ctrl_range){.min = 0, .max = 1, .step = 1, .def = 0});
	if (ret) {
		return ret;
	}

	ret = video_init_ctrl(&ctrls->vflip, dev, VIDEO_CID_VFLIP,
			      (struct video_ctrl_range){.min = 0, .max = 1, .step = 1, .def = 0});
	if (ret) {
		return ret;
	}

	ret = video_init_ctrl(&ctrls->ae, dev, VIDEO_CID_EXPOSURE,
			      (struct video_ctrl_range){.min = 0, .max = 1, .step = 1, .def = 1});
	if (ret) {
		return ret;
	}

	ret = video_init_ctrl(&ctrls->awb, dev, VIDEO_CID_WHITE_BALANCE_TEMPERATURE,
			      (struct video_ctrl_range){.min = 0, .max = 1, .step = 1, .def = 1});
	if (ret) {
		return ret;
	}

	ret = video_init_ctrl(&ctrls->gain, dev, VIDEO_CID_GAIN,
			      (struct video_ctrl_range){.min = 0, .max = 1, .step = 1, .def = 1});
	if (ret) {
		return ret;
	}

	ret = video_init_ctrl(&ctrls->brightness, dev, VIDEO_CID_BRIGHTNESS,
			      (struct video_ctrl_range){.min = -2, .max = 2, .step = 1, .def = 0});
	if (ret) {
		return ret;
	}

	ret = video_init_ctrl(&ctrls->contrast, dev, VIDEO_CID_CONTRAST,
			      (struct video_ctrl_range){.min = -2, .max = 2, .step = 1, .def = 0});
	if (ret) {
		return ret;
	}

	ret = video_init_ctrl(&ctrls->saturation, dev, VIDEO_CID_SATURATION,
			      (struct video_ctrl_range){.min = -2, .max = 2, .step = 1, .def = 0});
	if (ret) {
		return ret;
	}

	ret = video_init_ctrl(
		&ctrls->jpeg, dev, VIDEO_CID_JPEG_COMPRESSION_QUALITY,
		(struct video_ctrl_range){.min = 5, .max = 100, .step = 1, .def = 50});
	if (ret) {
		return ret;
	}

	ret = video_init_ctrl(&ctrls->test_pattern, dev, VIDEO_CID_TEST_PATTERN,
			       (struct video_ctrl_range){.min = 0, .max = 1, .step = 1, .def = 0});
	if (ret) {
		return ret;
	}

	ret = video_init_int_menu_ctrl(&ctrls->link_freq, dev, VIDEO_CID_LINK_FREQ,
				       0, ov2640_link_freq, ARRAY_SIZE(ov2640_link_freq));
	if (ret < 0) {
		return ret;
	}
	ctrls->link_freq.flags |= VIDEO_CTRL_FLAG_READ_ONLY;

	return 0;
}

static int ov2640_init(const struct device *dev)
{

#if DT_INST_NODE_HAS_PROP(0, pwdn_gpios) || DT_INST_NODE_HAS_PROP(0, reset_gpios)
	const struct ov2640_config *cfg = dev->config;
#endif
	struct video_format fmt = {
		.pixelformat = VIDEO_PIX_FMT_RGB565,
		.width = SVGA_HSIZE,
		.height = SVGA_VSIZE,
	};
	struct video_frmival frmival = {
		.numerator = 1,
		.denominator = 100,
	};
	int ret = 0;

#if DT_INST_NODE_HAS_PROP(0, pwdn_gpios)
	ret = gpio_pin_configure_dt(&cfg->pwdn_gpio, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		return ret;
	}

	k_sleep(K_MSEC(1));
#endif

#if DT_INST_NODE_HAS_PROP(0, reset_gpios)
	ret = gpio_pin_configure_dt(&cfg->reset_gpio, GPIO_OUTPUT_ACTIVE);
	if (ret) {
		return ret;
	}

	k_sleep(K_MSEC(1));
	gpio_pin_set_dt(&cfg->reset_gpio, 0);
	k_sleep(K_MSEC(1));
#endif

	ret = ov2640_check_connection(dev);

	if (ret) {
		return ret;
	}

	ov2640_soft_reset(dev);
	k_msleep(300);

	ov2640_write_all(dev, default_regs, ARRAY_SIZE(default_regs));

	ret = ov2640_set_format(dev, &fmt);
	if (ret) {
		LOG_ERR("Unable to configure default format");
		return ret;
	}

	ret = ov2640_set_frmival(dev, &frmival);
	if (ret) {
		LOG_ERR("Unable to configure default frame rate");
		return ret;
	}

	ret |= ov2640_set_exposure_ctrl(dev, 1);
	ret |= ov2640_set_white_bal(dev, 1);

	if (ret) {
		return ret;
	}

	/* Drive strength */

	val = ov2640_read_reg(&cfg->i2c, COM2);
	if (ret < 0) {
		return ret;
	}

	val &= ~COM2_OUTPUT_DRIVE_MASK;
	val |= cfg->drive_strength == 1 ? 0x0 : cfg->drive_strength == 2 ? 0x2 :
	       cfg->drive_strength == 3 ? 0x1 : cfg->drive_strength == 4 ? 0x3 : 0;

	ret = ov2640_write_reg(&cfg->i2c, COM2, val);
	if (ret < 0) {
		return ret;
	}

	/* Initialize controls */
	return ov2640_init_controls(dev);
}

static const struct ov2640_config ov2640_cfg_0 = {
	.i2c = I2C_DT_SPEC_INST_GET(0),
#if DT_INST_NODE_HAS_PROP(0, reset_gpios)
	.reset_gpio = GPIO_DT_SPEC_INST_GET(0, reset_gpios),
#endif
#if DT_INST_NODE_HAS_PROP(0, pwdn_gpios)
	.pwdn_gpio = GPIO_DT_SPEC_INST_GET(0, pwdn_gpios),
#endif
	.clock_multiplier = DT_INST_PROP(0, clock_multiplier),
	.drive_strength = DT_INST_PROP(0, drive_strength),
	.jpeg_hsync = DT_INST_PROP(0, jpeg_hsync),
};
static struct ov2640_data ov2640_data_0;

static int ov2640_init_0(const struct device *dev)
{
	const struct ov2640_config *cfg = dev->config;

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

#if DT_INST_NODE_HAS_PROP(0, reset_gpios)
	if (!gpio_is_ready_dt(&cfg->reset_gpio)) {
		LOG_ERR("%s: device %s is not ready", dev->name, cfg->reset_gpio.port->name);
		return -ENODEV;
	}
#endif

#if DT_INST_NODE_HAS_PROP(0, pwdn_gpios)
	if (!gpio_is_ready_dt(&cfg->pwdn_gpio)) {
		LOG_ERR("%s: device %s is not ready", dev->name, cfg->pwdn_gpio.port->name);
		return -ENODEV;
	}
#endif

	uint32_t i2c_cfg = I2C_MODE_CONTROLLER | I2C_SPEED_SET(I2C_SPEED_STANDARD);

	if (i2c_configure(cfg->i2c.bus, i2c_cfg)) {
		LOG_ERR("Failed to configure ov2640 i2c interface.");
	}

	return ov2640_init(dev);
}

DEVICE_DT_INST_DEFINE(0, &ov2640_init_0, NULL, &ov2640_data_0, &ov2640_cfg_0, POST_KERNEL,
		      CONFIG_VIDEO_INIT_PRIORITY, &ov2640_driver_api);

VIDEO_DEVICE_DEFINE(ov2640, DEVICE_DT_INST_GET(0), NULL);
