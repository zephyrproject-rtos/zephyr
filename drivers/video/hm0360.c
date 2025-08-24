/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT himax_hm0360

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video-controls.h>
#include <zephyr/logging/log.h>

#include "video_ctrls.h"
#include "video_device.h"

LOG_MODULE_REGISTER(video_hm0360, CONFIG_VIDEO_LOG_LEVEL);

struct hm0360_config {
	struct i2c_dt_spec bus;
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
	struct gpio_dt_spec reset;
#endif
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(pwdn_gpios)
	struct gpio_dt_spec pwdn;
#endif
};

struct hm0360_reg {
	uint16_t addr;
	uint8_t val;
};

struct hm0360_ctrls {
	struct video_ctrl hflip;
	struct video_ctrl vflip;
	struct video_ctrl test_pattern;
};

struct hm0360_data {
	struct hm0360_ctrls ctrls;
	struct video_format fmt;
	uint16_t cur_frmrate;
};

/* hm0360 registers */
#define MODEL_ID_H        0x0000
#define MODEL_ID_L        0x0001
#define SILICON_REV       0x0002
#define FRAME_COUNT_H     0x0005
#define FRAME_COUNT_L     0x0006
#define PIXEL_ORDER       0x0007
/* Sensor mode control */
#define MODE_SELECT       0x0100
#define IMG_ORIENTATION   0x0101
#define EMBEDDED_LINE_EN  0x0102
#define SW_RESET          0x0103
#define COMMAND_UPDATE    0x0104
/* Sensor exposure gain control */
#define INTEGRATION_H     0x0202
#define INTEGRATION_L     0x0203
#define ANALOG_GAIN       0x0205
#define DIGITAL_GAIN_H    0x020E
#define DIGITAL_GAIN_L    0x020F
/* Clock control */
#define PLL1_CONFIG       0x0300
#define PLL2_CONFIG       0x0301
#define PLL3_CONFIG       0x0302
/* Frame timing control */
#define FRAME_LEN_LINES_H 0x0340
#define FRAME_LEN_LINES_L 0x0341
#define LINE_LEN_PCK_H    0x0342
#define LINE_LEN_PCK_L    0x0343
/* Monochrome programming */
#define MONO_MODE         0x0370
#define MONO_MODE_ISP     0x0371
#define MONO_MODE_SEL     0x0372
/* Binning mode control */
#define H_SUBSAMPLE       0x0380
#define V_SUBSAMPLE       0x0381
#define BINNING_MODE      0x0382
/* Test pattern control */
#define TEST_PATTERN_MODE 0x0601
/* Black level control */
#define BLC_TGT           0x1004
#define BLC2_TGT          0x1009
#define MONO_CTRL         0x100A
/* VSYNC / HSYNC / pixel shift registers  */
#define OPFM_CTRL         0x1014
/* Tone mapping registers */
#define CMPRS_CTRL        0x102F
#define CMPRS_01          0x1030
#define CMPRS_02          0x1031
#define CMPRS_03          0x1032
#define CMPRS_04          0x1033
#define CMPRS_05          0x1034
#define CMPRS_06          0x1035
#define CMPRS_07          0x1036
#define CMPRS_08          0x1037
#define CMPRS_09          0x1038
#define CMPRS_10          0x1039
#define CMPRS_11          0x103A
#define CMPRS_12          0x103B
#define CMPRS_13          0x103C
#define CMPRS_14          0x103D
#define CMPRS_15          0x103E
#define CMPRS_16          0x103F
/* Automatic exposure control */
#define AE_CTRL           0x2000
#define AE_CTRL1          0x2001
#define CNT_ORGH_H        0x2002
#define CNT_ORGH_L        0x2003
#define CNT_ORGV_H        0x2004
#define CNT_ORGV_L        0x2005
#define CNT_STH_H         0x2006
#define CNT_STH_L         0x2007
#define CNT_STV_H         0x2008
#define CNT_STV_L         0x2009
#define CTRL_PG_SKIPCNT   0x200A
#define BV_WIN_WEIGHT_EN  0x200D
#define MAX_INTG_H        0x2029
#define MAX_INTG_L        0x202A
#define MAX_AGAIN         0x202B
#define MAX_DGAIN_H       0x202C
#define MAX_DGAIN_L       0x202D
#define MIN_INTG          0x202E
#define MIN_AGAIN         0x202F
#define MIN_DGAIN         0x2030
#define T_DAMPING         0x2031
#define N_DAMPING         0x2032
#define ALC_TH            0x2033
#define AE_TARGET_MEAN    0x2034
#define AE_MIN_MEAN       0x2035
#define AE_TARGET_ZONE    0x2036
#define CONVERGE_IN_TH    0x2037
#define CONVERGE_OUT_TH   0x2038
#define FS_CTRL           0x203B
#define FS_60HZ_H         0x203C
#define FS_60HZ_L         0x203D
#define FS_50HZ_H         0x203E
#define FS_50HZ_L         0x203F
#define FRAME_CNT_TH      0x205B
#define AE_MEAN           0x205D
#define AE_CONVERGE       0x2060
#define AE_BLI_TGT        0x2070
/* Interrupt control */
#define PULSE_MODE        0x2061
#define PULSE_TH_H        0x2062
#define PULSE_TH_L        0x2063
#define INT_INDIC         0x2064
#define INT_CLEAR         0x2065
/* Motion detection control */
#define MD_CTRL           0x2080
#define ROI_START_END_V   0x2081
#define ROI_START_END_H   0x2082
#define MD_TH_MIN         0x2083
#define MD_TH_STR_L       0x2084
#define MD_TH_STR_H       0x2085
#define MD_LIGHT_COEF     0x2099
#define MD_BLOCK_NUM_TH   0x209B
#define MD_LATENCY        0x209C
#define MD_LATENCY_TH     0x209D
#define MD_CTRL1          0x209E
/* Context switch control registers */
#define PMU_CFG_3         0x3024
#define PMU_CFG_4         0x3025
/* Operation mode control */
#define WIN_MODE          0x3030
/* IO and clock control */
#define PAD_REGISTER_07   0x3112

/* Register bits/values */
#define HIMAX_RESET                  0x01
#define HIMAX_MODE_STANDBY           0x00
#define HIMAX_MODE_STREAMING         0x01 /* I2C triggered streaming enable */
#define HIMAX_MODE_STREAMING_NFRAMES 0x03 /* Output N frames */
#define HIMAX_MODE_STREAMING_TRIG    0x05 /* Hardware Trigger */
#define HIMAX_SET_HMIRROR(r, x)      ((r & 0xFE) | ((x & 1) << 0))
#define HIMAX_SET_VMIRROR(r, x)      ((r & 0xFD) | ((x & 1) << 1))

#define PCLK_RISING_EDGE  0x00
#define PCLK_FALLING_EDGE 0x01
#define AE_CTRL_ENABLE    0x00
#define AE_CTRL_DISABLE   0x01

/* hm0360 definitions */
#define hm0360_PROD_ID         0x0360
#define HIMAX_LINE_LEN_PCK_VGA 0x0300
#define HIMAX_FRAME_LENGTH_VGA 0x0214

#define HIMAX_LINE_LEN_PCK_QVGA 0x0178
#define HIMAX_FRAME_LENGTH_QVGA 0x0109

#define HIMAX_LINE_LEN_PCK_QQVGA 0x0178
#define HIMAX_FRAME_LENGTH_QQVGA 0x0084

/* Initialization register structure */
static const struct hm0360_reg hm0360_default_regs[] = {
    {SW_RESET, 0x00},
    {MONO_MODE, 0x00},
    {MONO_MODE_ISP, 0x01},
    {MONO_MODE_SEL, 0x01},

    /* BLC control */
    {0x1000, 0x01},
    {0x1003, 0x04},
    {BLC_TGT, 0x04},
    {0x1007, 0x01},
    {0x1008, 0x04},
    {BLC2_TGT, 0x04},
    {MONO_CTRL, 0x01},

    /* Output format control */
    {OPFM_CTRL, 0x0C},

    /* Reserved regs */
    {0x101D, 0x00},
    {0x101E, 0x01},
    {0x101F, 0x00},
    {0x1020, 0x01},
    {0x1021, 0x00},

    {CMPRS_CTRL, 0x00},
    {CMPRS_01, 0x09},
    {CMPRS_02, 0x12},
    {CMPRS_03, 0x23},
    {CMPRS_04, 0x31},
    {CMPRS_05, 0x3E},
    {CMPRS_06, 0x4B},
    {CMPRS_07, 0x56},
    {CMPRS_08, 0x5E},
    {CMPRS_09, 0x65},
    {CMPRS_10, 0x72},
    {CMPRS_11, 0x7F},
    {CMPRS_12, 0x8C},
    {CMPRS_13, 0x98},
    {CMPRS_14, 0xB2},
    {CMPRS_15, 0xCC},
    {CMPRS_16, 0xE6},

    {0x3112, 0x00}, /* PCLKO_polarity falling */

    {PLL1_CONFIG, 0x08}, /* Core = 24MHz PCLKO = 24MHz I2C = 12MHz */
    {PLL2_CONFIG, 0x0A}, /* MIPI pre-dev (default) */
    {PLL3_CONFIG, 0x77}, /* PMU/MIPI pre-dev (default) */

    {PMU_CFG_3, 0x08},       /* Disable context switching */
    {PAD_REGISTER_07, 0x00}, /* PCLKO_polarity falling */

    {AE_CTRL, 0x5F}, /* Automatic Exposure (NOTE: Auto framerate enabled) */
    {AE_CTRL1, 0x00},
    {T_DAMPING, 0x20},       /* AE T damping factor */
    {N_DAMPING, 0x00},       /* AE N damping factor */
    {AE_TARGET_MEAN, 0x64},  /* AE target */
    {AE_MIN_MEAN, 0x0A},     /* AE min target mean */
    {AE_TARGET_ZONE, 0x23},  /* AE target zone */
    {CONVERGE_IN_TH, 0x03},  /* AE converge in threshold */
    {CONVERGE_OUT_TH, 0x05}, /* AE converge out threshold */
    {MAX_INTG_H, (HIMAX_FRAME_LENGTH_QVGA - 4) >> 8},
    {MAX_INTG_L, (HIMAX_FRAME_LENGTH_QVGA - 4) & 0xFF},

    {MAX_AGAIN, 0x04}, /* Maximum analog gain */
    {MAX_DGAIN_H, 0x03},
    {MAX_DGAIN_L, 0x3F},
    {INTEGRATION_H, 0x01},
    {INTEGRATION_L, 0x08},

    {MD_CTRL, 0x6A},
    {MD_TH_MIN, 0x01},
    {MD_BLOCK_NUM_TH, 0x01},
    {MD_CTRL1, 0x06},
    {PULSE_MODE, 0x00}, /* Interrupt in level mode. */
    {ROI_START_END_V, 0xF0},
    {ROI_START_END_H, 0xF0},

    {FRAME_LEN_LINES_H, HIMAX_FRAME_LENGTH_QVGA >> 8},
    {FRAME_LEN_LINES_L, HIMAX_FRAME_LENGTH_QVGA & 0xFF},
    {LINE_LEN_PCK_H, HIMAX_LINE_LEN_PCK_QVGA >> 8},
    {LINE_LEN_PCK_L, HIMAX_LINE_LEN_PCK_QVGA & 0xFF},
    {H_SUBSAMPLE, 0x01},
    {V_SUBSAMPLE, 0x01},
    {BINNING_MODE, 0x00},
    {WIN_MODE, 0x00},
    {IMG_ORIENTATION, 0x00},
    {COMMAND_UPDATE, 0x01},

    /* SYNC function config. */
    {0x3010, 0x00},
    {0x3013, 0x01},
    {0x3019, 0x00},
    {0x301A, 0x00},
    {0x301B, 0x20},
    {0x301C, 0xFF},

    /* PREMETER config. */
    {0x3026, 0x03},
    {0x3027, 0x81},
    {0x3028, 0x01},
    {0x3029, 0x00},
    {0x302A, 0x30},
    {0x302E, 0x00},
    {0x302F, 0x00},

    /* Magic regs 🪄. */
    {0x302B, 0x2A},
    {0x302C, 0x00},
    {0x302D, 0x03},
    {0x3031, 0x01},
    {0x3051, 0x00},
    {0x305C, 0x03},
    {0x3060, 0x00},
    {0x3061, 0xFA},
    {0x3062, 0xFF},
    {0x3063, 0xFF},
    {0x3064, 0xFF},
    {0x3065, 0xFF},
    {0x3066, 0xFF},
    {0x3067, 0xFF},
    {0x3068, 0xFF},
    {0x3069, 0xFF},
    {0x306A, 0xFF},
    {0x306B, 0xFF},
    {0x306C, 0xFF},
    {0x306D, 0xFF},
    {0x306E, 0xFF},
    {0x306F, 0xFF},
    {0x3070, 0xFF},
    {0x3071, 0xFF},
    {0x3072, 0xFF},
    {0x3073, 0xFF},
    {0x3074, 0xFF},
    {0x3075, 0xFF},
    {0x3076, 0xFF},
    {0x3077, 0xFF},
    {0x3078, 0xFF},
    {0x3079, 0xFF},
    {0x307A, 0xFF},
    {0x307B, 0xFF},
    {0x307C, 0xFF},
    {0x307D, 0xFF},
    {0x307E, 0xFF},
    {0x307F, 0xFF},
    {0x3080, 0x01},
    {0x3081, 0x01},
    {0x3082, 0x03},
    {0x3083, 0x20},
    {0x3084, 0x00},
    {0x3085, 0x20},
    {0x3086, 0x00},
    {0x3087, 0x20},
    {0x3088, 0x00},
    {0x3089, 0x04},
    {0x3094, 0x02},
    {0x3095, 0x02},
    {0x3096, 0x00},
    {0x3097, 0x02},
    {0x3098, 0x00},
    {0x3099, 0x02},
    {0x309E, 0x05},
    {0x309F, 0x02},
    {0x30A0, 0x02},
    {0x30A1, 0x00},
    {0x30A2, 0x08},
    {0x30A3, 0x00},
    {0x30A4, 0x20},
    {0x30A5, 0x04},
    {0x30A6, 0x02},
    {0x30A7, 0x02},
    {0x30A8, 0x01},
    {0x30A9, 0x00},
    {0x30AA, 0x02},
    {0x30AB, 0x34},
    {0x30B0, 0x03},
    {0x30C4, 0x10},
    {0x30C5, 0x01},
    {0x30C6, 0xBF},
    {0x30C7, 0x00},
    {0x30C8, 0x00},
    {0x30CB, 0xFF},
    {0x30CC, 0xFF},
    {0x30CD, 0x7F},
    {0x30CE, 0x7F},
    {0x30D3, 0x01},
    {0x30D4, 0xFF},
    {0x30D5, 0x00},
    {0x30D6, 0x40},
    {0x30D7, 0x00},
    {0x30D8, 0xA7},
    {0x30D9, 0x05},
    {0x30DA, 0x01},
    {0x30DB, 0x40},
    {0x30DC, 0x00},
    {0x30DD, 0x27},
    {0x30DE, 0x05},
    {0x30DF, 0x07},
    {0x30E0, 0x40},
    {0x30E1, 0x00},
    {0x30E2, 0x27},
    {0x30E3, 0x05},
    {0x30E4, 0x47},
    {0x30E5, 0x30},
    {0x30E6, 0x00},
    {0x30E7, 0x27},
    {0x30E8, 0x05},
    {0x30E9, 0x87},
    {0x30EA, 0x30},
    {0x30EB, 0x00},
    {0x30EC, 0x27},
    {0x30ED, 0x05},
    {0x30EE, 0x00},
    {0x30EF, 0x40},
    {0x30F0, 0x00},
    {0x30F1, 0xA7},
    {0x30F2, 0x05},
    {0x30F3, 0x01},
    {0x30F4, 0x40},
    {0x30F5, 0x00},
    {0x30F6, 0x27},
    {0x30F7, 0x05},
    {0x30F8, 0x07},
    {0x30F9, 0x40},
    {0x30FA, 0x00},
    {0x30FB, 0x27},
    {0x30FC, 0x05},
    {0x30FD, 0x47},
    {0x30FE, 0x30},
    {0x30FF, 0x00},
    {0x3100, 0x27},
    {0x3101, 0x05},
    {0x3102, 0x87},
    {0x3103, 0x30},
    {0x3104, 0x00},
    {0x3105, 0x27},
    {0x3106, 0x05},
    {0x310B, 0x10},
    {0x3113, 0xA0},
    {0x3114, 0x67},
    {0x3115, 0x42},
    {0x3116, 0x10},
    {0x3117, 0x0A},
    {0x3118, 0x3F},
    {0x311C, 0x10},
    {0x311D, 0x06},
    {0x311E, 0x0F},
    {0x311F, 0x0E},
    {0x3120, 0x0D},
    {0x3121, 0x0F},
    {0x3122, 0x00},
    {0x3123, 0x1D},
    {0x3126, 0x03},
    {0x3128, 0x57},
    {0x312A, 0x11},
    {0x312B, 0x41},
    {0x312E, 0x00},
    {0x312F, 0x00},
    {0x3130, 0x0C},
    {0x3141, 0x2A},
    {0x3142, 0x9F},
    {0x3147, 0x18},
    {0x3149, 0x18},
    {0x314B, 0x01},
    {0x3150, 0x50},
    {0x3152, 0x00},
    {0x3156, 0x2C},
    {0x315A, 0x0A},
    {0x315B, 0x2F},
    {0x315C, 0xE0},
    {0x315F, 0x02},
    {0x3160, 0x1F},
    {0x3163, 0x1F},
    {0x3164, 0x7F},
    {0x3165, 0x7F},
    {0x317B, 0x94},
    {0x317C, 0x00},
    {0x317D, 0x02},
    {0x318C, 0x00},

    {0x310F, 0x00}, /* puts it in 8bit mode */
    {0x3112, 0x04}, /* was 0x0c */

    {COMMAND_UPDATE, 0x01},
};

static const struct hm0360_reg hm0360_vga_regs[] = {
    {PLL1_CONFIG,         0x08}, /* Core=24MHz PCLKO=24MHz I2C= 12MHz */
    {H_SUBSAMPLE,         0x00},
    {V_SUBSAMPLE,         0x00},
    {BINNING_MODE,        0x00},
    {WIN_MODE,            0x01},
    {MAX_INTG_H,          (HIMAX_FRAME_LENGTH_VGA - 4) >> 8},
    {MAX_INTG_L,          (HIMAX_FRAME_LENGTH_VGA - 4) & 0xFF},
    {FRAME_LEN_LINES_H,   (HIMAX_FRAME_LENGTH_VGA >> 8)},
    {FRAME_LEN_LINES_L,   (HIMAX_FRAME_LENGTH_VGA & 0xFF)},
    {LINE_LEN_PCK_H,      (HIMAX_LINE_LEN_PCK_VGA >> 8)},
    {LINE_LEN_PCK_L,      (HIMAX_LINE_LEN_PCK_VGA & 0xFF)},
    {ROI_START_END_H,     0xF0},
    {ROI_START_END_V,     0xE0},
    {0x310F,              0x00}, /* puts it in 8bit mode */
    {0x3112,              0x04}, /* was 0x0c */
    {COMMAND_UPDATE,      0x01},
};

static const struct hm0360_reg hm0360_qvga_regs[] = {
    {PLL1_CONFIG,         0x09},  /* Core=24MHz PCLKO=24MHz I2C= 12MHz */
    {H_SUBSAMPLE,         0x01},
    {V_SUBSAMPLE,         0x01},
    {BINNING_MODE,        0x00},
    {WIN_MODE,            0x01},
    {MAX_INTG_H,          (HIMAX_FRAME_LENGTH_QVGA - 4) >> 8},
    {MAX_INTG_L,          (HIMAX_FRAME_LENGTH_QVGA - 4) & 0xFF},
    {FRAME_LEN_LINES_H,   (HIMAX_FRAME_LENGTH_QVGA >> 8)},
    {FRAME_LEN_LINES_L,   (HIMAX_FRAME_LENGTH_QVGA & 0xFF)},
    {LINE_LEN_PCK_H,      (HIMAX_LINE_LEN_PCK_QVGA >> 8)},
    {LINE_LEN_PCK_L,      (HIMAX_LINE_LEN_PCK_QVGA & 0xFF)},
    {ROI_START_END_H,     0xF0},
    {ROI_START_END_V,     0xE0},
    {0x310F,              0x00}, /* added to get a good pic */
    {0x3112,              0x04}, /* was 0x0c */
    {COMMAND_UPDATE,      0x01},
};

static const struct hm0360_reg hm0360_qqvga_regs[] = {
    {PLL1_CONFIG, 0x09}, /* Core = 12MHz PCLKO = 24MHz I2C = 12MHz */
    {H_SUBSAMPLE, 0x02},
    {V_SUBSAMPLE, 0x02},
    {BINNING_MODE, 0x00},
    {WIN_MODE, 0x01},
    {MAX_INTG_H, (HIMAX_FRAME_LENGTH_QQVGA - 4) >> 8},
    {MAX_INTG_L, (HIMAX_FRAME_LENGTH_QQVGA - 4) & 0xFF},
    {FRAME_LEN_LINES_H, (HIMAX_FRAME_LENGTH_QQVGA >> 8)},
    {FRAME_LEN_LINES_L, (HIMAX_FRAME_LENGTH_QQVGA & 0xFF)},
    {LINE_LEN_PCK_H, (HIMAX_LINE_LEN_PCK_QQVGA >> 8)},
    {LINE_LEN_PCK_L, (HIMAX_LINE_LEN_PCK_QQVGA & 0xFF)},
    {ROI_START_END_H, 0xF0},
    {ROI_START_END_V, 0xD0},
    {COMMAND_UPDATE, 0x01},
};

#define hm0360_VIDEO_FORMAT_CAP(width, height, format)                                             \
	{                                                                                          \
		.pixelformat = (format), .width_min = (width), .width_max = (width),               \
		.height_min = (height), .height_max = (height), .width_step = 0, .height_step = 0  \
	}

static const struct video_format_cap fmts[] = {
	hm0360_VIDEO_FORMAT_CAP(160, 140, VIDEO_PIX_FMT_GREY ), /* QQVGA  */
	hm0360_VIDEO_FORMAT_CAP(320, 240, VIDEO_PIX_FMT_GREY ), /* QVGA  */
	hm0360_VIDEO_FORMAT_CAP(640, 480, VIDEO_PIX_FMT_GREY ), /* VGA   */
	{0}
};

static int hm0360_read_reg(const struct i2c_dt_spec *spec, const uint16_t addr, void *val,
			   const uint8_t val_size)
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

static int hm0360_write_reg(const struct i2c_dt_spec *spec, const uint16_t addr, const uint8_t val)
{
	uint8_t addr_buf[2];
	struct i2c_msg msg[2];

	addr_buf[1] = addr & 0xFF;
	addr_buf[0] = addr >> 8;
	msg[0].buf = addr_buf;
	msg[0].len = 2U;
	msg[0].flags = I2C_MSG_WRITE;

	msg[1].buf = (uint8_t *)&val;
	msg[1].len = 1;
	msg[1].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	return i2c_transfer_dt(spec, msg, 2);
}

static int hm0360_write_multi_regs(const struct i2c_dt_spec *spec, const struct hm0360_reg *regs,
				   const uint32_t num_regs)
{
	int ret;

	for (int i = 0; i < num_regs; i++) {
		ret = hm0360_write_reg(spec, regs[i].addr, regs[i].val);
		if (ret) {
			return ret;
		}
	}

	return 0;
}

static int hm0360_set_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct hm0360_config *config = dev->config;
	struct video_format fmt;
	struct hm0360_data *drv_data = dev->data;

	uint8_t ret;
	uint16_t i = 0U;

	uint32_t pll_cfg = 0;
	uint32_t osc_div = 0;
	bool highres = false;

	video_get_format(dev, &fmt);

	/* Set output resolution */
	while (fmts[i].pixelformat) {
		if (fmts[i].width_min == fmt.width && fmts[i].height_min == fmt.height) {
			/* Set output format */
			switch (fmts[i].width_min) {
			case 160: /* QQVGA */
				highres = false;
				break;
			case 320: /* QVGA */
				highres = false;
				break;
			case 640: /* VGA */
				highres = true;
				break;
			default:
				LOG_ERR("Resolution not set!");
				return -ENOTSUP;
			}
		}
		i++;
	}

	drv_data->cur_frmrate = frmival->numerator;

	if (frmival->numerator <= 10) {
		osc_div = 0x03;
	} else if (frmival->numerator <= 15) {
		osc_div = (highres == true) ? 0x02 : 0x03;
	} else if (frmival->numerator <= 30) {
		osc_div = (highres == true) ? 0x01 : 0x02;
	} else {
		/* Set to the max possible FPS at this resolution. */
		osc_div = (highres == true) ? 0x00 : 0x01;
	}

	ret = hm0360_write_reg(&config->bus, PLL1_CONFIG, pll_cfg);
	if (ret) {
		LOG_ERR("Could not read PLL1 Config");
		return ret;
	}

	ret = hm0360_write_reg(&config->bus, PLL1_CONFIG, (pll_cfg & 0xFC) | osc_div);
	if (ret) {
		LOG_ERR("Could not set PLL1 Config");
		return ret;
	}

	LOG_ERR("FrameRate selected: %d", frmival->numerator);
	LOG_ERR("HIRES Selecte: %d", highres);
	LOG_ERR("OSC DIV: %d", osc_div);

	return 0;
}

static int hm0360_get_frmival(const struct device *dev, struct video_frmival *frmival)
{
	struct hm0360_data *drv_data = dev->data;

	frmival->numerator = drv_data->cur_frmrate;
	frmival->denominator = 1;

	return 0;
}

static int hm0360_get_caps(const struct device *dev, struct video_caps *caps)
{
	caps->format_caps = fmts;
	return 0;
}

static int hm0360_set_fmt(const struct device *dev, struct video_format *fmt)
{
	const struct hm0360_config *config = dev->config;
	struct hm0360_data *data = dev->data;
	int ret;
	uint16_t i = 0U;

	if (fmt->pixelformat != VIDEO_PIX_FMT_GREY) {
		LOG_ERR("Only Grayscale supported!");
		return -ENOTSUP;
	}

	if (!memcmp(&data->fmt, fmt, sizeof(data->fmt))) {
		/* nothing to do */
		return 0;
	}

	memcpy(&data->fmt, fmt, sizeof(data->fmt));

	k_msleep(300);
	/* Note: hm0360 only supports grayscale so pixelformat does not need to be set */
	LOG_INF("hm0360_set_fmt: W:%u H:%u fmt:%x", fmt->width, fmt->height, fmt->pixelformat);
	/* Set output resolution */
	while (fmts[i].pixelformat) {
		if (fmts[i].width_min == fmt->width && fmts[i].height_min == fmt->height &&
		    fmts[i].pixelformat == fmt->pixelformat) {
			/* Set output format */
			switch (fmts[i].width_min) {
			case 160: /* QQVGA */
				ret = hm0360_write_multi_regs(&config->bus, hm0360_qqvga_regs,
							      ARRAY_SIZE(hm0360_qqvga_regs));
				break;
			case 320: /* QVGA */
				ret = hm0360_write_multi_regs(&config->bus, hm0360_qvga_regs,
							      ARRAY_SIZE(hm0360_qvga_regs));
				break;
			default: /* VGA */
				ret = hm0360_write_multi_regs(&config->bus, hm0360_vga_regs,
							      ARRAY_SIZE(hm0360_vga_regs));
				break;
			}
			if (ret < 0) {
				LOG_ERR("Resolution not set!");
				return ret;
			}
		}
		i++;
	}

	k_msleep(200);

	return 1;
}

static int hm0360_get_fmt(const struct device *dev, struct video_format *fmt)
{
	struct hm0360_data *data = dev->data;

	memcpy(fmt, &data->fmt, sizeof(data->fmt));
	return 0;
}

static int hm0360_init_controls(const struct device *dev)
{
	int ret;
	LOG_ERR("Controls initialized!\n");
	struct hm0360_data *drv_data = dev->data;
	struct hm0360_ctrls *ctrls = &drv_data->ctrls;

	ret = video_init_ctrl(&ctrls->hflip, dev, VIDEO_CID_HFLIP,
			      (struct video_ctrl_range){.min = 0, .max = 1, .step = 1, .def = 0});
	if (ret) {
		return ret;
	}

	return video_init_ctrl(&ctrls->vflip, dev, VIDEO_CID_VFLIP,
			       (struct video_ctrl_range){.min = 0, .max = 1, .step = 1, .def = 0});
}

static int hm0360_init(const struct device *dev)
{
	const struct hm0360_config *config = dev->config;
	struct video_frmival frmival;

	int ret;

	struct video_format fmt = {
		.pixelformat = VIDEO_PIX_FMT_GREY,
		.width = 320,
		.height = 240,
	};

	if (!i2c_is_ready_dt(&config->bus)) {
		/* I2C device is not ready, return */
		return -ENODEV;
	}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(pwdn_gpios)
	/* Power up camera module */
	if (config->pwdn.port != NULL) {
		if (!gpio_is_ready_dt(&config->pwdn)) {
			return -ENODEV;
		}
		ret = gpio_pin_configure_dt(&config->pwdn, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Could not clear power down pin: %d", ret);
			return ret;
		}
	}
	gpio_pin_set_dt(&config->pwdn, 1);
#endif
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
	/* Reset camera module */
	if (config->reset.port != NULL) {
		if (!gpio_is_ready_dt(&config->reset)) {
			return -ENODEV;
		}
		ret = gpio_pin_configure_dt(&config->reset, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Could not set reset pin: %d", ret);
			return ret;
		}
		/* Reset is active high, has 1ms settling time*/
		k_msleep(1);
		gpio_pin_set_dt(&config->reset, 0);
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
	uint8_t Data;
	uint16_t pid = 0x0000;

	/*  Read Product ID and check if PID OK  */
	ret = hm0360_read_reg(&config->bus, MODEL_ID_H, &Data, sizeof(Data));
	if (ret < 0) {
		LOG_ERR("Could not read product ID: %d", ret);
		return ret;
	}
	pid = (Data << 8);

	ret = hm0360_read_reg(&config->bus, MODEL_ID_L, &Data, sizeof(Data));
	if (ret < 0) {
		LOG_ERR("Could not read product ID: %d", ret);
		return ret;
	}
	pid |= Data;

	if (pid != hm0360_PROD_ID) {
		LOG_ERR("Incorrect product ID: 0x%02X", pid);
		return -ENODEV;
	}
	LOG_ERR("Product ID is: %x", pid);

	/* Reset camera registers */
	ret = hm0360_write_reg(&config->bus, SW_RESET, HIMAX_RESET);

	if (ret) {
		LOG_ERR("Could not write reset: %d", ret);
		return ret;
	}
	/* Delay for 1ms. */
	k_msleep(1);

	/* Write Default registers */
	ret = hm0360_write_multi_regs(&config->bus, hm0360_default_regs,
				      ARRAY_SIZE(hm0360_default_regs));
	if (ret < 0) {
		LOG_ERR("Failed to set default registers");
		return ret;
	}

	hm0360_set_fmt(dev, &fmt);
	if (ret) {
		return ret;
	}

	frmival.numerator = 15;
	frmival.denominator = 1;
	if (hm0360_set_frmival(dev, &frmival)) {
		printk("ERROR: Unable to set up video format\n");
		return false;
	}

	/* Initialize controls */
	return hm0360_init_controls(dev);
}

static int hm0360_set_stream(const struct device *dev, bool enable, enum video_buf_type type)
{
	uint8_t ret;
	const struct hm0360_config *config = dev->config;

	ret = hm0360_write_reg(&config->bus, MODE_SELECT, HIMAX_MODE_STREAMING);
	if (ret) {
		LOG_ERR("Could not set streaming");
		return ret;
	}
	k_msleep(4);

	return 0;
}

static int hm0360_set_ctrl(const struct device *dev, uint32_t id)
{
	const struct hm0360_config *config = dev->config;
	struct hm0360_data *drv_data = dev->data;
	struct hm0360_ctrls *ctrls = &drv_data->ctrls;

	uint8_t ret;
	uint8_t data;

	switch (id) {
	case VIDEO_CID_TEST_PATTERN:
		ret = hm0360_write_reg(&config->bus, TEST_PATTERN_MODE,
				       ctrls->test_pattern.val & 0x01);
		ret |= hm0360_write_reg(&config->bus, COMMAND_UPDATE, 0x01);
		return ret;
	case VIDEO_CID_HFLIP:
		ret = hm0360_read_reg(&config->bus, IMG_ORIENTATION, &data, sizeof(data));
		ret |= hm0360_write_reg(&config->bus, IMG_ORIENTATION,
					HIMAX_SET_HMIRROR(data, ctrls->hflip.val));
		ret |= hm0360_write_reg(&config->bus, COMMAND_UPDATE, 0x01);
		return ret;
	case VIDEO_CID_VFLIP:
		ret = hm0360_read_reg(&config->bus, IMG_ORIENTATION, &data, sizeof(data));
		ret |= hm0360_write_reg(&config->bus, IMG_ORIENTATION,
					HIMAX_SET_VMIRROR(data, ctrls->vflip.val));
		ret |= hm0360_write_reg(&config->bus, COMMAND_UPDATE, 0x01);
		return ret;
	default:
		return -ENOTSUP;
	}
}

static DEVICE_API(video, hm0360_api) = {
	.set_format = hm0360_set_fmt,
	.get_format = hm0360_get_fmt,
	.get_caps = hm0360_get_caps,
	.set_stream = hm0360_set_stream,
	.set_ctrl = hm0360_set_ctrl,
	.set_frmival = hm0360_set_frmival,
	.get_frmival = hm0360_get_frmival,
};

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
#define HM0360_RESET_GPIO(inst) .reset = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {}),
#else
#define HM0360_RESET_GPIO(inst)
#endif

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(pwdn_gpios)
#define HM0360_PWDN_GPIO(inst) .pwdn = GPIO_DT_SPEC_INST_GET_OR(inst, pwdn_gpios, {}),
#else
#define HM0360_PWDN_GPIO(inst)
#endif

#define HM0360_INIT(inst)                                                                          \
	const struct hm0360_config hm0360_config_##inst = {.bus = I2C_DT_SPEC_INST_GET(inst),      \
							   HM0360_RESET_GPIO(inst)                 \
								   HM0360_PWDN_GPIO(inst)};        \
	struct hm0360_data hm0360_data_##inst;                                                     \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, hm0360_init, NULL, &hm0360_data_##inst, &hm0360_config_##inst, \
			      POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY, &hm0360_api);               \
                                                                                                   \
	VIDEO_DEVICE_DEFINE(hm0360_##inst, DEVICE_DT_INST_GET(inst), NULL);

DT_INST_FOREACH_STATUS_OKAY(HM0360_INIT)
