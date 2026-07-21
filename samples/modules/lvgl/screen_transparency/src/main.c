/*
 * Copyright (c) 2024 Martin Stumpf <finomnis@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>

#include <lvgl.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app, CONFIG_LOG_DEFAULT_LEVEL);

#ifdef CONFIG_APP_DRAW_BACKGROUND_CHECKERBOARD

/* Number of checkerboard squares along the X axis. */
#define CHECKER_COLS 40

static void checkerboard_draw_cb(lv_event_t *e)
{
	lv_layer_t *layer = lv_event_get_layer(e);
	lv_display_t *disp = lv_display_get_default();
	int32_t hor = lv_display_get_horizontal_resolution(disp);
	int32_t ver = lv_display_get_vertical_resolution(disp);
	int32_t sq = LV_MAX(hor / CHECKER_COLS, 1);
	lv_draw_rect_dsc_t dsc;

	lv_draw_rect_dsc_init(&dsc);
	dsc.bg_opa = LV_OPA_COVER;
	dsc.bg_color = lv_color_black();

	/* The bottom layer already has an opaque white background, so only the
	 * black squares need to be drawn on top of it.
	 */
	for (int32_t y = 0; y < ver; y += sq) {
		for (int32_t x = 0; x < hor; x += sq) {
			if ((((x / sq) + (y / sq)) & 1) == 0) {
				continue;
			}

			lv_area_t coords = {
				.x1 = x,
				.y1 = y,
				.x2 = x + sq - 1,
				.y2 = y + sq - 1,
			};

			lv_draw_rect(layer, &dsc, &coords);
		}
	}
}

#endif /* CONFIG_APP_DRAW_BACKGROUND_CHECKERBOARD */

/* Pick a font that scales with the display: larger screens get a larger font so
 * the labels stay legible. Falls back to the default font when the larger sizes
 * are not built in.
 */
static const lv_font_t *ui_font(int32_t screen_dim)
{
#if LV_FONT_MONTSERRAT_48
	if (screen_dim >= 480) {
		return &lv_font_montserrat_48;
	}
#endif
#if LV_FONT_MONTSERRAT_28
	if (screen_dim >= 240) {
		return &lv_font_montserrat_28;
	}
#endif
	return LV_FONT_DEFAULT;
}

/* Create a centered label that stays readable over the high-contrast
 * checkerboard: colored text on a semi-transparent dark rounded background (the
 * checkerboard still shows through the background).
 */
static void add_label(const char *text, lv_color_t color, const lv_font_t *font,
		      int32_t x_ofs, int32_t y_ofs)
{
	int32_t lh = lv_font_get_line_height(font);
	lv_obj_t *label = lv_label_create(lv_screen_active());

	lv_label_set_text(label, text);
	lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
	lv_obj_set_style_text_color(label, color, LV_PART_MAIN);

	lv_obj_set_style_bg_opa(label, LV_OPA_50, LV_PART_MAIN);
	lv_obj_set_style_bg_color(label, lv_color_black(), LV_PART_MAIN);
	lv_obj_set_style_pad_all(label, lh / 4, LV_PART_MAIN);
	lv_obj_set_style_radius(label, lh / 4, LV_PART_MAIN);

	lv_obj_align(label, LV_ALIGN_CENTER, x_ofs, y_ofs);
}

static void initialize_gui(void)
{
	lv_display_t *disp = lv_display_get_default();
	int32_t screen_dim = LV_MIN(lv_display_get_horizontal_resolution(disp),
				    lv_display_get_vertical_resolution(disp));
	const lv_font_t *font = ui_font(screen_dim);

	/* Scale the label offsets with the chosen font's line height (relative to
	 * the default font), so the layout grows with the font and does not
	 * overlap on larger screens.
	 */
	int32_t lh = lv_font_get_line_height(font);
	int32_t base_lh = lv_font_get_line_height(LV_FONT_DEFAULT);

	/* Configure the screen for transparency */
	lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_TRANSP, LV_PART_MAIN);

#ifdef CONFIG_APP_DRAW_BACKGROUND_CHECKERBOARD
	/* This board has no separate hardware layer behind the LVGL framebuffer,
	 * so paint an opaque black/white checkerboard on the bottom layer to give
	 * the transparent screen a defined background to composite over.
	 */
	lv_obj_set_style_bg_opa(lv_layer_bottom(), LV_OPA_COVER, LV_PART_MAIN);
	lv_obj_set_style_bg_color(lv_layer_bottom(), lv_color_white(), LV_PART_MAIN);
	lv_obj_add_event_cb(lv_layer_bottom(), checkerboard_draw_cb, LV_EVENT_DRAW_MAIN, NULL);
	lv_obj_invalidate(lv_layer_bottom());
#else
	/* Default: keep the bottom layer transparent, so a separate hardware
	 * layer (e.g. a video plane for an OSD overlay) shows through.
	 */
	lv_obj_set_style_bg_opa(lv_layer_bottom(), LV_OPA_TRANSP, LV_PART_MAIN);
#endif

	/* Create the labels, scaling their offsets with the font line height so
	 * they stay laid out on larger screens.
	 */
	add_label("Hello, world!", lv_color_hex(0xff00ff), font, 0, -20 * lh / base_lh);
	add_label("RED", lv_color_hex(0xff0000), font, -70 * lh / base_lh, 20 * lh / base_lh);
	add_label("GREEN", lv_color_hex(0x00ff00), font, 0, 20 * lh / base_lh);
	add_label("BLUE", lv_color_hex(0x0000ff), font, 70 * lh / base_lh, 20 * lh / base_lh);
}

int main(void)
{
	const struct device *display_dev;
	int ret;

	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev)) {
		LOG_ERR("Device not ready, aborting test");
		return -ENODEV;
	}

	initialize_gui();

	lv_timer_handler();
	ret = display_blanking_off(display_dev);
	if (ret < 0 && ret != -ENOSYS) {
		LOG_ERR("Failed to turn blanking off (error %d)", ret);
		return 0;
	}

	while (1) {
		uint32_t sleep_ms = lv_timer_handler();

		k_msleep(MIN(sleep_ms, INT32_MAX));
	}

	return 0;
}
