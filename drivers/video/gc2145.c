/*
 * Copyright (c) 2024 Felipe Neves
 * Copyright (c) 2025 STMicroelectronics.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT galaxycore_gc2145
#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video-controls.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/dt-bindings/video/video-interfaces.h>
#include <zephyr/logging/log.h>

#include "video_common.h"
#include "video_ctrls.h"
#include "video_device.h"

LOG_MODULE_REGISTER(video_gc2145, CONFIG_VIDEO_LOG_LEVEL);

#define GC2145_REG8(addr)	((addr) | VIDEO_REG_ADDR8_DATA8)
#define GC2145_REG16_LE(addr)	((addr) | VIDEO_REG_ADDR8_DATA16_LE)
#define GC2145_REG16_BE(addr)	((addr) | VIDEO_REG_ADDR8_DATA16_BE)

#define GC2145_REG_AMODE1               GC2145_REG8(0x17)
#define GC2145_REG_AMODE1_DEF           0x14
#define GC2145_REG_AMODE1_UPDOWN	BIT(1)
#define GC2145_REG_AMODE1_MIRROR	BIT(0)
#define GC2145_REG_OUTPUT_FMT           0x84
#define GC2145_REG_OUTPUT_FMT_MASK      0x1F
#define GC2145_REG_OUTPUT_FMT_RGB565    0x06
#define GC2145_REG_OUTPUT_FMT_YCBYCR    0x02
#define GC2145_REG_SYNC_MODE            0x86
#define GC2145_REG_SYNC_MODE_DEF        0x03
#define GC2145_REG_SYNC_MODE_COL_SWITCH 0x10
#define GC2145_REG_SYNC_MODE_ROW_SWITCH 0x20
#define GC2145_REG_BYPASS_MODE          0x89
#define GC2145_REG_BYPASS_MODE_SWITCH   BIT(5)
#define GC2145_REG_CHIP_ID		GC2145_REG16_BE(0xF0)
#define GC2145_REG_RESET                0xFE
#define GC2145_REG_SW_RESET             0x80
#define GC2145_REG_RESET_P0_REGS        0x00
#define GC2145_REG_RESET_P1_REGS        0x01
#define GC2145_REG_RESET_P2_REGS        0x02
#define GC2145_REG_RESET_P3_REGS        0x03
#define GC2145_REG_CROP_ENABLE          GC2145_REG8(0x90)
#define GC2145_CROP_SET_ENABLE          0x01
#define GC2145_REG_WIN_ROW_START	GC2145_REG16_BE(0x09)
#define GC2145_REG_WIN_COL_START	GC2145_REG16_BE(0x0b)
#define GC2145_REG_WIN_HEIGHT		GC2145_REG16_BE(0x0d)
#define GC2145_REG_WIN_WIDTH		GC2145_REG16_BE(0x0f)
#define GC2145_REG_OUT_WIN_ROW_START	GC2145_REG16_BE(0x91)
#define GC2145_REG_OUT_WIN_COL_START	GC2145_REG16_BE(0x93)
#define GC2145_REG_OUT_WIN_HEIGHT	GC2145_REG16_BE(0x95)
#define GC2145_REG_OUT_WIN_WIDTH	GC2145_REG16_BE(0x97)
#define GC2145_REG_SUBSAMPLE		GC2145_REG8(0x99)
#define GC2145_REG_SUBSAMPLE_MODE       GC2145_REG8(0x9A)
#define GC2145_SUBSAMPLE_MODE_SMOOTH    0x0E

/* MIPI-CSI registers - on page 3 */
#define GC2145_REG_DPHY_MODE1		0x01
#define GC2145_DPHY_MODE1_CLK_EN	BIT(0)
#define GC2145_DPHY_MODE1_LANE0_EN	BIT(1)
#define GC2145_DPHY_MODE1_LANE1_EN	BIT(2)
#define GC2145_DPHY_MODE1_CLK_LANE_P2S_SEL	BIT(7)

#define GC2145_REG_DPHY_MODE2		0x02
#define GC2145_DPHY_MODE2_CLK_DIFF(a)		((a) & 0x07)
#define GC2145_DPHY_MODE2_LANE0_DIFF(a)	(((a) & 0x07) << 4)

#define GC2145_REG_DPHY_MODE3		0x03
#define GC2145_DPHY_MODE3_LANE1_DIFF(a)	((a) & 0x07)
#define GC2145_DPHY_MODE3_CLK_DELAY		BIT(4)
#define GC2145_DPHY_MODE3_LANE0_DELAY		BIT(5)
#define GC2145_DPHY_MODE3_LANE1_DELAY		BIT(6)

#define GC2145_REG_FIFO_FULL_LVL	GC2145_REG16_LE(0x04)
#define GC2145_REG_FIFO_MODE		0x06
#define GC2145_FIFO_MODE_READ_GATE	BIT(3)
#define GC2145_FIFO_MODE_MIPI_CLK_MODULE	BIT(7)

#define GC2145_REG_BUF_CSI2_MODE	GC2145_REG8(0x10)
#define GC2145_CSI2_MODE_DOUBLE		BIT(0)
#define GC2145_CSI2_MODE_RAW8		BIT(2)
#define GC2145_CSI2_MODE_MIPI_EN	BIT(4)
#define GC2145_CSI2_MODE_EN		BIT(7)

#define GC2145_REG_MIPI_DT		GC2145_REG8(0x11)
#define GC2145_REG_LWC			GC2145_REG16_LE(0x12)
#define GC2145_REG_DPHY_MODE		0x15
#define GC2145_DPHY_MODE_TRIGGER_PROG	BIT(4)

#define GC2145_REG_FIFO_GATE_MODE	GC2145_REG8(0x17)
#define GC2145_REG_T_LPX		0x21
#define GC2145_REG_T_CLK_HS_PREPARE	0x22
#define GC2145_REG_T_CLK_ZERO		0x23
#define GC2145_REG_T_CLK_PRE		0x24
#define GC2145_REG_T_CLK_POST		0x25
#define GC2145_REG_T_CLK_TRAIL		0x26
#define GC2145_REG_T_HS_EXIT		0x27
#define GC2145_REG_T_WAKEUP		0x28
#define GC2145_REG_T_HS_PREPARE		0x29
#define GC2145_REG_T_HS_ZERO		0x2a
#define GC2145_REG_T_HS_TRAIL		0x2b

#define UXGA_HSIZE 1600
#define UXGA_VSIZE 1200

static const struct video_reg8 default_regs[] = {
	{GC2145_REG_RESET, 0xf0},
	{GC2145_REG_RESET, 0xf0},
	{GC2145_REG_RESET, 0xf0},
	{0xfc, 0x06},
	{0xf6, 0x00},
	{0xf7, 0x1d},
	{0xf8, 0x85},
	{0xfa, 0x00},
	{0xf9, 0xfe},
	{0xf2, 0x00},

	/* ISP settings */
	{GC2145_REG_RESET, GC2145_REG_RESET_P0_REGS},
	{0x03, 0x04},
	{0x04, 0xe2},

	{0x09, 0x00},
	{0x0a, 0x00},

	{0x0b, 0x00},
	{0x0c, 0x00},

	{0x0d, 0x04}, /* Window height */
	{0x0e, 0xc0},

	{0x0f, 0x06}, /* Window width */
	{0x10, 0x52},

	{0x99, 0x11}, /* Subsample */
	{0x9a, 0x0E}, /* Subsample mode */

	{0x12, 0x2e},
	{0x17, 0x14}, /* Analog Mode 1 (vflip/mirror[1:0]) */
	{0x18, 0x22}, /* Analog Mode 2 */
	{0x19, 0x0e},
	{0x1a, 0x01},
	{0x1b, 0x4b},
	{0x1c, 0x07},
	{0x1d, 0x10},
	{0x1e, 0x88},
	{0x1f, 0x78},
	{0x20, 0x03},
	{0x21, 0x40},
	{0x22, 0xa0},
	{0x24, 0x16},
	{0x25, 0x01},
	{0x26, 0x10},
	{0x2d, 0x60},
	{0x30, 0x01},
	{0x31, 0x90},
	{0x33, 0x06},
	{0x34, 0x01},
	{0x80, 0x7f},
	{0x81, 0x26},
	{0x82, 0xfa},
	{0x83, 0x00},
	{GC2145_REG_OUTPUT_FMT, GC2145_REG_OUTPUT_FMT_RGB565},
	{GC2145_REG_SYNC_MODE, GC2145_REG_SYNC_MODE_DEF},
	{0x88, 0x03},
	{GC2145_REG_BYPASS_MODE, 0x03},
	{0x85, 0x08},
	{0x8a, 0x00},
	{0x8b, 0x00},
	{0xb0, 0x55},
	{0xc3, 0x00},
	{0xc4, 0x80},
	{0xc5, 0x90},
	{0xc6, 0x3b},
	{0xc7, 0x46},
	{0xec, 0x06},
	{0xed, 0x04},
	{0xee, 0x60},
	{0xef, 0x90},
	{0xb6, 0x01},

	{0x90, 0x01},
	{0x91, 0x00},
	{0x92, 0x00},
	{0x93, 0x00},
	{0x94, 0x00},
	{0x95, 0x02},
	{0x96, 0x58},
	{0x97, 0x03},
	{0x98, 0x20},
	{0x99, 0x22},
	{0x9a, 0x0E},

	{0x9b, 0x00},
	{0x9c, 0x00},
	{0x9d, 0x00},
	{0x9e, 0x00},
	{0x9f, 0x00},
	{0xa0, 0x00},
	{0xa1, 0x00},
	{0xa2, 0x00},

	/* BLK Settings */
	{0x40, 0x42},
	{0x41, 0x00},
	{0x43, 0x5b},
	{0x5e, 0x00},
	{0x5f, 0x00},
	{0x60, 0x00},
	{0x61, 0x00},
	{0x62, 0x00},
	{0x63, 0x00},
	{0x64, 0x00},
	{0x65, 0x00},
	{0x66, 0x20},
	{0x67, 0x20},
	{0x68, 0x20},
	{0x69, 0x20},
	{0x76, 0x00},
	{0x6a, 0x08},
	{0x6b, 0x08},
	{0x6c, 0x08},
	{0x6d, 0x08},
	{0x6e, 0x08},
	{0x6f, 0x08},
	{0x70, 0x08},
	{0x71, 0x08},
	{0x76, 0x00},
	{0x72, 0xf0},
	{0x7e, 0x3c},
	{0x7f, 0x00},
	{GC2145_REG_RESET, GC2145_REG_RESET_P2_REGS},
	{0x48, 0x15},
	{0x49, 0x00},
	{0x4b, 0x0b},

	/* AEC Settings */
	{GC2145_REG_RESET, GC2145_REG_RESET_P1_REGS},
	{0x01, 0x04},
	{0x02, 0xc0},
	{0x03, 0x04},
	{0x04, 0x90},
	{0x05, 0x30},
	{0x06, 0x90},
	{0x07, 0x30},
	{0x08, 0x80},
	{0x09, 0x00},
	{0x0a, 0x82},
	{0x0b, 0x11},
	{0x0c, 0x10},
	{0x11, 0x10},
	{0x13, 0x68},
	{0x84, 0x00},
	{0x1c, 0x11},
	{0x1e, 0x61},
	{0x1f, 0x35},
	{0x20, 0x40},
	{0x22, 0x40},
	{0x23, 0x20},
	{GC2145_REG_RESET, GC2145_REG_RESET_P2_REGS},
	{0x0f, 0x04},
	{GC2145_REG_RESET, GC2145_REG_RESET_P1_REGS},
	{0x12, 0x30},
	{0x15, 0xb0},
	{0x10, 0x31},
	{0x3e, 0x28},
	{0x3f, 0xb0},
	{0x40, 0x90},
	{0x41, 0x0f},
	{GC2145_REG_RESET, GC2145_REG_RESET_P2_REGS},
	{0x90, 0x6c},
	{0x91, 0x03},
	{0x92, 0xcb},
	{0x94, 0x33},
	{0x95, 0x84},
	{0x97, 0x65},
	{0xa2, 0x11},
	{0x80, 0xc1},
	{0x81, 0x08},
	{0x82, 0x05},
	{0x83, 0x08},
	{0x84, 0x0a},
	{0x86, 0xf0},
	{0x87, 0x50},
	{0x88, 0x15},
	{0x89, 0xb0},
	{0x8a, 0x30},
	{0x8b, 0x10},
	{GC2145_REG_RESET, GC2145_REG_RESET_P1_REGS},
	{0x21, 0x04},
	{GC2145_REG_RESET, GC2145_REG_RESET_P2_REGS},
	{0xa3, 0x50},
	{0xa4, 0x20},
	{0xa5, 0x40},
	{0xa6, 0x80},
	{0xab, 0x40},
	{0xae, 0x0c},
	{0xb3, 0x46},
	{0xb4, 0x64},
	{0xb6, 0x38},
	{0xb7, 0x01},
	{0xb9, 0x2b},
	{0x3c, 0x04},
	{0x3d, 0x15},
	{0x4b, 0x06},
	{0x4c, 0x20},
	/* Gamma Control */
	{0x10, 0x09},
	{0x11, 0x0d},
	{0x12, 0x13},
	{0x13, 0x19},
	{0x14, 0x27},
	{0x15, 0x37},
	{0x16, 0x45},
	{0x84, 0x53},
	{0x18, 0x69},
	{0x19, 0x7d},
	{0x1a, 0x8f},
	{0x1b, 0x9d},
	{0x1c, 0xa9},
	{0x1d, 0xbd},
	{0x1e, 0xcd},
	{0x1f, 0xd9},
	{0x20, 0xe3},
	{0x21, 0xea},
	{0x22, 0xef},
	{0x23, 0xf5},
	{0x24, 0xf9},
	{0x25, 0xff},
	{GC2145_REG_RESET, GC2145_REG_RESET_P0_REGS},
	{0xc6, 0x20},
	{0xc7, 0x2b},
	{GC2145_REG_RESET, GC2145_REG_RESET_P2_REGS},
	{0x26, 0x0f},
	{0x27, 0x14},
	{0x28, 0x19},
	{0x29, 0x1e},
	{0x2a, 0x27},
	{0x2b, 0x33},
	{0x2c, 0x3b},
	{0x2d, 0x45},
	{0x2e, 0x59},
	{0x2f, 0x69},
	{0x30, 0x7c},
	{0x31, 0x89},
	{0x32, 0x98},
	{0x33, 0xae},
	{0x34, 0xc0},
	{0x35, 0xcf},
	{0x36, 0xda},
	{0x37, 0xe2},
	{0x38, 0xe9},
	{0x39, 0xf3},
	{0x3a, 0xf9},
	{0x3b, 0xff},
	{0xd1, 0x32},
	{0xd2, 0x32},
	{0xd3, 0x40},
	{0xd6, 0xf0},
	{0xd7, 0x10},
	{0xd8, 0xda},
	{0xdd, 0x14},
	{0xde, 0x86},
	{0xed, 0x80},
	{0xee, 0x00},
	{0xef, 0x3f},
	{0xd8, 0xd8},
	{GC2145_REG_RESET, GC2145_REG_RESET_P1_REGS},
	{0x9f, 0x40},
	{0xc2, 0x14},
	{0xc3, 0x0d},
	{0xc4, 0x0c},
	{0xc8, 0x15},
	{0xc9, 0x0d},
	{0xca, 0x0a},
	{0xbc, 0x24},
	{0xbd, 0x10},
	{0xbe, 0x0b},
	{0xb6, 0x25},
	{0xb7, 0x16},
	{0xb8, 0x15},
	{0xc5, 0x00},
	{0xc6, 0x00},
	{0xc7, 0x00},
	{0xcb, 0x00},
	{0xcc, 0x00},
	{0xcd, 0x00},
	{0xbf, 0x07},
	{0xc0, 0x00},
	{0xc1, 0x00},
	{0xb9, 0x00},
	{0xba, 0x00},
	{0xbb, 0x00},
	{0xaa, 0x01},
	{0xab, 0x01},
	{0xac, 0x00},
	{0xad, 0x05},
	{0xae, 0x06},
	{0xaf, 0x0e},
	{0xb0, 0x0b},
	{0xb1, 0x07},
	{0xb2, 0x06},
	{0xb3, 0x17},
	{0xb4, 0x0e},
	{0xb5, 0x0e},
	{0xd0, 0x09},
	{0xd1, 0x00},
	{0xd2, 0x00},
	{0xd6, 0x08},
	{0xd7, 0x00},
	{0xd8, 0x00},
	{0xd9, 0x00},
	{0xda, 0x00},
	{0xdb, 0x00},
	{0xd3, 0x0a},
	{0xd4, 0x00},
	{0xd5, 0x00},
	{0xa4, 0x00},
	{0xa5, 0x00},
	{0xa6, 0x77},
	{0xa7, 0x77},
	{0xa8, 0x77},
	{0xa9, 0x77},
	{0xa1, 0x80},
	{0xa2, 0x80},
	{0xdf, 0x0d},
	{0xdc, 0x25},
	{0xdd, 0x30},
	{0xe0, 0x77},
	{0xe1, 0x80},
	{0xe2, 0x77},
	{0xe3, 0x90},
	{0xe6, 0x90},
	{0xe7, 0xa0},
	{0xe8, 0x90},
	{0xe9, 0xa0},
	{0x4f, 0x00},
	{0x4f, 0x00},
	{0x4b, 0x01},
	{0x4f, 0x00},
	{0x4c, 0x01},
	{0x4d, 0x71},
	{0x4e, 0x01},
	{0x4c, 0x01},
	{0x4d, 0x91},
	{0x4e, 0x01},
	{0x4c, 0x01},
	{0x4d, 0x70},
	{0x4e, 0x01},
	{0x4c, 0x01},
	{0x4d, 0x90},
	{0x4e, 0x02},
	{0x4c, 0x01},
	{0x4d, 0xb0},
	{0x4e, 0x02},
	{0x4c, 0x01},
	{0x4d, 0x8f},
	{0x4e, 0x02},
	{0x4c, 0x01},
	{0x4d, 0x6f},
	{0x4e, 0x02},
	{0x4c, 0x01},
	{0x4d, 0xaf},
	{0x4e, 0x02},
	{0x4c, 0x01},
	{0x4d, 0xd0},
	{0x4e, 0x02},
	{0x4c, 0x01},
	{0x4d, 0xf0},
	{0x4e, 0x02},
	{0x4c, 0x01},
	{0x4d, 0xcf},
	{0x4e, 0x02},
	{0x4c, 0x01},
	{0x4d, 0xef},
	{0x4e, 0x02},
	{0x4c, 0x01},
	{0x4d, 0x6e},
	{0x4e, 0x03},
	{0x4c, 0x01},
	{0x4d, 0x8e},
	{0x4e, 0x03},
	{0x4c, 0x01},
	{0x4d, 0xae},
	{0x4e, 0x03},
	{0x4c, 0x01},
	{0x4d, 0xce},
	{0x4e, 0x03},
	{0x4c, 0x01},
	{0x4d, 0x4d},
	{0x4e, 0x03},
	{0x4c, 0x01},
	{0x4d, 0x6d},
	{0x4e, 0x03},
	{0x4c, 0x01},
	{0x4d, 0x8d},
	{0x4e, 0x03},
	{0x4c, 0x01},
	{0x4d, 0xad},
	{0x4e, 0x03},
	{0x4c, 0x01},
	{0x4d, 0xcd},
	{0x4e, 0x03},
	{0x4c, 0x01},
	{0x4d, 0x4c},
	{0x4e, 0x03},
	{0x4c, 0x01},
	{0x4d, 0x6c},
	{0x4e, 0x03},
	{0x4c, 0x01},
	{0x4d, 0x8c},
	{0x4e, 0x03},
	{0x4c, 0x01},
	{0x4d, 0xac},
	{0x4e, 0x03},
	{0x4c, 0x01},
	{0x4d, 0xcc},
	{0x4e, 0x03},
	{0x4c, 0x01},
	{0x4d, 0xcb},
	{0x4e, 0x03},
	{0x4c, 0x01},
	{0x4d, 0x4b},
	{0x4e, 0x03},
	{0x4c, 0x01},
	{0x4d, 0x6b},
	{0x4e, 0x03},
	{0x4c, 0x01},
	{0x4d, 0x8b},
	{0x4e, 0x03},
	{0x4c, 0x01},
	{0x4d, 0xab},
	{0x4e, 0x03},
	{0x4c, 0x01},
	{0x4d, 0x8a},
	{0x4e, 0x04},
	{0x4c, 0x01},
	{0x4d, 0xaa},
	{0x4e, 0x04},
	{0x4c, 0x01},
	{0x4d, 0xca},
	{0x4e, 0x04},
	{0x4c, 0x01},
	{0x4d, 0xca},
	{0x4e, 0x04},
	{0x4c, 0x01},
	{0x4d, 0xc9},
	{0x4e, 0x04},
	{0x4c, 0x01},
	{0x4d, 0x8a},
	{0x4e, 0x04},
	{0x4c, 0x01},
	{0x4d, 0x89},
	{0x4e, 0x04},
	{0x4c, 0x01},
	{0x4d, 0xa9},
	{0x4e, 0x04},
	{0x4c, 0x02},
	{0x4d, 0x0b},
	{0x4e, 0x05},
	{0x4c, 0x02},
	{0x4d, 0x0a},
	{0x4e, 0x05},
	{0x4c, 0x01},
	{0x4d, 0xeb},
	{0x4e, 0x05},
	{0x4c, 0x01},
	{0x4d, 0xea},
	{0x4e, 0x05},
	{0x4c, 0x02},
	{0x4d, 0x09},
	{0x4e, 0x05},
	{0x4c, 0x02},
	{0x4d, 0x29},
	{0x4e, 0x05},
	{0x4c, 0x02},
	{0x4d, 0x2a},
	{0x4e, 0x05},
	{0x4c, 0x02},
	{0x4d, 0x4a},
	{0x4e, 0x05},
	{0x4c, 0x02},
	{0x4d, 0x8a},
	{0x4e, 0x06},
	{0x4c, 0x02},
	{0x4d, 0x49},
	{0x4e, 0x06},
	{0x4c, 0x02},
	{0x4d, 0x69},
	{0x4e, 0x06},
	{0x4c, 0x02},
	{0x4d, 0x89},
	{0x4e, 0x06},
	{0x4c, 0x02},
	{0x4d, 0xa9},
	{0x4e, 0x06},
	{0x4c, 0x02},
	{0x4d, 0x48},
	{0x4e, 0x06},
	{0x4c, 0x02},
	{0x4d, 0x68},
	{0x4e, 0x06},
	{0x4c, 0x02},
	{0x4d, 0x69},
	{0x4e, 0x06},
	{0x4c, 0x02},
	{0x4d, 0xca},
	{0x4e, 0x07},
	{0x4c, 0x02},
	{0x4d, 0xc9},
	{0x4e, 0x07},
	{0x4c, 0x02},
	{0x4d, 0xe9},
	{0x4e, 0x07},
	{0x4c, 0x03},
	{0x4d, 0x09},
	{0x4e, 0x07},
	{0x4c, 0x02},
	{0x4d, 0xc8},
	{0x4e, 0x07},
	{0x4c, 0x02},
	{0x4d, 0xe8},
	{0x4e, 0x07},
	{0x4c, 0x02},
	{0x4d, 0xa7},
	{0x4e, 0x07},
	{0x4c, 0x02},
	{0x4d, 0xc7},
	{0x4e, 0x07},
	{0x4c, 0x02},
	{0x4d, 0xe7},
	{0x4e, 0x07},
	{0x4c, 0x03},
	{0x4d, 0x07},
	{0x4e, 0x07},

	{0x4f, 0x01},
	{0x50, 0x80},
	{0x51, 0xa8},
	{0x52, 0x47},
	{0x53, 0x38},
	{0x54, 0xc7},
	{0x56, 0x0e},
	{0x58, 0x08},
	{0x5b, 0x00},
	{0x5c, 0x74},
	{0x5d, 0x8b},
	{0x61, 0xdb},
	{0x62, 0xb8},
	{0x63, 0x86},
	{0x64, 0xc0},
	{0x65, 0x04},
	{0x67, 0xa8},
	{0x68, 0xb0},
	{0x69, 0x00},
	{0x6a, 0xa8},
	{0x6b, 0xb0},
	{0x6c, 0xaf},
	{0x6d, 0x8b},
	{0x6e, 0x50},
	{0x6f, 0x18},
	{0x73, 0xf0},
	{0x70, 0x0d},
	{0x71, 0x60},
	{0x72, 0x80},
	{0x74, 0x01},
	{0x75, 0x01},
	{0x7f, 0x0c},
	{0x76, 0x70},
	{0x77, 0x58},
	{0x78, 0xa0},
	{0x79, 0x5e},
	{0x7a, 0x54},
	{0x7b, 0x58},
	{GC2145_REG_RESET, GC2145_REG_RESET_P2_REGS},
	{0xc0, 0x01},
	{0xc1, 0x44},
	{0xc2, 0xfd},
	{0xc3, 0x04},
	{0xc4, 0xF0},
	{0xc5, 0x48},
	{0xc6, 0xfd},
	{0xc7, 0x46},
	{0xc8, 0xfd},
	{0xc9, 0x02},
	{0xca, 0xe0},
	{0xcb, 0x45},
	{0xcc, 0xec},
	{0xcd, 0x48},
	{0xce, 0xf0},
	{0xcf, 0xf0},
	{0xe3, 0x0c},
	{0xe4, 0x4b},
	{0xe5, 0xe0},
	{GC2145_REG_RESET, GC2145_REG_RESET_P1_REGS},
	{0x9f, 0x40},

	/* Output Control  */
	{GC2145_REG_RESET, GC2145_REG_RESET_P2_REGS},
	{0x40, 0xbf},
	{0x46, 0xcf},

	{GC2145_REG_RESET, GC2145_REG_RESET_P0_REGS},
	{0x05, 0x01},
	{0x06, 0x1C},
	{0x07, 0x00},
	{0x08, 0x32},
	{0x11, 0x00},
	{0x12, 0x1D},
	{0x13, 0x00},
	{0x14, 0x00},

	{GC2145_REG_RESET, GC2145_REG_RESET_P1_REGS},
	{0x3c, 0x00},
	{0x3d, 0x04},
	{GC2145_REG_RESET, GC2145_REG_RESET_P0_REGS},
	{0x00, 0x00},
};

static const struct video_reg8 default_mipi_csi_regs[] = {
	/* Switch to page 3 */
	{GC2145_REG_RESET, GC2145_REG_RESET_P3_REGS},
	{GC2145_REG_DPHY_MODE1, GC2145_DPHY_MODE1_CLK_EN |
				GC2145_DPHY_MODE1_LANE0_EN | GC2145_DPHY_MODE1_LANE1_EN |
				GC2145_DPHY_MODE1_CLK_LANE_P2S_SEL},
	{GC2145_REG_DPHY_MODE2, GC2145_DPHY_MODE2_CLK_DIFF(2) |
				GC2145_DPHY_MODE2_LANE0_DIFF(2)},
	{GC2145_REG_DPHY_MODE3, GC2145_DPHY_MODE3_LANE1_DIFF(0) |
				GC2145_DPHY_MODE3_CLK_DELAY},
	{GC2145_REG_FIFO_MODE, GC2145_FIFO_MODE_READ_GATE |
			       GC2145_FIFO_MODE_MIPI_CLK_MODULE},
	{GC2145_REG_DPHY_MODE, GC2145_DPHY_MODE_TRIGGER_PROG},

	/* Clock & Data lanes timing */
	{GC2145_REG_T_LPX, 0x10},
	{GC2145_REG_T_CLK_HS_PREPARE, 0x04},
	{GC2145_REG_T_CLK_ZERO, 0x10},
	{GC2145_REG_T_CLK_PRE, 0x10},
	{GC2145_REG_T_CLK_POST, 0x10},
	{GC2145_REG_T_CLK_TRAIL, 0x05},
	{GC2145_REG_T_HS_PREPARE, 0x03},
	{GC2145_REG_T_HS_ZERO, 0x0a},
	{GC2145_REG_T_HS_TRAIL, 0x06},
};

struct gc2145_config {
	struct i2c_dt_spec i2c;
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(pwdn_gpios)
	struct gpio_dt_spec pwdn_gpio;
#endif
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
	struct gpio_dt_spec reset_gpio;
#endif
	int bus_type;
};

struct gc2145_ctrls {
	struct video_ctrl hflip;
	struct video_ctrl vflip;
	struct video_ctrl linkfreq;
};

struct gc2145_data {
	struct gc2145_ctrls ctrls;
	struct video_format fmt;
	struct video_rect crop;
	uint16_t format_width;
	uint16_t format_height;
	uint8_t ratio;
};

#define GC2145_VIDEO_FORMAT_CAP_HL(width_l, width_h, height_l, height_h, format, step_w, step_h) \
	{										\
		.pixelformat = (format),						\
		.width_min = (width_l), .width_max = (width_h),				\
		.height_min = (height_l), .height_max = (height_h),			\
		.width_step = (step_w), .height_step = (step_h),			\
	}

#define GC2145_VIDEO_FORMAT_CAP(width, height, format)	\
	GC2145_VIDEO_FORMAT_CAP_HL((width), (width), (height), (height), (format), 0, 0)

#define RESOLUTION_QVGA_W	320
#define RESOLUTION_QVGA_H	240

#define RESOLUTION_VGA_W	640
#define RESOLUTION_VGA_H	480

#define RESOLUTION_UXGA_W	1600
#define RESOLUTION_UXGA_H	1200

#define RESOLUTION_MAX_W	RESOLUTION_UXGA_W
#define RESOLUTION_MAX_H	RESOLUTION_UXGA_H

/* Min not defined - smallest seen implementation is for QQVGA */
#define RESOLUTION_MIN_W	160
#define RESOLUTION_MIN_H	120

static const struct video_format_cap fmts[] = {
	GC2145_VIDEO_FORMAT_CAP(RESOLUTION_QVGA_W, RESOLUTION_QVGA_H, VIDEO_PIX_FMT_RGB565),
	GC2145_VIDEO_FORMAT_CAP(RESOLUTION_VGA_W, RESOLUTION_VGA_H, VIDEO_PIX_FMT_RGB565),
	GC2145_VIDEO_FORMAT_CAP(RESOLUTION_UXGA_W, RESOLUTION_UXGA_H, VIDEO_PIX_FMT_RGB565),
	GC2145_VIDEO_FORMAT_CAP(RESOLUTION_QVGA_W, RESOLUTION_QVGA_H, VIDEO_PIX_FMT_YUYV),
	GC2145_VIDEO_FORMAT_CAP(RESOLUTION_VGA_W, RESOLUTION_VGA_H, VIDEO_PIX_FMT_YUYV),
	GC2145_VIDEO_FORMAT_CAP(RESOLUTION_UXGA_W, RESOLUTION_UXGA_H, VIDEO_PIX_FMT_YUYV),
	/* Add catchall resolution  */
	GC2145_VIDEO_FORMAT_CAP_HL(RESOLUTION_MIN_W, RESOLUTION_MAX_W, RESOLUTION_MIN_H,
				   RESOLUTION_MAX_H, VIDEO_PIX_FMT_RGB565, 1, 1),
	GC2145_VIDEO_FORMAT_CAP_HL(RESOLUTION_MIN_W, RESOLUTION_MAX_W, RESOLUTION_MIN_H,
				   RESOLUTION_MAX_H, VIDEO_PIX_FMT_YUYV, 1, 1),
	{0},
};

static int gc2145_soft_reset(const struct device *dev)
{
	const struct gc2145_config *cfg = dev->config;
	int ret;

	/* Initiate system reset */
	ret = video_write_cci_reg(&cfg->i2c, GC2145_REG8(GC2145_REG_RESET), GC2145_REG_SW_RESET);
	if (ret) {
		return ret;
	}

	k_msleep(300);

	return 0;
}

static int gc2145_set_output_format(const struct device *dev, int output_format)
{
	const struct gc2145_config *cfg = dev->config;
	uint8_t bypass_switch = 0;
	int ret;

	ret = video_write_cci_reg(&cfg->i2c, GC2145_REG8(GC2145_REG_RESET),
				  GC2145_REG_RESET_P0_REGS);
	if (ret < 0) {
		return ret;
	}

	/* Map format to sensor format */
	if (output_format == VIDEO_PIX_FMT_RGB565) {
		output_format = GC2145_REG_OUTPUT_FMT_RGB565;

		/*
		 * For RGB565 format on CSI, it is necessary to set switch bit in order to
		 * have proper RGB565 CSI format (aka _LE) generated
		 */
		if (cfg->bus_type == VIDEO_BUS_TYPE_CSI2_DPHY) {
			bypass_switch = GC2145_REG_BYPASS_MODE_SWITCH;
		}
	} else if (output_format == VIDEO_PIX_FMT_YUYV) {
		output_format = GC2145_REG_OUTPUT_FMT_YCBYCR;
	} else {
		LOG_ERR("Image format not supported");
		return -ENOTSUP;
	}

	ret = video_modify_cci_reg(&cfg->i2c, GC2145_REG8(GC2145_REG_OUTPUT_FMT),
				   GC2145_REG_OUTPUT_FMT_MASK, output_format);
	if (ret < 0) {
		return ret;
	}

	if (cfg->bus_type == VIDEO_BUS_TYPE_CSI2_DPHY) {
		ret = video_modify_cci_reg(&cfg->i2c, GC2145_REG8(GC2145_REG_BYPASS_MODE),
					   GC2145_REG_BYPASS_MODE_SWITCH, bypass_switch);
		if (ret < 0) {
			return ret;
		}
	}

	k_sleep(K_MSEC(30));

	return 0;
}

static int gc2145_set_crop_registers(const struct gc2145_config *cfg, uint32_t x, uint32_t y,
				     uint32_t w, uint32_t h)
{
	int ret;

	ret = video_write_cci_reg(&cfg->i2c, GC2145_REG_OUT_WIN_ROW_START, y);
	if (ret < 0) {
		return ret;
	}
	ret = video_write_cci_reg(&cfg->i2c, GC2145_REG_OUT_WIN_COL_START, x);
	if (ret < 0) {
		return ret;
	}
	ret = video_write_cci_reg(&cfg->i2c, GC2145_REG_OUT_WIN_HEIGHT, h);
	if (ret < 0) {
		return ret;
	}
	ret = video_write_cci_reg(&cfg->i2c, GC2145_REG_OUT_WIN_WIDTH, w);
	if (ret < 0) {
		return ret;
	}

	/* Enable crop */
	return video_write_cci_reg(&cfg->i2c, GC2145_REG_CROP_ENABLE, GC2145_CROP_SET_ENABLE);
}

static int gc2145_set_resolution(const struct device *dev, uint32_t w, uint32_t h)
{
	const struct gc2145_config *cfg = dev->config;
	struct gc2145_data *drv_data = dev->data;
	int ret;

	uint16_t win_w;
	uint16_t win_h;
	uint16_t win_x;
	uint16_t win_y;

	/* If we are called from set_format, then we compute ratio and initialize crop */
	drv_data->ratio = MIN(RESOLUTION_UXGA_W / w, RESOLUTION_UXGA_H / h);

	/* make sure we don't end up with ratio of 0 */
	if (drv_data->ratio == 0) {
		return -EIO;
	}

	/* Restrict ratio to 3 for faster refresh ? */
	if (drv_data->ratio > 3) {
		drv_data->ratio = 3;
	}

	/* remember the width and height passed in */
	drv_data->format_width = w;
	drv_data->format_height = h;

	/* Default to crop rectangle being same size as passed in resolution */
	drv_data->crop.left = 0;
	drv_data->crop.top = 0;
	drv_data->crop.width = w;
	drv_data->crop.height = h;

	/* Calculates the window boundaries to obtain the desired resolution */

	win_w = w * drv_data->ratio;
	win_h = h * drv_data->ratio;
	win_x = ((UXGA_HSIZE - win_w) / 2);
	win_y = ((UXGA_VSIZE - win_h) / 2);

	ret = video_write_cci_reg(&cfg->i2c, GC2145_REG8(GC2145_REG_RESET),
				  GC2145_REG_RESET_P0_REGS);
	if (ret < 0) {
		return ret;
	}

	/* Set readout window first. */
	ret = video_write_cci_reg(&cfg->i2c, GC2145_REG_WIN_ROW_START, win_y);
	if (ret < 0) {
		return ret;
	}
	ret = video_write_cci_reg(&cfg->i2c, GC2145_REG_WIN_COL_START, win_x);
	if (ret < 0) {
		return ret;
	}
	ret = video_write_cci_reg(&cfg->i2c, GC2145_REG_WIN_HEIGHT, win_h + 8);
	if (ret < 0) {
		return ret;
	}
	ret = video_write_cci_reg(&cfg->i2c, GC2145_REG_WIN_WIDTH, win_w + 16);
	if (ret < 0) {
		return ret;
	}

	/* Set cropping window next. */
	ret = gc2145_set_crop_registers(cfg, 0, 0, w, h);
	if (ret < 0) {
		return ret;
	}

	/* Set Sub-sampling ratio and mode */
	ret = video_write_cci_reg(&cfg->i2c, GC2145_REG_SUBSAMPLE,
				  (drv_data->ratio << 4) | drv_data->ratio);
	if (ret < 0) {
		return ret;
	}

	ret = video_write_cci_reg(&cfg->i2c, GC2145_REG_SUBSAMPLE_MODE,
				  GC2145_SUBSAMPLE_MODE_SMOOTH);
	if (ret < 0) {
		return ret;
	}
	/*
	 * Galaxy Core datasheet does not document the reason behind of this
	 * and other short delay requirements, but the reason exposed by them
	 * is to give enough time for the sensor DSP to handle the I2C transaction
	 * give some time time to apply the changes before the next instruction.
	 */
	k_sleep(K_MSEC(30));

	return 0;
}

static int gc2145_set_crop(const struct device *dev, struct video_selection *sel)
{
	/* set the crop rectangle */
	int ret;
	const struct gc2145_config *cfg = dev->config;
	struct gc2145_data *drv_data = dev->data;

	/* Verify the passed in rectangle is valid */
	if (((sel->rect.left + sel->rect.width) > drv_data->format_width) ||
	    ((sel->rect.top + sel->rect.height) > drv_data->format_height)) {
		LOG_ERR("Crop rectangle is invalid(%u %u) %ux%u > %ux%u",
			sel->rect.left, sel->rect.top, sel->rect.width, sel->rect.height,
			drv_data->format_width, drv_data->format_height);
		return -EINVAL;
	}

	/* if rectangle passed in is same as current, simply return */
	if ((drv_data->crop.left == sel->rect.left) && (drv_data->crop.top == sel->rect.top) &&
	    (drv_data->crop.width == sel->rect.width) &&
	    (drv_data->crop.height == sel->rect.height)) {
		return 0;
	}

	/* save out the updated crop window registers */
	ret = video_write_cci_reg(&cfg->i2c, GC2145_REG8(GC2145_REG_RESET),
				  GC2145_REG_RESET_P0_REGS);
	if (ret < 0) {
		return ret;
	}

	ret = gc2145_set_crop_registers(cfg, sel->rect.left, sel->rect.top,
					sel->rect.width, sel->rect.height);
	if (ret < 0) {
		return ret;
	}

	drv_data->crop = sel->rect;

	/* enqueue/dequeue depend on this being set as well as the crop */
	drv_data->fmt.width = drv_data->crop.width;
	drv_data->fmt.height = drv_data->crop.height;

	return 0;
}

static int gc2145_check_connection(const struct device *dev)
{
	const struct gc2145_config *cfg = dev->config;
	uint32_t chip_id;
	int ret;

	ret = video_read_cci_reg(&cfg->i2c, GC2145_REG_CHIP_ID, &chip_id);
	if (ret < 0) {
		return ret;
	}

	if (chip_id != 0x2145 && chip_id != 0x2155) {
		LOG_WRN("Unexpected GC2145 chip ID: 0x%04x", chip_id);
	}

	return 0;
}

#define GC2145_640_480_LINK_FREQ	120000000
#define GC2145_640_480_LINK_FREQ_ID	0
#define GC2145_1600_1200_LINK_FREQ	240000000
#define GC2145_1600_1200_LINK_FREQ_ID	1
const int64_t gc2145_link_frequency[] = {
	GC2145_640_480_LINK_FREQ, GC2145_1600_1200_LINK_FREQ,
};
static int gc2145_config_csi(const struct device *dev, uint32_t pixelformat,
			     uint32_t width, uint32_t height)
{
	const struct gc2145_config *cfg = dev->config;
	struct gc2145_data *drv_data = dev->data;
	struct gc2145_ctrls *ctrls = &drv_data->ctrls;
	uint16_t fifo_full_level = width == 1600 ? 0x0001 : 0x0190;
	uint16_t lwc = width * video_bits_per_pixel(pixelformat) / BITS_PER_BYTE;
	uint8_t csi_dt;
	int ret;

	switch (pixelformat) {
	case VIDEO_PIX_FMT_RGB565:
		csi_dt = VIDEO_MIPI_CSI2_DT_RGB565;
		break;
	case VIDEO_PIX_FMT_YUYV:
		csi_dt = VIDEO_MIPI_CSI2_DT_YUV422_8;
		break;
	default:
		LOG_ERR("Unsupported pixelformat for CSI");
		return -EINVAL;
	}

	/* Only VGA & UXGA work (currently) in CSI */
	if (width == RESOLUTION_VGA_W && height == RESOLUTION_VGA_H) {
		ctrls->linkfreq.val = GC2145_640_480_LINK_FREQ_ID;
	} else if (width == RESOLUTION_UXGA_W && height == RESOLUTION_UXGA_H) {
		ctrls->linkfreq.val = GC2145_1600_1200_LINK_FREQ_ID;
	} else {
		LOG_ERR("Unsupported resolution 320x240 for CSI");
		return -EINVAL;
	}

	/* Apply fixed settings for MIPI-CSI. After that active page is 3 */
	ret = video_write_cci_multiregs8(&cfg->i2c, default_mipi_csi_regs,
					 ARRAY_SIZE(default_mipi_csi_regs));
	if (ret < 0) {
		return ret;
	}

	ret = video_write_cci_reg(&cfg->i2c, GC2145_REG_LWC, lwc);
	if (ret < 0) {
		return ret;
	}

	ret = video_write_cci_reg(&cfg->i2c, GC2145_REG_FIFO_FULL_LVL, fifo_full_level);
	if (ret < 0) {
		return ret;
	}

	ret = video_write_cci_reg(&cfg->i2c, GC2145_REG_FIFO_GATE_MODE, 0xf0);
	if (ret < 0) {
		return ret;
	}

	ret = video_write_cci_reg(&cfg->i2c, GC2145_REG_MIPI_DT, csi_dt);
	if (ret < 0) {
		return ret;
	}

	return video_write_cci_reg(&cfg->i2c, GC2145_REG8(GC2145_REG_RESET),
				   GC2145_REG_RESET_P0_REGS);
}

static int gc2145_set_fmt(const struct device *dev, struct video_format *fmt)
{
	struct gc2145_data *drv_data = dev->data;
	const struct gc2145_config *cfg = dev->config;
	size_t idx;
	int ret;

	if (memcmp(&drv_data->fmt, fmt, sizeof(drv_data->fmt)) == 0) {
		/* nothing to do */
		return 0;
	}

	/* Check if camera is capable of handling given format */
	ret = video_format_caps_index(fmts, fmt, &idx);
	if (ret < 0) {
		LOG_ERR("Image format not supported");
		return ret;
	}

	/* Set output format */
	ret = gc2145_set_output_format(dev, fmt->pixelformat);
	if (ret < 0) {
		LOG_ERR("Failed to set the output format");
		return ret;
	}

	/* Set window size */
	ret = gc2145_set_resolution(dev, fmt->width, fmt->height);

	if (ret < 0) {
		LOG_ERR("Failed to set the resolution");
		return ret;
	}

	if (cfg->bus_type == VIDEO_BUS_TYPE_CSI2_DPHY) {
		ret = gc2145_config_csi(dev, fmt->pixelformat, fmt->width, fmt->height);
		if (ret < 0) {
			LOG_ERR("Failed to configure MIPI-CSI");
			return ret;
		}
	}

	drv_data->fmt = *fmt;

	return 0;
}

static int gc2145_get_fmt(const struct device *dev, struct video_format *fmt)
{
	struct gc2145_data *drv_data = dev->data;

	*fmt = drv_data->fmt;

	return 0;
}

static int gc2145_set_stream_dvp(const struct device *dev, bool enable)
{
	const struct gc2145_config *cfg = dev->config;

	return video_write_cci_reg(&cfg->i2c, GC2145_REG8(0xf2), enable ? 0x0f : 0x00);
}

static int gc2145_set_stream_csi(const struct device *dev, bool enable)
{
	const struct gc2145_config *cfg = dev->config;
	int ret;

	ret = video_write_cci_reg(&cfg->i2c, GC2145_REG8(GC2145_REG_RESET),
				  GC2145_REG_RESET_P3_REGS);
	if (ret < 0) {
		return ret;
	}

	ret = video_write_cci_reg(&cfg->i2c, GC2145_REG_BUF_CSI2_MODE, enable ?
				  GC2145_CSI2_MODE_RAW8 | GC2145_CSI2_MODE_DOUBLE |
				  GC2145_CSI2_MODE_EN | GC2145_CSI2_MODE_MIPI_EN :
				  0);
	if (ret < 0) {
		return ret;
	}

	return video_write_cci_reg(&cfg->i2c, GC2145_REG8(GC2145_REG_RESET),
				   GC2145_REG_RESET_P0_REGS);
}

static int gc2145_set_stream(const struct device *dev, bool enable, enum video_buf_type type)
{
	const struct gc2145_config *cfg = dev->config;

	return cfg->bus_type == VIDEO_BUS_TYPE_PARALLEL ?
	       gc2145_set_stream_dvp(dev, enable) :
	       gc2145_set_stream_csi(dev, enable);
}

static int gc2145_get_caps(const struct device *dev, struct video_caps *caps)
{
	caps->format_caps = fmts;
	return 0;
}

static int gc2145_set_ctrl(const struct device *dev, uint32_t id)
{
	const struct gc2145_config *cfg = dev->config;
	struct gc2145_data *drv_data = dev->data;

	switch (id) {
	case VIDEO_CID_HFLIP:
		/* Set the horizontal mirror state */
		return video_modify_cci_reg(&cfg->i2c, GC2145_REG_AMODE1, GC2145_REG_AMODE1_MIRROR,
					    drv_data->ctrls.hflip.val ?
					    GC2145_REG_AMODE1_MIRROR : 0);
	case VIDEO_CID_VFLIP:
		/* Set the vertical flip state */
		return video_modify_cci_reg(&cfg->i2c, GC2145_REG_AMODE1, GC2145_REG_AMODE1_UPDOWN,
					    drv_data->ctrls.vflip.val ?
					    GC2145_REG_AMODE1_UPDOWN : 0);
	default:
		return -ENOTSUP;
	}
}

static int gc2145_set_selection(const struct device *dev, struct video_selection *sel)
{
	if (sel->type != VIDEO_BUF_TYPE_OUTPUT) {
		return -EINVAL;
	}

	if (sel->target != VIDEO_SEL_TGT_CROP) {
		return -EINVAL;
	}

	return gc2145_set_crop(dev, sel);
}

static int gc2145_get_selection(const struct device *dev, struct video_selection *sel)
{
	struct gc2145_data *drv_data = dev->data;

	if (sel->type != VIDEO_BUF_TYPE_OUTPUT) {
		return -EINVAL;
	}

	switch (sel->target) {
	case VIDEO_SEL_TGT_CROP:
		sel->rect = drv_data->crop;
		break;

	case VIDEO_SEL_TGT_NATIVE_SIZE:
		sel->rect.top = 0;
		sel->rect.left = 0;
		sel->rect.width = drv_data->format_width;
		sel->rect.height = drv_data->format_height;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static DEVICE_API(video, gc2145_driver_api) = {
	.set_format = gc2145_set_fmt,
	.get_format = gc2145_get_fmt,
	.get_caps = gc2145_get_caps,
	.set_stream = gc2145_set_stream,
	.set_ctrl = gc2145_set_ctrl,
	.set_selection = gc2145_set_selection,
	.get_selection = gc2145_get_selection,
};

static int gc2145_init_controls(const struct device *dev)
{
	int ret;
	struct gc2145_data *drv_data = dev->data;
	struct gc2145_ctrls *ctrls = &drv_data->ctrls;

	ret = video_init_ctrl(&ctrls->hflip, dev, VIDEO_CID_HFLIP,
			      (struct video_ctrl_range){.min = 0, .max = 1, .step = 1, .def = 0});
	if (ret) {
		return ret;
	}

	ret = video_init_ctrl(&ctrls->vflip, dev, VIDEO_CID_VFLIP,
			      (struct video_ctrl_range){.min = 0, .max = 1, .step = 1, .def = 0});
	if (ret < 0) {
		return ret;
	}

	ret = video_init_int_menu_ctrl(&ctrls->linkfreq, dev, VIDEO_CID_LINK_FREQ,
				       GC2145_640_480_LINK_FREQ_ID, gc2145_link_frequency,
				       ARRAY_SIZE(gc2145_link_frequency));
	if (ret < 0) {
		return ret;
	}

	ctrls->linkfreq.flags |= VIDEO_CTRL_FLAG_READ_ONLY;

	return 0;
}

static int gc2145_init(const struct device *dev)
{
	/* set default/init format VGA RGB565 */
	struct video_format fmt = {
		.pixelformat = VIDEO_PIX_FMT_RGB565,
		.width = RESOLUTION_VGA_W,
		.height = RESOLUTION_VGA_H,
	};
	int ret;
	const struct gc2145_config *cfg = dev->config;
	(void) cfg;

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(pwdn_gpios)
	if (cfg->pwdn_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->pwdn_gpio)) {
			LOG_ERR("%s: device %s is not ready", dev->name, cfg->pwdn_gpio.port->name);
			return -ENODEV;
		}
		ret = gpio_pin_configure_dt(&cfg->pwdn_gpio, GPIO_OUTPUT_INACTIVE);
		if (ret) {
			return ret;
		}
	}
	k_sleep(K_MSEC(10));
#endif
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
	if (cfg->reset_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->reset_gpio)) {
			LOG_ERR("%s: device %s is not ready", dev->name,
				cfg->reset_gpio.port->name);
			return -ENODEV;
		}
		ret = gpio_pin_configure_dt(&cfg->reset_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret) {
			return ret;
		}

		k_sleep(K_MSEC(1));
		gpio_pin_set_dt(&cfg->reset_gpio, 0);
		k_sleep(K_MSEC(1));
	}
#endif

	ret = gc2145_check_connection(dev);
	if (ret) {
		return ret;
	}

	ret = gc2145_soft_reset(dev);
	if (ret < 0) {
		return ret;
	}

	ret = video_write_cci_multiregs8(&cfg->i2c, default_regs, ARRAY_SIZE(default_regs));
	if (ret < 0) {
		return ret;
	}

	ret = gc2145_set_fmt(dev, &fmt);
	if (ret) {
		LOG_ERR("Unable to configure default format");
		return ret;
	}

	/* Initialize controls */
	return gc2145_init_controls(dev);
}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
#define GC2145_GET_RESET_GPIO(n)	.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(n, reset_gpios, {0}),
#else
#define GC2145_GET_RESET_GPIO(n)
#endif
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(pwdn_gpios)
#define GC2145_GET_PWDN_GPIO(n)	.pwdn_gpio = GPIO_DT_SPEC_INST_GET_OR(n, pwdn_gpios, {0}),
#else
#define GC2145_GET_PWDN_GPIO(n)
#endif

#define GC2145_INIT(n)										\
	static struct gc2145_data gc2145_data_##n;						\
	static const struct gc2145_config gc2145_cfg_##n = {					\
		.i2c = I2C_DT_SPEC_INST_GET(n),							\
		GC2145_GET_PWDN_GPIO(n)								\
		GC2145_GET_RESET_GPIO(n)							\
		.bus_type = DT_PROP_OR(DT_INST_ENDPOINT_BY_ID(n, 0, 0), bus_type,		\
				       VIDEO_BUS_TYPE_PARALLEL),				\
	};											\
												\
	DEVICE_DT_INST_DEFINE(n, &gc2145_init, NULL, &gc2145_data_##n, &gc2145_cfg_##n,		\
			      POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY, &gc2145_driver_api);	\
												\
	VIDEO_DEVICE_DEFINE(gc2145_##n, DEVICE_DT_INST_GET(n), NULL);

DT_INST_FOREACH_STATUS_OKAY(GC2145_INIT)
