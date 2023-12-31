/*
 * Copyright (c) 2024 Martin Kiepfer <mrmarteng@teleschirm.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <lvgl.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <lvgl_input_device.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app);

static lv_obj_t *canvas = NULL;

#ifdef CONFIG_GPIO
static struct gpio_dt_spec button_gpio = GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw0), gpios, {0});
static struct gpio_callback button_callback;

static void button_isr_callback(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	if (canvas != NULL) {
		lv_canvas_fill_bg(canvas, lv_color_black(), LV_OPA_TRANSP);
	}
}
#endif /* CONFIG_GPIO */

static void lv_draw_pressing_callback(lv_event_t *e)
{
	lv_indev_t *indev = lv_indev_get_act();
	lv_point_t point;
	lv_draw_rect_dsc_t dsc;
	static lv_obj_t *canvas = e->user_data;

	lv_indev_get_point(indev, &point);

	lv_draw_rect_dsc_init(&dsc);
	dsc.bg_color = lv_palette_main(LV_PALETTE_GREY);
	dsc.border_width = 0U;
	dsc.outline_color = lv_palette_main(LV_PALETTE_GREY);
	dsc.outline_width = 3U;
	dsc.outline_pad = 0U;
	dsc.outline_opa = LV_OPA_COVER;
	dsc.radius = 5U;

	lv_canvas_draw_rect(canvas, point.x, point.y, 10U, 10U, &dsc);
}

int main(void)
{
	const struct device *display_dev;
	lv_obj_t *touch_label;

	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev)) {
		LOG_ERR("Device not ready, aborting test");
		return 0;
	}

	/**
	 * The required canvas buf size is calculated by
	 * (lv_img_color_format_get_px_size(cf) * width) / 8 * height)
	 * 2 bpp are used here to reduce overall memory requirements
	 **/
	lv_opa_t *mask_map = lv_mem_alloc(LV_VER_RES * LV_HOR_RES * sizeof(lv_opa_t) / 4U);

	if (mask_map == NULL) {
		LOG_ERR("Allocating canvas layer failed. Pleas review LVGL mem pool size.");
		return 0;
	}

	touch_label = lv_label_create(lv_scr_act());
	lv_label_set_text(touch_label, "touch me!");
	lv_obj_align(touch_label, LV_ALIGN_CENTER, 0, 0);

	/* Create canvas layer for "drawing" touch events */
	canvas = lv_canvas_create(lv_scr_act());
	lv_canvas_set_buffer(canvas, mask_map, LV_HOR_RES, LV_VER_RES, LV_IMG_CF_ALPHA_2BIT);
	lv_canvas_fill_bg(canvas, lv_color_black(), LV_OPA_TRANSP);
	lv_obj_center(canvas);
	lv_obj_add_event_cb(lv_scr_act(), lv_draw_pressing_callback, LV_EVENT_PRESSING, canvas);

#ifdef CONFIG_GPIO
	if (gpio_is_ready_dt(&button_gpio)) {
		int err;

		err = gpio_pin_configure_dt(&button_gpio, GPIO_INPUT);
		if (err) {
			LOG_ERR("failed to configure button gpio: %d", err);
			return 0;
		}

		gpio_init_callback(&button_callback, button_isr_callback, BIT(button_gpio.pin));

		err = gpio_add_callback(button_gpio.port, &button_callback);
		if (err) {
			LOG_ERR("failed to add button callback: %d", err);
			return 0;
		}

		err = gpio_pin_interrupt_configure_dt(&button_gpio, GPIO_INT_EDGE_TO_ACTIVE);
		if (err) {
			LOG_ERR("failed to enable button callback: %d", err);
			return 0;
		}
	}
#endif /* CONFIG_GPIO */

	lv_task_handler();
	display_blanking_off(display_dev);

	while (true) {
		lv_task_handler();
		k_sleep(K_MSEC(10));
	}
}
