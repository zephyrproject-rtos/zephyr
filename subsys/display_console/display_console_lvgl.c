/*
 * SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Renderer drawing the text grid with the LVGL text renderer. The rows are
 * drawn from the draw event of the active screen, so they need no LVGL
 * object and no LVGL memory beyond the draw tasks of one refresh.
 */

#include <string.h>

#include <lvgl.h>
#include <lvgl_zephyr.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/display/display_console.h>
#include <zephyr/drivers/display.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include "display_console_renderer.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(display_console, CONFIG_DISPLAY_CONSOLE_LOG_LEVEL);

#ifdef CONFIG_LV_Z_AUTO_INIT
BUILD_ASSERT(CONFIG_DISPLAY_CONSOLE_LVGL_INIT_PRIORITY > CONFIG_LV_Z_INIT_PRIORITY,
	     "The screen is set up after LVGL is initialized");
#endif

struct display_console_lvgl {
	/* Rows in display order, read by the draw event under the LVGL lock. */
	char text[CONFIG_DISPLAY_CONSOLE_MAX_ROWS][CONFIG_DISPLAY_CONSOLE_MAX_COLS];
	uint8_t attr[CONFIG_DISPLAY_CONSOLE_MAX_ROWS][CONFIG_DISPLAY_CONSOLE_MAX_COLS];
	lv_obj_t *scr;
	const lv_font_t *font;
	int32_t cell_w;
	int32_t cell_h;
	uint16_t rows;
	uint16_t cols;
	bool ready;
	bool shown;
};

static struct display_console_lvgl display_console_lvgl_inst;

static lv_color_t display_console_lvgl_color(const struct display_console_rgb *rgb)
{
	return lv_color_make(rgb->r, rgb->g, rgb->b);
}

static bool display_console_lvgl_blank(const char *text, uint16_t len)
{
	for (uint16_t i = 0; i < len; i++) {
		if (text[i] != ' ') {
			return false;
		}
	}

	return true;
}

/* Draw the rows crossing the area being refreshed, one label per colour run. */
static void display_console_lvgl_draw(lv_event_t *e)
{
	const struct display_console_lvgl *l = &display_console_lvgl_inst;
	lv_layer_t *layer = lv_event_get_layer(e);
	const lv_area_t *clip = &layer->_clip_area;
	lv_draw_label_dsc_t dsc;
	int32_t first;
	int32_t last;

	first = MAX(clip->y1, 0) / l->cell_h;
	last = MIN(clip->y2 / l->cell_h, (int32_t)l->rows - 1);

	lv_draw_label_dsc_init(&dsc);
	dsc.font = l->font;
	dsc.flag = LV_TEXT_FLAG_EXPAND;

	for (int32_t r = first; r <= last; r++) {
		uint16_t c0 = 0;

		while (c0 < l->cols) {
			const uint8_t a = l->attr[r][c0];
			uint16_t n = 1;
			lv_area_t area;

			while (c0 + n < l->cols && l->attr[r][c0 + n] == a) {
				n++;
			}

			if (!display_console_lvgl_blank(&l->text[r][c0], n)) {
				area.x1 = c0 * l->cell_w;
				area.y1 = r * l->cell_h;
				area.x2 = area.x1 + n * l->cell_w - 1;
				area.y2 = area.y1 + l->cell_h - 1;
				dsc.text = &l->text[r][c0];
				dsc.text_length = n;
				dsc.color = display_console_lvgl_color(&display_console_fg_rgb[a]);
				lv_draw_label(layer, &dsc, &area);
			}

			c0 += n;
		}
	}
}

/* Turn display blanking off once the first frame holding the console is on the panel. */
static void display_console_lvgl_refr_ready(lv_event_t *e)
{
	struct display_console_lvgl *l = &display_console_lvgl_inst;
	const struct device *dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	int err;

	ARG_UNUSED(e);

	if (l->shown) {
		return;
	}
	l->shown = true;

	err = display_blanking_off(dev);
	if (err != 0 && err != -ENOSYS) {
		LOG_ERR("turning display blanking off failed (%d)", err);
	}
}

static int display_console_lvgl_render(const struct display_console_snapshot *s)
{
	struct display_console_lvgl *l = &display_console_lvgl_inst;

	/* Until the screen is set up; the redraw requested then shows every row. */
	if (!l->ready) {
		return 0;
	}

	lvgl_lock();
	for (uint16_t r = 0; r < l->rows; r++) {
		const uint16_t phys = (s->top + r) % s->rows;
		lv_area_t area;

		if (!s->dirty[phys]) {
			continue;
		}

		memcpy(l->text[r], s->text[phys], l->cols);
		memcpy(l->attr[r], s->attr[phys], l->cols);

		area.x1 = 0;
		area.y1 = r * l->cell_h;
		area.x2 = l->cols * l->cell_w - 1;
		area.y2 = area.y1 + l->cell_h - 1;
		lv_obj_invalidate_area(l->scr, &area);
	}
	lvgl_unlock();

	return 0;
}

/* Runs before LVGL is initialized: only the font metrics and the display size are needed. */
static int display_console_lvgl_init(uint16_t *rows, uint16_t *cols)
{
	struct display_console_lvgl *l = &display_console_lvgl_inst;
	const struct device *dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	const lv_font_t *font = lv_font_get_default();
	struct display_capabilities caps;
	int32_t cw;
	int32_t ch;

	if (!device_is_ready(dev)) {
		LOG_ERR_DEVICE_NOT_READY(dev);
		return -ENODEV;
	}

	display_get_capabilities(dev, &caps);

	cw = lv_font_get_glyph_width(font, 'M', 'M');
	ch = lv_font_get_line_height(font);
	if (cw <= 0 || ch <= 0) {
		LOG_ERR("default LVGL font without glyph metrics");
		return -EINVAL;
	}

	/* Cells are as wide as 'M'. */
	for (uint32_t c = ' '; c <= '~'; c++) {
		if (lv_font_get_glyph_width(font, c, 0) != cw) {
			LOG_WRN("default LVGL font is not monospaced");
			break;
		}
	}

	l->font = font;
	l->cell_w = cw;
	l->cell_h = ch;
	*rows = caps.y_resolution / ch;
	*cols = caps.x_resolution / cw;

	return 0;
}

/* Runs after LVGL is initialized: hook the drawing into the active screen. */
static int display_console_lvgl_attach(void)
{
	struct display_console_lvgl *l = &display_console_lvgl_inst;
	const struct display_console_rgb *bg = &display_console_bg_rgb;
	lv_display_t *disp;
	uint16_t rows;
	uint16_t cols;
	int err;

	err = display_console_size(&rows, &cols);
	if (err != 0) {
		/* The console is not in use. */
		return 0;
	}

	disp = lv_display_get_default();
	if (disp == NULL) {
		LOG_ERR("LVGL has no display");
		return -ENODEV;
	}

	lvgl_lock();
	l->rows = rows;
	l->cols = cols;
	for (uint16_t r = 0; r < rows; r++) {
		memset(l->text[r], ' ', cols);
	}

	l->scr = lv_display_get_screen_active(disp);
	lv_obj_set_style_bg_color(l->scr, display_console_lvgl_color(bg), 0);
	lv_obj_set_style_bg_opa(l->scr, LV_OPA_COVER, 0);
	lv_obj_add_event_cb(l->scr, display_console_lvgl_draw, LV_EVENT_DRAW_MAIN_END, NULL);
	lv_display_add_event_cb(disp, display_console_lvgl_refr_ready, LV_EVENT_REFR_READY, NULL);
	lv_obj_invalidate(l->scr);
	l->ready = true;
	lvgl_unlock();

	display_console_redraw();

	return 0;
}

SYS_INIT(display_console_lvgl_attach, APPLICATION, CONFIG_DISPLAY_CONSOLE_LVGL_INIT_PRIORITY);

const struct display_console_renderer_api display_console_renderer = {
	.init = display_console_lvgl_init,
	.render = display_console_lvgl_render,
};
