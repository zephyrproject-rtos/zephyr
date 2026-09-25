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

/* SET_PIXEL_FORMAT (3Ah) parameter values. */
#define ILI98XX_DSI_COLMOD_RGB565 0x50
#define ILI98XX_DSI_COLMOD_RGB888 0x70

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

#endif /* ZEPHYR_DRIVERS_DISPLAY_DISPLAY_ILI98XX_DSI_H_ */
