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
#define H_SIZE  0x51
#define V_SIZE  0x52
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

#define CTRLI       0x50
#define CTRLI_LP_DP 0x80

#define CTRL0        0xC2
#define CTRL0_YUV422 0x08
#define CTRL0_YUV_EN 0x04
#define CTRL0_RGB_EN 0x02

#define CTRL1  0xC3
#define CTRL1_AWB 0x08

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
#define R_BYPASS_DSP_NO    0x00
#define R_BYPASS_DSP_YES   0x01

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

#define OV2640_MARGIN 10
#define OV2640_BUS_WIDTH 8

/* Sensor register bank FF=0x01*/

#define COM1        0x03
#define REG_PID     0x0A
#define REG_PID_VAL 0x26
#define REG_VER     0x0B
#define REG_VER_VAL 0x42
#define AEC         0x10
#define CLKRC       0x11
#define HREFST      0x17
#define HREFEND     0x18
#define VSTRT       0x19
#define VEND        0x1A
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
#define COM2_STANDBY BIT(4)

#define COM3             0x0C
#define COM3_DEFAULT     0x38
#define COM3_BAND_AUTO   0x02
#define COM3_BAND_SET(x) (COM3_DEFAULT | x)

#define COM7           0x12
#define COM7_SRST      0x80
#define COM7_RES_UXGA  0x00
#define COM7_ZOOM_EN   0x04
#define COM7_COLOR_BAR 0x02

#define COM8         0x13
#define COM8_DEFAULT 0xC0
#define COM8_BNDF_EN 0x20
#define COM8_AGC_EN  0x04
#define COM8_AEC_EN  0x01
#define COM8_SET(x)  (COM8_DEFAULT | x)

#define COM9             0x14
#define COM9_DEFAULT     0x08
#define COM9_AGC_GAIN_8x 0x02
#define COM9_AGC_SET(x)  (COM9_DEFAULT | (x << 5))

/* Does not seem to have any effect, see devicetree bindings */
#define COM10 0x15

#define VV                  0x26
#define VV_AGC_TH_SET(h, l) ((h << 4) | (l & 0x0F))

#define REG32               0x32

#define OV2640_BASE_FPS_24MHZ 15
#define OV2640_NATIVE_WIDTH 1600
#define OV2640_NATIVE_HEIGHT 1200

static const uint8_t ov2640_clock_dividers[] = {1, 2, 4};

struct ov2640_config {
	struct i2c_dt_spec i2c;
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
	struct gpio_dt_spec reset_gpios;
#endif
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(powerdown_gpios)
	struct gpio_dt_spec powerdown_gpios;
#endif
	uint32_t mclk_freq;
	uint8_t drive_strength : 4;
	uint8_t clock_multiplier : 2;
	uint8_t vsync_active : 1;
	uint8_t jpeg_hsync : 1;
};

struct ov2640_ctrls {
	struct video_ctrl hflip;
	struct video_ctrl vflip;
	struct video_ctrl exposure_auto;
	struct video_ctrl auto_white_balance;
	struct video_ctrl autogain;
	struct video_ctrl contrast;
	struct video_ctrl brightness;
	struct video_ctrl saturation;
	struct video_ctrl jpeg;
	struct video_ctrl test_pattern;
	struct video_ctrl link_freq;
	struct video_ctrl vblank;
};

struct ov2640_data {
	struct ov2640_ctrls ctrls;
	struct video_format fmt;
	int64_t link_freq[ARRAY_SIZE(ov2640_clock_dividers)];
	uint64_t frmival_msec;
	uint8_t clock_divider;
	uint8_t bank;
};

struct ov2640_reg {
	uint8_t addr;
	uint8_t val;
};

static const char *const ov2640_test_pattern_menu[] = {
	"Disabled",
	"Color bars",
	NULL
};

static const struct ov2640_reg ov2640_default_regs[] = {
	{BANK_SEL, BANK_SEL_DSP},
	{0x2c, 0xff},
	{0x2e, 0xdf},
	{BANK_SEL, BANK_SEL_SENSOR},
	{0x3c, 0x32},
	{0x11, 0x80},
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

	/* Configure the DSP image source to the native resolution */

	{BANK_SEL, BANK_SEL_SENSOR},
	{HSIZE8, (OV2640_NATIVE_WIDTH >> 3)},
	{VSIZE8, (OV2640_NATIVE_HEIGHT >> 3)},
	{XOFFL, 0x00},
	{YOFFL, 0x00},
	{H_SIZE, ((OV2640_NATIVE_WIDTH >> 2) & 0xFF)},
	{V_SIZE, ((OV2640_NATIVE_HEIGHT >> 2) & 0xFF)},
	{VHYX, ((OV2640_NATIVE_HEIGHT >> 3) & 0x80) | ((OV2640_NATIVE_WIDTH >> 7) & 0x08)},
	{TEST, (OV2640_NATIVE_WIDTH >> 4) & 0x80},
	{BANK_SEL, BANK_SEL_SENSOR},
	{COM7, COM7_RES_UXGA},
	{COM1, 0x0F},
	{HREFST, 0x11},
	{HREFEND, 0x75},
	{VSTRT, 0x01},
	{VEND, 0x97},
	{0x3d, 0x34},
	{0x35, 0x88},
	{0x22, 0x0a},
	{0x37, 0x40},
	{0x34, 0xa0},
	{0x06, 0x02},
	{0x0d, 0xb7},
	{0x0e, 0x01},
	{0x42, 0x83},

	{BANK_SEL, BANK_SEL_DSP},
	{R_BYPASS, R_BYPASS_DSP_YES},
	{RESET, RESET_DVP},
	{CTRL2, CTRL2_DCW_EN | CTRL2_SDE_EN | CTRL2_UV_AVG_EN | CTRL2_CMX_EN | CTRL2_UV_ADJ_EN},
	{CTRLI, CTRLI_LP_DP | 0x00},
	{R_BYPASS, R_BYPASS_DSP_NO},
	{RESET, 0x00},
};

#define NUM_BRIGHTNESS_LEVELS (5)
static const uint8_t brightness_regs[NUM_BRIGHTNESS_LEVELS + 1][5] = {
	{BPADDR, BPDATA, BPADDR, BPDATA, BPDATA}, /* val addr */
	{0x00, 0x04, 0x09, 0x00, 0x00},           /* -2 */
	{0x00, 0x04, 0x09, 0x10, 0x00},           /* -1 */
	{0x00, 0x04, 0x09, 0x20, 0x00},           /*  0 */
	{0x00, 0x04, 0x09, 0x30, 0x00},           /* +1 */
	{0x00, 0x04, 0x09, 0x40, 0x00},           /* +2 */
};

#define NUM_CONTRAST_LEVELS (5)
static const uint8_t contrast_regs[NUM_CONTRAST_LEVELS + 1][7] = {
	{BPADDR, BPDATA, BPADDR, BPDATA, BPDATA, BPDATA, BPDATA}, /* val addr */
	{0x00, 0x04, 0x07, 0x20, 0x18, 0x34, 0x06},               /* -2 */
	{0x00, 0x04, 0x07, 0x20, 0x1c, 0x2a, 0x06},               /* -1 */
	{0x00, 0x04, 0x07, 0x20, 0x20, 0x20, 0x06},               /*  0 */
	{0x00, 0x04, 0x07, 0x20, 0x24, 0x16, 0x06},               /* +1 */
	{0x00, 0x04, 0x07, 0x20, 0x28, 0x0c, 0x06},               /* +2 */
};

#define NUM_SATURATION_LEVELS (5)
static const uint8_t saturation_regs[NUM_SATURATION_LEVELS + 1][5] = {
	{BPADDR, BPDATA, BPADDR, BPDATA, BPDATA}, /* val addr */
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

static const struct video_format_cap ov2640_fmts_with_dsp[] = {
	OV2640_VIDEO_FORMAT_CAP(VIDEO_PIX_FMT_RGB565),
	OV2640_VIDEO_FORMAT_CAP(VIDEO_PIX_FMT_RGB565X),
	OV2640_VIDEO_FORMAT_CAP(VIDEO_PIX_FMT_YUYV),
	OV2640_VIDEO_FORMAT_CAP(VIDEO_PIX_FMT_UYVY),
	OV2640_VIDEO_FORMAT_CAP(VIDEO_PIX_FMT_JPEG),
	{0},
};

static const struct video_format_cap ov2640_fmts_without_dsp[] = {
	/* With DSP element turned off, no subsampling */
	{
		.pixelformat = VIDEO_PIX_FMT_SBGGR8,
		.width_min = 1600, .width_max = 1600, .width_step = 0,
		.height_min = 1200, .height_max = 1200, .height_step = 0,
	},
	{0},
};

static int ov2640_write_reg(const struct i2c_dt_spec *spec, uint8_t addr, uint8_t val)
{
	int ret;

	/**
	 * It rarely happens that the camera does not respond with ACK signal.
	 * In that case it usually responds on 2nd try but there is a 3rd one
	 * just to be sure that the connection error is not caused by driver
	 * itself.
	 */
	for (int tries = 3; tries > 0; tries--) {
		ret = i2c_reg_write_byte_dt(spec, addr, val);
		if (ret == 0) {
			return 0;
		}
		k_msleep(5);
	}

	LOG_ERR("failed to write 0x%x to 0x%x", val, addr);
	return ret;
}

static int ov2640_write_dsp_reg(const struct device *dev, uint8_t addr, uint8_t val)
{
	const struct ov2640_config *cfg = dev->config;
	struct ov2640_data *data = dev->data;
	int ret;

	if (data->bank != BANK_SEL_DSP) {
		ret = ov2640_write_reg(&cfg->i2c, BANK_SEL, BANK_SEL_DSP);
		if (ret < 0) {
			return ret;
		}
		data->bank = BANK_SEL_DSP;
	}

	return ov2640_write_reg(&cfg->i2c, addr, val);
}

static int ov2640_write_sensor_reg(const struct device *dev, uint8_t addr, uint8_t val)
{
	const struct ov2640_config *cfg = dev->config;
	struct ov2640_data *data = dev->data;
	int ret;

	if (data->bank != BANK_SEL_SENSOR) {
		ret = ov2640_write_reg(&cfg->i2c, BANK_SEL, BANK_SEL_SENSOR);
		if (ret < 0) {
			return ret;
		}
		data->bank = BANK_SEL_SENSOR;
	}

	return ov2640_write_reg(&cfg->i2c, addr, val);
}

static int ov2640_read_reg(const struct i2c_dt_spec *spec, uint8_t addr, uint8_t *val)
{
	int ret;

	/**
	 * It rarely happens that the camera does not respond with ACK signal.
	 * In that case it usually responds on 2nd try but there is a 3rd one
	 * just to be sure that the connection error is not caused by driver
	 * itself.
	 */
	for (uint8_t tries = 3; tries > 0; tries--) {
		ret = i2c_reg_read_byte_dt(spec, addr, val);
		if (ret == 0) {
			return 0;
		}
		k_msleep(5);
	}

	LOG_ERR("failed to read 0x%x register", addr);

	return ret;
}

static int ov2640_read_dsp_reg(const struct device *dev, uint8_t addr, uint8_t *val)
{
	const struct ov2640_config *cfg = dev->config;
	struct ov2640_data *data = dev->data;
	int ret;

	if (data->bank != BANK_SEL_DSP) {
		ret = ov2640_write_reg(&cfg->i2c, BANK_SEL, BANK_SEL_DSP);
		if (ret < 0) {
			return ret;
		}
		data->bank = BANK_SEL_DSP;
	}

	return ov2640_read_reg(&cfg->i2c, addr, val);
}

static int ov2640_read_sensor_reg(const struct device *dev, uint8_t addr, uint8_t *val)
{
	const struct ov2640_config *cfg = dev->config;
	struct ov2640_data *data = dev->data;
	int ret;

	if (data->bank != BANK_SEL_SENSOR) {
		ret = ov2640_write_reg(&cfg->i2c, BANK_SEL, BANK_SEL_SENSOR);
		if (ret < 0) {
			return ret;
		}
		data->bank = BANK_SEL_SENSOR;
	}

	return ov2640_read_reg(&cfg->i2c, addr, val);
}

static int ov2640_write_all(const struct device *dev, const struct ov2640_reg *regs, size_t num)
{
	const struct ov2640_config *cfg = dev->config;
	struct ov2640_data *data = dev->data;
	int ret;
	int ret2 = 0;

	for (size_t i = 0; i < num; i++) {
		ret2 = ov2640_write_reg(&cfg->i2c, regs[i].addr, regs[i].val);
		if (ret2 < 0) {
			break;
		}
	}

	ret = ov2640_write_reg(&cfg->i2c, BANK_SEL, data->bank);
	if (ret < 0) {
		return ret;
	}

	return ret2;
}

static int ov2640_apply_config(const struct device *dev)
{
	const struct ov2640_config *cfg = dev->config;
	struct ov2640_data *data = dev->data;
	uint32_t sysclk_freq;
	uint32_t target_freq;
	uint32_t pclk_divider;
	int ret;

	/* Disable DSP */

	ret = ov2640_write_dsp_reg(dev, R_BYPASS, R_BYPASS_DSP_YES);
	if (ret < 0) {
		return ret;
	}

	/* Pixel format */

	switch (data->fmt.pixelformat) {
	case VIDEO_PIX_FMT_SBGGR8:
		/* Keep the DSP cores turned off and return now, this will output bayer data */
		return 0;
	case VIDEO_PIX_FMT_JPEG:
		if (cfg->jpeg_hsync) {
			ret = ov2640_write_dsp_reg(
				dev, IMAGE_MODE, IMAGE_MODE_JPEG_EN | IMAGE_MODE_HREF_IS_VSYNC);
		} else {
			ret = ov2640_write_dsp_reg(
				dev, IMAGE_MODE, IMAGE_MODE_JPEG_EN);
		}
		break;
	case VIDEO_PIX_FMT_RGB565:
		ret = ov2640_write_dsp_reg(
			dev, IMAGE_MODE, IMAGE_MODE_DVP_RGB565);
		break;
	case VIDEO_PIX_FMT_RGB565X:
		ret = ov2640_write_dsp_reg(
			dev, IMAGE_MODE, IMAGE_MODE_DVP_RGB565 | IMAGE_MODE_BYTE_SWAP);
		break;
	case VIDEO_PIX_FMT_YUYV:
		ret = ov2640_write_dsp_reg(
			dev, IMAGE_MODE, IMAGE_MODE_DVP_YUV422);
		break;
	case VIDEO_PIX_FMT_UYVY:
		ret = ov2640_write_dsp_reg(
			dev, IMAGE_MODE, IMAGE_MODE_DVP_YUV422 | IMAGE_MODE_BYTE_SWAP);
		break;
	default:
		CODE_UNREACHABLE;
	}
	if (ret < 0) {
		return ret;
	}

	/* Output width */

	ret = ov2640_write_dsp_reg(dev, ZMOW, (data->fmt.width / 4) & 0xFF);
	if (ret < 0) {
		return ret;
	}

	ret = ov2640_write_dsp_reg(dev, ZMOH, (data->fmt.height / 4) & 0xFF);
	if (ret < 0) {
		return ret;
	}

	ret = ov2640_write_dsp_reg(dev, ZMHH,
				(((data->fmt.height / 4) >> 8) & 0x1) << 2
				| (((data->fmt.width / 4) >> 8) & 0x3) << 0);
	if (ret < 0) {
		return ret;
	}

	/* Frame interval and clock config */

	ret = ov2640_write_sensor_reg(dev, CLKRC,
		(cfg->clock_multiplier - 1) << 7 | (data->clock_divider - 1));
	if (ret < 0) {
		return ret;
	}

	k_msleep(1);

	ret = ov2640_write_dsp_reg(dev, SIZEL,
		(data->fmt.width >> 10) << 6 |
		(data->fmt.width & 0x7) << 3 |
		(data->fmt.height & 0x7) << 0);
	if (ret < 0) {
		return ret;
	}

	/* Scale PCLK down to the minimum */

	if (data->fmt.pixelformat != VIDEO_PIX_FMT_JPEG) {
		sysclk_freq = 3 * cfg->mclk_freq * cfg->clock_multiplier / data->clock_divider;

		target_freq = (uint64_t)
			(data->fmt.height + data->ctrls.vblank.val)
			* (data->fmt.width + OV2640_MARGIN)
			* video_bits_per_pixel(data->fmt.pixelformat) / OV2640_BUS_WIDTH
			/ data->frmival_msec * MSEC_PER_SEC;

		pclk_divider = MIN(sysclk_freq / target_freq, 0x1f);

		ret = ov2640_write_dsp_reg(dev, R_DVP_SP, pclk_divider);
		if (ret < 0) {
			return ret;
		}

		LOG_DBG("Applied clock divider %u", pclk_divider);
	}

	/* Enable DSP engine converting the pixels */

	ret = ov2640_write_dsp_reg(dev, R_BYPASS, R_BYPASS_DSP_NO);
	if (ret < 0) {
		return ret;
	}

	LOG_DBG("Applied clock multiplier %u, clock divider %u",
		cfg->clock_multiplier, data->clock_divider);

	k_msleep(30);

	return ret;
}

static const struct video_format_cap *ov2640_get_format_caps(const struct device *dev)
{
	const struct ov2640_config *cfg = dev->config;

	/* See devicetree bindings. */
	if (cfg->vsync_active) {
		return ov2640_fmts_with_dsp;
	} else {
		return ov2640_fmts_without_dsp;
	}
}

static int ov2640_get_caps(const struct device *dev, struct video_caps *caps)
{
	caps->format_caps = ov2640_get_format_caps(dev);

	return 0;
}

static int ov2640_set_format(const struct device *dev, struct video_format *fmt)
{
	struct ov2640_data *data = dev->data;
	uint32_t index;
	int ret = 0;

	if (!memcmp(&data->fmt, fmt, sizeof(data->fmt))) {
		/* nothing to do */
		return 0;
	}

	ret = video_format_caps_index(ov2640_get_format_caps(dev), fmt, &index);
	if (ret < 0) {
		LOG_ERR("Format %s %ux%u not supported",
			VIDEO_FOURCC_TO_STR(fmt->pixelformat), fmt->width, fmt->height);
		return ret;
	}

	data->fmt = *fmt;

	return 0;
}

static int ov2640_get_format(const struct device *dev, struct video_format *fmt)
{
	struct ov2640_data *data = dev->data;

	*fmt = data->fmt;

	return 0;
}

static int ov2640_enum_frmival(const struct device *dev, struct video_frmival_enum *fie)
{
	const struct ov2640_config *cfg = dev->config;

	if (fie->index >= ARRAY_SIZE(ov2640_clock_dividers)) {
		return -EINVAL;
	}

	fie->type = VIDEO_FRMIVAL_TYPE_DISCRETE;
	fie->discrete.numerator =
		(uint64_t)USEC_PER_SEC / OV2640_BASE_FPS_24MHZ * cfg->mclk_freq / MHZ(24)
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

	return 0;
}

static int ov2640_get_frmival(const struct device *dev, struct video_frmival *frmival)
{
	struct ov2640_data *data = dev->data;

	frmival->numerator = data->frmival_msec;
	frmival->denominator = MSEC_PER_SEC;

	return 0;
}

static int ov2640_set_stream(const struct device *dev, bool stream, enum video_buf_type type)
{
	int ret;

	if (stream) {
		ret = ov2640_apply_config(dev);
		if (ret < 0) {
			return ret;
		}

		ret = ov2640_write_dsp_reg(dev, RESET, 0x00);
		if (ret < 0) {
			return ret;
		}
	} else {
		ret = ov2640_write_dsp_reg(dev, RESET, 0xff);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int ov2640_get_volatile_ctrl(const struct device *dev, uint32_t id)
{
	struct ov2640_data *data = dev->data;
	struct ov2640_ctrls *ctrls = &data->ctrls;

	switch (id) {
	case VIDEO_CID_LINK_FREQ:
		for (int i = 0; i < ARRAY_SIZE(ov2640_clock_dividers); i++) {
			if (data->clock_divider == ov2640_clock_dividers[i]) {
				ctrls->link_freq.val = i;
				return 0;
			}
		}
		return -ENOENT;
	default:
		CODE_UNREACHABLE;
		return -EINVAL;
	}

	return 0;
}

static int ov2640_set_level(const struct device *dev, int level, int max_level, int cols,
			    const uint8_t regs[][cols])
{
	int ret;

	level += max_level / 2 + 1;

	for (int i = 0; i < (ARRAY_SIZE(regs[0]) / sizeof(regs[0][0])); i++) {
		ret = ov2640_write_dsp_reg(dev, regs[0][i], regs[level][i]);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int ov2640_set_ctrl(const struct device *dev, uint32_t id)
{
	struct ov2640_data *data = dev->data;
	struct ov2640_ctrls *ctrls = &data->ctrls;
	uint8_t val;
	int ret;

	switch (id) {

	case VIDEO_CID_HFLIP:
		ret = ov2640_read_sensor_reg(dev, REG04, &val);
		if (ret < 0) {
			return ret;
		}

		if (ctrls->hflip.val) {
			val |= REG04_HFLIP_IMG;
		} else {
			val &= ~REG04_HFLIP_IMG;
		}

		return ov2640_write_sensor_reg(dev, REG04, val);

	case VIDEO_CID_VFLIP:
		ret = ov2640_read_sensor_reg(dev, REG04, &val);
		if (ret < 0) {
			return ret;
		}

		if (ctrls->vflip.val) {
			val |= REG04_VFLIP_IMG | REG04_VREF_EN;
		} else {
			val &= ~(REG04_VFLIP_IMG | REG04_VREF_EN);
		}

		return ov2640_write_sensor_reg(dev, REG04, val);

	case VIDEO_CID_EXPOSURE_AUTO:
		if (ctrls->exposure_auto.val > 1) {
			LOG_WRN("Exposure other than manual/auto not supported");
			return -ENOTSUP;
		}

		ret = ov2640_read_sensor_reg(dev, COM8, &val);
		if (ret < 0) {
			return ret;
		}

		if (!ctrls->exposure_auto.val) {
			val |= COM8_AEC_EN;
		} else {
			val &= ~COM8_AEC_EN;
		}

		return ov2640_write_sensor_reg(dev, COM8, val);

	case VIDEO_CID_AUTO_WHITE_BALANCE:
		ret = ov2640_read_dsp_reg(dev, CTRL1, &val);
		if (ret < 0) {
			return ret;
		}

		if (ctrls->auto_white_balance.val) {
			val |= CTRL1_AWB;
		} else {
			val &= ~CTRL1_AWB;
		}

		return ov2640_write_dsp_reg(dev, CTRL1, val);

	case VIDEO_CID_AUTOGAIN:
		ret = ov2640_read_sensor_reg(dev, COM8, &val);
		if (ret < 0) {
			return ret;
		}

		if (ctrls->autogain.val) {
			val |= COM8_AGC_EN;
		} else {
			val &= ~COM8_AGC_EN;
		}

		return ov2640_write_sensor_reg(dev, COM8, val);

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
		return ov2640_write_dsp_reg(dev, QS, ctrls->jpeg.val);

	case VIDEO_CID_TEST_PATTERN:
		ret = ov2640_read_sensor_reg(dev, COM7, &val);
		if (ret < 0) {
			return ret;
		}

		if (ctrls->test_pattern.val) {
			val |= COM7_COLOR_BAR;
		} else {
			val &= ~COM7_COLOR_BAR;
		}

		return ov2640_write_sensor_reg(dev, COM7, val);

	case VIDEO_CID_VBLANK:
		ret = ov2640_write_sensor_reg(dev, ADDVSH, ctrls->vblank.val >> 8);
		if (ret < 0) {
			return ret;
		}

		return ov2640_write_sensor_reg(dev, ADDVSL, ctrls->vblank.val & 0xFF);

	default:
		CODE_UNREACHABLE;
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
	.get_volatile_ctrl = ov2640_get_volatile_ctrl,
};

static int ov2640_init_controls(const struct device *dev)
{
	struct ov2640_data *data = dev->data;
	struct ov2640_ctrls *ctrls = &data->ctrls;
	int ret;

	ret = video_init_ctrl(&ctrls->hflip, dev, VIDEO_CID_HFLIP,
			      (struct video_ctrl_range){.min = 0, .max = 1, .step = 1, .def = 0});
	if (ret < 0) {
		return ret;
	}

	ret = video_init_ctrl(&ctrls->vflip, dev, VIDEO_CID_VFLIP,
			      (struct video_ctrl_range){.min = 0, .max = 1, .step = 1, .def = 0});
	if (ret < 0) {
		return ret;
	}

	ret = video_init_menu_ctrl(&ctrls->exposure_auto, dev, VIDEO_CID_EXPOSURE_AUTO, 0, NULL);
	if (ret < 0) {
		return ret;
	}

	ret = video_init_ctrl(
		&ctrls->auto_white_balance, dev, VIDEO_CID_AUTO_WHITE_BALANCE,
		(struct video_ctrl_range){.min = 0, .max = 1, .step = 1, .def = 1});
	if (ret < 0) {
		return ret;
	}

	ret = video_init_ctrl(&ctrls->autogain, dev, VIDEO_CID_AUTOGAIN,
			      (struct video_ctrl_range){.min = 0, .max = 1, .step = 1, .def = 1});
	if (ret < 0) {
		return ret;
	}

	ret = video_init_ctrl(&ctrls->brightness, dev, VIDEO_CID_BRIGHTNESS,
			      (struct video_ctrl_range){.min = -2, .max = 2, .step = 1, .def = 0});
	if (ret < 0) {
		return ret;
	}

	ret = video_init_ctrl(&ctrls->contrast, dev, VIDEO_CID_CONTRAST,
			      (struct video_ctrl_range){.min = -2, .max = 2, .step = 1, .def = 0});
	if (ret < 0) {
		return ret;
	}

	ret = video_init_ctrl(&ctrls->saturation, dev, VIDEO_CID_SATURATION,
			      (struct video_ctrl_range){.min = -2, .max = 2, .step = 1, .def = 0});
	if (ret < 0) {
		return ret;
	}

	ret = video_init_ctrl(
		&ctrls->jpeg, dev, VIDEO_CID_JPEG_COMPRESSION_QUALITY,
		(struct video_ctrl_range){.min = 5, .max = 100, .step = 1, .def = 50});
	if (ret < 0) {
		return ret;
	}

	ret = video_init_menu_ctrl(&ctrls->test_pattern, dev, VIDEO_CID_TEST_PATTERN, 0,
				   ov2640_test_pattern_menu);
	if (ret < 0) {
		return ret;
	}

	ret = video_init_ctrl(
		&ctrls->vblank, dev, VIDEO_CID_VBLANK,
		(struct video_ctrl_range){.min = 0, .max = UINT16_MAX, .step = 1, .def = 0});
	if (ret < 0) {
		return ret;
	}

	ret = video_init_int_menu_ctrl(&ctrls->link_freq, dev, VIDEO_CID_LINK_FREQ,
				       0, data->link_freq, ARRAY_SIZE(data->link_freq));
	if (ret < 0) {
		return ret;
	}
	ctrls->link_freq.flags |= VIDEO_CTRL_FLAG_READ_ONLY;
	ctrls->link_freq.flags |= VIDEO_CTRL_FLAG_VOLATILE;

	return 0;
}

static int ov2640_init(const struct device *dev)
{
	const struct ov2640_config *cfg = dev->config;
	struct ov2640_data *data = dev->data;
	uint8_t val;
	int ret;

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	for (size_t i = 0; i < ARRAY_SIZE(ov2640_clock_dividers); i++) {
		data->link_freq[i] =
			cfg->mclk_freq * 3 * cfg->clock_multiplier / ov2640_clock_dividers[i];
	}

	ret = ov2640_init_controls(dev);
	if (ret < 0) {
		LOG_ERR("Failed to initialize controls");
		return ret;
	}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(powerdown_gpios)
	if (cfg->powerdown_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->powerdown_gpio)) {
			LOG_ERR("%s is not ready", cfg->powerdown_gpio.port->name);
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&cfg->powerdown_gpio, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			return ret;
		}

		k_sleep(K_MSEC(1));
	}
#endif

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
	if (cfg->reset_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->reset_gpio)) {
			LOG_ERR("%s is not ready", cfg->reset_gpio.port->name);
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&cfg->reset_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			return ret;
		}

		k_sleep(K_MSEC(1));
		gpio_pin_set_dt(&cfg->reset_gpio, 0);
		k_sleep(K_MSEC(1));
	}
#endif

	ret = ov2640_write_reg(&cfg->i2c, BANK_SEL, BANK_SEL_SENSOR);
	if (ret < 0) {
		return ret;
	}
	data->bank = BANK_SEL_SENSOR;

	/* Check connection */

	ret = ov2640_read_sensor_reg(dev, REG_PID, &val);
	if (ret < 0) {
		return ret;
	}
	if (val != REG_PID_VAL) {
		LOG_ERR("Invalid product ID, expected 0x%02x, got 0x%02x", REG_PID_VAL, val);
		return -ENODEV;
	}

	ret = ov2640_read_sensor_reg(dev, REG_VER, &val);
	if (ret < 0) {
		return ret;
	}
	if (val != REG_VER_VAL) {
		LOG_ERR("Invalid version, expected 0x%02x, got 0x%02x", REG_VER_VAL, val);
		return -ENODEV;
	}

	/* Soft reset */

	ret = ov2640_write_sensor_reg(dev, COM7, COM7_SRST);
	if (ret < 0) {
		return ret;
	}

	k_msleep(300);

	/* Common configuration */

	ret = ov2640_write_all(dev, ov2640_default_regs, ARRAY_SIZE(ov2640_default_regs));
	if (ret < 0) {
		return ret;
	}

	/* Defaults */

	ret = ov2640_set_format(dev, &(struct video_format){
		.pixelformat = ov2640_get_format_caps(dev)->pixelformat,
		.width = OV2640_NATIVE_WIDTH,
		.height = OV2640_NATIVE_HEIGHT,
	});
	if (ret < 0) {
		return ret;
	}

	ret = ov2640_set_frmival(dev, &(struct video_frmival){.numerator = 1, .denominator = 100});
	if (ret < 0) {
		return ret;
	}

	ret = ov2640_set_stream(dev, false, VIDEO_BUF_TYPE_OUTPUT);
	if (ret < 0) {
		return ret;
	}

	/* Drive strength */

	ret = ov2640_read_sensor_reg(dev, COM2, &val);
	if (ret < 0) {
		return ret;
	}

	val &= ~COM2_OUTPUT_DRIVE_MASK;
	val |= cfg->drive_strength == 1 ? 0x0 : cfg->drive_strength == 2 ? 0x2 :
	       cfg->drive_strength == 3 ? 0x1 : cfg->drive_strength == 4 ? 0x3 : 0;

	ret = ov2640_write_sensor_reg(dev, COM2, val);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
#define OV2640_RESET_GPIO(n)									\
	.reset_gpios = GPIO_DT_SPEC_INST_GET_OR(n, reset_gpios, {0}),
#else
#define OV2640_RESET_GPIO(n)
#endif

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(powerdown_gpios)
#define OV2640_POWERDOWN_GPIO(n)								\
	.powerdown_gpio = GPIO_DT_SPEC_INST_GET_OR(n, powerdown_gpios, {0}),
#else
#define OV2640_POWERDOWN_GPIO(n)
#endif

#define OV2640_INIT(n)										\
	static const struct ov2640_config ov2640_config_##n = {					\
		.i2c = I2C_DT_SPEC_INST_GET(n),							\
		OV2640_RESET_GPIO(n)								\
		OV2640_POWERDOWN_GPIO(n)							\
		.mclk_freq = DT_PROP(DT_INST_PHANDLE(n, clocks), clock_frequency),		\
		.clock_multiplier = DT_INST_PROP(n, clock_multiplier),				\
		.drive_strength = DT_INST_PROP(n, drive_strength),				\
		.jpeg_hsync = DT_INST_PROP(n, jpeg_hsync),					\
		.vsync_active = DT_PROP(DT_INST_ENDPOINT_BY_ID(n, 0, 0), vsync_active),		\
	};											\
												\
	static struct ov2640_data ov2640_data_##n;						\
												\
	DEVICE_DT_INST_DEFINE(n, &ov2640_init, NULL, &ov2640_data_##n, &ov2640_config_##n,	\
			      POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY, &ov2640_driver_api);	\
												\
	VIDEO_DEVICE_DEFINE(ov2640_##n, DEVICE_DT_INST_GET(n), NULL);

DT_INST_FOREACH_STATUS_OKAY(OV2640_INIT)
