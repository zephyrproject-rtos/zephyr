/*
 * Copyright (c) 2018 Jan Van Winkel <jan.van_winkel@dxplore.eu>
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

static uint32_t count;

#ifdef CONFIG_GPIO
static struct gpio_dt_spec button_gpio = GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw0), gpios, {0});
static struct gpio_callback button_callback;

static void button_isr_callback(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	count = 0;
}
#endif /* CONFIG_GPIO */

#ifdef CONFIG_LV_Z_ENCODER_INPUT
static const struct device *lvgl_encoder =
	DEVICE_DT_GET(DT_COMPAT_GET_ANY_STATUS_OKAY(zephyr_lvgl_encoder_input));
#endif /* CONFIG_LV_Z_ENCODER_INPUT */

#ifdef CONFIG_LV_Z_KEYPAD_INPUT
static const struct device *lvgl_keypad =
	DEVICE_DT_GET(DT_COMPAT_GET_ANY_STATUS_OKAY(zephyr_lvgl_keypad_input));
#endif /* CONFIG_LV_Z_KEYPAD_INPUT */

static void lv_btn_click_callback(lv_event_t *e)
{
#ifdef CONFIG_SAMPLE_LVGL_TOUCH_DRAWING
	lv_obj_t *canvas = (lv_obj_t *)e->user_data;

	lv_canvas_fill_bg(canvas, lv_color_black(), LV_OPA_TRANSP);
#else
	ARG_UNUSED(e);
#endif

	count = 0;
}

#ifdef CONFIG_SAMPLE_LVGL_TOUCH_DRAWING
static void lv_draw_pressing_callback(lv_event_t *e)
{
	lv_indev_t *indev = lv_indev_get_act();
	lv_point_t point;
	lv_draw_rect_dsc_t dsc;
	lv_obj_t *canvas = (lv_obj_t *)(e->user_data);

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
#endif

int main(void)
{
	char count_str[11] = {0};
	const struct device *display_dev;
	lv_obj_t *hello_world_label;
	lv_obj_t *count_label;

	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev)) {
		LOG_ERR("Device not ready, aborting test");
		return 0;
	}

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

#ifdef CONFIG_LV_Z_ENCODER_INPUT
	lv_obj_t *arc;
	lv_group_t *arc_group;

	arc = lv_arc_create(lv_scr_act());
	lv_obj_align(arc, LV_ALIGN_CENTER, 0, -15);
	lv_obj_set_size(arc, 150, 150);

	arc_group = lv_group_create();
	lv_group_add_obj(arc_group, arc);
	lv_indev_set_group(lvgl_input_get_indev(lvgl_encoder), arc_group);
#endif /* CONFIG_LV_Z_ENCODER_INPUT */

#ifdef CONFIG_LV_Z_KEYPAD_INPUT
	lv_obj_t *btn_matrix;
	lv_group_t *btn_matrix_group;
	static const char *const btnm_map[] = {"1", "2", "3", "4", ""};

	btn_matrix = lv_btnmatrix_create(lv_scr_act());
	lv_obj_align(btn_matrix, LV_ALIGN_CENTER, 0, 70);
	lv_btnmatrix_set_map(btn_matrix, (const char **)btnm_map);
	lv_obj_set_size(btn_matrix, 100, 50);

	btn_matrix_group = lv_group_create();
	lv_group_add_obj(btn_matrix_group, btn_matrix);
	lv_indev_set_group(lvgl_input_get_indev(lvgl_keypad), btn_matrix_group);
#endif /* CONFIG_LV_Z_KEYPAD_INPUT */

	if (IS_ENABLED(CONFIG_LV_Z_POINTER_KSCAN) || IS_ENABLED(CONFIG_LV_Z_POINTER_INPUT)) {
		lv_obj_t *hello_world_button;
		lv_obj_t *canvas = NULL;

#ifdef CONFIG_SAMPLE_LVGL_TOUCH_DRAWING
		/**
		 * The required canvas buf size is caluclated by
		 * (lv_img_color_format_get_px_size(cf) * width) / 8 * height)
		 * 2 bpp are used here to reduce overall memory requiremenets
		 **/
		lv_opa_t *mask_map = lv_mem_alloc(LV_VER_RES * LV_HOR_RES * sizeof(lv_opa_t) / 4U);

		if (mask_map == NULL) {
			LOG_ERR("Allocating canvas layer failed. Pleas review LVGL mem pool size.");
			return 0;
		}

		/* Create canvas layer for "drawing" touch events */
		canvas = lv_canvas_create(lv_scr_act());
		lv_canvas_set_buffer(canvas, mask_map, LV_HOR_RES, LV_VER_RES,
				     LV_IMG_CF_ALPHA_2BIT);
		lv_canvas_fill_bg(canvas, lv_color_black(), LV_OPA_TRANSP);
		lv_obj_center(canvas);
		lv_obj_add_event_cb(lv_scr_act(), lv_draw_pressing_callback, LV_EVENT_PRESSING,
				    canvas);
#endif

		/* create hello world button */
		hello_world_button = lv_btn_create(lv_scr_act());
		lv_obj_align(hello_world_button, LV_ALIGN_CENTER, 0, -15);
		lv_obj_add_event_cb(hello_world_button, lv_btn_click_callback, LV_EVENT_CLICKED,
				    canvas);
		hello_world_label = lv_label_create(hello_world_button);

	} else {
		hello_world_label = lv_label_create(lv_scr_act());
	}

	lv_label_set_text(hello_world_label, "Hello world!");
	lv_obj_align(hello_world_label, LV_ALIGN_CENTER, 0, 0);

	count_label = lv_label_create(lv_scr_act());
	lv_obj_align(count_label, LV_ALIGN_BOTTOM_MID, 0, 0);

	lv_task_handler();
	display_blanking_off(display_dev);

	while (1) {
		if ((count % 100) == 0U) {
			sprintf(count_str, "%d", count / 100U);
			lv_label_set_text(count_label, count_str);
		}
		lv_task_handler();
		++count;
		k_sleep(K_MSEC(10));
	}
}
