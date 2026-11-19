/*
 * Copyright (C) 2024 Alif Semiconductor.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _DISPLAY_CDC200_H_
#define _DISPLAY_CDC200_H_

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>

/* Global Registers */
#define CDC_HW_VER          0x00 /* HW Version Register */
#define CDC_LCNT            0x04 /* Layer Count Register */
#define CDC_SYNC_SIZE_CFG   0x08 /* Sync Size Register */
#define CDC_BP_CFG          0x0C /* Back Porch Register */
#define CDC_ACTW_CFG        0x10 /* Active Width Register */
#define CDC_TOTALW_CFG      0x14 /* Total Width Register */
#define CDC_GLB_CTRL        0x18 /* Global Control Register */
#define CDC_CFG1            0x1C /* Global Config 1 Register */
#define CDC_CFG2            0x20 /* Global Config 2 Register */
#define CDC_SRCTRL          0x24 /* Shadow Reload Control Register */
#define CDC_GAMMA_CORR      0x28 /* Gamma Correction Register */
#define CDC_BACKGND_COLOR   0x2C /* Background Color Register */
#define CDC_IRQ_MASK0       0x34 /* IRQ Enable Register */
#define CDC_IRQ_STATUS0     0x38 /* IRQ Status Register */
#define CDC_IRQ_CLEAR0      0x3C /* IRQ Clear Register */
#define CDC_LINE_IRQ_POS    0x40 /* Line IRQ Pos-Ctrl Register */
#define CDC_POS_STAT        0x44 /* Position Status Register */
#define CDC_SYNC_BLANK_STAT 0x48 /* Sync/Blank Status Register */
#define CDC_IRQ_MASK1       0x64 /* Secondary IRQ Enable Register */
#define CDC_IRQ_STATUS1     0x68 /* Secondary IRQ Status Register */
#define CDC_IRQ_CLEAR1      0x6C /* Secondary IRQ Clear Register */
#define CDC_SLINE_IRQ_POS   0x70 /* Secure Line IRQ Pos-Ctrl Register */

/* Layer 1 Register */
#define CDC_L1_CFG1        0x100 /* Configuration 1 Register */
#define CDC_L1_CFG2        0x104 /* Configuration 2 Register */
#define CDC_L1_REL_CTRL    0x108 /* Shadow Reload Control Register */
#define CDC_L1_CTRL        0x10C /* Control Register */
#define CDC_L1_WIN_HPOS    0x110 /* Window H-Position Register */
#define CDC_L1_WIN_VPOS    0x114 /* Window V-Position Register */
#define CDC_L1_CKEY        0x118 /* Color Key Register */
#define CDC_L1_PIX_FORMAT  0x11C /* Pixel Format Register */
#define CDC_L1_CONST_ALPHA 0x120 /* Constant Alpha Register */
#define CDC_L1_DFLT_COLOR  0x124 /* Default Color Register */
#define CDC_L1_BLEND_CFG   0x128 /* Blending Factors Register */
#define CDC_L1_FB_BCTRL    0x12C /* FB Bus Control Register */
#define CDC_L1_CFB_ADDR    0x134 /* Color FB Address Register */
#define CDC_L1_CFB_LENGTH  0x138 /* Color FB Length Register */
#define CDC_L1_CFB_LINES   0x13C /* Color FB Lines Register */
#define CDC_L1_CLUT_WRACC  0x150 /* CLUT Write Access Register */

/* Layer 2 Register */
#define CDC_L2_CFG1        0x200 /* Configuration 1 Register */
#define CDC_L2_CFG2        0x204 /* Configuration 2 Register */
#define CDC_L2_REL_CTRL    0x208 /* Shadow Reload Control Register */
#define CDC_L2_CTRL        0x20C /* Control Register */
#define CDC_L2_WIN_HPOS    0x210 /* Window H-Position Register */
#define CDC_L2_WIN_VPOS    0x214 /* Window V-Position Register */
#define CDC_L2_CKEY        0x218 /* Color Key Register */
#define CDC_L2_PIX_FORMAT  0x21C /* Pixel Format Register */
#define CDC_L2_CONST_ALPHA 0x220 /* Constant Alpha Register */
#define CDC_L2_DFLT_COLOR  0x224 /* Default Color Register */
#define CDC_L2_BLEND_CFG   0x228 /* Blending Factors Register */
#define CDC_L2_FB_BCTRL    0x22C /* FB Bus Control Register */
#define CDC_L2_CFB_ADDR    0x234 /* Color FB Address Register */
#define CDC_L2_CFB_LENGTH  0x238 /* Color FB Length Register */
#define CDC_L2_CFB_LINES   0x23C /* Color FB Lines Register */
#define CDC_L2_CLUT_WRACC  0x250 /* CLUT Write Access Register */

/* Registers Bit-fields */

/*
 * Global Register Bit-fields
 */
/* HW Version Register */
#define CDC_HW_VER_MAJVER_MASK  GENMASK(7, 0)
#define CDC_HW_VER_MAJVER_SHIFT 16
#define CDC_HW_VER_MINVER_MASK  GENMASK(7, 0)
#define CDC_HW_VER_MINVER_SHIFT 8
#define CDC_HW_VER_REV_MASK     GENMASK(7, 0)
#define CDC_HW_VER_REV_SHIFT    0

/* Sync Size Register */
#define CDC_SYNC_SIZE_CFG_HSW_MASK  GENMASK(15, 0)
#define CDC_SYNC_SIZE_CFG_HSW_SHIFT 16
#define CDC_SYNC_SIZE_CFG_VSW_MASK  GENMASK(15, 0)
#define CDC_SYNC_SIZE_CFG_VSW_SHIFT 0

/* Back-Porch Register */
#define CDC_BP_CFG_AHBP_MASK  GENMASK(15, 0)
#define CDC_BP_CFG_AHBP_SHIFT 16
#define CDC_BP_CFG_AVBP_MASK  GENMASK(15, 0)
#define CDC_BP_CFG_AVBP_SHIFT 0

/* Active-width Register */
#define CDC_ACTW_CFG_AAW_MASK  GENMASK(15, 0)
#define CDC_ACTW_CFG_AAW_SHIFT 16
#define CDC_ACTW_CFG_AAH_MASK  GENMASK(15, 0)
#define CDC_ACTW_CFG_AAH_SHIFT 0

/* Total Width Register */
#define CDC_TOTALW_CFG_TOTALW_MASK  GENMASK(15, 0)
#define CDC_TOTALW_CFG_TOTALW_SHIFT 16
#define CDC_TOTALW_CFG_TOTALH_MASK  GENMASK(15, 0)
#define CDC_TOTALW_CFG_TOTALH_SHIFT 0

/* Global Control Register */
#define CDC_GLB_CTRL_HSPOL     BIT(31)
#define CDC_GLB_CTRL_VSPOL     BIT(30)
#define CDC_GLB_CTRL_BLPOL     BIT(29)
#define CDC_GLB_CTRL_PCLKPOL   BIT(28)
#define CDC_GLB_CTRL_DITHER_EN BIT(16)
#define CDC_GLB_CTRL_GAMMA_EN  BIT(1)
#define CDC_GLB_CTRL_CDC_EN    BIT(0)

/* Shadow Reload Control Register */
#define CDC_SRCTRL_VBR BIT(1)
#define CDC_SRCTRL_IMR BIT(0)

/* Gamma Correction Register */
#define CDC_GAMMA_CORR_R_CLUT          BIT(18)
#define CDC_GAMMA_CORR_G_CLUT          BIT(17)
#define CDC_GAMMA_CORR_B_CLUT          BIT(16)
#define CDC_GAMMA_CORR_RGB_VAL_MASK    GENMASK(7, 0)
#define CDC_GAMMA_CORR_RGB_VAL_SHIFT   8
#define CDC_GAMMA_CORR_CLUT_ADDR_MASK  GENMASK(7, 0)
#define CDC_GAMMA_CORR_CLUT_ADDR_SHIFT 0

/* IRQ Bit-fields */
#define CDC_IRQ_REG_RELOAD    BIT(3)
#define CDC_IRQ_BUS_ERROR0    BIT(2)
#define CDC_IRQ_FIFO_UNDERRUN BIT(1)
#define CDC_IRQ_LINE          BIT(0)

/* Line IRQ Position */
#define CDC_LINE_IRQ_POS_MASK  GENMASK(15, 0)
#define CDC_LINE_IRQ_POS_SHIFT 0

/* Position Status Register */
#define CDC_POS_STAT_CXPOS_MASK  GENMASK(15, 0)
#define CDC_POS_STAT_CXPOS_SHIFT 16
#define CDC_POS_STAT_CYPOS_MASK  GENMASK(15, 0)
#define CDC_POS_STAT_CYPOS_SHIFT 0

/*
 * Layer Registers Bit-fields
 */
/* Layer Reload control register */
#define CDC_LN_REL_CTRL_SH_MASK   BIT(2)
#define CDC_LN_REL_CTRL_SH_VBLANK BIT(1)
#define CDC_LN_REL_CTRL_SH_IM     BIT(0)

/* Layer Control Register */
#define CDC_LN_CTRL_DFLT_CBLEND_EN BIT(9)
#define CDC_LN_CTRL_CLUT_EN        BIT(4)
#define CDC_LN_CTRL_LAYER_EN       BIT(0)

/* Horizontal Position Register */
#define CDC_LN_WIN_HPOS_STOP_POS_MASK   GENMASK(15, 0)
#define CDC_LN_WIN_HPOS_STOP_POS_SHIFT  16
#define CDC_LN_WIN_HPOS_START_POS_MASK  GENMASK(15, 0)
#define CDC_LN_WIN_HPOS_START_POS_SHIFT 0

/* Vertical Position Register */
#define CDC_LN_WIN_VPOS_STOP_POS_MASK   GENMASK(15, 0)
#define CDC_LN_WIN_VPOS_STOP_POS_SHIFT  16
#define CDC_LN_WIN_VPOS_START_POS_MASK  GENMASK(15, 0)
#define CDC_LN_WIN_VPOS_START_POS_SHIFT 0

/* Layer Pixel Format Register */
#define CDC_LN_PIX_FORMAT_PIXF_MASK  GENMASK(2, 0)
#define CDC_LN_PIX_FORMAT_PIXF_SHIFT 0

/* Layer Constant Alpha Register */
#define CDC_LN_CONST_ALPHA_CONSTA_MASK  GENMASK(7, 0)
#define CDC_LN_CONST_ALPHA_CONSTA_SHIFT 0

/* Layer Blend Config */
#define CDC_LN_BLEND_CFG_F1_SEL_MASK  GENMASK(2, 0)
#define CDC_LN_BLEND_CFG_F1_SEL_SHIFT 8
#define CDC_LN_BLEND_CFG_F2_SEL_MASK  GENMASK(2, 0)
#define CDC_LN_BLEND_CFG_F2_SEL_SHIFT 0

/* Layer FB Bus control Register */
#define CDC_LN_FB_BCTRL_WNUM_MASK  GENMASK(7, 0)
#define CDC_LN_FB_BCTRL_WNUM_SHIFT 0

/* Layer FB Length Register */
#define CDC_LN_CFB_LENGTH_PITCH_MASK    GENMASK(15, 0)
#define CDC_LN_CFB_LENGTH_PITCH_SHIFT   16
#define CDC_LN_CFB_LENGTH_LLENGTH_MASK  GENMASK(15, 0)
#define CDC_LN_CFB_LENGTH_LLENGTH_SHIFT 0

/* Layer Color FB lines Register */
#define CDC_LN_CFB_LINES_LNUM_MASK  GENMASK(15, 0)
#define CDC_LN_CFB_LINES_LNUM_SHIFT 0

/* Layer Color Look-up Table Register */
#define CDC_LN_CLUT_WRACC_ADDR_MASK     GENMASK(7, 0)
#define CDC_LN_CLUT_WRACC_ADDR_SHIFT    24
#define CDC_LN_CLUT_WRACC_RGB_VAL_MASK  GENMASK(23, 0)
#define CDC_LN_CLUT_WRACC_RGB_VAL_SHIFT 0

/*
 * CDC configurations
 */
#define BUS_WIDTH (7)

/*
 * CDC Supported Pixel format.
 */
enum cdc_pixel_format {
	CDC_PIXEL_FORMAT_ARGB8888,  /**< 32-bit ARGB */
	CDC_PIXEL_FORMAT_RGB888,    /**< 24-bit RGB */
	CDC_PIXEL_FORMAT_RGB565,    /**< 16-bit RGB */
	CDC_PIXEL_FORMAT_RGBA8888,  /**< 32-bit RGBA */
	CDC_PIXEL_FORMAT_AL44,      /**< 8-bit alpha + luminance */
	CDC_PIXEL_FORMAT_AL8,       /**< 8-bit single channel */
	CDC_PIXEL_FORMAT_ARGB1555,  /**< 16-bit ARGB with 1 bit alpha */
	CDC_PIXEL_FORMAT_ARGB4444,  /**< 16-bit ARGB with 4 bits alpha */
	CDC_PIXEL_FORMAT_UNDEFINED, /**< Undefined */
};

/*
 * CDC layers supported
 */
enum cdc_layer {
	CDC_LAYER_1, /**< cdc layer 1 */
	CDC_LAYER_2, /**< cdc layer 2 */
	CDC_LAYER_MAX,
};

enum cdc_blend_factor {
	CDC_BLEND_CONST_ALPHA = 4,
	CDC_BLEND_CONST_ALPHA_INV = 5,
	CDC_BLEND_PIXEL_ALPHA_X_CONST_ALPHA = 6,
	CDC_BLEND_PIXEL_ALPHA_X_CONST_ALPHA_INV = 7,
};

/*
 * CDC panel configuration. Contains:
 *	1. Polarity of signals
 *	2. Video Timing information.
 */
struct cdc200_panel_config {
	uint32_t polarity;

	uint32_t hsync_len;
	uint32_t vsync_len;

	uint32_t hbp;
	uint32_t vbp;

	uint32_t hfp;
	uint32_t vfp;

	uint32_t active_width;
	uint32_t active_height;
};

/*
 * CDC per layer configuration data. Includes:
 *	1. FB information.
 *	2. Default layer color.
 *	3. Layer Format.
 *	4. Blending configuration.
 *	5. Bytes per pixel for this layer.
 *	6. Enablement status of different features
 *	7. Windowing parameters.
 */
struct cdc200_layer_config {
	uint8_t *fb;
	size_t fb_size;

	uint32_t def_color;
	enum cdc_pixel_format fmt;
	size_t pixel_size;

	uint32_t curr_layer_bf_f1: 3;
	uint32_t subj_layer_bf_f2: 3;
	uint32_t def_color_en: 1;
	uint32_t layer_en: 1;
	uint8_t const_alpha;

	uint16_t x0;
	uint16_t x1;
	uint16_t y0;
	uint16_t y1;
};

/*
 * Device config structure. Includes:
 *	1. Device MMIO information
 *	2. Pinctrl-config
 *	3. Global device config
 *	4. Panel config
 *	5. Layer config
 */
struct cdc200_config {
	DEVICE_MMIO_ROM;
	void (*irq_config_func)(const struct device *dev);

	uint32_t irq;
	const struct pinctrl_dev_config *pcfg;

	/* Global Background Layer Color*/
	uint32_t bg_color;

	struct cdc200_panel_config panel_cfg;
	struct cdc200_layer_config layer[CDC_LAYER_MAX];
	const struct device *panel_dev;

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(clocks)
	const struct device *clk_dev;
	clock_control_subsys_t pix_cid;
	clock_control_subsys_t dpi_cid;
#endif /* DT_ANY_INST_HAS_PROP_STATUS_OKAY(clocks) */
};

/*
 * Device data structure. Includes:
 *	1. Device MMIO range incase of MMU config.
 *	2. Layer count.
 */
struct cdc200_data {
	DEVICE_MMIO_RAM;

	const struct device *dev;
	uint8_t layer_count;

	uint32_t clk_freq;

	uint8_t *curr_fb[CDC_LAYER_MAX];
	uint8_t *next_fb[CDC_LAYER_MAX];

	struct k_sem vsync_sem;
};

#endif /* _DISPLAY_CDC200_H_ */
