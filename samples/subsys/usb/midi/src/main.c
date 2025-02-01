/*
 * Copyright (c) 2024 Titouan Christophe
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file
 * @brief Sample application for USB MIDI 2.0 device class
 */

#include <sample_usbd.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/input/input.h>
#include <zephyr/usb/class/usbd_midi2.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <lvgl.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <lvgl_input_device.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sample_usb_midi, LOG_LEVEL_INF);

static const struct device *const midi = DEVICE_DT_GET(DT_NODELABEL(usb_midi));

static struct gpio_dt_spec led = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led0), gpios, {0});

/* white key notes */
static const int32_t white_key_notes[] = {60, 62, 64, 65, 67, 69, 71};

/* black key notes */
static const int32_t black_key_notes[] = {61, 63, 0, 66, 68, 70, 0};

static lv_obj_t *white_key_widgets[128] = {NULL};
static lv_obj_t *black_key_widgets[128] = {NULL};

static void key_press(struct input_event *evt, void *user_data)
{
	/* Only handle key presses in the 7bit MIDI range */
	if (evt->type != INPUT_EV_KEY || evt->code > 0x7f) {
		return;
	}
	uint8_t command = evt->value ? UMP_MIDI_NOTE_ON : UMP_MIDI_NOTE_OFF;
	uint8_t channel = 0;
	uint8_t note = evt->code;
	uint8_t velocity = 100;

	struct midi_ump ump = UMP_MIDI1_CHANNEL_VOICE(0, command, channel, note, velocity);
	usbd_midi_send(midi, ump);
}
INPUT_CALLBACK_DEFINE(NULL, key_press, NULL);

static void on_midi_packet(const struct device *dev, const struct midi_ump ump)
{
	LOG_INF("Received MIDI packet (MT=%X)", UMP_MT(ump));

	/* Only send MIDI1 channel voice messages back to the host */
	if (UMP_MT(ump) != UMP_MT_MIDI1_CHANNEL_VOICE) {
		return;
	}

	/*if (UMP_MIDI_COMMAND(ump) == UMP_MIDI_NOTE_ON) {
		uint8_t note = UMP_MIDI1_P1(ump);
		if (white_key_widgets[note]) {
			lv_obj_set_style_bg_color(white_key_widgets[note], lv_color_hex(0xAAAAAA), LV_PART_MAIN);
		} else if (black_key_widgets[note]) {
			lv_obj_set_style_bg_color(black_key_widgets[note], lv_color_hex(0xAAAAAA), LV_PART_MAIN);
		}
	} else if (UMP_MIDI_COMMAND(ump) == UMP_MIDI_NOTE_OFF) {
		uint8_t note = UMP_MIDI1_P1(ump);
		if (white_key_widgets[note]) {
			lv_obj_set_style_bg_color(white_key_widgets[note], lv_color_hex(0xFFFFFF), LV_PART_MAIN);
		} else if (black_key_widgets[note]) {
			lv_obj_set_style_bg_color(black_key_widgets[note], lv_color_hex(0x000000), LV_PART_MAIN);
		}
	}*/
}

static void on_device_ready(const struct device *dev, const bool ready)
{
	/* Light up the LED (if any) when USB-MIDI2.0 is enabled */
	if (led.port) {
		gpio_pin_set_dt(&led, ready);
	}
}

static const struct usbd_midi_ops ops = {
	.rx_packet_cb = on_midi_packet,
	.ready_cb = on_device_ready,
};

static uint32_t count;

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
	printk("Key clicked\n");
}

// Callback for button events
void key_event_cb(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t *key = lv_event_get_target(e);
	const char *key_name = lv_obj_get_user_data(key);
	uint32_t pitch = lv_event_get_user_data(e);

	if (code == LV_EVENT_PRESSED) {
		printf("Key pressed - color: %s, pitch: %d\n", key_name, pitch);
		lv_obj_set_style_bg_color(key, lv_color_hex(0xAAAAAA),
					  LV_PART_MAIN); // Highlight the key

		struct midi_ump ump = UMP_MIDI1_CHANNEL_VOICE(0, UMP_MIDI_NOTE_ON, 0, pitch, 100);
		usbd_midi_send(midi, ump);
	} else if (code == LV_EVENT_RELEASED) {
		lv_color_t default_color = strcmp(key_name, "black") == 0 ? lv_color_hex(0x000000)
									  : lv_color_hex(0xFFFFFF);
		lv_obj_set_style_bg_color(key, default_color, LV_PART_MAIN); // Reset the color
		struct midi_ump ump = UMP_MIDI1_CHANNEL_VOICE(0, UMP_MIDI_NOTE_OFF, 0, pitch, 100);
		usbd_midi_send(midi, ump);
	}
}

void draw_piano_keyboard(lv_obj_t *parent)
{
	const lv_coord_t screen_width = 320;
	const lv_coord_t screen_height = 240;

	// Dimensions for white keys
	const lv_coord_t white_key_width = screen_width / 7; // Divide the screen width by 7 keys
	const lv_coord_t white_key_height = screen_height;

	// Dimensions for black keys
	const lv_coord_t black_key_width = white_key_width * 0.6; // Black keys are narrower
	const lv_coord_t black_key_height = screen_height * 0.6;  // Black keys are shorter

	// Draw white keys
	for (int i = 0; i < 7; i++) {
		lv_obj_t *white_key = lv_btn_create(parent);
		lv_obj_set_size(white_key, white_key_width, white_key_height);
		lv_obj_set_pos(white_key, i * white_key_width, 0);
		lv_obj_set_style_bg_color(white_key, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
		lv_obj_set_style_border_width(white_key, 1, LV_PART_MAIN);
		lv_obj_set_user_data(white_key, "white");
		lv_obj_add_event_cb(white_key, key_event_cb, LV_EVENT_ALL,
				    (void *)(intptr_t)white_key_notes[i]); // set pitch as user data
		white_key_widgets[white_key_notes[i]] = white_key;
	}

	// Draw black keys
	for (int i = 0; i < 7; i++) {
		if (i == 2 || i == 6) {
			continue; // No black keys between B/C and E/F
		}

		lv_obj_t *black_key = lv_btn_create(parent);
		lv_obj_set_size(black_key, black_key_width, black_key_height);
		lv_obj_set_pos(black_key, (i + 1) * white_key_width - (black_key_width / 2), 0);
		lv_obj_set_style_bg_color(black_key, lv_color_hex(0x000000), LV_PART_MAIN);
		lv_obj_set_style_border_width(black_key, 1, LV_PART_MAIN);
		lv_obj_set_user_data(black_key, "black");
		lv_obj_add_event_cb(black_key, key_event_cb, LV_EVENT_ALL,
				    (void *)(intptr_t)black_key_notes[i]); // set pitch as user data
		black_key_widgets[black_key_notes[i]] = black_key;
	}
}

int main(void)
{
	struct usbd_context *sample_usbd;

	memset(white_key_widgets, 0, sizeof(white_key_widgets));
	memset(black_key_widgets, 0, sizeof(black_key_widgets));

	if (!device_is_ready(midi)) {
		LOG_ERR("MIDI device not ready");
		return -1;
	}

	if (led.port) {
		if (gpio_pin_configure_dt(&led, GPIO_OUTPUT)) {
			LOG_ERR("Unable to setup LED, not using it");
			memset(&led, 0, sizeof(led));
		}
	}

	usbd_midi_set_ops(midi, &ops);

	sample_usbd = sample_usbd_init_device(NULL);
	if (sample_usbd == NULL) {
		LOG_ERR("Failed to initialize USB device");
		return -1;
	}

	if (usbd_enable(sample_usbd)) {
		LOG_ERR("Failed to enable device support");
		return -1;
	}

	char count_str[11] = {0};
	const struct device *display_dev;

	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev)) {
		LOG_ERR("Device not ready, aborting test");
		return 0;
	}

	draw_piano_keyboard(lv_scr_act());

	lv_task_handler();
	display_blanking_off(display_dev);

	while (1) {
		lv_task_handler();
		++count;
		k_sleep(K_MSEC(10));
	}
}
