/*
 * Copyright (c) 2026 NotioNext LTD.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/display.h>
#include <zephyr/logging/log.h>
#include <lvgl.h>
#include <lvgl_zephyr.h>
#include <string.h>

LOG_MODULE_REGISTER(esp32s3_box3_demo, LOG_LEVEL_INF);

/* GPIO definitions */
#define BACKLIGHT_NODE DT_PATH(leds, lcd_backlight)

/* Display definitions */
#define DISPLAY_NODE DT_CHOSEN(zephyr_display)

static const struct gpio_dt_spec backlight = GPIO_DT_SPEC_GET(BACKLIGHT_NODE, gpios);
static const struct device *display_dev = DEVICE_DT_GET(DISPLAY_NODE);

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

void create_hello_world_ui(void)
{
	LOG_INF("Creating LVGL Hello world UI...");

	/* Get active screen */
	lv_obj_t *scr = lv_scr_act();
	LOG_INF("Got active screen: %p", scr);

	/* LVGL expects RGB */
	lv_color_t green_color = lv_color_make(0, 0, 255);
	lv_obj_set_style_bg_color(scr, green_color, LV_PART_MAIN);
	lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);
	LOG_INF("Set background color to green with BGR handling");

	/* Create a label for "Hello world" */
	lv_obj_t *label = lv_label_create(scr);
	LOG_INF("Created label: %p", label);

	if (label) {
		lv_label_set_text(label, "Hello World!");
		LOG_INF("Set label text");

		/* Set text color to white */
		lv_obj_set_style_text_color(label, lv_color_make(255, 255, 255), LV_PART_MAIN);
		LOG_INF("Set text color to white");

		/* Center the label */
		lv_obj_center(label);
		LOG_INF("Centered label");
	} else {
		LOG_ERR("Failed to create label");
	}

	LOG_INF("LVGL Hello World UI created successfully");
}

int main(void)
{
	LOG_INF("=== ESP32-S3-BOX-3 LVGL Hello world Demo ===");
	LOG_INF("Board: %s", CONFIG_BOARD);

	/* Enable backlight first */
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
	LOG_INF("Waiting for display to stabilize...");
	k_sleep(K_MSEC(200));

	/* Manually initialize LVGL since we disabled automatic init */
	LOG_INF("Manually initializing LVGL...");

	/* Initialize LVGL */
	lv_init();
	LOG_INF("LVGL core initialized");

	/* Initialize LVGL display driver */
	int ret = lvgl_init();
	if (ret != 0) {
		LOG_ERR("Failed to initialize LVGL display driver: %d", ret);
		return -1;
	}
	LOG_INF("LVGL display driver initialized successfully");

	/* Test basic LVGL functionality */
	LOG_INF("Testing basic LVGL...");

	/* Try to get active screen - this is the first LVGL call */
	lv_obj_t *scr = lv_scr_act();
	if (!scr) {
		LOG_ERR("Failed to get LVGL active screen!");
		return -1;
	}
	LOG_INF("LVGL active screen obtained: %p", scr);

	/* Create the Hello world UI */
	LOG_INF("Creating LVGL UI...");
	create_hello_world_ui();

	LOG_INF("LVGL demo started - entering main loop");

	/* Main loop - handle LVGL tasks */
	while (1) {
		lv_timer_handler();
		k_sleep(K_MSEC(10));

	}

	return 0;
}
