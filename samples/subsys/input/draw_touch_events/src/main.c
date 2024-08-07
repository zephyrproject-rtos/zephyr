/*
 * Copyright (c) 2024 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/input/input.h>
#include <zephyr/drivers/display.h>

LOG_MODULE_REGISTER(sample, LOG_LEVEL_INF);

#if !DT_NODE_EXISTS(DT_CHOSEN(zephyr_touch))
#error "Unsupported board: zephyr,touch is not assigned"
#endif

#if !DT_NODE_EXISTS(DT_CHOSEN(zephyr_display))
#error "Unsupported board: zephyr,display is not assigned"
#endif

#define WIDTH     (DT_PROP(DT_CHOSEN(zephyr_display), width))
#define HEIGHT    (DT_PROP(DT_CHOSEN(zephyr_display), height))
#define CROSS_DIM (WIDTH / CONFIG_SCREEN_WIDTH_TO_CROSS_DIM)

#define PIXEL_FORMAT (DT_PROP_OR(DT_CHOSEN(zephyr_display), pixel_format, PIXEL_FORMAT_ARGB_8888))
#define BPP          ((DISPLAY_BITS_PER_PIXEL(PIXEL_FORMAT)) / 8)

#define BUFFER_SIZE  (CROSS_DIM * CROSS_DIM * BPP)
#define REFRESH_RATE 100

static const struct device *const display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
static const struct device *const touch_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_touch));
static struct display_buffer_descriptor buf_desc = {
	.buf_size = BUFFER_SIZE, .pitch = CROSS_DIM, .width = CROSS_DIM, .height = CROSS_DIM};

static uint8_t buffer_cross[BUFFER_SIZE];
static const uint8_t buffer_cross_empty[BUFFER_SIZE];
static struct k_sem sync;

static struct {
	size_t x;
	size_t y;
	bool pressed;
} touch_point, touch_point_drawn;

static void touch_event_callback(struct input_event *evt, void *user_data)
{
	if (evt->code == INPUT_ABS_X) {
		touch_point.x = evt->value;
	}
	if (evt->code == INPUT_ABS_Y) {
		touch_point.y = evt->value;
	}
	if (evt->code == INPUT_BTN_TOUCH) {
		touch_point.pressed = evt->value;
	}
	if (evt->sync) {
		k_sem_give(&sync);
	}
}
INPUT_CALLBACK_DEFINE(touch_dev, touch_event_callback, NULL);

static void clear_screen(void)
{
	int x;
	int y;

	for (x = 0; x < WIDTH; x += CROSS_DIM) {
		for (y = 0; y < HEIGHT; y += CROSS_DIM) {
			display_write(display_dev, x, y, &buf_desc, buffer_cross_empty);
		}
	}
}

static void fill_cross_buffer(void)
{
	int i;
	int x;
	int y;
	int index;

	for (i = 0; i < BPP; i++) {
		for (x = 0; x < CROSS_DIM; x++) {
			index = BPP * (CROSS_DIM / 2 * CROSS_DIM + x);
			buffer_cross[index + i] = -1;
		}
		for (y = 0; y < CROSS_DIM; y++) {
			index = BPP * (y * CROSS_DIM + CROSS_DIM / 2);
			buffer_cross[index + i] = -1;
		}
	}
}

static int get_draw_position(int value, int upper_bound)
{
	if (value < CROSS_DIM / 2) {
		return 0;
	}

	if (value + CROSS_DIM / 2 > upper_bound) {
		return upper_bound - CROSS_DIM;
	}

	return value - CROSS_DIM / 2;
}

int main(void)
{

	LOG_INF("Touch sample for touchscreen: %s, dc: %s", touch_dev->name, display_dev->name);

	if (!device_is_ready(touch_dev)) {
		LOG_ERR("Device %s not found. Aborting sample.", touch_dev->name);
		return 0;
	}

	if (!device_is_ready(display_dev)) {
		LOG_ERR("Device %s not found. Aborting sample.", display_dev->name);
		return 0;
	}

	if (BPP == 0 || BPP > 4) {
		LOG_ERR("Unsupported BPP=%d", BPP);
		return 0;
	}
	fill_cross_buffer();
	display_blanking_off(display_dev);

	clear_screen();
	touch_point_drawn.x = CROSS_DIM / 2;
	touch_point_drawn.y = CROSS_DIM / 2;
	touch_point.x = -1;
	touch_point.y = -1;

	k_sem_init(&sync, 0, 1);

	while (1) {
		k_msleep(REFRESH_RATE);
		k_sem_take(&sync, K_FOREVER);
		LOG_INF("TOUCH %s X, Y: (%d, %d)", touch_point.pressed ? "PRESS" : "RELEASE",
			touch_point.x, touch_point.y);

		display_write(display_dev, get_draw_position(touch_point_drawn.x, WIDTH),
			      get_draw_position(touch_point_drawn.y, HEIGHT), &buf_desc,
			      buffer_cross_empty);

		display_write(display_dev, get_draw_position(touch_point.x, WIDTH),
			      get_draw_position(touch_point.y, HEIGHT), &buf_desc, buffer_cross);

		touch_point_drawn.x = touch_point.x;
		touch_point_drawn.y = touch_point.y;
	}
	return 0;
}
