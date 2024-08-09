/*
 * Copyright (c) 2024 Martin Kiepfer <mrmarteng@teleschirm.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <stdlib.h>
#include <lvgl.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <lvgl_input_device.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app);

struct pressing_callback_context {
	lv_obj_t *canvas;
	lv_indev_t *indev;
};

#ifdef CONFIG_GPIO
struct button_callback_context {
	struct gpio_callback callback;
	lv_obj_t *canvas;
};
static struct gpio_dt_spec button_gpio = GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw0), gpios, {0});

static void button_isr_callback(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	struct button_callback_context *ctx =
		CONTAINER_OF(cb, struct button_callback_context, callback);

	lv_canvas_fill_bg(ctx->canvas, lv_color_black(), LV_OPA_TRANSP);
}
#endif /* CONFIG_GPIO */

static void lv_draw_pressing_callback(lv_event_t *e)
{
	lv_point_t point;
	lv_draw_rect_dsc_t dsc;
	struct pressing_callback_context *ctx = e->user_data;

	lv_indev_get_point(ctx->indev, &point);

	lv_draw_rect_dsc_init(&dsc);
	dsc.bg_color = lv_palette_main(LV_PALETTE_GREY);
	dsc.border_width = 0U;
	dsc.outline_color = lv_palette_main(LV_PALETTE_GREY);
	dsc.outline_width = 3U;
	dsc.outline_pad = 0U;
	dsc.outline_opa = LV_OPA_COVER;
	dsc.radius = 5U;

	lv_canvas_draw_rect(ctx->canvas, point.x, point.y, 10U, 10U, &dsc);
}

int main(void)
{
	const struct device *display_dev;
	const struct device *pointer_dev;
	lv_obj_t *touch_label;
	lv_opa_t *mask_map;
	static struct pressing_callback_context pressing_cb_ctx;
#ifdef CONFIG_GPIO
	static struct button_callback_context btn_cb_ctx;

	if (gpio_is_ready_dt(&button_gpio)) {
		int err;

		err = gpio_pin_configure_dt(&button_gpio, GPIO_INPUT);
		if (err) {
			LOG_ERR("Failed to configure button gpio: %d", err);
			return -EIO;
		}

		gpio_init_callback(&btn_cb_ctx.callback, button_isr_callback, BIT(button_gpio.pin));
		btn_cb_ctx.canvas = pressing_cb_ctx.canvas;

		err = gpio_add_callback(button_gpio.port, &btn_cb_ctx.callback);
		if (err) {
			LOG_ERR("Failed to add button callback: %d", err);
			return -EIO;
		}

		err = gpio_pin_interrupt_configure_dt(&button_gpio, GPIO_INT_EDGE_TO_ACTIVE);
		if (err) {
			LOG_ERR("Failed to configure button callback: %d", err);
			return -EIO;
		}
	}
#endif

	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev)) {
		LOG_ERR("Device not ready, aborting test");
		return -ENODEV;
	}

	pointer_dev = DEVICE_DT_GET(DT_COMPAT_GET_ANY_STATUS_OKAY(zephyr_lvgl_pointer_input));
	if (!device_is_ready(pointer_dev)) {
		LOG_ERR("Device not ready, aborting test");
		return -ENODEV;
	}
	pressing_cb_ctx.indev = lvgl_input_get_indev(pointer_dev);

	/**
	 * The required canvas buf size is calculated by
	 * (lv_img_color_format_get_px_size(cf) * width) / 8 * height)
	 * 2 bpp are used here to reduce overall memory requirements
	 **/
	mask_map = (lv_opa_t *)malloc(LV_VER_RES * LV_HOR_RES * sizeof(lv_opa_t) / 4U);
	if (mask_map == NULL) {
		LOG_ERR("Allocating canvas layer failed. Please review LVGL mem pool size.");
		return -ENOMEM;
	}

	/* Create canvas layer for "drawing" touch events */
	pressing_cb_ctx.canvas = lv_canvas_create(lv_scr_act());
	lv_canvas_set_buffer(pressing_cb_ctx.canvas, mask_map, LV_HOR_RES, LV_VER_RES,
			     LV_IMG_CF_ALPHA_2BIT);
	lv_canvas_fill_bg(pressing_cb_ctx.canvas, lv_color_black(), LV_OPA_TRANSP);
	lv_obj_center(pressing_cb_ctx.canvas);
	lv_obj_add_event_cb(lv_scr_act(), lv_draw_pressing_callback, LV_EVENT_PRESSING,
			    &pressing_cb_ctx);

	/* Create label with "touch me" text */
	touch_label = lv_label_create(lv_scr_act());
	lv_label_set_text(touch_label, "touch me!");
	lv_obj_align(touch_label, LV_ALIGN_CENTER, 0, 0);

	lv_task_handler();
	display_blanking_off(display_dev);

	while (true) {
		lv_task_handler();
		k_sleep(K_MSEC(10));
	}
}
