/*
 * Copyright (c) 2026 NotioNext Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/display.h>
#include <zephyr/input/input.h>
#include <zephyr/logging/log.h>
#include <lvgl.h>
#include <lvgl_zephyr.h>
#include <string.h>
#include "bt.h"

LOG_MODULE_REGISTER(esp32s3_box3_lvgl_touch, LOG_LEVEL_INF);

/* GPIO and device definitions */
#define BACKLIGHT_NODE DT_PATH(leds, lcd_backlight)
#define DISPLAY_NODE DT_CHOSEN(zephyr_display)
#define GT911_NODE DT_NODELABEL(gt911)

static const struct gpio_dt_spec backlight = GPIO_DT_SPEC_GET(BACKLIGHT_NODE, gpios);
static const struct device *display_dev = DEVICE_DT_GET(DISPLAY_NODE);

/* LVGL objects */
static lv_obj_t *screen1;
static lv_obj_t *screen2;
static lv_obj_t *next_btn;

/* Touch input variables */
static int16_t touch_x = -1;
static int16_t touch_y = -1;
static bool touch_pressed = false;
static bool button_clicked = false;  /* Debounce flag */

/* Forward declarations */
static void create_screen1(void);
static void create_screen2(void);
static void next_btn_event_cb(lv_event_t *e);

void enable_backlight(void)
{
	LOG_INF("Configuring backlight...");

	if (!gpio_is_ready_dt(&backlight)) {
		LOG_ERR("Backlight GPIO not ready");
		return;
	}

	int ret = gpio_pin_configure_dt(&backlight, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure backlight GPIO: %d", ret);
		return;
	}

	gpio_pin_set_dt(&backlight, 1);
	LOG_INF("Backlight enabled successfully");
}

static void input_callback(struct input_event *evt, void *user_data)
{
	switch (evt->code) {
	case INPUT_ABS_X:
		touch_x = evt->value;
		break;

	case INPUT_ABS_Y:
		touch_y = evt->value;
		break;

	case INPUT_BTN_TOUCH:
		if (evt->value == 1) {
			/* Touch pressed */
			touch_pressed = true;
			button_clicked = false;  /* Reset click flag */
		} else {
			/* Touch released */
			touch_pressed = false;
		}
		break;

	default:
		break;
	}

	/* Handle button click only once per touch (debounce) */
	if (evt->sync && touch_pressed && !button_clicked && touch_x >= 0 && touch_y >= 0) {
		/* Check if touch is on the Next button area */
		if (lv_scr_act() == screen1 && next_btn) {
			lv_area_t btn_area;
			lv_obj_get_coords(next_btn, &btn_area);

			if (touch_x >= btn_area.x1 && touch_x <= btn_area.x2 &&
			    touch_y >= btn_area.y1 && touch_y <= btn_area.y2) {
				LOG_INF("Next button clicked! Switching to Screen 2");
				lv_scr_load(screen2);
				button_clicked = true;  /* Prevent multiple clicks */
			}
		}
	}
}

INPUT_CALLBACK_DEFINE(DEVICE_DT_GET(GT911_NODE), input_callback, NULL);

static void next_btn_event_cb(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	if (code == LV_EVENT_CLICKED) {
		LOG_INF("Next button clicked via LVGL event! Switching to Screen 2");
		lv_scr_load(screen2);
	}
}

static void create_screen1(void)
{
	LOG_INF("Creating Screen 1...");

	/* Create screen 1 */
	screen1 = lv_obj_create(NULL);

	/* Set background color to light blue */
	lv_obj_set_style_bg_color(screen1, lv_color_make(173, 216, 230), LV_PART_MAIN);
	lv_obj_set_style_bg_opa(screen1, LV_OPA_COVER, LV_PART_MAIN);

	/* Create welcome label */
	lv_obj_t *welcome_label = lv_label_create(screen1);
	lv_label_set_text(welcome_label, "Click Next for next screen");
	lv_obj_set_style_text_color(welcome_label, lv_color_make(0, 0, 0), LV_PART_MAIN);
	lv_obj_set_style_text_font(welcome_label, &lv_font_montserrat_14, LV_PART_MAIN);
	lv_obj_align(welcome_label, LV_ALIGN_CENTER, 0, -40);

	/* Create Next button */
	next_btn = lv_btn_create(screen1);
	lv_obj_set_size(next_btn, 100, 50);
	lv_obj_align(next_btn, LV_ALIGN_CENTER, 0, 40);
	lv_obj_add_event_cb(next_btn, next_btn_event_cb, LV_EVENT_CLICKED, NULL);

	/* Button label */
	lv_obj_t *btn_label = lv_label_create(next_btn);
	lv_label_set_text(btn_label, "Next");
	lv_obj_set_style_text_color(btn_label, lv_color_make(255, 255, 255), LV_PART_MAIN);
	lv_obj_center(btn_label);

	LOG_INF("Screen 1 created successfully");
}

static void create_screen2(void)
{
	LOG_INF("Creating Screen 2...");

	/* Create screen 2 */
	screen2 = lv_obj_create(NULL);

	/* Set background color to light green */
	lv_obj_set_style_bg_color(screen2, lv_color_make(144, 238, 144), LV_PART_MAIN);
	lv_obj_set_style_bg_opa(screen2, LV_OPA_COVER, LV_PART_MAIN);

	/* Create welcome label */
	lv_obj_t *welcome_label = lv_label_create(screen2);
	lv_label_set_text(welcome_label, "Hello World!");
	lv_obj_set_style_text_color(welcome_label, lv_color_make(0, 0, 0), LV_PART_MAIN);
	lv_obj_set_style_text_font(welcome_label, &lv_font_montserrat_14, LV_PART_MAIN);
	lv_obj_center(welcome_label);

	LOG_INF("Screen 2 created successfully");
}

int main(void)
{
	LOG_INF("=== ESP32-S3-BOX-3 LVGL Touch Demo ===");
	LOG_INF("Board: %s", CONFIG_BOARD);

	/* Initialize Bluetooth beacon */
	LOG_INF("Initializing Bluetooth beacon...");
	int bt_ret = init_bluetooth();
	if (bt_ret) {
		LOG_ERR("Failed to initialize Bluetooth: %d", bt_ret);
		/* Continue with LVGL demo even if Bluetooth fails */
	} else {
		LOG_INF("Bluetooth beacon initialized successfully");
	}

	/* Check if GT911 touch is ready */
	const struct device *gt911_dev = DEVICE_DT_GET(GT911_NODE);
	if (!device_is_ready(gt911_dev)) {
		LOG_ERR("GT911 touch controller not ready!");
		return -1;
	}
	LOG_INF("✅ GT911 touch controller ready");

	/* Enable backlight */
	enable_backlight();

	/* Check if display is ready */
	LOG_INF("Checking display device...");
	if (!device_is_ready(display_dev)) {
		LOG_ERR("Display device NOT ready!");
		return -1;
	}
	LOG_INF("Display device is ready");

	/* Turn on display */
	LOG_INF("Turning on display...");
	display_blanking_off(display_dev);

	/* Wait for display to stabilize */
	k_sleep(K_MSEC(200));

	/* Initialize LVGL */
	LOG_INF("Initializing LVGL...");
	lv_init();
	LOG_INF("LVGL core initialized");

	/* Initialize LVGL display driver */
	int ret = lvgl_init();
	if (ret != 0) {
		LOG_ERR("Failed to initialize LVGL display driver: %d", ret);
		return -1;
	}
	LOG_INF("LVGL display driver initialized successfully");

	/* Create screens */
	create_screen1();
	create_screen2();

	/* Load screen 1 initially */
	lv_scr_load(screen1);
	LOG_INF("Screen 1 loaded - showing 'Click Next for next screen' with Next button");

	LOG_INF("LVGL Touch demo started - touch the Next button to navigate!");

	/* Main loop - handle LVGL tasks */
	while (1) {
		lv_timer_handler();
		k_sleep(K_MSEC(10));
	}

	return 0;
}
