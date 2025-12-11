/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT himax_hm0360

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video-controls.h>
#include <zephyr/logging/log.h>

#include "video_common.h"
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

struct hm0360_ctrls {
	struct video_ctrl hflip;
	struct video_ctrl vflip;
};

struct hm0360_data {
	struct hm0360_ctrls ctrls;
	struct video_format fmt;
	uint16_t cur_frmrate;
};

struct video_frmival default_frmival = {
	.numerator = 1,
	.denominator = 15
};

enum {
	HM0360_60_FPS,
	HM0360_30_FPS,
	HM0360_15_FPS,
	HM0360_10_FPS,
};

/* hm0360 registers */
#define HM0360_REG8(addr)         ((addr) | VIDEO_REG_ADDR16_DATA8)
#define HM0360_REG16(addr)        ((addr) | VIDEO_REG_ADDR16_DATA16_BE)
#define HM0360_CCI_ID             HM0360_REG16(0x0000)
#define HM0360_CCI_RESET          HM0360_REG8(0x0103)
#define HM0360_CCI_MODE           HM0360_REG8(0x0100)
#define HM0360_CCI_ORIENTATION		HM0360_REG8(0x0101)
#define HM0360_CCI_COMMAND_UPDATE HM0360_REG8(0x0104)
#define HM0360_CCI_PLL1_CONFIG     HM0360_REG8(0x0300)
#define HM0360_CCI_H_SUBSAMPLE     HM0360_REG8(0x0380)
#define HM0360_CCI_V_SUBSAMPLE     HM0360_REG8(0x0381)
#define HM0360_CCI_BINNING_MODE    HM0360_REG8(0x0382)
#define HM0360_CCI_WIN_MODE        HM0360_REG8(0x3030)
#define HM0360_CCI_MAX_INTG        HM0360_REG16(0x2029)
#define HM0360_CCI_FRAME_LEN_LINES HM0360_REG16(0x0340)
#define HM0360_CCI_LINE_LEN_PCK    HM0360_REG16(0x0342)
#define HM0360_CCI_ROI_START_END_H HM0360_REG8(0x2082)
#define HM0360_CCI_ROI_START_END_V HM0360_REG8(0x2081)
#define HM0360_CCI_BIT_WIDTH        HM0360_REG8(0x310F)
#define HM0360_CCI_PAD_REGISTER_07 HM0360_REG8(0x3112)

/* Sensor mode control */
#define HM0360_IMG_ORIENTATION   0x0101
#define HM0360_EMBEDDED_LINE_EN  0x0102
#define HM0360_COMMAND_UPDATE    0x0104
/* Sensor exposure gain control */
#define HM0360_INTEGRATION_H     0x0202
#define HM0360_INTEGRATION_L     0x0203
#define HM0360_ANALOG_GAIN       0x0205
#define HM0360_DIGITAL_GAIN_H    0x020E
#define HM0360_DIGITAL_GAIN_L    0x020F
/* Clock control */
#define HM0360_PLL1_CONFIG       0x0300
#define HM0360_PLL2_CONFIG       0x0301
#define HM0360_PLL3_CONFIG       0x0302
/* Frame timing control */
#define HM0360_FRAME_LEN_LINES_H 0x0340
#define HM0360_FRAME_LEN_LINES_L 0x0341
#define HM0360_LINE_LEN_PCK_H    0x0342
#define HM0360_LINE_LEN_PCK_L    0x0343
/* Monochrome programming */
#define HM0360_MONO_MODE         0x0370
#define HM0360_MONO_MODE_ISP     0x0371
#define HM0360_MONO_MODE_SEL     0x0372
/* Binning mode control */
#define HM0360_H_SUBSAMPLE       0x0380
#define HM0360_V_SUBSAMPLE       0x0381
#define HM0360_BINNING_MODE      0x0382
/* Test pattern control */
#define HM0360_TEST_PATTERN_MODE 0x0601
/* Black level control */
#define HM0360_BLC_TGT           0x1004
#define HM0360_BLC2_TGT          0x1009
#define HM0360_MONO_CTRL         0x100A
/* VSYNC / HSYNC / pixel shift registers  */
#define HM0360_OPFM_CTRL         0x1014
/* Tone mapping registers */
#define HM0360_CMPRS_CTRL        0x102F
#define HM0360_CMPRS_01          0x1030
#define HM0360_CMPRS_02          0x1031
#define HM0360_CMPRS_03          0x1032
#define HM0360_CMPRS_04          0x1033
#define HM0360_CMPRS_05          0x1034
#define HM0360_CMPRS_06          0x1035
#define HM0360_CMPRS_07          0x1036
#define HM0360_CMPRS_08          0x1037
#define HM0360_CMPRS_09          0x1038
#define HM0360_CMPRS_10          0x1039
#define HM0360_CMPRS_11          0x103A
#define HM0360_CMPRS_12          0x103B
#define HM0360_CMPRS_13          0x103C
#define HM0360_CMPRS_14          0x103D
#define HM0360_CMPRS_15          0x103E
#define HM0360_CMPRS_16          0x103F
/* Automatic exposure control */
#define HM0360_AE_CTRL           0x2000
#define HM0360_AE_CTRL1          0x2001
#define HM0360_CNT_ORGH_H        0x2002
#define HM0360_CNT_ORGH_L        0x2003
#define HM0360_CNT_ORGV_H        0x2004
#define HM0360_CNT_ORGV_L        0x2005
#define HM0360_CNT_STH_H         0x2006
#define HM0360_CNT_STH_L         0x2007
#define HM0360_CNT_STV_H         0x2008
#define HM0360_CNT_STV_L         0x2009
#define HM0360_CTRL_PG_SKIPCNT   0x200A
#define HM0360_BV_WIN_WEIGHT_EN  0x200D
#define HM0360_MAX_INTG_H        0x2029
#define HM0360_MAX_INTG_L        0x202A
#define HM0360_MAX_AGAIN         0x202B
#define HM0360_MAX_DGAIN_H       0x202C
#define HM0360_MAX_DGAIN_L       0x202D
#define HM0360_MIN_INTG          0x202E
#define HM0360_MIN_AGAIN         0x202F
#define HM0360_MIN_DGAIN         0x2030
#define HM0360_T_DAMPING         0x2031
#define HM0360_N_DAMPING         0x2032
#define HM0360_ALC_TH            0x2033
#define HM0360_AE_TARGET_MEAN    0x2034
#define HM0360_AE_MIN_MEAN       0x2035
#define HM0360_AE_TARGET_ZONE    0x2036
#define HM0360_CONVERGE_IN_TH    0x2037
#define HM0360_CONVERGE_OUT_TH   0x2038
#define HM0360_FS_CTRL           0x203B
#define HM0360_FS_60HZ_H         0x203C
#define HM0360_FS_60HZ_L         0x203D
#define HM0360_FS_50HZ_H         0x203E
#define HM0360_FS_50HZ_L         0x203F
#define HM0360_FRAME_CNT_TH      0x205B
#define HM0360_AE_MEAN           0x205D
#define HM0360_AE_CONVERGE       0x2060
#define HM0360_AE_BLI_TGT        0x2070
/* Interrupt control */
#define HM0360_PULSE_MODE        0x2061
#define HM0360_PULSE_TH_H        0x2062
#define HM0360_PULSE_TH_L        0x2063
#define HM0360_INT_INDIC         0x2064
#define HM0360_INT_CLEAR         0x2065
/* Motion detection control */
#define HM0360_MD_CTRL           0x2080
#define HM0360_ROI_START_END_V   0x2081
#define HM0360_ROI_START_END_H   0x2082
#define HM0360_MD_TH_MIN         0x2083
#define HM0360_MD_TH_STR_L       0x2084
#define HM0360_MD_TH_STR_H       0x2085
#define HM0360_MD_LIGHT_COEF     0x2099
#define HM0360_MD_BLOCK_NUM_TH   0x209B
#define HM0360_MD_LATENCY        0x209C
#define HM0360_MD_LATENCY_TH     0x209D
#define HM0360_MD_CTRL1          0x209E
/* Context switch control registers */
#define HM0360_PMU_CFG_3         0x3024
#define HM0360_PMU_CFG_4         0x3025
/* Operation mode control */
#define HM0360_WIN_MODE          0x3030
/* IO and clock control */
#define HM0360_PAD_REGISTER_07   0x3112

/* Register bits/values */
#define HM0360_RESET                  0x01
#define HM0360_MODE_STANDBY           0x00
#define HM0360_MODE_STREAMING         0x01 /* I2C triggered streaming enable */
#define HM0360_MODE_STREAMING_NFRAMES 0x03 /* Output N frames */
#define HM0360_MODE_STREAMING_TRIG    0x05 /* Hardware Trigger */
#define HM0360_ORIENTATION_HMIRROR BIT(0)
#define HM0360_ORIENTATION_VMIRROR BIT(1)

#define HM0360_PCLK_RISING_EDGE  0x00
#define HM0360_PCLK_FALLING_EDGE 0x01
#define HM0360_AE_CTRL_ENABLE    0x00
#define HM0360_AE_CTRL_DISABLE   0x01

/* hm0360 definitions */
#define HM0360_PROD_ID          0x0360
#define HM0360_LINE_LEN_PCK_VGA 0x0300
#define HM0360_FRAME_LENGTH_VGA 0x0214

#define HM0360_LINE_LEN_PCK_QVGA 0x0178
#define HM0360_FRAME_LENGTH_QVGA 0x0109

#define HM0360_LINE_LEN_PCK_QQVGA 0x0178
#define HM0360_FRAME_LENGTH_QQVGA 0x0084

/* Initialization register structure */
static const struct video_reg16 hm0360_default_regs[] = {
	{HM0360_MONO_MODE, 0x00},
	{HM0360_MONO_MODE_ISP, 0x01},
	{HM0360_MONO_MODE_SEL, 0x01},

	/* BLC control */
	{0x1000, 0x01},
	{0x1003, 0x04},
	{HM0360_BLC_TGT, 0x04},
	{0x1007, 0x01},
	{0x1008, 0x04},
	{HM0360_BLC2_TGT, 0x04},
	{HM0360_MONO_CTRL, 0x01},

	/* Output format control */
	{HM0360_OPFM_CTRL, 0x0C},

	/* Reserved regs */
	{0x101D, 0x00},
	{0x101E, 0x01},
	{0x101F, 0x00},
	{0x1020, 0x01},
	{0x1021, 0x00},

	{HM0360_CMPRS_CTRL, 0x00},
	{HM0360_CMPRS_01, 0x09},
	{HM0360_CMPRS_02, 0x12},
	{HM0360_CMPRS_03, 0x23},
	{HM0360_CMPRS_04, 0x31},
	{HM0360_CMPRS_05, 0x3E},
	{HM0360_CMPRS_06, 0x4B},
	{HM0360_CMPRS_07, 0x56},
	{HM0360_CMPRS_08, 0x5E},
	{HM0360_CMPRS_09, 0x65},
	{HM0360_CMPRS_10, 0x72},
	{HM0360_CMPRS_11, 0x7F},
	{HM0360_CMPRS_12, 0x8C},
	{HM0360_CMPRS_13, 0x98},
	{HM0360_CMPRS_14, 0xB2},
	{HM0360_CMPRS_15, 0xCC},
	{HM0360_CMPRS_16, 0xE6},

	{HM0360_PLL1_CONFIG, 0x08}, /* Core = 24MHz PCLKO = 24MHz I2C = 12MHz */
	{HM0360_PLL2_CONFIG, 0x0A}, /* MIPI pre-dev (default) */
	{HM0360_PLL3_CONFIG, 0x77}, /* PMU/MIPI pre-dev (default) */

	{HM0360_PMU_CFG_3, 0x08},       /* Disable context switching */
	{HM0360_PAD_REGISTER_07, 0x00}, /* PCLKO_polarity falling */

	{HM0360_AE_CTRL, 0x5F}, /* Automatic Exposure (NOTE: Auto framerate enabled) */
	{HM0360_AE_CTRL1, 0x00},
	{HM0360_T_DAMPING, 0x20},       /* AE T damping factor */
	{HM0360_N_DAMPING, 0x00},       /* AE N damping factor */
	{HM0360_AE_TARGET_MEAN, 0x64},  /* AE target */
	{HM0360_AE_MIN_MEAN, 0x0A},     /* AE min target mean */
	{HM0360_AE_TARGET_ZONE, 0x23},  /* AE target zone */
	{HM0360_CONVERGE_IN_TH, 0x03},  /* AE converge in threshold */
	{HM0360_CONVERGE_OUT_TH, 0x05}, /* AE converge out threshold */
	{HM0360_MAX_INTG_H, (HM0360_FRAME_LENGTH_QVGA - 4) >> 8},
	{HM0360_MAX_INTG_L, (HM0360_FRAME_LENGTH_QVGA - 4) & 0xFF},

	{HM0360_MAX_AGAIN, 0x04}, /* Maximum analog gain */
	{HM0360_MAX_DGAIN_H, 0x03},
	{HM0360_MAX_DGAIN_L, 0x3F},
	{HM0360_INTEGRATION_H, 0x01},
	{HM0360_INTEGRATION_L, 0x08},

	{HM0360_MD_CTRL, 0x6A},
	{HM0360_MD_TH_MIN, 0x01},
	{HM0360_MD_BLOCK_NUM_TH, 0x01},
	{HM0360_MD_CTRL1, 0x06},
	{HM0360_PULSE_MODE, 0x00}, /* Interrupt in level mode. */
	{HM0360_ROI_START_END_V, 0xF0},
	{HM0360_ROI_START_END_H, 0xF0},

	{HM0360_FRAME_LEN_LINES_H, HM0360_FRAME_LENGTH_QVGA >> 8},
	{HM0360_FRAME_LEN_LINES_L, HM0360_FRAME_LENGTH_QVGA & 0xFF},
	{HM0360_LINE_LEN_PCK_H, HM0360_LINE_LEN_PCK_QVGA >> 8},
	{HM0360_LINE_LEN_PCK_L, HM0360_LINE_LEN_PCK_QVGA & 0xFF},
	{HM0360_H_SUBSAMPLE, 0x01},
	{HM0360_V_SUBSAMPLE, 0x01},
	{HM0360_BINNING_MODE, 0x00},
	{HM0360_WIN_MODE, 0x00},
	{HM0360_IMG_ORIENTATION, 0x00},
	{HM0360_COMMAND_UPDATE, 0x01},

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

	/* Magic regs */
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

	{HM0360_COMMAND_UPDATE, 0x01},
};

static const struct video_reg hm0360_qvga_regs[] = {
	{HM0360_CCI_PLL1_CONFIG, 0x09}, /* Core=24MHz PCLKO=24MHz I2C= 12MHz */
	{HM0360_CCI_H_SUBSAMPLE, 1},
	{HM0360_CCI_V_SUBSAMPLE, 1},
	{HM0360_CCI_BINNING_MODE, 0x00},
	{HM0360_CCI_WIN_MODE, 0x01},
	{HM0360_CCI_MAX_INTG, HM0360_FRAME_LENGTH_QVGA - 4},
	{HM0360_CCI_FRAME_LEN_LINES, HM0360_FRAME_LENGTH_QVGA},
	{HM0360_CCI_LINE_LEN_PCK, HM0360_LINE_LEN_PCK_QVGA},
	{HM0360_CCI_ROI_START_END_H, 0xF0},
	{HM0360_CCI_ROI_START_END_V, 0xE0},
	{HM0360_CCI_BIT_WIDTH, 0x00}, /* added to get a good pic */
	{HM0360_CCI_PAD_REGISTER_07, 0x04}, /* was 0x0c */
	{HM0360_CCI_COMMAND_UPDATE, 0x01},
};

static const struct video_reg hm0360_vga_regs[] = {
	{HM0360_CCI_PLL1_CONFIG, 0x08}, /* Core=24MHz PCLKO=24MHz I2C= 12MHz */
	{HM0360_CCI_H_SUBSAMPLE, 0x00},
	{HM0360_CCI_V_SUBSAMPLE, 0x00},
	{HM0360_CCI_BINNING_MODE, 0x00},
	{HM0360_CCI_WIN_MODE, 0x01},
	{HM0360_CCI_MAX_INTG, HM0360_FRAME_LENGTH_VGA - 4},
	{HM0360_CCI_FRAME_LEN_LINES, HM0360_FRAME_LENGTH_VGA},
	{HM0360_CCI_LINE_LEN_PCK, HM0360_LINE_LEN_PCK_VGA},
	{HM0360_CCI_ROI_START_END_H, 0xF0},
	{HM0360_CCI_ROI_START_END_V, 0xE0},
	{HM0360_CCI_BIT_WIDTH, 0x00}, /* added to get a good pic */
	{HM0360_CCI_PAD_REGISTER_07, 0x04}, /* was 0x0c */
	{HM0360_CCI_COMMAND_UPDATE, 0x01},
};

static const struct video_reg hm0360_qqvga_regs[] = {
	{HM0360_CCI_PLL1_CONFIG, 0x09}, /* Core=24MHz PCLKO=24MHz I2C= 12MHz */
	{HM0360_CCI_H_SUBSAMPLE, 0x02},
	{HM0360_CCI_V_SUBSAMPLE, 0x02},
	{HM0360_CCI_BINNING_MODE, 0x00},
	{HM0360_CCI_WIN_MODE, 0x01},
	{HM0360_CCI_MAX_INTG, HM0360_FRAME_LENGTH_QQVGA - 4},
	{HM0360_CCI_FRAME_LEN_LINES, HM0360_FRAME_LENGTH_QQVGA},
	{HM0360_CCI_LINE_LEN_PCK, HM0360_LINE_LEN_PCK_QQVGA},
	{HM0360_CCI_ROI_START_END_H, 0xF0},
	{HM0360_CCI_ROI_START_END_V, 0xD0},
	{HM0360_CCI_COMMAND_UPDATE, 0x01},
};

#define HM0360_VIDEO_FORMAT_CAP(width, height, format)                                             \
	{                                                                                          \
		.pixelformat = (format), .width_min = (width), .width_max = (width),               \
		.height_min = (height), .height_max = (height), .width_step = 0, .height_step = 0  \
	}

static const struct video_format_cap fmts[] = {
	HM0360_VIDEO_FORMAT_CAP(160, 120, VIDEO_PIX_FMT_GREY), /* QQVGA  */
	HM0360_VIDEO_FORMAT_CAP(320, 240, VIDEO_PIX_FMT_GREY), /* QVGA  */
	HM0360_VIDEO_FORMAT_CAP(640, 480, VIDEO_PIX_FMT_GREY), /* VGA   */
	{0}};

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
	size_t idx;

	/* Note: hm0360 only supports grayscale */
	LOG_DBG("HM01B0 set_fmt: %d x %d, fmt: %s", fmt->width, fmt->height,
		VIDEO_FOURCC_TO_STR(fmt->pixelformat));

	ret = video_format_caps_index(fmts, fmt, &idx);
	if (ret < 0) {
		return ret;
	}

	if (!memcmp(&data->fmt, fmt, sizeof(data->fmt))) {
		/* nothing to do */
		return 0;
	}

	memcpy(&data->fmt, fmt, sizeof(data->fmt));

	switch (fmts[idx].width_min) {
	case 160: /* QQVGA */
		ret = video_write_cci_multiregs(&config->bus, hm0360_qqvga_regs,
						ARRAY_SIZE(hm0360_qqvga_regs));
		break;
	case 320: /* QVGA */
		ret = video_write_cci_multiregs(&config->bus, hm0360_qvga_regs,
						ARRAY_SIZE(hm0360_qvga_regs));
		break;
	case 640: /* VGA */
		ret = video_write_cci_multiregs(&config->bus, hm0360_vga_regs,
						ARRAY_SIZE(hm0360_vga_regs));
		break;
	default:
		CODE_UNREACHABLE;
	}

	return ret;
}

static int hm0360_get_fmt(const struct device *dev, struct video_format *fmt)
{
	struct hm0360_data *data = dev->data;

	*fmt = data->fmt;

	return 0;
}

int hm0360_enum_frmival(const struct device *dev, struct video_frmival_enum *fie)
{
	switch (fie->index) {
	case HM0360_60_FPS:
		fie->discrete.denominator = 60;
		break;
	case HM0360_30_FPS:
		fie->discrete.denominator = 30;
		break;
	case HM0360_15_FPS:
		fie->discrete.denominator = 15;
		break;
	case HM0360_10_FPS:
		fie->discrete.denominator = 10;
		break;
	default:
		return -EINVAL;
	}

	fie->type = VIDEO_FRMIVAL_TYPE_DISCRETE;
	fie->discrete.numerator = 1;

	return 0;
}


static int hm0360_set_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct hm0360_config *config = dev->config;
	struct hm0360_data *drv_data = dev->data;
	struct video_frmival_enum fie = {.format = &drv_data->fmt, .discrete = *frmival};
	int ret;
	uint32_t osc_div = 0;
	bool highres = false;

	if (drv_data->fmt.width == 640 && drv_data->fmt.height == 480) {
		highres = true;
	}

	video_closest_frmival(dev, &fie);
	LOG_DBG("Applying frame interval number %u", fie.index);

	switch (fie.index) {
	case HM0360_60_FPS:
		osc_div = highres ? 0x00 : 0x01;
		break;
	case HM0360_30_FPS:
		osc_div = highres ? 0x01 : 0x02;
		break;
	case HM0360_15_FPS:
		osc_div = highres ? 0x02 : 0x03;
		break;
	case HM0360_10_FPS:
		osc_div = 0x03;
		break;
	default:
		CODE_UNREACHABLE;
	}

	ret = video_modify_cci_reg(&config->bus, HM0360_CCI_PLL1_CONFIG, 0x03, osc_div);
	if (ret < 0) {
		LOG_ERR("Could not set PLL1 Config");
		return ret;
	}

	LOG_DBG("FrameRate selected: %d", frmival->denominator);

	frmival->denominator = fie.discrete.denominator;
	drv_data->cur_frmrate = fie.discrete.denominator;

	LOG_DBG("FrameRate rounded to: %d", fie.discrete.denominator);
	LOG_DBG("HIRES Selected: %d", highres);
	LOG_DBG("OSC DIV: 0x%x", osc_div);

	return 0;
}

static int hm0360_get_frmival(const struct device *dev, struct video_frmival *frmival)
{
	struct hm0360_data *drv_data = dev->data;

	frmival->denominator = drv_data->cur_frmrate;
	frmival->numerator = 1;

	return 0;
}

static int hm0360_init_controls(const struct device *dev)
{
	int ret;
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
	int ret;

	struct video_format fmt = {
		.pixelformat = fmts[0].pixelformat,
		.width = fmts[0].width_min,
		.height = fmts[0].height_min,
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
		ret = gpio_pin_set_dt(&config->reset, 0);
		if (ret < 0) {
			LOG_ERR("Error setting GPIO: %d\n", ret);
			return ret;
		}
		k_msleep(1);
	}
#endif

	uint32_t pid = 0x0000;

	/*  Read Product ID and check if PID OK  */
	ret = video_read_cci_reg(&config->bus, HM0360_CCI_ID, &pid);
	if (ret < 0) {
		LOG_ERR("Could not read product ID: %d", ret);
		return ret;
	}

	if (pid != HM0360_PROD_ID) {
		LOG_ERR("Incorrect product ID: 0x%02X", pid);
		return -ENODEV;
	}
	LOG_DBG("Product ID is: %x", pid);

	/* Reset camera registers */
	ret = video_write_cci_reg(&config->bus, HM0360_CCI_RESET, 0x01);
	if (ret < 0) {
		LOG_ERR("Could not write reset: %d", ret);
		return ret;
	}
	/* Delay for 1ms. */
	k_msleep(1);

	/* Write Default registers */
	ret = video_write_cci_multiregs16(&config->bus, hm0360_default_regs,
					  ARRAY_SIZE(hm0360_default_regs));
	if (ret < 0) {
		LOG_ERR("Failed to set default registers");
		return ret;
	}

	ret = hm0360_set_fmt(dev, &fmt);
	if (ret < 0) {
		LOG_ERR("ERROR: Unable to set format");
		return ret;
	}

	ret = hm0360_set_frmival(dev, &default_frmival);
	if (ret < 0) {
		LOG_ERR("ERROR: Unable to set frame rate");
		return ret;
	}

	/* Initialize controls */
	return hm0360_init_controls(dev);
}

static int hm0360_set_stream(const struct device *dev, bool enable, enum video_buf_type type)
{
	int ret;
	const struct hm0360_config *config = dev->config;

	ret = video_write_cci_reg(&config->bus, HM0360_CCI_MODE, HM0360_MODE_STREAMING);
	if (ret < 0) {
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

	int ret;

	switch (id) {
	case VIDEO_CID_HFLIP:
		ret = video_modify_cci_reg(&config->bus, HM0360_CCI_ORIENTATION,
					HM0360_ORIENTATION_HMIRROR,
					FIELD_PREP(HM0360_ORIENTATION_HMIRROR, ctrls->hflip.val));
		if (ret < 0) {
			return ret;
		}
		break;
	case VIDEO_CID_VFLIP:
		ret = video_modify_cci_reg(&config->bus, HM0360_CCI_ORIENTATION,
					HM0360_ORIENTATION_VMIRROR,
					FIELD_PREP(HM0360_ORIENTATION_VMIRROR, ctrls->vflip.val));
		if (ret < 0) {
			return ret;
		}
		break;
	default:
		CODE_UNREACHABLE;
	}

	return video_write_cci_reg(&config->bus, HM0360_CCI_COMMAND_UPDATE, 0x01);

}

static DEVICE_API(video, hm0360_api) = {
	.set_format = hm0360_set_fmt,
	.get_format = hm0360_get_fmt,
	.get_caps = hm0360_get_caps,
	.set_stream = hm0360_set_stream,
	.set_ctrl = hm0360_set_ctrl,
	.set_frmival = hm0360_set_frmival,
	.get_frmival = hm0360_get_frmival,
	.enum_frmival = hm0360_enum_frmival,
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
