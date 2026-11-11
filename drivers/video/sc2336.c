/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT smartsens_sc2336

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/video.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/video/video.h>

#include "video_common.h"

LOG_MODULE_REGISTER(sc2336, CONFIG_VIDEO_LOG_LEVEL);

#define SC2336_CHIP_ID 0xcb3a

#define SC2336_REG8(addr)  ((addr) | VIDEO_REG_ADDR16_DATA8)
#define SC2336_REG16(addr) ((addr) | VIDEO_REG_ADDR16_DATA16_BE)

#define SC2336_CCI_MODE_SELECT     SC2336_REG8(0x0100)
#define SC2336_MODE_STANDBY        0x00
#define SC2336_MODE_STREAMING      0x01
#define SC2336_CCI_SOFTWARE_RESET  SC2336_REG8(0x0103)
#define SC2336_CCI_CHIP_ID         SC2336_REG16(0x3107)
#define SC2336_CCI_FLIP_MIRROR     SC2336_REG8(0x3221)
#define SC2336_FLIP_MIRROR_HFLIP   (BIT(1) | BIT(2))
#define SC2336_FLIP_MIRROR_VFLIP   (BIT(5) | BIT(6))
#define SC2336_CCI_EXPOSURE_H      SC2336_REG8(0x3e00)
#define SC2336_CCI_EXPOSURE_M      SC2336_REG8(0x3e01)
#define SC2336_CCI_EXPOSURE_L      SC2336_REG8(0x3e02)
#define SC2336_CCI_DIG_GAIN_FINE   SC2336_REG8(0x3e07)
#define SC2336_CCI_ANA_GAIN        SC2336_REG8(0x3e09)
#define SC2336_CCI_TEST_PATTERN    SC2336_REG8(0x4501)
#define SC2336_TEST_PATTERN_ENABLE BIT(3)
#define SC2336_CCI_PLL_CTRL0       SC2336_REG8(0x36e9)
#define SC2336_CCI_PLL_CTRL1       SC2336_REG8(0x37f9)
#define SC2336_PLL_BYPASS          0x80
#define SC2336_CCI_VTS             SC2336_REG16(0x320e)
#define SC2336_CCI_GROUP_HOLD      SC2336_REG8(0x3812)
#define SC2336_GROUP_HOLD_START    0x00
#define SC2336_GROUP_HOLD_LAUNCH   0x30

#define SC2336_INPUT_CLK_HZ MHZ(24)

/* Exposure is counted in lines and must stay 6 lines below the frame length */
#define SC2336_EXPOSURE_MIN        1
#define SC2336_EXPOSURE_MAX_OFFSET 6
#define SC2336_EXPOSURE_DEFAULT    0x37e

/* Digital gain in 1/128 units, from 1x to almost 2x */
#define SC2336_DIG_GAIN_MIN 128
#define SC2336_DIG_GAIN_MAX 252

struct sc2336_mode {
	const struct video_reg16 *regs;
	size_t num_regs;
	uint16_t vts;
	uint8_t pll_ctrl0;
	uint8_t pll_ctrl1;
	uint8_t fps;
};

struct sc2336_ctrls {
	struct video_ctrl linkfreq;
	struct video_ctrl pixel_rate;
	struct video_ctrl test_pattern;
	struct video_ctrl hflip;
	struct video_ctrl vflip;
	struct video_ctrl exposure;
	struct video_ctrl analogue_gain;
	struct video_ctrl digital_gain;
};

struct sc2336_data {
	struct sc2336_ctrls ctrls;
	struct video_format fmt;
	size_t mode;
	uint16_t vts;
	uint8_t fps;
	bool streaming;
};

struct sc2336_config {
	struct i2c_dt_spec i2c;
	uint32_t input_clk;
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
	struct gpio_dt_spec reset_gpio;
#endif
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(powerdown_gpios)
	struct gpio_dt_spec powerdown_gpio;
#endif
};

static const struct video_reg16 sc2336_common_regs[] = {
	{0x3106, 0x05},
	{0x3248, 0x04},
	{0x3249, 0x0b},
	{0x3253, 0x08},
	{0x3301, 0x09},
	{0x3302, 0xff},
	{0x3303, 0x10},
	{0x3307, 0x02},
	{0x330a, 0x01},
	{0x330c, 0x16},
	{0x3318, 0x02},
	{0x3321, 0x0a},
	{0x3327, 0x0e},
	{0x332b, 0x12},
	{0x3333, 0x10},
	{0x3334, 0x40},
	{0x335e, 0x06},
	{0x335f, 0x0a},
	{0x3364, 0x1f},
	{0x337c, 0x02},
	{0x337d, 0x0e},
	{0x3390, 0x09},
	{0x3391, 0x0f},
	{0x3392, 0x1f},
	{0x3393, 0x20},
	{0x3394, 0x20},
	{0x33a2, 0x04},
	{0x33b1, 0x80},
	{0x33b2, 0x68},
	{0x33b3, 0x42},
	{0x33fc, 0x0f},
	{0x33fd, 0x1f},
	{0x349f, 0x03},
	{0x34a6, 0x0f},
	{0x34a7, 0x1f},
	{0x34a8, 0x42},
	{0x34a9, 0x06},
	{0x34aa, 0x01},
	{0x34ac, 0x01},
	{0x3630, 0xf4},
	{0x3633, 0x22},
	{0x3639, 0xf4},
	{0x363c, 0x47},
	{0x3670, 0x09},
	{0x3674, 0xf4},
	{0x3675, 0xfb},
	{0x3676, 0xed},
	{0x367c, 0x09},
	{0x367d, 0x0f},
	{0x3698, 0x89},
	{0x3699, 0x96},
	{0x369a, 0xd0},
	{0x369b, 0xd0},
	{0x369c, 0x09},
	{0x369d, 0x0f},
	{0x36a2, 0x09},
	{0x36a3, 0x0f},
	{0x36a4, 0x1f},
	{0x36d0, 0x01},
	{0x3722, 0xe1},
	{0x3724, 0x41},
	{0x3725, 0xc1},
	{0x3728, 0x20},
	{0x37fb, 0x32},
	{0x37fc, 0x11},
	{0x3900, 0x0d},
	{0x3905, 0x98},
	{0x391b, 0x81},
	{0x391c, 0x10},
	{0x3933, 0x81},
	{0x3934, 0xc5},
	{0x3940, 0x68},
	{0x3941, 0x00},
	{0x3942, 0x01},
	{0x3943, 0xc6},
	{0x3952, 0x02},
	{0x3953, 0x0f},
	{0x3e01, 0x37},
	{0x3e02, 0xe0},
	{0x3e08, 0x1f},
	{0x3e1b, 0x14},
	{0x4509, 0x38},
	{0x4823, 0x03},
	{0x4827, 0x03},
	{0x5799, 0x06},
	{0x5ae0, 0xfe},
	{0x5ae1, 0x40},
	{0x5ae2, 0x30},
	{0x5ae3, 0x28},
	{0x5ae4, 0x20},
	{0x5ae5, 0x30},
	{0x5ae6, 0x28},
	{0x5ae7, 0x20},
	{0x5ae8, 0x3c},
	{0x5ae9, 0x30},
	{0x5aea, 0x28},
	{0x5aeb, 0x3c},
	{0x5aec, 0x30},
	{0x5aed, 0x28},
	{0x5aee, 0xfe},
	{0x5aef, 0x40},
	{0x5af4, 0x30},
	{0x5af5, 0x28},
	{0x5af6, 0x20},
	{0x5af7, 0x30},
	{0x5af8, 0x28},
	{0x5af9, 0x20},
	{0x5afa, 0x3c},
	{0x5afb, 0x30},
	{0x5afc, 0x28},
	{0x5afd, 0x3c},
	{0x5afe, 0x30},
	{0x5aff, 0x28},
};

static const struct video_reg16 sc2336_1024x600_raw8_regs[] = {
	{0x301f, 0xc7},
	{0x3031, 0x08},
	{0x3037, 0x00},
	{0x3200, 0x01},
	{0x3201, 0xb4},
	{0x3202, 0x00},
	{0x3203, 0xf0},
	{0x3204, 0x05},
	{0x3205, 0xd3},
	{0x3206, 0x03},
	{0x3207, 0x4f},
	{0x3208, 0x04},
	{0x3209, 0x00},
	{0x320a, 0x02},
	{0x320b, 0x58},
	{0x320c, 0x09},
	{0x320d, 0x60},
	{0x320e, 0x03},
	{0x320f, 0xe8},
	{0x3210, 0x00},
	{0x3211, 0x10},
	{0x3212, 0x00},
	{0x3213, 0x04},
	{0x3306, 0x60},
	{0x330b, 0x10},
	{0x330d, 0xff},
	{0x3395, 0xff},
	{0x33f9, 0x78},
	{0x33fb, 0xd8},
	{0x34ab, 0x28},
	{0x34ad, 0x90},
	{0x3690, 0x22},
	{0x3691, 0x22},
	{0x3692, 0x22},
	{0x36ea, 0x08},
	{0x36eb, 0x0a},
	{0x36ec, 0x1a},
	{0x36ed, 0x18},
	{0x37fa, 0x08},
	{0x37fd, 0x37},
	{0x4819, 0x05},
	{0x481b, 0x03},
	{0x481d, 0x0a},
	{0x481f, 0x02},
	{0x4821, 0x08},
	{0x4825, 0x02},
	{0x4829, 0x04},
};

static const struct video_reg16 sc2336_1280x720_raw10_regs[] = {
	{0x301f, 0x30},
	{0x3200, 0x01},
	{0x3201, 0x34},
	{0x3202, 0x00},
	{0x3203, 0xb4},
	{0x3204, 0x06},
	{0x3205, 0x53},
	{0x3206, 0x03},
	{0x3207, 0x8b},
	{0x3208, 0x05},
	{0x3209, 0x00},
	{0x320a, 0x02},
	{0x320b, 0xd0},
	{0x320c, 0x07},
	{0x320d, 0x08},
	{0x320e, 0x02},
	{0x320f, 0xee},
	{0x3210, 0x00},
	{0x3211, 0x10},
	{0x3212, 0x00},
	{0x3213, 0x04},
	{0x3306, 0x60},
	{0x330b, 0x10},
	{0x330d, 0xff},
	{0x3395, 0x30},
	{0x33f9, 0x70},
	{0x33fb, 0xd0},
	{0x34ab, 0x23},
	{0x34ad, 0x84},
	{0x3641, 0x03},
	{0x3690, 0x33},
	{0x3691, 0x33},
	{0x3692, 0x43},
	{0x36ea, 0x09},
	{0x36eb, 0x0c},
	{0x36ec, 0x1c},
	{0x36ed, 0x28},
	{0x37fa, 0x09},
	{0x37fd, 0x37},
	{0x440e, 0x02},
	{0x4819, 0x06},
	{0x481b, 0x03},
	{0x481d, 0x0b},
	{0x481f, 0x03},
	{0x4821, 0x08},
	{0x4825, 0x03},
	{0x4829, 0x05},
};

static const struct video_reg16 sc2336_1920x1080_raw8_regs[] = {
	{0x301f, 0x8e},
	{0x3031, 0x08},
	{0x3037, 0x00},
	{0x320c, 0x08},
	{0x320d, 0xc0},
	{0x320e, 0x04},
	{0x320f, 0xe2},
	{0x3306, 0x68},
	{0x330b, 0x18},
	{0x330d, 0xd3},
	{0x3395, 0xff},
	{0x33f9, 0x78},
	{0x33fb, 0xe0},
	{0x34ab, 0x28},
	{0x34ad, 0x90},
	{0x3690, 0x22},
	{0x3691, 0x22},
	{0x3692, 0x22},
	{0x36ea, 0x0e},
	{0x36eb, 0x0a},
	{0x36ec, 0x1a},
	{0x36ed, 0x18},
	{0x37fa, 0x15},
	{0x37fd, 0x17},
	{0x4819, 0x06},
	{0x481b, 0x04},
	{0x481d, 0x0c},
	{0x481f, 0x03},
	{0x4821, 0x0a},
	{0x4825, 0x03},
	{0x4829, 0x05},
};

static const struct video_reg16 sc2336_1920x1080_raw10_regs[] = {
	{0x301f, 0x02},
	{0x320c, 0x08},
	{0x320d, 0xca},
	{0x320e, 0x04},
	{0x320f, 0xb0},
	{0x3306, 0x60},
	{0x330b, 0x10},
	{0x330d, 0xff},
	{0x3395, 0xff},
	{0x33f9, 0x70},
	{0x33fb, 0xd0},
	{0x34ab, 0x23},
	{0x34ad, 0x84},
	{0x3690, 0x33},
	{0x3691, 0x33},
	{0x3692, 0x43},
	{0x36ea, 0x09},
	{0x36eb, 0x0c},
	{0x36ec, 0x1c},
	{0x36ed, 0x28},
	{0x37fa, 0x09},
	{0x37fd, 0x37},
	{0x440e, 0x02},
	{0x4819, 0x06},
	{0x481b, 0x03},
	{0x481d, 0x0b},
	{0x481f, 0x03},
	{0x4821, 0x08},
	{0x4825, 0x03},
	{0x4829, 0x05},
};

static const struct video_reg16 sc2336_1280x720_raw8_regs[] = {
	{0x301f, 0x8e},
	{0x3031, 0x08},
	{0x3037, 0x00},
	{0x3200, 0x01},
	{0x3201, 0x34},
	{0x3202, 0x00},
	{0x3203, 0xb4},
	{0x3204, 0x06},
	{0x3205, 0x53},
	{0x3206, 0x03},
	{0x3207, 0x8b},
	{0x3208, 0x05},
	{0x3209, 0x00},
	{0x320a, 0x02},
	{0x320b, 0xd0},
	{0x320c, 0x08},
	{0x320d, 0xc0},
	{0x320e, 0x04},
	{0x320f, 0xe2},
	{0x3210, 0x00},
	{0x3211, 0x10},
	{0x3212, 0x00},
	{0x3213, 0x04},
	{0x3306, 0x68},
	{0x330b, 0x18},
	{0x330d, 0xd3},
	{0x3395, 0xff},
	{0x33f9, 0x78},
	{0x33fb, 0xe0},
	{0x34ab, 0x28},
	{0x34ad, 0x90},
	{0x3690, 0x22},
	{0x3691, 0x22},
	{0x3692, 0x22},
	{0x36ea, 0x0e},
	{0x36eb, 0x0a},
	{0x36ec, 0x1a},
	{0x36ed, 0x18},
	{0x37fa, 0x15},
	{0x37fd, 0x17},
	{0x4819, 0x06},
	{0x481b, 0x04},
	{0x481d, 0x0c},
	{0x481f, 0x03},
	{0x4821, 0x0a},
	{0x4825, 0x03},
	{0x4829, 0x05},
};

static const struct video_reg16 sc2336_800x800_raw8_regs[] = {
	{0x301f, 0xa6},
	{0x3031, 0x08},
	{0x3037, 0x00},
	{0x3200, 0x02},
	{0x3201, 0x24},
	{0x3202, 0x00},
	{0x3203, 0x8c},
	{0x3204, 0x05},
	{0x3205, 0x63},
	{0x3206, 0x03},
	{0x3207, 0xb3},
	{0x3208, 0x03},
	{0x3209, 0x20},
	{0x320a, 0x03},
	{0x320b, 0x20},
	{0x320c, 0x08},
	{0x320d, 0xc0},
	{0x320e, 0x04},
	{0x320f, 0xe2},
	{0x3210, 0x00},
	{0x3211, 0x10},
	{0x3212, 0x00},
	{0x3213, 0x04},
	{0x3306, 0x68},
	{0x330b, 0x18},
	{0x330d, 0xd3},
	{0x3395, 0xff},
	{0x33f9, 0x78},
	{0x33fb, 0xe0},
	{0x34ab, 0x28},
	{0x34ad, 0x90},
	{0x3690, 0x22},
	{0x3691, 0x22},
	{0x3692, 0x22},
	{0x36ea, 0x0e},
	{0x36eb, 0x0a},
	{0x36ec, 0x1a},
	{0x36ed, 0x18},
	{0x37fa, 0x15},
	{0x37fd, 0x17},
	{0x440e, 0x02},
	{0x4819, 0x06},
	{0x481b, 0x04},
	{0x481d, 0x0c},
	{0x481f, 0x03},
	{0x4821, 0x0a},
	{0x4825, 0x03},
	{0x4829, 0x05},
};

static const struct video_reg16 sc2336_640x480_raw10_regs[] = {
	{0x301f, 0xa8},
	{0x3200, 0x02},
	{0x3201, 0x74},
	{0x3202, 0x01},
	{0x3203, 0x2c},
	{0x3204, 0x05},
	{0x3205, 0x13},
	{0x3206, 0x03},
	{0x3207, 0x13},
	{0x3208, 0x02},
	{0x3209, 0x80},
	{0x320a, 0x01},
	{0x320b, 0xe0},
	{0x320c, 0x06},
	{0x320d, 0x40},
	{0x320e, 0x02},
	{0x320f, 0x0d},
	{0x3210, 0x00},
	{0x3211, 0x10},
	{0x3212, 0x00},
	{0x3213, 0x04},
	{0x3250, 0x40},
	{0x3306, 0x60},
	{0x330b, 0x10},
	{0x330d, 0xff},
	{0x3395, 0x20},
	{0x33f9, 0x78},
	{0x33fb, 0xd0},
	{0x34ab, 0x28},
	{0x34ad, 0x84},
	{0x3690, 0x22},
	{0x3691, 0x22},
	{0x3692, 0x22},
	{0x36ea, 0x07},
	{0x36eb, 0x1c},
	{0x36ec, 0x1c},
	{0x36ed, 0x28},
	{0x37fa, 0x07},
	{0x37fd, 0x37},
	{0x440e, 0x02},
	{0x4800, 0x44},
	{0x4819, 0x03},
	{0x481b, 0x02},
	{0x481d, 0x06},
	{0x481f, 0x02},
	{0x4821, 0x07},
	{0x4823, 0x02},
	{0x4825, 0x02},
	{0x4827, 0x02},
	{0x4829, 0x03},
};

enum {
	SC2336_MODE_1024X600_RAW8,
	SC2336_MODE_1280X720_RAW10,
	SC2336_MODE_1920X1080_RAW8,
	SC2336_MODE_1920X1080_RAW10,
	SC2336_MODE_1280X720_RAW8,
	SC2336_MODE_800X800_RAW8,
	SC2336_MODE_640X480_RAW10,
};

#define SC2336_MODE(name, v, pll0, pll1, rate)                                                     \
	{                                                                                          \
		.regs = sc2336_##name##_regs,                                                      \
		.num_regs = ARRAY_SIZE(sc2336_##name##_regs),                                      \
		.vts = (v),                                                                        \
		.pll_ctrl0 = (pll0),                                                               \
		.pll_ctrl1 = (pll1),                                                               \
		.fps = (rate),                                                                     \
	}

static const struct sc2336_mode sc2336_modes[] = {
	[SC2336_MODE_1024X600_RAW8] = SC2336_MODE(1024x600_raw8, 1000, 0x53, 0x53, 30),
	[SC2336_MODE_1280X720_RAW10] = SC2336_MODE(1280x720_raw10, 750, 0x53, 0x53, 60),
	[SC2336_MODE_1920X1080_RAW8] = SC2336_MODE(1920x1080_raw8, 1250, 0x54, 0x54, 30),
	[SC2336_MODE_1920X1080_RAW10] = SC2336_MODE(1920x1080_raw10, 1200, 0x53, 0x53, 30),
	[SC2336_MODE_1280X720_RAW8] = SC2336_MODE(1280x720_raw8, 1250, 0x54, 0x54, 30),
	[SC2336_MODE_800X800_RAW8] = SC2336_MODE(800x800_raw8, 1250, 0x54, 0x54, 30),
	[SC2336_MODE_640X480_RAW10] = SC2336_MODE(640x480_raw10, 525, 0x24, 0x20, 50),
};

#define SC2336_FMT_CAP(fmt, w, h)                                                                  \
	{                                                                                          \
		.pixelformat = (fmt),                                                              \
		.width_min = (w), .width_max = (w), .width_step = 0,                               \
		.height_min = (h), .height_max = (h), .height_step = 0,                            \
	}

static const struct video_format_cap sc2336_fmts[] = {
	[SC2336_MODE_1024X600_RAW8] = SC2336_FMT_CAP(VIDEO_PIX_FMT_SBGGR8, 1024, 600),
	[SC2336_MODE_1280X720_RAW10] = SC2336_FMT_CAP(VIDEO_PIX_FMT_SBGGR10P, 1280, 720),
	[SC2336_MODE_1920X1080_RAW8] = SC2336_FMT_CAP(VIDEO_PIX_FMT_SBGGR8, 1920, 1080),
	[SC2336_MODE_1920X1080_RAW10] = SC2336_FMT_CAP(VIDEO_PIX_FMT_SBGGR10P, 1920, 1080),
	[SC2336_MODE_1280X720_RAW8] = SC2336_FMT_CAP(VIDEO_PIX_FMT_SBGGR8, 1280, 720),
	[SC2336_MODE_800X800_RAW8] = SC2336_FMT_CAP(VIDEO_PIX_FMT_SBGGR8, 800, 800),
	[SC2336_MODE_640X480_RAW10] = SC2336_FMT_CAP(VIDEO_PIX_FMT_SBGGR10P, 640, 480),
	{0},
};

/* Link frequency is half the lane bit rate of each mode */
static const int64_t sc2336_link_frequency[] = {
	[SC2336_MODE_1024X600_RAW8] = 144000000,
	[SC2336_MODE_1280X720_RAW10] = 202500000,
	[SC2336_MODE_1920X1080_RAW8] = 168000000,
	[SC2336_MODE_1920X1080_RAW10] = 202500000,
	[SC2336_MODE_1280X720_RAW8] = 168000000,
	[SC2336_MODE_800X800_RAW8] = 168000000,
	[SC2336_MODE_640X480_RAW10] = 105000000,
};

static const int64_t sc2336_pixel_rate[] = {
	[SC2336_MODE_1024X600_RAW8] = 72000000,
	[SC2336_MODE_1280X720_RAW10] = 81000000,
	[SC2336_MODE_1920X1080_RAW8] = 84000000,
	[SC2336_MODE_1920X1080_RAW10] = 81000000,
	[SC2336_MODE_1280X720_RAW8] = 84000000,
	[SC2336_MODE_800X800_RAW8] = 84000000,
	[SC2336_MODE_640X480_RAW10] = 42000000,
};

/* Frame rates reached by stretching the frame length of a mode */
static const uint8_t sc2336_frame_rates[] = {60, 50, 30, 25, 20, 15, 10, 5};

/* Analogue gain of 2^N for a control value of N */
static const uint8_t sc2336_ana_gain_regs[] = {0x00, 0x08, 0x09, 0x0b, 0x0f, 0x1f};

static int sc2336_set_pll(const struct device *dev, uint8_t pll_ctrl0, uint8_t pll_ctrl1)
{
	const struct sc2336_config *cfg = dev->config;
	int ret;

	ret = video_write_cci_reg(&cfg->i2c, SC2336_CCI_PLL_CTRL0, pll_ctrl0);
	if (ret < 0) {
		return ret;
	}

	return video_write_cci_reg(&cfg->i2c, SC2336_CCI_PLL_CTRL1, pll_ctrl1);
}

static void sc2336_update_vts(struct sc2336_data *drv_data, uint16_t vts)
{
	struct video_ctrl *exposure = &drv_data->ctrls.exposure;

	drv_data->vts = vts;
	exposure->range.max = vts - SC2336_EXPOSURE_MAX_OFFSET;
	exposure->range.def = MIN(SC2336_EXPOSURE_DEFAULT, exposure->range.max);
	exposure->val = MIN(exposure->val, exposure->range.max);
}

static int sc2336_set_exposure(const struct device *dev)
{
	const struct sc2336_config *cfg = dev->config;
	struct sc2336_data *drv_data = dev->data;
	uint32_t lines = drv_data->ctrls.exposure.val;
	struct video_reg regs[] = {
		{SC2336_CCI_GROUP_HOLD, SC2336_GROUP_HOLD_START},
		{SC2336_CCI_VTS, drv_data->vts},
		{SC2336_CCI_EXPOSURE_H, (lines >> 12) & 0x0f},
		{SC2336_CCI_EXPOSURE_M, (lines >> 4) & 0xff},
		{SC2336_CCI_EXPOSURE_L, (lines & 0x0f) << 4},
		{SC2336_CCI_GROUP_HOLD, SC2336_GROUP_HOLD_LAUNCH},
	};

	return video_write_cci_multiregs(&cfg->i2c, regs, ARRAY_SIZE(regs));
}

static int sc2336_set_ctrl(const struct device *dev, uint32_t cid)
{
	const struct sc2336_config *cfg = dev->config;
	struct sc2336_data *drv_data = dev->data;
	struct sc2336_ctrls *ctrls = &drv_data->ctrls;

	switch (cid) {
	case VIDEO_CID_TEST_PATTERN:
		return video_modify_cci_reg(
			&cfg->i2c, SC2336_CCI_TEST_PATTERN, SC2336_TEST_PATTERN_ENABLE,
			ctrls->test_pattern.val != 0 ? SC2336_TEST_PATTERN_ENABLE : 0);
	case VIDEO_CID_HFLIP:
		return video_modify_cci_reg(&cfg->i2c, SC2336_CCI_FLIP_MIRROR,
					    SC2336_FLIP_MIRROR_HFLIP,
					    ctrls->hflip.val != 0 ? SC2336_FLIP_MIRROR_HFLIP : 0);
	case VIDEO_CID_VFLIP:
		return video_modify_cci_reg(&cfg->i2c, SC2336_CCI_FLIP_MIRROR,
					    SC2336_FLIP_MIRROR_VFLIP,
					    ctrls->vflip.val != 0 ? SC2336_FLIP_MIRROR_VFLIP : 0);
	case VIDEO_CID_EXPOSURE:
		return sc2336_set_exposure(dev);
	case VIDEO_CID_ANALOGUE_GAIN:
		return video_write_cci_reg(&cfg->i2c, SC2336_CCI_ANA_GAIN,
					   sc2336_ana_gain_regs[ctrls->analogue_gain.val]);
	case VIDEO_CID_DIGITAL_GAIN:
		return video_write_cci_reg(&cfg->i2c, SC2336_CCI_DIG_GAIN_FINE,
					   ctrls->digital_gain.val);
	default:
		return -ENOTSUP;
	}
}

static int sc2336_set_fmt(const struct device *dev, struct video_format *fmt)
{
	static const uint32_t cids[] = {
		VIDEO_CID_TEST_PATTERN, VIDEO_CID_HFLIP, VIDEO_CID_VFLIP,
		VIDEO_CID_EXPOSURE,     VIDEO_CID_ANALOGUE_GAIN, VIDEO_CID_DIGITAL_GAIN,
	};
	const struct sc2336_config *cfg = dev->config;
	struct sc2336_data *drv_data = dev->data;
	const struct sc2336_mode *mode;
	size_t idx;
	int ret;

	if (fmt->pixelformat == drv_data->fmt.pixelformat && fmt->width == drv_data->fmt.width &&
	    fmt->height == drv_data->fmt.height) {
		return 0;
	}

	if (drv_data->streaming) {
		LOG_ERR("Cannot set format while streaming");
		return -EBUSY;
	}

	ret = video_format_caps_index(sc2336_fmts, fmt, &idx);
	if (ret < 0) {
		LOG_ERR("Format '%s' %ux%u not supported", VIDEO_FOURCC_TO_STR(fmt->pixelformat),
			fmt->width, fmt->height);
		return -ENOTSUP;
	}

	mode = &sc2336_modes[idx];

	ret = video_write_cci_reg(&cfg->i2c, SC2336_CCI_SOFTWARE_RESET, 1);
	if (ret < 0) {
		return ret;
	}

	k_sleep(K_MSEC(5));

	ret = sc2336_set_pll(dev, SC2336_PLL_BYPASS, SC2336_PLL_BYPASS);
	if (ret < 0) {
		return ret;
	}

	ret = video_write_cci_multiregs16(&cfg->i2c, sc2336_common_regs,
					  ARRAY_SIZE(sc2336_common_regs));
	if (ret < 0) {
		return ret;
	}

	ret = video_write_cci_multiregs16(&cfg->i2c, mode->regs, mode->num_regs);
	if (ret < 0) {
		return ret;
	}

	ret = sc2336_set_pll(dev, mode->pll_ctrl0, mode->pll_ctrl1);
	if (ret < 0) {
		return ret;
	}

	sc2336_update_vts(drv_data, mode->vts);

	ARRAY_FOR_EACH(cids, i) {
		ret = sc2336_set_ctrl(dev, cids[i]);
		if (ret < 0) {
			return ret;
		}
	}

	drv_data->mode = idx;
	drv_data->fps = mode->fps;
	drv_data->ctrls.linkfreq.val = idx;
	drv_data->ctrls.pixel_rate.val64 = sc2336_pixel_rate[idx];
	drv_data->fmt = *fmt;

	return 0;
}

static int sc2336_get_fmt(const struct device *dev, struct video_format *fmt)
{
	struct sc2336_data *drv_data = dev->data;

	*fmt = drv_data->fmt;

	return 0;
}

static int sc2336_get_caps(const struct device *dev, struct video_caps *caps)
{
	caps->format_caps = sc2336_fmts;

	return 0;
}

static int sc2336_enum_frmival(const struct device *dev, struct video_frmival_enum *fie)
{
	size_t count = 0;
	size_t idx;
	int ret;

	ret = video_format_caps_index(sc2336_fmts, fie->format, &idx);
	if (ret < 0) {
		return ret;
	}

	ARRAY_FOR_EACH(sc2336_frame_rates, i) {
		if (sc2336_frame_rates[i] > sc2336_modes[idx].fps) {
			continue;
		}

		if (count == fie->index) {
			fie->type = VIDEO_FRMIVAL_TYPE_DISCRETE;
			fie->discrete.numerator = 1;
			fie->discrete.denominator = sc2336_frame_rates[i];
			return 0;
		}

		count++;
	}

	return -EINVAL;
}

static int sc2336_get_frmival(const struct device *dev, struct video_frmival *frmival)
{
	struct sc2336_data *drv_data = dev->data;

	frmival->numerator = 1;
	frmival->denominator = drv_data->fps;

	return 0;
}

static int sc2336_set_frmival(const struct device *dev, struct video_frmival *frmival)
{
	struct sc2336_data *drv_data = dev->data;
	const struct sc2336_mode *mode = &sc2336_modes[drv_data->mode];
	struct video_frmival_enum fie = {
		.format = &drv_data->fmt,
		.type = VIDEO_FRMIVAL_TYPE_DISCRETE,
		.discrete = *frmival,
	};
	uint16_t vts;
	int ret;

	ret = video_closest_frmival(dev, &fie);
	if (ret < 0) {
		return ret;
	}

	ret = sc2336_enum_frmival(dev, &fie);
	if (ret < 0) {
		return ret;
	}

	vts = DIV_ROUND_CLOSEST(mode->vts * mode->fps, fie.discrete.denominator);
	sc2336_update_vts(drv_data, vts);

	ret = sc2336_set_exposure(dev);
	if (ret < 0) {
		return ret;
	}

	drv_data->fps = fie.discrete.denominator;
	*frmival = fie.discrete;

	return 0;
}

static int sc2336_set_stream(const struct device *dev, bool on, enum video_buf_type type)
{
	const struct sc2336_config *cfg = dev->config;
	struct sc2336_data *drv_data = dev->data;
	int ret;

	ret = video_write_cci_reg(&cfg->i2c, SC2336_CCI_MODE_SELECT,
				  on ? SC2336_MODE_STREAMING : SC2336_MODE_STANDBY);
	if (ret < 0) {
		return ret;
	}

	drv_data->streaming = on;

	return 0;
}

static DEVICE_API(video, sc2336_driver_api) = {
	.set_format = sc2336_set_fmt,
	.get_format = sc2336_get_fmt,
	.get_caps = sc2336_get_caps,
	.set_stream = sc2336_set_stream,
	.set_ctrl = sc2336_set_ctrl,
	.set_frmival = sc2336_set_frmival,
	.get_frmival = sc2336_get_frmival,
	.enum_frmival = sc2336_enum_frmival,
};

static const char *const sc2336_test_pattern_menu[] = {
	"Off",
	"Gradient",
	NULL,
};

static int sc2336_init_ctrls(const struct device *dev)
{
	struct sc2336_data *drv_data = dev->data;
	struct sc2336_ctrls *ctrls = &drv_data->ctrls;
	int ret;

	ret = video_init_int_menu_ctrl(&ctrls->linkfreq, dev, VIDEO_CID_LINK_FREQ,
				       SC2336_MODE_1024X600_RAW8, sc2336_link_frequency,
				       ARRAY_SIZE(sc2336_link_frequency));
	if (ret < 0) {
		return ret;
	}

	ctrls->linkfreq.flags |= VIDEO_CTRL_FLAG_READ_ONLY;

	ret = video_init_ctrl(&ctrls->pixel_rate, dev, VIDEO_CID_PIXEL_RATE,
			      (struct video_ctrl_range){
				      .min64 = sc2336_pixel_rate[SC2336_MODE_640X480_RAW10],
				      .max64 = sc2336_pixel_rate[SC2336_MODE_1920X1080_RAW8],
				      .step64 = 1,
				      .def64 = sc2336_pixel_rate[SC2336_MODE_1024X600_RAW8],
			      });
	if (ret < 0) {
		return ret;
	}

	ret = video_init_menu_ctrl(&ctrls->test_pattern, dev, VIDEO_CID_TEST_PATTERN, 0,
				   sc2336_test_pattern_menu);
	if (ret < 0) {
		return ret;
	}

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

	ret = video_init_ctrl(&ctrls->exposure, dev, VIDEO_CID_EXPOSURE,
			      (struct video_ctrl_range){
				      .min = SC2336_EXPOSURE_MIN,
				      .max = sc2336_modes[SC2336_MODE_1920X1080_RAW8].vts -
					     SC2336_EXPOSURE_MAX_OFFSET,
				      .step = 1,
				      .def = SC2336_EXPOSURE_DEFAULT,
			      });
	if (ret < 0) {
		return ret;
	}

	ret = video_init_ctrl(&ctrls->analogue_gain, dev, VIDEO_CID_ANALOGUE_GAIN,
			      (struct video_ctrl_range){
				      .min = 0,
				      .max = ARRAY_SIZE(sc2336_ana_gain_regs) - 1,
				      .step = 1,
				      .def = 0,
			      });
	if (ret < 0) {
		return ret;
	}

	return video_init_ctrl(&ctrls->digital_gain, dev, VIDEO_CID_DIGITAL_GAIN,
			       (struct video_ctrl_range){
				       .min = SC2336_DIG_GAIN_MIN,
				       .max = SC2336_DIG_GAIN_MAX,
				       .step = 1,
				       .def = SC2336_DIG_GAIN_MIN,
			       });
}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios) ||                                               \
	DT_ANY_INST_HAS_PROP_STATUS_OKAY(powerdown_gpios)
static int sc2336_assert_gpio(const struct gpio_dt_spec *gpio)
{
	if (gpio->port == NULL) {
		return 0;
	}

	if (!gpio_is_ready_dt(gpio)) {
		LOG_ERR("GPIO %s is not ready", gpio->port->name);
		return -ENODEV;
	}

	return gpio_pin_configure_dt(gpio, GPIO_OUTPUT_ACTIVE);
}

static int sc2336_release_gpio(const struct gpio_dt_spec *gpio)
{
	int ret;

	if (gpio->port == NULL) {
		return 0;
	}

	ret = gpio_pin_set_dt(gpio, 0);
	if (ret < 0) {
		return ret;
	}

	k_sleep(K_MSEC(10));

	return 0;
}

static int sc2336_power_on(const struct sc2336_config *cfg)
{
	int ret;

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(powerdown_gpios)
	ret = sc2336_assert_gpio(&cfg->powerdown_gpio);
	if (ret < 0) {
		return ret;
	}
#endif

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
	ret = sc2336_assert_gpio(&cfg->reset_gpio);
	if (ret < 0) {
		return ret;
	}
#endif

	k_sleep(K_MSEC(10));

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(powerdown_gpios)
	ret = sc2336_release_gpio(&cfg->powerdown_gpio);
	if (ret < 0) {
		return ret;
	}
#endif

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
	ret = sc2336_release_gpio(&cfg->reset_gpio);
	if (ret < 0) {
		return ret;
	}
#endif

	return 0;
}
#endif

static int sc2336_init(const struct device *dev)
{
	const struct sc2336_config *cfg = dev->config;
	struct video_format fmt = {
		.pixelformat = sc2336_fmts[SC2336_MODE_1024X600_RAW8].pixelformat,
		.width = sc2336_fmts[SC2336_MODE_1024X600_RAW8].width_min,
		.height = sc2336_fmts[SC2336_MODE_1024X600_RAW8].height_min,
	};
	uint32_t chip_id;
	int ret;

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("I2C bus %s is not ready", cfg->i2c.bus->name);
		return -ENODEV;
	}

	if (cfg->input_clk != SC2336_INPUT_CLK_HZ) {
		LOG_ERR("Unsupported input clock %u Hz", cfg->input_clk);
		return -ENOTSUP;
	}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios) ||                                               \
	DT_ANY_INST_HAS_PROP_STATUS_OKAY(powerdown_gpios)
	ret = sc2336_power_on(cfg);
	if (ret < 0) {
		return ret;
	}
#endif

	ret = video_read_cci_reg(&cfg->i2c, SC2336_CCI_CHIP_ID, &chip_id);
	if (ret < 0) {
		LOG_ERR("Unable to read the chip ID (%d)", ret);
		return ret;
	}

	if (chip_id != SC2336_CHIP_ID) {
		LOG_ERR("Wrong chip ID 0x%04x instead of 0x%04x", chip_id, SC2336_CHIP_ID);
		return -ENODEV;
	}

	ret = sc2336_init_ctrls(dev);
	if (ret < 0) {
		return ret;
	}

	return sc2336_set_fmt(dev, &fmt);
}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
#define SC2336_GET_RESET_GPIO(n) .reset_gpio = GPIO_DT_SPEC_INST_GET_OR(n, reset_gpios, {0}),
#else
#define SC2336_GET_RESET_GPIO(n)
#endif

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(powerdown_gpios)
#define SC2336_GET_POWERDOWN_GPIO(n)                                                               \
	.powerdown_gpio = GPIO_DT_SPEC_INST_GET_OR(n, powerdown_gpios, {0}),
#else
#define SC2336_GET_POWERDOWN_GPIO(n)
#endif

#define SC2336_INIT(n)                                                                             \
	static struct sc2336_data sc2336_data_##n;                                                 \
                                                                                                   \
	static const struct sc2336_config sc2336_cfg_##n = {                                       \
		.i2c = I2C_DT_SPEC_INST_GET(n),                                                    \
		.input_clk = DT_INST_PROP_BY_PHANDLE(n, clocks, clock_frequency),                  \
		SC2336_GET_RESET_GPIO(n)                                                           \
		SC2336_GET_POWERDOWN_GPIO(n)                                                       \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, sc2336_init, NULL, &sc2336_data_##n, &sc2336_cfg_##n,             \
			      POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY, &sc2336_driver_api);        \
                                                                                                   \
	VIDEO_DEVICE_DEFINE(sc2336_##n, DEVICE_DT_INST_GET(n), NULL);

DT_INST_FOREACH_STATUS_OKAY(SC2336_INIT)
