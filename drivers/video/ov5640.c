/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ovti_ov5640

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/video.h>
#include <zephyr/kernel.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ov5640);

#include <zephyr/sys/byteorder.h>

#define CHIP_ID_REG 0x300a
#define CHIP_ID_VAL 0x5640

#define SYS_CTRL0_REG     0x3008
#define SYS_CTRL0_SW_PWDN 0x42
#define SYS_CTRL0_SW_PWUP 0x02
#define SYS_CTRL0_SW_RST  0x82

#define SYS_RESET00_REG      0x3000
#define SYS_RESET02_REG      0x3002
#define SYS_CLK_ENABLE00_REG 0x3004
#define SYS_CLK_ENABLE02_REG 0x3006
#define IO_MIPI_CTRL00_REG   0x300e
#define SYSTEM_CONTROL1_REG  0x302e
#define SCCB_SYS_CTRL1_REG   0x3103
#define TIMING_TC_REG20_REG  0x3820
#define TIMING_TC_REG21_REG  0x3821
#define HZ5060_CTRL01_REG    0x3c01
#define ISP_CTRL01_REG       0x5001

#define SC_PLL_CTRL0_REG 0x3034
#define SC_PLL_CTRL1_REG 0x3035
#define SC_PLL_CTRL2_REG 0x3036
#define SC_PLL_CTRL3_REG 0x3037
#define SYS_ROOT_DIV_REG 0x3108
#define PCLK_PERIOD_REG  0x4837

#define AEC_CTRL00_REG 0x3a00
#define AEC_CTRL0F_REG 0x3a0f
#define AEC_CTRL10_REG 0x3a10
#define AEC_CTRL11_REG 0x3a11
#define AEC_CTRL1B_REG 0x3a1b
#define AEC_CTRL1E_REG 0x3a1e
#define AEC_CTRL1F_REG 0x3a1f

#define BLC_CTRL01_REG 0x4001
#define BLC_CTRL04_REG 0x4004
#define BLC_CTRL05_REG 0x4005

#define AWB_CTRL00_REG 0x5180
#define AWB_CTRL01_REG 0x5181
#define AWB_CTRL02_REG 0x5182
#define AWB_CTRL03_REG 0x5183
#define AWB_CTRL04_REG 0x5184
#define AWB_CTRL05_REG 0x5185
#define AWB_CTRL17_REG 0x5191
#define AWB_CTRL18_REG 0x5192
#define AWB_CTRL19_REG 0x5193
#define AWB_CTRL20_REG 0x5194
#define AWB_CTRL21_REG 0x5195
#define AWB_CTRL22_REG 0x5196
#define AWB_CTRL23_REG 0x5197
#define AWB_CTRL30_REG 0x519e

#define SDE_CTRL0_REG  0x5580
#define SDE_CTRL3_REG  0x5583
#define SDE_CTRL4_REG  0x5584
#define SDE_CTRL9_REG  0x5589
#define SDE_CTRL10_REG 0x558a
#define SDE_CTRL11_REG 0x558b

#define DEFAULT_MIPI_CHANNEL 0

#define OV5640_RESOLUTION_PARAM_NUM 24

/* Enum for bus types */
#define OV5640_BUS_TYPE_PARALLEL		1
#define OV5640_BUS_TYPE_MIPI_CSI_2_D_PHY	2
#define OV5640_BUS_TYPE_MIPI_CSI_2_C_PHY	3
#define OV5640_BUS_TYPE_MIPI_CSI_1		4

struct ov5640_config {
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec reset_gpio;
	struct gpio_dt_spec powerdown_gpio;
	uint8_t bus_type;
	uint8_t bus_width;
};

struct ov5640_data {
	struct video_format fmt;
};

struct ov5640_reg {
	uint16_t addr;
	uint8_t val;
};

struct ov5640_mipi_clock_config {
	uint8_t pllCtrl1;
	uint8_t pllCtrl2;
};

struct ov5640_resolution_config {
	uint16_t width;
	uint16_t height;
	uint16_t array_size_res_params;
	const struct ov5640_reg *res_params;
	const struct ov5640_mipi_clock_config mipi_pclk;
};

static const struct ov5640_reg ov5640_init_params_common[] = {
	/* Power down */
	{SYS_CTRL0_REG, SYS_CTRL0_SW_PWDN},

	/* System setting. */
	{SCCB_SYS_CTRL1_REG, 0x13},
	{SCCB_SYS_CTRL1_REG, 0x03},
	{SYS_RESET00_REG, 0x00},
	{SYS_CLK_ENABLE00_REG, 0xff},
	{SYS_RESET02_REG, 0x1c},
	{SYS_CLK_ENABLE02_REG, 0xc3},
	{SYSTEM_CONTROL1_REG, 0x08},
	{0x3618, 0x00},
	{0x3612, 0x29},
	{0x3708, 0x64},
	{0x3709, 0x52},
	{0x370c, 0x03},
	{TIMING_TC_REG20_REG, 0x41},
	{TIMING_TC_REG21_REG, 0x07},
	{0x3630, 0x36},
	{0x3631, 0x0e},
	{0x3632, 0xe2},
	{0x3633, 0x12},
	{0x3621, 0xe0},
	{0x3704, 0xa0},
	{0x3703, 0x5a},
	{0x3715, 0x78},
	{0x3717, 0x01},
	{0x370b, 0x60},
	{0x3705, 0x1a},
	{0x3905, 0x02},
	{0x3906, 0x10},
	{0x3901, 0x0a},
	{0x3731, 0x12},
	{0x3600, 0x08},
	{0x3601, 0x33},
	{0x302d, 0x60},
	{0x3620, 0x52},
	{0x371b, 0x20},
	{0x471c, 0x50},
	{0x3a13, 0x43},
	{0x3a18, 0x00},
	{0x3a19, 0x7c},
	{0x3635, 0x13},
	{0x3636, 0x03},
	{0x3634, 0x40},
	{0x3622, 0x01},
	{HZ5060_CTRL01_REG, 0x00},
	{AEC_CTRL00_REG, 0x58},
	{BLC_CTRL01_REG, 0x02},
	{BLC_CTRL04_REG, 0x02},
	{BLC_CTRL05_REG, 0x1a},
	{ISP_CTRL01_REG, 0xa3},

	/* AEC */
	{AEC_CTRL0F_REG, 0x30},
	{AEC_CTRL10_REG, 0x28},
	{AEC_CTRL1B_REG, 0x30},
	{AEC_CTRL1E_REG, 0x26},
	{AEC_CTRL11_REG, 0x60},
	{AEC_CTRL1F_REG, 0x14},

	/* AWB */
	{AWB_CTRL00_REG, 0xff},
	{AWB_CTRL01_REG, 0xf2},
	{AWB_CTRL02_REG, 0x00},
	{AWB_CTRL03_REG, 0x14},
	{AWB_CTRL04_REG, 0x25},
	{AWB_CTRL05_REG, 0x24},
	{0x5186, 0x09},
	{0x5187, 0x09},
	{0x5188, 0x09},
	{0x5189, 0x88},
	{0x518a, 0x54},
	{0x518b, 0xee},
	{0x518c, 0xb2},
	{0x518d, 0x50},
	{0x518e, 0x34},
	{0x518f, 0x6b},
	{0x5190, 0x46},
	{AWB_CTRL17_REG, 0xf8},
	{AWB_CTRL18_REG, 0x04},
	{AWB_CTRL19_REG, 0x70},
	{AWB_CTRL20_REG, 0xf0},
	{AWB_CTRL21_REG, 0xf0},
	{AWB_CTRL22_REG, 0x03},
	{AWB_CTRL23_REG, 0x01},
	{0x5198, 0x04},
	{0x5199, 0x6c},
	{0x519a, 0x04},
	{0x519b, 0x00},
	{0x519c, 0x09},
	{0x519d, 0x2b},
	{AWB_CTRL30_REG, 0x38},

	/* Color Matrix */
	{0x5381, 0x1e},
	{0x5382, 0x5b},
	{0x5383, 0x08},
	{0x5384, 0x0a},
	{0x5385, 0x7e},
	{0x5386, 0x88},
	{0x5387, 0x7c},
	{0x5388, 0x6c},
	{0x5389, 0x10},
	{0x538a, 0x01},
	{0x538b, 0x98},

	/* Sharp */
	{0x5300, 0x08},
	{0x5301, 0x30},
	{0x5302, 0x10},
	{0x5303, 0x00},
	{0x5304, 0x08},
	{0x5305, 0x30},
	{0x5306, 0x08},
	{0x5307, 0x16},
	{0x5309, 0x08},
	{0x530a, 0x30},
	{0x530b, 0x04},
	{0x530c, 0x06},

	/* Gamma */
	{0x5480, 0x01},
	{0x5481, 0x08},
	{0x5482, 0x14},
	{0x5483, 0x28},
	{0x5484, 0x51},
	{0x5485, 0x65},
	{0x5486, 0x71},
	{0x5487, 0x7d},
	{0x5488, 0x87},
	{0x5489, 0x91},
	{0x548a, 0x9a},
	{0x548b, 0xaa},
	{0x548c, 0xb8},
	{0x548d, 0xcd},
	{0x548e, 0xdd},
	{0x548f, 0xea},
	{0x5490, 0x1d},

	/* UV adjust. */
	{SDE_CTRL0_REG, 0x02},
	{SDE_CTRL3_REG, 0x40},
	{SDE_CTRL4_REG, 0x10},
	{SDE_CTRL9_REG, 0x10},
	{SDE_CTRL10_REG, 0x00},
	{SDE_CTRL11_REG, 0xf8},

	/* Lens correction. */
	{0x5800, 0x23},
	{0x5801, 0x14},
	{0x5802, 0x0f},
	{0x5803, 0x0f},
	{0x5804, 0x12},
	{0x5805, 0x26},
	{0x5806, 0x0c},
	{0x5807, 0x08},
	{0x5808, 0x05},
	{0x5809, 0x05},
	{0x580a, 0x08},
	{0x580b, 0x0d},
	{0x580c, 0x08},
	{0x580d, 0x03},
	{0x580e, 0x00},
	{0x580f, 0x00},
	{0x5810, 0x03},
	{0x5811, 0x09},
	{0x5812, 0x07},
	{0x5813, 0x03},
	{0x5814, 0x00},
	{0x5815, 0x01},
	{0x5816, 0x03},
	{0x5817, 0x08},
	{0x5818, 0x0d},
	{0x5819, 0x08},
	{0x581a, 0x05},
	{0x581b, 0x06},
	{0x581c, 0x08},
	{0x581d, 0x0e},
	{0x581e, 0x29},
	{0x581f, 0x17},
	{0x5820, 0x11},
	{0x5821, 0x11},
	{0x5822, 0x15},
	{0x5823, 0x28},
	{0x5824, 0x46},
	{0x5825, 0x26},
	{0x5826, 0x08},
	{0x5827, 0x26},
	{0x5828, 0x64},
	{0x5829, 0x26},
	{0x582a, 0x24},
	{0x582b, 0x22},
	{0x582c, 0x24},
	{0x582d, 0x24},
	{0x582e, 0x06},
	{0x582f, 0x22},
	{0x5830, 0x40},
	{0x5831, 0x42},
	{0x5832, 0x24},
	{0x5833, 0x26},
	{0x5834, 0x24},
	{0x5835, 0x22},
	{0x5836, 0x22},
	{0x5837, 0x26},
	{0x5838, 0x44},
	{0x5839, 0x24},
	{0x583a, 0x26},
	{0x583b, 0x28},
	{0x583c, 0x42},
	{0x583d, 0xce},
	{0x5000, 0xa7},
};

/* FIXME: Check with values are really necessary. */
static const struct ov5640_reg ov5640_init_params_dvp[] = {
	{0x4740, 0x21},
	{0x4050, 0x6e},
	{0x4051, 0x8f},
	{0x3017, 0xff},
	{0x3018, 0xff},
	{0x302c, 0x02},
	{0x3108, 0x01},
	{0x3630, 0x2e},
	{0x3a18, 0x00},
	{0x3a19, 0xf8},
	{0x3635, 0x1c},
	{0x3c04, 0x28},
	{0x3c05, 0x98},
	{0x3c06, 0x00},
	{0x3c07, 0x08},
	{0x3c08, 0x00},
	{0x3c09, 0x1c},
	{0x3c0a, 0x9c},
	{0x3c0b, 0x40},
	{TIMING_TC_REG20_REG, 0x47},
	{TIMING_TC_REG21_REG, 0x01},
	{0x3800, 0x00},
	{0x3801, 0x00},
	{0x3802, 0x00},
	{0x3803, 0x04},
	{0x3804, 0x0a},
	{0x3805, 0x3f},
	{0x3806, 0x07},
	{0x3807, 0x9b},
	{0x3808, 0x05},
	{0x3809, 0x00},
	{0x380a, 0x03},
	{0x380b, 0xc0},
	{0x3810, 0x00},
	{0x3811, 0x10},
	{0x3812, 0x00},
	{0x3813, 0x06},
	{0x3814, 0x31},
	{0x3815, 0x31},
	{0x3034, 0x1a},
	{0x3035, 0x11},
	{0x3036, 0x64},
	{0x3037, 0x13},
	{0x3038, 0x00},
	{0x3039, 0x00},
	{0x380c, 0x07},
	{0x380d, 0x68},
	{0x380e, 0x03},
	{0x380f, 0xd8},
	{0x3c01, 0xb4},
	{0x3c00, 0x04},
	{0x3a08, 0x00},
	{0x3a09, 0x93},
	{0x3a0e, 0x06},
	{0x3a0a, 0x00},
	{0x3a0b, 0x7b},
	{0x3a0d, 0x08},
	{0x3a00, 0x38},
	{0x3a02, 0x05},
	{0x3a03, 0xc4},
	{0x3a14, 0x05},
	{0x3a15, 0xc4},
	{0x300e, 0x58},
	{0x302e, 0x00},
	{0x4300, 0x30},
	{0x501f, 0x00},
	{0x4713, 0x04},
	{0x4407, 0x04},
	{0x460b, 0x35},
	{0x460c, 0x22},
	{0x3824, 0x02},
	{0x3406, 0x01},
	{0x3400, 0x06},
	{0x3401, 0x80},
	{0x3402, 0x04},
	{0x3403, 0x00},
	{0x3404, 0x06},
	{0x3405, 0x00},
	{0x5688, 0x22},
	{0x5689, 0x22},
	{0x568a, 0x42},
	{0x568b, 0x24},
	{0x568c, 0x42},
	{0x568d, 0x24},
	{0x568e, 0x22},
	{0x568f, 0x22},
	{0x5025, 0x00},
	{0x3406, 0x00},
	{0x3503, 0x00},
	{0x3008, 0x02},
	{0x3a02, 0x07},
	{0x3a03, 0xae},
	{0x3a08, 0x01},
	{0x3a09, 0x27},
	{0x3a0a, 0x00},
	{0x3a0b, 0xf6},
	{0x3a0e, 0x06},
	{0x3a0d, 0x08},
	{0x3a14, 0x07},
	{0x3a15, 0xae},

	/* JPEG Control */
	{0x4401, 0x0d}, /* | Read SRAM enable when blanking | Read SRAM at first blanking */
	{0x4723, 0x03}, /* DVP JPEG Mode456 Skip Line Number */
};

static const struct ov5640_reg ov5640_low_res_params[] = {
	{0x3800, 0x00}, {0x3801, 0x00}, {0x3802, 0x00}, {0x3803, 0x04}, {0x3804, 0x0a},
	{0x3805, 0x3f}, {0x3806, 0x07}, {0x3807, 0x9b}, {0x3808, 0x02}, {0x3809, 0x80},
	{0x380a, 0x01}, {0x380b, 0xe0}, {0x380c, 0x07}, {0x380d, 0x68}, {0x380e, 0x03},
	{0x380f, 0xd8}, {0x3810, 0x00}, {0x3811, 0x10}, {0x3812, 0x00}, {0x3813, 0x06},
	{0x3814, 0x31}, {0x3815, 0x31}, {0x3824, 0x02}, {0x460c, 0x22}};

static const struct ov5640_reg ov5640_720p_res_params[] = {
	{0x3800, 0x00}, {0x3801, 0x00}, {0x3802, 0x00}, {0x3803, 0xfa}, {0x3804, 0x0a},
	{0x3805, 0x3f}, {0x3806, 0x06}, {0x3807, 0xa9}, {0x3808, 0x05}, {0x3809, 0x00},
	{0x380a, 0x02}, {0x380b, 0xd0}, {0x380c, 0x07}, {0x380d, 0x64}, {0x380e, 0x02},
	{0x380f, 0xe4}, {0x3810, 0x00}, {0x3811, 0x10}, {0x3812, 0x00}, {0x3813, 0x04},
	{0x3814, 0x31}, {0x3815, 0x31}, {0x3824, 0x04}, {0x460c, 0x20}};

static const struct ov5640_resolution_config csi2_resolution_params[] = {
	{.width = 640,
	 .height = 480,
	 .array_size_res_params = ARRAY_SIZE(ov5640_low_res_params),
	 .res_params = ov5640_low_res_params,
	 .mipi_pclk = {
			 .pllCtrl1 = 0x14,
			 .pllCtrl2 = 0x38,
		 }},
	{.width = 1280,
	 .height = 720,
	 .array_size_res_params = ARRAY_SIZE(ov5640_720p_res_params),
	 .res_params = ov5640_720p_res_params,
	 .mipi_pclk = {
			 .pllCtrl1 = 0x21,
			 .pllCtrl2 = 0x54,
		 }},
};

/* Initialization sequence for QQVGA resolution (160x120) */
static const struct ov5640_reg ov5640_160x120_dvp_res_params[] = {
	{0x3800, 0x00}, {0x3801, 0x08}, {0x3802, 0x00}, {0x3803, 0x02}, {0x3804, 0x0a},
	{0x3805, 0x37}, {0x3806, 0x07}, {0x3807, 0xa1}, {0x3808, 0x00}, {0x3809, 0xa0},
	{0x380a, 0x00}, {0x380b, 0x78}, {0x380c, 0x06}, {0x380d, 0x14}, {0x380e, 0x03},
	{0x380f, 0xe8}, {0x3810, 0x00}, {0x3811, 0x04}, {0x3812, 0x00}, {0x3813, 0x02},
	{0x3814, 0x31}, {0x3815, 0x31}, {0x3820, 0x47}, {0x3821, 0x01}, {0x4602, 0x00},
	{0x4603, 0xa0}, {0x4604, 0x00}, {0x4605, 0x78}
};

/* Initialization sequence for QVGA resolution (320x240) */
static const struct ov5640_reg ov5640_320x240_dvp_res_params[] = {
	{0x3800, 0x00}, {0x3801, 0x08}, {0x3802, 0x00}, {0x3803, 0x02}, {0x3804, 0x0a},
	{0x3805, 0x37}, {0x3806, 0x07}, {0x3807, 0xa1}, {0x3808, 0x01}, {0x3809, 0x40},
	{0x380a, 0x00}, {0x380b, 0xf0}, {0x380c, 0x06}, {0x380d, 0x14}, {0x380e, 0x03},
	{0x380f, 0xe8}, {0x3810, 0x00}, {0x3811, 0x04}, {0x3812, 0x00}, {0x3813, 0x02},
	{0x3814, 0x31}, {0x3815, 0x31}, {0x3820, 0x47}, {0x3821, 0x01}, {0x4602, 0x01},
	{0x4603, 0x40}, {0x4604, 0x00}, {0x4605, 0xf0}};

/* Initialization sequence for 480x272 resolution */
static const struct ov5640_reg ov5640_480x272_dvp_res_params[] = {
	{0x3800, 0x00}, {0x3801, 0x08}, {0x3802, 0x00}, {0x3803, 0x02},	{0x3804, 0x0a},
	{0x3805, 0x37}, {0x3806, 0x07}, {0x3807, 0xa1},	{0x3808, 0x01}, {0x3809, 0xe0},
	{0x380a, 0x01}, {0x380b, 0x10},	{0x380c, 0x06}, {0x380d, 0x14},	{0x380e, 0x03},
	{0x380f, 0xe8},	{0x3810, 0x00}, {0x3811, 0x04},	{0x3812, 0x00}, {0x3813, 0x79},
	{0x3814, 0x31}, {0x3815, 0x31},	{0x3820, 0x47},	{0x3821, 0x01},	{0x4602, 0x01},
	{0x4603, 0xe0},	{0x4604, 0x01}, {0x4605, 0x10}};

static const struct ov5640_resolution_config dvp_resolution_params[] = {
	{.width = 160,
	.height = 120,
	.array_size_res_params = ARRAY_SIZE(ov5640_160x120_dvp_res_params),
	.res_params = ov5640_160x120_dvp_res_params
	},
	{.width = 320,
	 .height = 240,
	 .array_size_res_params = ARRAY_SIZE(ov5640_320x240_dvp_res_params),
	 .res_params = ov5640_320x240_dvp_res_params
	},
	{.width = 480,
	 .height = 272,
	 .array_size_res_params = ARRAY_SIZE(ov5640_480x272_dvp_res_params),
	 .res_params = ov5640_480x272_dvp_res_params
	},
};

#define OV5640_VIDEO_FORMAT_CAP(width, height, format)                                             \
	{                                                                                          \
		.pixelformat = (format), .width_min = (width), .width_max = (width),               \
		.height_min = (height), .height_max = (height), .width_step = 0, .height_step = 0  \
	}

static const struct video_format_cap csi2_fmts[] = {
	OV5640_VIDEO_FORMAT_CAP(1280, 720, VIDEO_PIX_FMT_RGB565),
	OV5640_VIDEO_FORMAT_CAP(1280, 720, VIDEO_PIX_FMT_YUYV),
	OV5640_VIDEO_FORMAT_CAP(640, 480, VIDEO_PIX_FMT_RGB565),
	OV5640_VIDEO_FORMAT_CAP(640, 480, VIDEO_PIX_FMT_YUYV),
	{0}};

static const struct video_format_cap dvp_fmts[] = {
	OV5640_VIDEO_FORMAT_CAP(160, 120, VIDEO_PIX_FMT_RGB565),
	OV5640_VIDEO_FORMAT_CAP(320, 240, VIDEO_PIX_FMT_RGB565),
	OV5640_VIDEO_FORMAT_CAP(480, 272, VIDEO_PIX_FMT_RGB565),
	{0}};

static inline bool ov5640_is_dvp(const struct device *dev)
{
	const struct ov5640_config *cfg = dev->config;

	return cfg->bus_type == OV5640_BUS_TYPE_PARALLEL;
}

static int ov5640_read_reg(const struct i2c_dt_spec *spec, const uint16_t addr, void *val,
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

static int ov5640_write_reg(const struct i2c_dt_spec *spec, const uint16_t addr, const uint8_t val)
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

static int ov5640_modify_reg(const struct i2c_dt_spec *spec, const uint16_t addr,
			     const uint8_t mask, const uint8_t val)
{
	uint8_t regVal = 0;
	int ret = ov5640_read_reg(spec, addr, &regVal, sizeof(regVal));

	if (ret) {
		return ret;
	}

	return ov5640_write_reg(spec, addr, (regVal & ~mask) | (val & mask));
}

static int ov5640_write_multi_regs(const struct i2c_dt_spec *spec, const struct ov5640_reg *regs,
				   const uint32_t num_regs)
{
	int ret;

	for (int i = 0; i < num_regs; i++) {
		ret = ov5640_write_reg(spec, regs[i].addr, regs[i].val);
		if (ret) {
			return ret;
		}
	}

	return 0;
}

static int ov5640_set_fmt_dvp(const struct ov5640_config *cfg)
{
	uint8_t reg;
	int ret = 0;

	ret |= ov5640_read_reg(&cfg->i2c, TIMING_TC_REG21_REG, &reg, sizeof(reg));
	ret |= ov5640_write_reg(&cfg->i2c, TIMING_TC_REG21_REG, (reg & 0xDF) | 0x00);

	if (ret) {
		LOG_ERR("Unable to configure REG: %d on DVP", TIMING_TC_REG21_REG);
		return ret;
	}

	ret |= ov5640_read_reg(&cfg->i2c, SYS_RESET02_REG, &reg, sizeof(reg));
	ret |= ov5640_write_reg(&cfg->i2c, SYS_RESET02_REG, (reg & 0xE3) | 0x1C);

	if (ret) {
		LOG_ERR("Unable to configure REG: %d on DVP", SYS_RESET02_REG);
		return ret;
	}

	ret |= ov5640_read_reg(&cfg->i2c, SYS_CLK_ENABLE02_REG, &reg, sizeof(reg));
	ret |= ov5640_write_reg(&cfg->i2c, SYS_CLK_ENABLE02_REG, (reg & 0xD7) | 0x00);

	if (ret) {
		LOG_ERR("Unable to configure REG: %d on DVP", SYS_CLK_ENABLE02_REG);
		return ret;
	}

	return 0;
}

static int ov5640_set_fmt_csi2(const struct ov5640_config *cfg,
				const struct ov5640_resolution_config *resolution_params)
{
	int ret = 0;

	/* Configure MIPI pixel clock */
	ret |= ov5640_modify_reg(&cfg->i2c, SC_PLL_CTRL0_REG, 0x0f, 0x08);
	ret |= ov5640_modify_reg(&cfg->i2c, SC_PLL_CTRL1_REG, 0xff,
				 resolution_params->mipi_pclk.pllCtrl1);
	ret |= ov5640_modify_reg(&cfg->i2c, SC_PLL_CTRL2_REG, 0xff,
				 resolution_params->mipi_pclk.pllCtrl2);
	ret |= ov5640_modify_reg(&cfg->i2c, SC_PLL_CTRL3_REG, 0x1f, 0x13);
	ret |= ov5640_modify_reg(&cfg->i2c, SYS_ROOT_DIV_REG, 0x3f, 0x01);
	ret |= ov5640_write_reg(&cfg->i2c, PCLK_PERIOD_REG, 0x0a);

	if (ret) {
		LOG_ERR("Unable to configure MIPI pixel clock");
		return ret;
	}

	return 0;
}

static int ov5640_set_fmt(const struct device *dev, enum video_endpoint_id ep,
			  struct video_format *fmt)
{
	struct ov5640_data *drv_data = dev->data;
	const struct ov5640_config *cfg = dev->config;
	const struct video_format_cap *fmts = ov5640_is_dvp(dev) ? dvp_fmts : csi2_fmts;
	const size_t num_fmts = ov5640_is_dvp(dev) ? ARRAY_SIZE(dvp_fmts) : ARRAY_SIZE(csi2_fmts);
	const struct ov5640_resolution_config *resolution_params = ov5640_is_dvp(dev) ?
						dvp_resolution_params : csi2_resolution_params;
	const size_t num_res_params = ov5640_is_dvp(dev) ?
			ARRAY_SIZE(dvp_resolution_params) : ARRAY_SIZE(csi2_resolution_params);
	int ret;
	int i;

	for (i = 0; i < num_fmts; ++i) {
		if (fmt->pixelformat == fmts[i].pixelformat && fmt->width >= fmts[i].width_min &&
		    fmt->width <= fmts[i].width_max && fmt->height >= fmts[i].height_min &&
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

	/* Set resolution parameters */
	for (i = 0; i < num_res_params; i++) {
		if (fmt->width == resolution_params[i].width &&
		    fmt->height == resolution_params[i].height) {
			ret = ov5640_write_multi_regs(&cfg->i2c, resolution_params[i].res_params,
						      resolution_params[i].array_size_res_params);
			if (ret) {
				LOG_ERR("Unable to set resolution parameters");
				return ret;
			}
			break;
		}
	}

	/* Set pixel format, default to VIDEO_PIX_FMT_RGB565 */
	struct ov5640_reg fmt_params[2] = {
		{0x4300, 0x6f},
		{0x501f, 0x01},
	};

	if (fmt->pixelformat == VIDEO_PIX_FMT_YUYV) {
		fmt_params[0].val = 0x3f;
		fmt_params[1].val = 0x00;
	}

	ret = ov5640_write_multi_regs(&cfg->i2c, fmt_params, ARRAY_SIZE(fmt_params));
	if (ret) {
		LOG_ERR("Unable to set pixel format");
		return ret;
	}

	if (ov5640_is_dvp(dev)) {
		return ov5640_set_fmt_dvp(cfg);
	}

	return ov5640_set_fmt_csi2(cfg, &resolution_params[i]);
}

static int ov5640_get_fmt(const struct device *dev, enum video_endpoint_id ep,
			  struct video_format *fmt)
{
	struct ov5640_data *drv_data = dev->data;

	*fmt = drv_data->fmt;

	return 0;
}

static int ov5640_get_caps(const struct device *dev, enum video_endpoint_id ep,
			   struct video_caps *caps)
{
	caps->format_caps = ov5640_is_dvp(dev) ? dvp_fmts : csi2_fmts;
	return 0;
}

static int ov5640_stream_start(const struct device *dev)
{
	const struct ov5640_config *cfg = dev->config;

	if (ov5640_is_dvp(dev)) {
		return 0;
	}

	/* Power up MIPI PHY HS Tx & LP Rx in 2 data lanes mode */
	int ret = ov5640_write_reg(&cfg->i2c, IO_MIPI_CTRL00_REG, 0x45);

	if (ret) {
		LOG_ERR("Unable to power up MIPI PHY");
		return ret;
	}
	return ov5640_write_reg(&cfg->i2c, SYS_CTRL0_REG, SYS_CTRL0_SW_PWUP);
}

static int ov5640_stream_stop(const struct device *dev)
{
	const struct ov5640_config *cfg = dev->config;

	if (ov5640_is_dvp(dev)) {
		return 0;
	}

	/* Power down MIPI PHY HS Tx & LP Rx */
	int ret = ov5640_write_reg(&cfg->i2c, IO_MIPI_CTRL00_REG, 0x40);

	if (ret) {
		LOG_ERR("Unable to power down MIPI PHY");
		return ret;
	}
	return ov5640_write_reg(&cfg->i2c, SYS_CTRL0_REG, SYS_CTRL0_SW_PWDN);
}

static const struct video_driver_api ov5640_driver_api = {
	.set_format = ov5640_set_fmt,
	.get_format = ov5640_get_fmt,
	.get_caps = ov5640_get_caps,
	.stream_start = ov5640_stream_start,
	.stream_stop = ov5640_stream_stop,
};

static int ov5640_init(const struct device *dev)
{
	const struct ov5640_config *cfg = dev->config;
	struct video_format fmt;
	uint16_t chip_id;
	int ret;

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&cfg->reset_gpio)) {
		LOG_ERR("%s: device %s is not ready", dev->name, cfg->reset_gpio.port->name);
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&cfg->powerdown_gpio)) {
		LOG_ERR("%s: device %s is not ready", dev->name, cfg->powerdown_gpio.port->name);
		return -ENODEV;
	}

	/* Power up sequence */
	if (cfg->powerdown_gpio.port != NULL) {
		ret = gpio_pin_configure_dt(&cfg->powerdown_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret) {
			return ret;
		}
	}

	if (cfg->reset_gpio.port != NULL) {
		ret = gpio_pin_configure_dt(&cfg->reset_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret) {
			return ret;
		}
	}

	k_sleep(K_MSEC(5));

	if (cfg->powerdown_gpio.port != NULL) {
		gpio_pin_set_dt(&cfg->powerdown_gpio, 0);
	}

	k_sleep(K_MSEC(1));

	if (cfg->reset_gpio.port != NULL) {
		gpio_pin_set_dt(&cfg->reset_gpio, 0);
	}

	k_sleep(K_MSEC(20));

	/* Reset all registers */
	ret = ov5640_write_reg(&cfg->i2c, SCCB_SYS_CTRL1_REG, 0x11);
	if (ret) {
		LOG_ERR("Unable to write to reset all registers");
		return -EIO;
	}

	/* Software reset */
	ret = ov5640_write_reg(&cfg->i2c, SYS_CTRL0_REG, SYS_CTRL0_SW_RST);
	if (ret) {
		LOG_ERR("Unable to perform software reset");
		return -EIO;
	}

	k_sleep(K_MSEC(5));

	/* Initialize register values */
	ret = ov5640_write_multi_regs(&cfg->i2c, ov5640_init_params_common,
				ARRAY_SIZE(ov5640_init_params_common));
	if (ret) {
		LOG_ERR("Unable to initialize the sensor with common parameters");
		return -EIO;
	}

	if (ov5640_is_dvp(dev)) {
		ret = ov5640_write_multi_regs(&cfg->i2c, ov5640_init_params_dvp,
				ARRAY_SIZE(ov5640_init_params_dvp));
	}

	if (ret) {
		LOG_ERR("Unable to initialize the sensor with DVP parameters");
		return -EIO;
	}

	/* Set virtual channel */
	if (!ov5640_is_dvp(dev)) {
		ret = ov5640_modify_reg(&cfg->i2c, 0x4814, 3U << 6,
				(uint8_t)(DEFAULT_MIPI_CHANNEL) << 6);
		if (ret) {
			LOG_ERR("Unable to set virtual channel");
			return -EIO;
		}
	}

	/* Check sensor chip id */
	ret = ov5640_read_reg(&cfg->i2c, CHIP_ID_REG, &chip_id, sizeof(chip_id));
	if (ret) {
		LOG_ERR("Unable to read sensor chip ID, ret = %d", ret);
		return -ENODEV;
	}

	if (chip_id != CHIP_ID_VAL) {
		LOG_ERR("Wrong chip ID: %04x (expected %04x)", chip_id, CHIP_ID_VAL);
		return -ENODEV;
	}

	if (ov5640_is_dvp(dev)) {
		/* Set default format to 480x272 RGB565 */
		fmt.pixelformat = VIDEO_PIX_FMT_RGB565;
		fmt.width = 480;
		fmt.height = 272;
	} else {
		/* Set default format to 720p RGB565 */
		fmt.pixelformat = VIDEO_PIX_FMT_RGB565;
		fmt.width = 1280;
		fmt.height = 720;
	}

	fmt.pitch = fmt.width * 2;
	ret = ov5640_set_fmt(dev, VIDEO_EP_OUT, &fmt);
	if (ret) {
		LOG_ERR("Unable to configure default format");
		return -EIO;
	}

	return 0;
}

#define OV5640_INIT(n)                                                                             \
	static struct ov5640_data ov5640_data_##n;                                                 \
                                                                                                   \
	static const struct ov5640_config ov5640_cfg_##n = {                                       \
		.i2c = I2C_DT_SPEC_INST_GET(n),                                                    \
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(n, reset_gpios, {0}),                       \
		.powerdown_gpio = GPIO_DT_SPEC_INST_GET_OR(n, powerdown_gpios, {0}),               \
		.bus_type = DT_PROP(DT_DRV_INST(n), bus_type),                                     \
		.bus_width = DT_PROP(DT_DRV_INST(n), bus_width),                                   \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &ov5640_init, NULL, &ov5640_data_##n, &ov5640_cfg_##n,            \
			      POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY, &ov5640_driver_api);

DT_INST_FOREACH_STATUS_OKAY(OV5640_INIT)
