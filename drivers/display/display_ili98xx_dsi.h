/*
 * SPDX-FileCopyrightText: 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_DRIVERS_DISPLAY_DISPLAY_ILI98XX_DSI_H_
#define ZEPHYR_DRIVERS_DISPLAY_DISPLAY_ILI98XX_DSI_H_

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mipi_dsi.h>

#include "display_ili9xxx.h"

/* Panel video timings are taken from the LCD controller the panel hangs off. */
BUILD_ASSERT(DT_HAS_COMPAT_STATUS_OKAY(zephyr_panel_timing),
	     "No device tree node with compatible \"zephyr,panel-timing\" found");
#define LCD_TIMINGS_NODE DT_INST(0, zephyr_panel_timing)

#define LCD_HTIMMING_SYNC        DT_PROP(LCD_TIMINGS_NODE, hsync_len)
#define LCD_HTIMMING_BACK_PORCH  DT_PROP(LCD_TIMINGS_NODE, hback_porch)
#define LCD_HTIMMING_FRONT_PORCH DT_PROP(LCD_TIMINGS_NODE, hfront_porch)

#define LCD_VTIMMING_SYNC        DT_PROP(LCD_TIMINGS_NODE, vsync_len)
#define LCD_VTIMMING_BACK_PORCH  DT_PROP(LCD_TIMINGS_NODE, vback_porch)
#define LCD_VTIMMING_FRONT_PORCH DT_PROP(LCD_TIMINGS_NODE, vfront_porch)

#define ILI98XX_DSI_HSYNC LCD_HTIMMING_SYNC
#define ILI98XX_DSI_HBP   LCD_HTIMMING_BACK_PORCH - LCD_HTIMMING_SYNC
#define ILI98XX_DSI_HFP   LCD_HTIMMING_FRONT_PORCH

#define ILI98XX_DSI_VSYNC LCD_VTIMMING_SYNC
#define ILI98XX_DSI_VBP   LCD_VTIMMING_BACK_PORCH - LCD_VTIMMING_SYNC
#define ILI98XX_DSI_VFP   LCD_VTIMMING_FRONT_PORCH

/*
 * Register definitions shared by the ILI9806E and ILI9881C. The active command
 * page is selected by writing ENEXTC; its parameter list is controller specific.
 */

/* Page 0 commands */
#define ILI98XX_DSI_ENEXTC 0xFF /* EXTC command set enable register */

/* Page 0: MADCTL (36h) fields not defined in display_ili9xxx.h */
#define ILI98XX_DSI_MADCTL_GS BIT(0) /* Gate driver scan direction (vertical flip) */
#define ILI98XX_DSI_MADCTL_SS BIT(1) /* Source driver scan direction (horizontal flip) */

/* Page 1 commands */
#define ILI98XX_DSI_RDID4_1      0x00 /* Read ID4[23:16] */
#define ILI98XX_DSI_RDID4_2      0x01 /* Read ID4[15:8] */
#define ILI98XX_DSI_RDID4_3      0x02 /* Read ID4[7:0] */
#define ILI98XX_DSI_BLKPRH_VFP   0x25 /* Blanking Porch: VFP */
#define ILI98XX_DSI_BLKPRH_VBP   0x26 /* Blanking Porch: VBP */
#define ILI98XX_DSI_INVTR        0x31 /* Display Inversion Control */
#define ILI98XX_DSI_DITHE        0x34 /* Dithering Enable */
#define ILI98XX_DSI_VREG1        0x50 /* VREG1OUT voltage (ILI9806E: PWCTRL 9) */
#define ILI98XX_DSI_VREG2        0x51 /* VREG2OUT voltage (ILI9806E: PWCTRL 10) */
#define ILI98XX_DSI_VCM1_MSB     0x52 /* VCM1[8] (ILI9806E: VMCTRL1) */
#define ILI98XX_DSI_VCM1_LSB     0x53 /* VCM1[7:0] (ILI9806E: VMCTRL2) */
#define ILI98XX_DSI_VCM2_MSB     0x54 /* VCM2[8] (ILI9806E: VMCTRL3) */
#define ILI98XX_DSI_VCM2_LSB     0x55 /* VCM2[7:0] (ILI9806E: VMCTRL4) */
#define ILI98XX_DSI_VCM_NVM      0x56 /* VCOM from NV memory select (ILI9806E: VMCTRL5) */
#define ILI98XX_DSI_ETMOD        0x58 /* Entry Mode Set */
#define ILI98XX_DSI_SOURCE_TIMING_ADJUST_1 0x60 /* Source Timing Adjust 1 (SDT) */
#define ILI98XX_DSI_SOURCE_TIMING_ADJUST_2 0x61 /* Source Timing Adjust 2 (CRT) */
#define ILI98XX_DSI_SOURCE_TIMING_ADJUST_3 0x62 /* Source Timing Adjust 3 (EQT) */
#define ILI98XX_DSI_SOURCE_TIMING_ADJUST_4 0x63 /* Source Timing Adjust 4 (PCT) */

/*
 * Page 1: Positive Gamma Control 1~16. The gray level each entry adjusts is
 * controller specific; the ILI9881C has four more entries at B0h~B3h.
 */
#define ILI98XX_DSI_PGAMCTRL1  0xA0
#define ILI98XX_DSI_PGAMCTRL2  0xA1
#define ILI98XX_DSI_PGAMCTRL3  0xA2
#define ILI98XX_DSI_PGAMCTRL4  0xA3
#define ILI98XX_DSI_PGAMCTRL5  0xA4
#define ILI98XX_DSI_PGAMCTRL6  0xA5
#define ILI98XX_DSI_PGAMCTRL7  0xA6
#define ILI98XX_DSI_PGAMCTRL8  0xA7
#define ILI98XX_DSI_PGAMCTRL9  0xA8
#define ILI98XX_DSI_PGAMCTRL10 0xA9
#define ILI98XX_DSI_PGAMCTRL11 0xAA
#define ILI98XX_DSI_PGAMCTRL12 0xAB
#define ILI98XX_DSI_PGAMCTRL13 0xAC
#define ILI98XX_DSI_PGAMCTRL14 0xAD
#define ILI98XX_DSI_PGAMCTRL15 0xAE
#define ILI98XX_DSI_PGAMCTRL16 0xAF

/*
 * Page 1: Negative Gamma Correction 1~16. The gray level each entry adjusts is
 * controller specific; the ILI9881C has four more entries at D0h~D3h.
 */
#define ILI98XX_DSI_NGAMCTRL1  0xC0
#define ILI98XX_DSI_NGAMCTRL2  0xC1
#define ILI98XX_DSI_NGAMCTRL3  0xC2
#define ILI98XX_DSI_NGAMCTRL4  0xC3
#define ILI98XX_DSI_NGAMCTRL5  0xC4
#define ILI98XX_DSI_NGAMCTRL6  0xC5
#define ILI98XX_DSI_NGAMCTRL7  0xC6
#define ILI98XX_DSI_NGAMCTRL8  0xC7
#define ILI98XX_DSI_NGAMCTRL9  0xC8
#define ILI98XX_DSI_NGAMCTRL10 0xC9
#define ILI98XX_DSI_NGAMCTRL11 0xCA
#define ILI98XX_DSI_NGAMCTRL12 0xCB
#define ILI98XX_DSI_NGAMCTRL13 0xCC
#define ILI98XX_DSI_NGAMCTRL14 0xCD
#define ILI98XX_DSI_NGAMCTRL15 0xCE
#define ILI98XX_DSI_NGAMCTRL16 0xCF

/* Page 1: NV memory */
#define ILI98XX_DSI_NVMWR1   0xE0 /* NV Memory Write 1: PGM_DATA[7:0] */
#define ILI98XX_DSI_NVMWR2   0xE1 /* NV Memory Write 2: PGM_ADR[7:0] */
#define ILI98XX_DSI_NVMPKEY1 0xE3 /* NV Memory Protection Key 1: KEY[23:16] */
#define ILI98XX_DSI_NVMPKEY2 0xE4 /* NV Memory Protection Key 2: KEY[15:8] */
#define ILI98XX_DSI_NVMPKEY3 0xE5 /* NV Memory Protection Key 3: KEY[7:0] */
#define ILI98XX_DSI_RDNVM1   0xE6 /* NV Memory Status Read 1: ID1/ID2 marks */
#define ILI98XX_DSI_RDNVM2   0xE7 /* NV Memory Status Read 2: ID3 mark */
#define ILI98XX_DSI_RDNVM3   0xE8 /* NV Memory Status Read 3: VCOM/gamma marks */
#define ILI98XX_DSI_RDNVM4   0xE9 /* NV Memory Status Read 4: OTP busy */

/* SET_PIXEL_FORMAT (3Ah) parameter values. */
#define ILI98XX_DSI_COLMOD_RGB565 0x50
#define ILI98XX_DSI_COLMOD_RGB888 0x70

/** Longest parameter list used by any of the supported panels. */
#define ILI98XX_DSI_MAX_CMD_LEN 5U

/** One entry of a panel power-on command sequence. */
struct ili98xx_dsi_cmd {
	uint8_t reg;
	uint8_t cmd_len;
	uint8_t cmd[ILI98XX_DSI_MAX_CMD_LEN];
} __packed;

struct ili98xx_dsi_config {
	const struct device *mipi_dsi;
	const struct gpio_dt_spec reset;
	const struct gpio_dt_spec backlight;
	enum display_pixel_format pixel_format;
	uint8_t data_lanes;
	uint16_t width;
	uint16_t height;
	uint8_t channel;
	uint32_t mode_flags;
	int (*regs_init_fn)(const struct device *dev);
};

/**
 * @brief Write a panel command sequence.
 *
 * @param dev ILI9XXX DSI device instance
 * @param cmds Command sequence
 * @param nr_cmds Number of entries in @p cmds
 * @return 0 on success, errno otherwise.
 */
int ili98xx_dsi_write_sequence(const struct device *dev, const struct ili98xx_dsi_cmd *cmds,
			       size_t nr_cmds);


/**
 * @brief Initialize an ILI98XX DSI device.
 *
 * @param dev ILI9XXX DSI device instance
 * @return 0 on success, errno otherwise.
 */
int ili98xx_dsi_init(const struct device *dev);

/* API functions implemented by commont driver */
int ili98xx_dsi_blanking_on(const struct device *dev);
int ili98xx_dsi_blanking_off(const struct device *dev);
int ili98xx_dsi_set_pixel_format(const struct device *dev, enum display_pixel_format format);
void ili98xx_dsi_get_capabilities(const struct device *dev,
				  struct display_capabilities *capabilities);

#define ILI98XX_DSI_API_DEFINE(name) \
	DEVICE_API(display, name) = { \
		.blanking_on = ili98xx_dsi_blanking_on, \
		.blanking_off = ili98xx_dsi_blanking_off, \
		.set_pixel_format = ili98xx_dsi_set_pixel_format, \
		.get_capabilities = ili98xx_dsi_get_capabilities, \
	};

#define _ILI98XX_DSI_INIT(node, init_fn, api_name)                                                 \
	static const struct ili98xx_dsi_config ili##node##_dsi_config = {                          \
		.mipi_dsi = DEVICE_DT_GET(DT_PARENT(node)),                                        \
		.reset = GPIO_DT_SPEC_GET_OR(node, reset_gpios, {0}),                              \
		.backlight = GPIO_DT_SPEC_GET_OR(node, bl_gpios, {0}),                             \
		.data_lanes = DT_PROP_BY_IDX(node, data_lanes, 0),                                 \
		.width = DT_PROP(node, width),                                                     \
		.height = DT_PROP(node, height),                                                   \
		.channel = DT_REG_ADDR(node),                                                      \
		.pixel_format = DT_PROP(node, pixel_format),                                       \
		.mode_flags = MIPI_DSI_MODE_VIDEO |                                                \
			COND_CODE_0(DT_PROP(node, hp_mode), (MIPI_DSI_MODE_LPM), (0)) |            \
			COND_CODE_1(DT_PROP(node, burst_mode), (MIPI_DSI_MODE_VIDEO_BURST), (0)),  \
		.regs_init_fn = init_fn,                                                           \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_DEFINE(node, ili98xx_dsi_init, NULL, NULL,                                       \
			 &ili##node##_dsi_config, POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY,   \
			 &api_name)

/**
 * @brief Instantiate an ILI98XX DSI device.
 *
 * @param node Device tree node identifier
 * @param init_fn Initialization function for the panel registers
 * @param api_name Name of the API structure to use for this device
 */
#define ILI98XX_DSI_INIT(node, init_fn, api_name) _ILI98XX_DSI_INIT(node, init_fn, api_name)


#endif /* ZEPHYR_DRIVERS_DISPLAY_DISPLAY_ILI98XX_DSI_H_ */
