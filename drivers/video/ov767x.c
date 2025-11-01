/*
 * Copyright 2024 NXP
 * Copyright 2021 Arduino SA
 * Copyright (c) 2017 OpenMV
 * Copyright (c) 2025 Michael Smorto
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ovti_ov7670

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video-controls.h>
#include <zephyr/logging/log.h>

#include "video_common.h"
#include "video_ctrls.h"
#include "video_device.h"

LOG_MODULE_REGISTER(video_ov767x, CONFIG_VIDEO_LOG_LEVEL);

struct ov767x_config {
	struct i2c_dt_spec bus;
	uint32_t camera_model;
	const struct video_format_cap *fmts;
#if DT_ANY_COMPAT_HAS_PROP_STATUS_OKAY(ovti_ov7670, reset_gpios) ||                                \
	DT_ANY_COMPAT_HAS_PROP_STATUS_OKAY(ovti_ov7675, reset_gpios)
	struct gpio_dt_spec reset;
#endif
#if DT_ANY_COMPAT_HAS_PROP_STATUS_OKAY(ovti_ov7670, pwdn_gpios) ||                                 \
	DT_ANY_COMPAT_HAS_PROP_STATUS_OKAY(ovti_ov7675, pwdn_gpios)
	struct gpio_dt_spec pwdn;
#endif
};

struct ov767x_ctrls {
	struct video_ctrl hflip;
	struct video_ctrl vflip;
};

struct ov767x_data {
	struct ov767x_ctrls ctrls;
	struct video_format fmt;
};

#define OV7670_REG8(addr)         ((addr) | VIDEO_REG_ADDR8_DATA8)
/* OV7670 registers */
#define OV7670_PID                0x0A
#define OV7670_COM7               0x12
#define OV7670_MVFP               0x1E
#define OV7670_COM10              0x15
#define OV7670_COM12              0x3C
#define OV7670_BRIGHT             0x55
#define OV7670_CLKRC              0x11
#define OV7670_SCALING_PCLK_DIV   0x73
#define OV7670_COM14              0x3E
#define OV7670_DBLV               0x6B
#define OV7670_SCALING_XSC        0x70
#define OV7670_SCALING_YSC        0x71
#define OV7670_COM2               0x09
#define OV7670_SCALING_PCLK_DELAY 0xA2
#define OV7670_BD50MAX            0xA5
#define OV7670_BD60MAX            0xAB
#define OV7670_HAECC7             0xAA
#define OV7670_COM3               0x0C
#define OV7670_COM4               0x0D
#define OV7670_COM6               0x0F
#define OV7670_COM11              0x3B
#define OV7670_EDGE               0x3F
#define OV7670_DNSTH              0x4C
#define OV7670_DM_LNL             0x92
#define OV7670_DM_LNH             0x93
#define OV7670_COM15              0x40
#define OV7670_TSLB               0x3A
#define OV7670_COM13              0x3D
#define OV7670_MANU               0x67
#define OV7670_MANV               0x68
#define OV7670_HSTART             0x17
#define OV7670_HSTOP              0x18
#define OV7670_VSTRT              0x19
#define OV7670_VSTOP              0x1A
#define OV7670_HREF               0x32
#define OV7670_VREF               0x03
#define OV7670_SCALING_DCWCTR     0x72
#define OV7670_GAIN               0x00
#define OV7670_AECHH              0x07
#define OV7670_AECH               0x10
#define OV7670_COM8               0x13
#define OV7670_COM9               0x14
#define OV7670_AEW                0x24
#define OV7670_AEB                0x25
#define OV7670_VPT                0x26
#define OV7670_AWBC1              0x43
#define OV7670_AWBC2              0x44
#define OV7670_AWBC3              0x45
#define OV7670_AWBC4              0x46
#define OV7670_AWBC5              0x47
#define OV7670_AWBC6              0x48
#define OV7670_MTX1               0x4F
#define OV7670_MTX2               0x50
#define OV7670_MTX3               0x51
#define OV7670_MTX4               0x52
#define OV7670_MTX5               0x53
#define OV7670_MTX6               0x54
#define OV7670_LCC1               0x62
#define OV7670_LCC2               0x63
#define OV7670_LCC3               0x64
#define OV7670_LCC4               0x65
#define OV7670_LCC5               0x66
#define OV7670_LCC6               0x94
#define OV7670_LCC7               0x95
#define OV7670_SLOP               0x7A
#define OV7670_GAM1               0x7B
#define OV7670_GAM2               0x7C
#define OV7670_GAM3               0x7D
#define OV7670_GAM4               0x7E
#define OV7670_GAM5               0x7F
#define OV7670_GAM6               0x80
#define OV7670_GAM7               0x81
#define OV7670_GAM8               0x82
#define OV7670_GAM9               0x83
#define OV7670_GAM10              0x84
#define OV7670_GAM11              0x85
#define OV7670_GAM12              0x86
#define OV7670_GAM13              0x87
#define OV7670_GAM14              0x88
#define OV7670_GAM15              0x89
#define OV7670_HAECC1             0x9F
#define OV7670_HAECC2             0xA0
#define OV7670_HSYEN              0x31
#define OV7670_HAECC3             0xA6
#define OV7670_HAECC4             0xA7
#define OV7670_HAECC5             0xA8
#define OV7670_HAECC6             0xA9

/* Addition defines to support OV7675 */
#define OV7670_RGB444               0x8C /* REG444 */
#define OV7675_COM3_DCW_EN          0x04 /* DCW enable */
#define OV7670_COM1                 0x04 /*Com Cntrl1 */
#define OV7675_COM7_RGB_FMT         0x04 /* Output format RGB        */
#define OV7675_COM13_GAMMA_EN       0x80 /* Gamma enable */
#define OV7675_COM13_UVSAT_AUTO     0x40 /* UV saturation level - UV auto adjustment. */
#define OV7675_COM15_OUT_00_FF      0xC0 /* Output data range 00 to FF  */
#define OV7675_COM15_FMT_RGB_NORMAL 0x00 /* Normal RGB normal output    */
#define OV7675_COM15_FMT_RGB565     0x10 /* Normal RGB 565 output       */

/* OV7670 definitions */
#define OV7670_PROD_ID 0x76
#define OV7670_MVFP_HFLIP 0x20
#define OV7670_MVFP_VFLIP 0x10

#define OV767X_MODEL_OV7670 7670
#define OV767X_MODEL_OV7675 7675

#define OV767X_VIDEO_FORMAT_CAP(width, height, format)                                             \
	{                                                                                          \
		.pixelformat = (format), .width_min = (width), .width_max = (width),               \
		.height_min = (height), .height_max = (height), .width_step = 0, .height_step = 0  \
	}

#if DT_HAS_COMPAT_STATUS_OKAY(ovti_ov7670)
static const struct video_format_cap ov7670_fmts[] = {
	OV767X_VIDEO_FORMAT_CAP(176, 144, VIDEO_PIX_FMT_RGB565), /* QCIF  */
	OV767X_VIDEO_FORMAT_CAP(320, 240, VIDEO_PIX_FMT_RGB565), /* QVGA  */
	OV767X_VIDEO_FORMAT_CAP(352, 288, VIDEO_PIX_FMT_RGB565), /* CIF   */
	OV767X_VIDEO_FORMAT_CAP(640, 480, VIDEO_PIX_FMT_RGB565), /* VGA   */
	OV767X_VIDEO_FORMAT_CAP(176, 144, VIDEO_PIX_FMT_YUYV),   /* QCIF  */
	OV767X_VIDEO_FORMAT_CAP(320, 240, VIDEO_PIX_FMT_YUYV),   /* QVGA  */
	OV767X_VIDEO_FORMAT_CAP(352, 288, VIDEO_PIX_FMT_YUYV),   /* CIF   */
	OV767X_VIDEO_FORMAT_CAP(640, 480, VIDEO_PIX_FMT_YUYV),   /* VGA   */
	{0}
};
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(ovti_ov7675)
static const struct video_format_cap ov7675_fmts[] = {
	OV767X_VIDEO_FORMAT_CAP(160, 120, VIDEO_PIX_FMT_RGB565), /* QQVGA  */
	OV767X_VIDEO_FORMAT_CAP(320, 240, VIDEO_PIX_FMT_RGB565), /* QVGA  */
	OV767X_VIDEO_FORMAT_CAP(640, 480, VIDEO_PIX_FMT_RGB565), /* VGA   */
	OV767X_VIDEO_FORMAT_CAP(160, 120, VIDEO_PIX_FMT_YUYV),   /* QQVGA  */
	OV767X_VIDEO_FORMAT_CAP(320, 240, VIDEO_PIX_FMT_YUYV),   /* QVGA  */
	OV767X_VIDEO_FORMAT_CAP(640, 480, VIDEO_PIX_FMT_YUYV),   /* VGA   */
	{0}
};
#endif

/*
 * This initialization table is based on the MCUX SDK driver for the OV7670.
 * Note that this table assumes the camera is fed a 6MHz XCLK signal
 */
static const struct video_reg8 ov767x_init_regtbl[] = {
	{OV7670_MVFP, 0x00}, /* MVFP: Mirror/VFlip,Normal image */

	/* configure the output timing */
	/* PCLK does not toggle during horizontal blank, one PCLK, one pixel */
	{OV7670_COM10, 0x20}, /* COM10 */
	{OV7670_COM12, 0x00}, /* COM12,No HREF when VSYNC is low */
	/* Brightness Control, with signal -128 to +128, 0x00 is middle value */
	{OV7670_BRIGHT, 0x2f},
	{OV7670_CLKRC, 0x81}, /* Clock Div, Input/(n+1), bit6 set to 1 to disable divider */

	/* SCALING_PCLK_DIV */
	/* 0: Enable clock divider, 010: Divided by 4 */
	{OV7670_SCALING_PCLK_DIV, 0xF1},
	/* Common Control 14, Bit[4]: DCW and scaling PCLK enable, Bit[3]: Manual scaling */
	{OV7670_COM14, 0x19},
	/* Common Control 3, Bit[2]: DCW enable, Bit[3]: Scale enable */
	{OV7670_COM3, 0x04},

	/* DBLV,Bit[7:6]: PLL control */
	/* 0:Bypass PLL, 40: Input clock x4  , 80: Input clock x6  ,C0: Input clock x8 */
	{OV7670_DBLV, 0x40},

	/* test pattern, useful in some case */
	{OV7670_SCALING_XSC, 0x3A},
	{OV7670_SCALING_YSC, 0x35},

	/* DCW Control */
	{OV7670_SCALING_DCWCTR, 0x11},

	/* Output Drive Capability */
	{OV7670_COM2, 0x00}, /* Common Control 2, Output Drive Capability: 1x */
	{OV7670_SCALING_PCLK_DELAY, 0x02},
	{OV7670_BD50MAX, 0x05},
	{OV7670_BD60MAX, 0x07},
	{OV7670_HAECC7, 0x94},

	{OV7670_COM4, 0x00},
	{OV7670_COM6, 0x4b},
	{OV7670_COM11, 0x9F}, /* Night mode */
	{OV7670_EDGE, 0x04},  /* Edge Enhancement Adjustment */
	{OV7670_DNSTH, 0x00}, /* De-noise Strength */

	{OV7670_DM_LNL, 0x00},
	{OV7670_DM_LNH, 0x00},

	/* reserved */
	{0x16, 0x02},
	{0x21, 0x02},
	{0x22, 0x91},
	{0x29, 0x07},
	{0x35, 0x0b},
	{0x33, 0x0b},
	{0x37, 0x1d},
	{0x38, 0x71},
	{0x39, 0x2a},
	{0x0e, 0x61},
	{0x56, 0x40},
	{0x57, 0x80},
	{0x69, 0x00},
	{0x74, 0x19},

	/* display , need retain */
	{OV7670_COM15, 0xD0}, /* Common Control 15 */
	{OV7670_TSLB, 0x0C},  /* Line Buffer Test Option */
	{OV7670_COM13, 0x80}, /* Common Control 13 */
	{OV7670_MANU, 0x11},  /* Manual U Value */
	{OV7670_MANV, 0xFF},  /* Manual V Value */
	/* config the output window data, this can be configed later */
	{OV7670_HSTART, 0x15}, /*  HSTART */
	{OV7670_HSTOP, 0x03},  /*  HSTOP */
	{OV7670_VSTRT, 0x02},  /*  VSTRT */
	{OV7670_VSTOP, 0x7a},  /*  VSTOP */
	{OV7670_HREF, 0x80},   /*  HREF */
	{OV7670_VREF, 0x0a},   /*  VREF */

	/* AGC/AEC - Automatic Gain Control/Automatic exposure Control */
	{OV7670_GAIN, 0x00},  /*  AGC */
	{OV7670_AECHH, 0x3F}, /* Exposure Value */
	{OV7670_AECH, 0xFF},
	{OV7670_COM8, 0x66},
	{OV7670_COM9, 0x21}, /*  limit the max gain */
	{OV7670_AEW, 0x75},
	{OV7670_AEB, 0x63},
	{OV7670_VPT, 0xA5},
	/* Automatic white balance control */
	{OV7670_AWBC1, 0x14},
	{OV7670_AWBC2, 0xf0},
	{OV7670_AWBC3, 0x34},
	{OV7670_AWBC4, 0x58},
	{OV7670_AWBC5, 0x28},
	{OV7670_AWBC6, 0x3a},
	/* Matrix Coefficient */
	{OV7670_MTX1, 0x80},
	{OV7670_MTX2, 0x80},
	{OV7670_MTX3, 0x00},
	{OV7670_MTX4, 0x22},
	{OV7670_MTX5, 0x5e},
	{OV7670_MTX6, 0x80},
	/* AWB Control */
	{0x59, 0x88},
	{0x5a, 0x88},
	{0x5b, 0x44},
	{0x5c, 0x67},
	{0x5d, 0x49},
	{0x5e, 0x0e},
	{0x6c, 0x0a},
	{0x6d, 0x55},
	{0x6e, 0x11},
	{0x6f, 0x9f},
	/* Lens Correction Option */
	{OV7670_LCC1, 0x00},
	{OV7670_LCC2, 0x00},
	{OV7670_LCC3, 0x04},
	{OV7670_LCC4, 0x20},
	{OV7670_LCC5, 0x05},
	{OV7670_LCC6, 0x04}, /* effective only when LCC5[2] is high */
	{OV7670_LCC7, 0x08}, /* effective only when LCC5[2] is high */
	/* Gamma Curve, needn't config */
	{OV7670_SLOP, 0x20},
	{OV7670_GAM1, 0x1c},
	{OV7670_GAM2, 0x28},
	{OV7670_GAM3, 0x3c},
	{OV7670_GAM4, 0x55},
	{OV7670_GAM5, 0x68},
	{OV7670_GAM6, 0x76},
	{OV7670_GAM7, 0x80},
	{OV7670_GAM8, 0x88},
	{OV7670_GAM9, 0x8f},
	{OV7670_GAM10, 0x96},
	{OV7670_GAM11, 0xa3},
	{OV7670_GAM12, 0xaf},
	{OV7670_GAM13, 0xc4},
	{OV7670_GAM14, 0xd7},
	{OV7670_GAM15, 0xe8},
	/* Histogram-based AEC/AGC Control */
	{OV7670_HAECC1, 0x78},
	{OV7670_HAECC2, 0x68},
	{OV7670_HSYEN, 0xff},
	{0xa1, 0x03},
	{OV7670_HAECC3, 0xdf},
	{OV7670_HAECC4, 0xdf},
	{OV7670_HAECC5, 0xf0},
	{OV7670_HAECC6, 0x90},
	/* Automatic black Level Compensation */
	{0xb0, 0x84},
	{0xb1, 0x0c},
	{0xb2, 0x0e},
	{0xb3, 0x82},
	{0xb8, 0x0a},
};

/* TODO: These registers probably need to be fixed too. */
static const struct video_reg8 ov767x_yuv422_regs[] = {
	{OV7670_COM7, 0x00},   /* Selects YUV mode */
	{OV7670_RGB444, 0x00}, /* No RGB444 please */
	{OV7670_COM1, 0x00},   /* CCIR601 */
	{OV7670_COM15, OV7675_COM15_OUT_00_FF},
	{OV7670_COM9, 0x48}, /* 32x gain ceiling; 0x8 is reserved bit */
	{0x4f, 0x80},        /* "matrix coefficient 1" */
	{0x50, 0x80},        /* "matrix coefficient 2" */
	{0x51, 0x00},        /* vb */
	{0x52, 0x22},        /* "matrix coefficient 4" */
	{0x53, 0x5e},        /* "matrix coefficient 5" */
	{0x54, 0x80},        /* "matrix coefficient 6" */
	{OV7670_COM13, OV7675_COM13_GAMMA_EN | OV7675_COM13_UVSAT_AUTO},
};

#if DT_HAS_COMPAT_STATUS_OKAY(ovti_ov7670)
static const struct video_reg8 ov7670_rgb565_regs[] = {
	{OV7670_COM7, OV7675_COM7_RGB_FMT}, /* Selects RGB mode */
	{OV7670_RGB444, 0x00},              /* No RGB444 please */
	{OV7670_COM1, 0x00},                /* CCIR601 */
	{OV7670_COM15, OV7675_COM15_OUT_00_FF | OV7675_COM15_FMT_RGB565},
	{OV7670_COM9, 0x6a}, /* 16x gain ceiling; 0x8 is reserved bit */
	{0x4f, 0xb3},        /* "matrix coefficient 1" */
	{0x50, 0xb3},        /* "matrix coefficient 2" */
	{0x51, 0x00},        /* "matrix Coefficient 3" */
	{0x52, 0x3d},        /* "matrix coefficient 4" */
	{0x53, 0xa7},        /* "matrix coefficient 5" */
	{0x54, 0xe4},        /* "matrix coefficient 6" */
	{OV7670_COM13, OV7675_COM13_UVSAT_AUTO},
};

/* Resolution settings for camera, based on those present in MCUX SDK */
static const struct video_reg8 ov7670_regs_qcif[] = {
	{OV7670_COM7, 0x2c},
	{OV7670_COM3, 0x00},
	{OV7670_COM14, 0x11},
	{OV7670_SCALING_XSC, 0x3a},
	{OV7670_SCALING_YSC, 0x35},
	{OV7670_SCALING_DCWCTR, 0x11},
	{OV7670_SCALING_PCLK_DIV, 0xf1},
	{OV7670_SCALING_PCLK_DELAY, 0x52},
};

static const struct video_reg8 ov7670_regs_qvga[] = {
	{OV7670_COM7, 0x14},
	{OV7670_COM3, 0x04},
	{OV7670_COM14, 0x19},
	{OV7670_SCALING_XSC, 0x3a},
	{OV7670_SCALING_YSC, 0x35},
	{OV7670_SCALING_DCWCTR, 0x11},
	{OV7670_SCALING_PCLK_DIV, 0xf1},
	{OV7670_SCALING_PCLK_DELAY, 0x02},
};

static const struct video_reg8 ov7670_regs_cif[] = {
	{OV7670_COM7, 0x24},
	{OV7670_COM3, 0x08},
	{OV7670_COM14, 0x11},
	{OV7670_SCALING_XSC, 0x3a},
	{OV7670_SCALING_YSC, 0x35},
	{OV7670_SCALING_DCWCTR, 0x11},
	{OV7670_SCALING_PCLK_DIV, 0xf1},
	{OV7670_SCALING_PCLK_DELAY, 0x02},
};

static const struct video_reg8 ov7670_regs_vga[] = {
	{OV7670_COM7, 0x04},
	{OV7670_COM3, 0x00},
	{OV7670_COM14, 0x00},
	{OV7670_SCALING_XSC, 0x3a},
	{OV7670_SCALING_YSC, 0x35},
	{OV7670_SCALING_DCWCTR, 0x11},
	{OV7670_SCALING_PCLK_DIV, 0xf0},
	{OV7670_SCALING_PCLK_DELAY, 0x02},
};
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(ovti_ov7675)
static const struct video_reg8 ov7675_rgb565_regs[] = {
	{OV7670_COM7, OV7675_COM7_RGB_FMT}, /* Selects RGB mode */
	{OV7670_RGB444, 0x00},              /* No RGB444 please */
	{OV7670_COM1, 0x00},                /* CCIR601 */
	{OV7670_COM15, OV7675_COM15_OUT_00_FF | OV7675_COM15_FMT_RGB565},
	{OV7670_COM9, 0x38}, /* 16x gain ceiling; 0x8 is reserved bit */
	{0x4f, 0xb3},        /* "matrix coefficient 1" */
	{0x50, 0xb3},        /* "matrix coefficient 2" */
	{0x51, 0x00},        /* "matrix Coefficient 3" */
	{0x52, 0x3d},        /* "matrix coefficient 4" */
	{0x53, 0xa7},        /* "matrix coefficient 5" */
	{0x54, 0xe4},        /* "matrix coefficient 6" */
	{OV7670_COM13, OV7675_COM13_GAMMA_EN | OV7675_COM13_UVSAT_AUTO},
};

static const struct video_reg8 ov7675_regs_vga[] = {
	{OV7670_COM3, 0x00},   {OV7670_COM14, 0x00}, {0x72, 0x11}, /* downsample by 4 */
	{0x73, 0xf0},                                              /* divide by 4 */
	{OV7670_HSTART, 0x12}, {OV7670_HSTOP, 0x00}, {OV7670_HREF, 0xb6},
	{OV7670_VSTRT, 0x02},  {OV7670_VSTOP, 0x7a}, {OV7670_VREF, 0x00},
};

static const struct video_reg8 ov7675_regs_qvga[] = {
	{OV7670_COM3, OV7675_COM3_DCW_EN},
	{OV7670_COM14, 0x11}, /* Divide by 2 */
	{0x72, 0x22},
	{0x73, 0xf2},
	{OV7670_HSTART, 0x15},
	{OV7670_HSTOP, 0x03},
	{OV7670_HREF, 0xC0},
	{OV7670_VSTRT, 0x03},
	{OV7670_VSTOP, 0x7B},
	{OV7670_VREF, 0xF0},
};

static const struct video_reg8 ov7675_regs_qqvga[] = {
	{OV7670_COM3, OV7675_COM3_DCW_EN},
	{OV7670_COM14, 0x11}, /* Divide by 2 */
	{0x72, 0x22},
	{0x73, 0xf2},
	{OV7670_HSTART, 0x16},
	{OV7670_HSTOP, 0x04},
	{OV7670_HREF, 0xa4},
	{OV7670_VSTRT, 0x22},
	{OV7670_VSTOP, 0x7a},
	{OV7670_VREF, 0xfa},
};
#endif

static int ov767x_get_caps(const struct device *dev, struct video_caps *caps)
{
	const struct ov767x_config *config = dev->config;

	caps->format_caps = config->fmts;
	return 0;
}

#if DT_HAS_COMPAT_STATUS_OKAY(ovti_ov7670)
static int ov7670_set_fmt(const struct device *dev, struct video_format *fmt)
{
	const struct ov767x_config *config = dev->config;
	int ret = -ENOTSUP;
	uint8_t i = 0U;

	/* Set RGB Format */
	if (fmt->pixelformat == VIDEO_PIX_FMT_RGB565) {
		ret = video_write_cci_multiregs8(&config->bus, ov7670_rgb565_regs,
						 ARRAY_SIZE(ov7670_rgb565_regs));
	} else if (fmt->pixelformat == VIDEO_PIX_FMT_YUYV) {
		ret = video_write_cci_multiregs8(&config->bus, ov767x_yuv422_regs,
						 ARRAY_SIZE(ov767x_yuv422_regs));
	} else {
		LOG_ERR("Image format not supported");
		ret = -ENOTSUP;
	}
	if (ret < 0) {
		LOG_ERR("Format not set!");
		return ret;
	}

	/* Set output resolution */
	while (config->fmts[i].pixelformat) {
		if (config->fmts[i].width_min == fmt->width &&
		    config->fmts[i].height_min == fmt->height &&
		    config->fmts[i].pixelformat == fmt->pixelformat) {
			/* Set output format */
			switch (config->fmts[i].width_min) {
			case 176: /* QCIF */
				ret = video_write_cci_multiregs8(&config->bus, ov7670_regs_qcif,
								 ARRAY_SIZE(ov7670_regs_qcif));
				break;
			case 352: /* QCIF */
				ret = video_write_cci_multiregs8(&config->bus, ov7670_regs_cif,
								 ARRAY_SIZE(ov7670_regs_cif));
				break;
			case 320: /* QVGA */
				ret = video_write_cci_multiregs8(&config->bus, ov7670_regs_qvga,
								 ARRAY_SIZE(ov7670_regs_qvga));
				break;
			case 640: /* VGA */
				ret = video_write_cci_multiregs8(&config->bus, ov7670_regs_vga,
								 ARRAY_SIZE(ov7670_regs_vga));
				break;
			default: /* QVGA */
				ret = video_write_cci_multiregs8(&config->bus, ov7670_regs_qvga,
								 ARRAY_SIZE(ov7670_regs_vga));
				break;
			}
			if (ret < 0) {
				LOG_ERR("Resolution not set!");
				return ret;
			}
		}
		i++;
	}
	if (ret < 0) {
		LOG_ERR("Resolution not supported!");
		return ret;
	}

	return 0;
}
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(ovti_ov7675)
static int ov7675_set_fmt(const struct device *dev, struct video_format *fmt)
{
	const struct ov767x_config *config = dev->config;
	int ret = -ENOTSUP;
	uint8_t i = 0U;

	/* Set RGB Format */
	if (fmt->pixelformat == VIDEO_PIX_FMT_RGB565) {
		ret = video_write_cci_multiregs8(&config->bus, ov7675_rgb565_regs,
						 ARRAY_SIZE(ov7675_rgb565_regs));
	} else if (fmt->pixelformat == VIDEO_PIX_FMT_YUYV) {
		ret = video_write_cci_multiregs8(&config->bus, ov767x_yuv422_regs,
						 ARRAY_SIZE(ov767x_yuv422_regs));
	} else {
		LOG_ERR("Image format not supported");
		ret = -ENOTSUP;
	}

	if (ret < 0) {
		LOG_ERR("Format not set!");
		return ret;
	}

	while (config->fmts[i].pixelformat) {
		if (config->fmts[i].width_min == fmt->width &&
		    config->fmts[i].height_min == fmt->height &&
		    config->fmts[i].pixelformat == fmt->pixelformat) {
			/* Set output format */
			switch (config->fmts[i].width_min) {
			case 160: /* QQVGA */
				ret = video_write_cci_multiregs8(&config->bus, ov7675_regs_qqvga,
								 ARRAY_SIZE(ov7675_regs_qqvga));
				break;
			case 320: /* QVGA */
				ret = video_write_cci_multiregs8(&config->bus, ov7675_regs_qvga,
								 ARRAY_SIZE(ov7675_regs_qvga));
				break;
			case 640: /* VGA */
				ret = video_write_cci_multiregs8(&config->bus, ov7675_regs_vga,
								 ARRAY_SIZE(ov7675_regs_vga));
				break;
			default: /* QVGA */
				ret = video_write_cci_multiregs8(&config->bus, ov7675_regs_qvga,
								 ARRAY_SIZE(ov7675_regs_qvga));
				break;
			}
			if (ret < 0) {
				LOG_ERR("Resolution not set!");
				return ret;
			}
		}
		i++;
	}
	if (ret < 0) {
		LOG_ERR("Resolution not supported!");
		return ret;
	}

	return 0;
}
#endif

static int ov767x_set_fmt(const struct device *dev, struct video_format *fmt)
{
	const struct ov767x_config __maybe_unused *config = dev->config;
	struct ov767x_data *data = dev->data;

	int ret __maybe_unused;

	if (fmt->pixelformat != VIDEO_PIX_FMT_RGB565 && fmt->pixelformat != VIDEO_PIX_FMT_YUYV) {
		LOG_ERR("Only RGB565 and YUYV supported!");
		return -ENOTSUP;
	}

	if (!memcmp(&data->fmt, fmt, sizeof(data->fmt))) {
		/* nothing to do */
		return 0;
	}

	memcpy(&data->fmt, fmt, sizeof(data->fmt));

#if DT_HAS_COMPAT_STATUS_OKAY(ovti_ov7670)
	if (config->camera_model == OV767X_MODEL_OV7670) {
		ret = ov7670_set_fmt(dev, fmt);
		if (ret < 0) {
			return ret;
		}
	}
#endif
#if DT_HAS_COMPAT_STATUS_OKAY(ovti_ov7675)
	if (config->camera_model == OV767X_MODEL_OV7675) {
		ret = ov7675_set_fmt(dev, fmt);
		if (ret < 0) {
			return ret;
		}
	}
#endif

	return 0;
}

static int ov767x_get_fmt(const struct device *dev, struct video_format *fmt)
{
	struct ov767x_data *data = dev->data;

	memcpy(fmt, &data->fmt, sizeof(data->fmt));
	return 0;
}

static int ov767x_init_controls(const struct device *dev)
{
	int ret;
	struct ov767x_data *drv_data = dev->data;
	struct ov767x_ctrls *ctrls = &drv_data->ctrls;

	ret = video_init_ctrl(&ctrls->hflip, dev, VIDEO_CID_HFLIP,
			      (struct video_ctrl_range){.min = 0, .max = 1, .step = 1, .def = 0});
	if (ret) {
		return ret;
	}

	return video_init_ctrl(&ctrls->vflip, dev, VIDEO_CID_VFLIP,
			       (struct video_ctrl_range){.min = 0, .max = 1, .step = 1, .def = 0});
}

static int ov767x_init(const struct device *dev)
{
	const struct ov767x_config *config = dev->config;
	int ret;
	uint8_t pid;
	struct video_format fmt = {
		.pixelformat = VIDEO_PIX_FMT_RGB565,
		.width = 320,
		.height = 240,
	};

	if (!i2c_is_ready_dt(&config->bus)) {
		/* I2C device is not ready, return */
		return -ENODEV;
	}

#if DT_ANY_COMPAT_HAS_PROP_STATUS_OKAY(ovti_ov7670, pwdn_gpios) ||                                 \
	DT_ANY_COMPAT_HAS_PROP_STATUS_OKAY(ovti_ov7675, pwdn_gpios)
	/* Power up camera module */
	if (config->pwdn.port != NULL) {
		if (!gpio_is_ready_dt(&config->pwdn)) {
			return -ENODEV;
		}
		ret = gpio_pin_configure_dt(&config->pwdn, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_ERR("Could not clear power down pin: %d", ret);
			return ret;
		}
	}
#endif

#if DT_ANY_COMPAT_HAS_PROP_STATUS_OKAY(ovti_ov7670, reset_gpios) ||                                \
	DT_ANY_COMPAT_HAS_PROP_STATUS_OKAY(ovti_ov7675, reset_gpios)
	/* Reset camera module */
	if (config->reset.port != NULL) {
		if (!gpio_is_ready_dt(&config->reset)) {
			return -ENODEV;
		}
		ret = gpio_pin_configure_dt(&config->reset, GPIO_OUTPUT);
		if (ret < 0) {
			LOG_ERR("Could not set reset pin: %d", ret);
			return ret;
		}
		/* Reset is active low, has 1ms settling time*/
		gpio_pin_set_dt(&config->reset, 0);
		k_msleep(1);
		gpio_pin_set_dt(&config->reset, 1);
		k_msleep(1);
	}
#endif

	/*
	 * Read product ID from camera. This camera implements the SCCB,
	 * spec- which *should* be I2C compatible, but in practice does
	 * not seem to respond when I2C repeated start commands are used.
	 * To work around this, use a write then a read to interface with
	 * registers.
	 */
	uint8_t cmd = OV7670_PID;

	ret = i2c_write_dt(&config->bus, &cmd, sizeof(cmd));
	if (ret < 0) {
		LOG_ERR("Could not request product ID: %d", ret);
		return ret;
	}
	ret = i2c_read_dt(&config->bus, &pid, sizeof(pid));
	if (ret < 0) {
		LOG_ERR("Could not read product ID: %d", ret);
		return ret;
	}

	if (pid != OV7670_PROD_ID) {
		LOG_ERR("Incorrect product ID: 0x%02X", pid);
		return -ENODEV;
	}

	/* Reset camera registers */
	ret = video_write_cci_reg(&config->bus, OV7670_REG8(OV7670_COM7), 0x80);
	if (ret < 0) {
		LOG_ERR("Could not reset camera: %d", ret);
		return ret;
	}
	/* Delay after reset */
	k_msleep(5);

	ret = ov767x_set_fmt(dev, &fmt);
	if (ret < 0) {
		return ret;
	}

	/* Write initialization values to OV7670 */
	ret = video_write_cci_multiregs8(&config->bus, ov767x_init_regtbl,
					 ARRAY_SIZE(ov767x_init_regtbl));
	if (ret < 0) {
		return ret;
	}

	/* Initialize controls */
	return ov767x_init_controls(dev);
}

static int ov767x_set_stream(const struct device *dev, bool enable, enum video_buf_type type)
{
	return 0;
}

static int ov767x_set_ctrl(const struct device *dev, uint32_t id)
{
	const struct ov767x_config *config = dev->config;
	struct ov767x_data *drv_data = dev->data;
	struct ov767x_ctrls *ctrls = &drv_data->ctrls;

	switch (id) {
	case VIDEO_CID_HFLIP:
		return i2c_reg_update_byte_dt(&config->bus, OV7670_MVFP, OV7670_MVFP_HFLIP,
					      ctrls->hflip.val ? OV7670_MVFP_HFLIP : 0);
	case VIDEO_CID_VFLIP:
		return i2c_reg_update_byte_dt(&config->bus, OV7670_MVFP, OV7670_MVFP_VFLIP,
					      ctrls->vflip.val ? OV7670_MVFP_VFLIP : 0);
	default:
		return -ENOTSUP;
	}
}

static DEVICE_API(video, ov767x_api) = {
	.set_format = ov767x_set_fmt,
	.get_format = ov767x_get_fmt,
	.get_caps = ov767x_get_caps,
	.set_stream = ov767x_set_stream,
	.set_ctrl = ov767x_set_ctrl,
};

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
#define OV767X_RESET_GPIO(inst) .reset = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {}),
#else
#define OV767X_RESET_GPIO(inst)
#endif

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(pwdn_gpios)
#define OV767X_PWDN_GPIO(inst) .pwdn = GPIO_DT_SPEC_INST_GET_OR(inst, pwdn_gpios, {}),
#else
#define OV767X_PWDN_GPIO(inst)
#endif

#define OV767X_INIT(inst, id)                                                                      \
	static const struct ov767x_config ov##id##_config##inst = {                                \
		.bus = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.camera_model = id,                                                                \
		.fmts = ov##id##_fmts,                                                             \
		OV767X_RESET_GPIO(inst)	OV767X_PWDN_GPIO(inst)};                                   \
                                                                                                   \
	static struct ov767x_data ov##id##_data##inst;                                             \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, ov767x_init, NULL, &ov##id##_data##inst,                       \
			      &ov##id##_config##inst, POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY,     \
			      &ov767x_api);                                                        \
                                                                                                   \
	VIDEO_DEVICE_DEFINE(ov##id##inst, DEVICE_DT_INST_GET(inst), NULL);

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ovti_ov7670
DT_INST_FOREACH_STATUS_OKAY_VARGS(OV767X_INIT, 7670)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ovti_ov7675
DT_INST_FOREACH_STATUS_OKAY_VARGS(OV767X_INIT, 7675)
