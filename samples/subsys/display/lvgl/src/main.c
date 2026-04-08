/*
 * Copyright (c) 2018 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <lvgl.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/input/input.h>
#include <zephyr/dt-bindings/input/input-event-codes.h>
#include <lvgl_input_device.h>
#include <string.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app);

#ifdef CONFIG_LV_Z_ENCODER_INPUT
static const struct device *lvgl_encoder =
	DEVICE_DT_GET(DT_COMPAT_GET_ANY_STATUS_OKAY(zephyr_lvgl_encoder_input));
#endif /* CONFIG_LV_Z_ENCODER_INPUT */

#ifdef CONFIG_LV_Z_KEYPAD_INPUT
static const struct device *lvgl_keypad =
	DEVICE_DT_GET(DT_COMPAT_GET_ANY_STATUS_OKAY(zephyr_lvgl_keypad_input));
#endif /* CONFIG_LV_Z_KEYPAD_INPUT */

static const struct pwm_dt_spec buzzer = PWM_DT_SPEC_GET(DT_ALIAS(buzzer));
static const struct gpio_dt_spec led_red = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led0), gpios, {0});
static const struct gpio_dt_spec led_green = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led1), gpios, {0});
static const struct gpio_dt_spec led_blue = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led2), gpios, {0});

static lv_obj_t *countdown_label;
static lv_obj_t *hint_label;
static lv_obj_t *countdown_bar;
static lv_obj_t *defuse_bar;
static uint32_t seconds_left;

static volatile bool button_down;
static volatile bool button_press_evt;
static volatile bool button_release_evt;
static volatile uint32_t button_press_ts;
static volatile uint32_t button_release_ts;
static volatile uint32_t button_last_evt_ts;
static bool suppress_next_release_arm;

enum led_color {
	LED_COLOR_OFF,
	LED_COLOR_GREEN,
	LED_COLOR_PINK,
	LED_COLOR_ORANGE,
	LED_COLOR_RED,
};

enum app_state {
	APP_STATE_STAND,
	APP_STATE_COUNTDOWN,
	APP_STATE_DEFUSED,
	APP_STATE_ZEROED,
};

#define COUNTDOWN_SECONDS 15U
#define DEFUSE_HOLD_MS 2000U
#define RETURN_STAND_HOLD_MS 5000U
#define BUTTON_DEBOUNCE_MS 40U
#define LABEL_MAX_CHARS_PER_LINE 12U

static enum app_state state;

static void set_led_color(enum led_color color);
static int beep_ms(uint32_t duration_ms, enum led_color color);
static void second_beep_pattern(uint32_t sec);
static void update_screen(void);
static void enter_state(enum app_state new_state);
static void update_defuse_progress(uint32_t held_ms, bool active);
static void update_countdown_progress(void);
static void lv_label_set_wrapped_text(lv_obj_t *label, const char *text);

static void button_input_cb(struct input_event *evt, void *user_data)
{
	ARG_UNUSED(user_data);

	if (evt->type != INPUT_EV_KEY || evt->code != INPUT_KEY_0) {
		return;
	}

	if ((k_uptime_get_32() - button_last_evt_ts) < BUTTON_DEBOUNCE_MS) {
		return;
	}
	button_last_evt_ts = k_uptime_get_32();

	if (evt->value) {
		button_down = true;
		button_press_ts = k_uptime_get_32();
		button_press_evt = true;
	} else {
		button_down = false;
		button_release_ts = k_uptime_get_32();
		button_release_evt = true;
	}
}

INPUT_CALLBACK_DEFINE(NULL, button_input_cb, NULL);

static void lv_btn_click_callback(lv_event_t *e)
{
	ARG_UNUSED(e);
	if (state == APP_STATE_STAND || state == APP_STATE_DEFUSED) {
		enter_state(APP_STATE_COUNTDOWN);
	}
}

static void set_led_color(enum led_color color)
{
	if (!gpio_is_ready_dt(&led_red) || !gpio_is_ready_dt(&led_green) || !gpio_is_ready_dt(&led_blue)) {
		return;
	}

	switch (color) {
	case LED_COLOR_GREEN:
		gpio_pin_set_dt(&led_red, 0);
		gpio_pin_set_dt(&led_green, 1);
		gpio_pin_set_dt(&led_blue, 0);
		break;
	case LED_COLOR_PINK:
		gpio_pin_set_dt(&led_red, 1);
		gpio_pin_set_dt(&led_green, 0);
		gpio_pin_set_dt(&led_blue, 1);
		break;
	case LED_COLOR_ORANGE:
		gpio_pin_set_dt(&led_red, 1);
		gpio_pin_set_dt(&led_green, 1);
		gpio_pin_set_dt(&led_blue, 0);
		break;
	case LED_COLOR_RED:
		gpio_pin_set_dt(&led_red, 1);
		gpio_pin_set_dt(&led_green, 0);
		gpio_pin_set_dt(&led_blue, 0);
		break;
	case LED_COLOR_OFF:
	default:
		gpio_pin_set_dt(&led_red, 0);
		gpio_pin_set_dt(&led_green, 0);
		gpio_pin_set_dt(&led_blue, 0);
		break;
	}
}

static int beep_ms(uint32_t duration_ms, enum led_color color)
{
	int err;

	if (!pwm_is_ready_dt(&buzzer)) {
		LOG_ERR("PWM buzzer not ready");
		return -1;
	}

	err = pwm_set_dt(&buzzer, buzzer.period, buzzer.period / 2U);
	if (err) {
		LOG_ERR("pwm_set_dt failed: %d", err);
		return err;
	}

	set_led_color(color);
	k_msleep(duration_ms);

	/* Stop PWM output between beeps. */
	pwm_set_dt(&buzzer, buzzer.period, 0);
	set_led_color(LED_COLOR_OFF);

	return 0;
}

static void second_beep_pattern(uint32_t sec)
{
	if (sec > 5U) {
		(void)beep_ms(80U, LED_COLOR_PINK);
		return;
	}

	/* In the final 5 seconds, use a quick double beep. */
	(void)beep_ms(50U, LED_COLOR_ORANGE);
	k_msleep(100U);
	(void)beep_ms(50U, LED_COLOR_ORANGE);
}

static void update_screen(void)
{
	static char sec_str[4];

	switch (state) {
	case APP_STATE_STAND:
		lv_label_set_text(countdown_label, "SAFE");
		lv_label_set_wrapped_text(hint_label, "Tap Restart to arm");
		break;
	case APP_STATE_DEFUSED:
		lv_label_set_text(countdown_label, "DEFUSED");
		lv_label_set_wrapped_text(hint_label, "Green steady");
		break;
	case APP_STATE_ZEROED:
		lv_label_set_text(countdown_label, "TIME UP");
		lv_label_set_wrapped_text(hint_label, "Hold button 5s for SAFE");
		break;
	case APP_STATE_COUNTDOWN:
	default:
		snprintk(sec_str, sizeof(sec_str), "%u", seconds_left);
		lv_label_set_text(countdown_label, sec_str);
		lv_label_set_wrapped_text(hint_label, "Hold button to defuse");
		break;
	}
}

static void lv_label_set_wrapped_text(lv_obj_t *label, const char *text)
{
	static char wrapped[128];
	size_t i = 0U;
	size_t w = 0U;

	if (!label || !text) {
		return;
	}

	while (text[i] != '\0' && w < sizeof(wrapped) - 1U) {
		size_t line_len = 0U;
		size_t line_start = i;
		size_t last_space_out = SIZE_MAX;

		while (text[i] != '\0' && text[i] != '\n' && line_len < LABEL_MAX_CHARS_PER_LINE && w < sizeof(wrapped) - 1U) {
			wrapped[w] = text[i];
			if (text[i] == ' ') {
				last_space_out = w;
			}
			w++;
			i++;
			line_len++;
		}

		if (text[i] == '\n') {
			wrapped[w++] = '\n';
			i++;
			continue;
		}

		if (line_len == LABEL_MAX_CHARS_PER_LINE && text[i] != '\0') {
			if (last_space_out != SIZE_MAX) {
				size_t backtrack = w - 1U - last_space_out;
				wrapped[last_space_out] = '\n';
				i -= backtrack;
				w = last_space_out + 1U;
			} else {
				wrapped[w++] = '\n';
			}
		} else if (text[i] != '\0' && text[i] != '\n') {
			wrapped[w++] = '\n';
		}

		if (i == line_start) {
			break;
		}
	}

	wrapped[w] = '\0';
	lv_label_set_text(label, wrapped);
}

static void update_defuse_progress(uint32_t held_ms, bool active)
{
	if (!defuse_bar || !countdown_bar) {
		return;
	}

	if (active) {
		if (held_ms > DEFUSE_HOLD_MS) {
			held_ms = DEFUSE_HOLD_MS;
		}
		lv_obj_add_flag(countdown_bar, LV_OBJ_FLAG_HIDDEN);
		lv_bar_set_value(defuse_bar, held_ms, LV_ANIM_OFF);
		lv_obj_clear_flag(defuse_bar, LV_OBJ_FLAG_HIDDEN);
	} else {
		lv_bar_set_value(defuse_bar, 0, LV_ANIM_OFF);
		lv_obj_add_flag(defuse_bar, LV_OBJ_FLAG_HIDDEN);
		if (state == APP_STATE_COUNTDOWN) {
			lv_obj_clear_flag(countdown_bar, LV_OBJ_FLAG_HIDDEN);
		}
	}
}

static void update_countdown_progress(void)
{
	if (!countdown_bar) {
		return;
	}

	if (state == APP_STATE_COUNTDOWN) {
		lv_bar_set_value(countdown_bar, seconds_left * 1000U, LV_ANIM_OFF);
		lv_obj_clear_flag(countdown_bar, LV_OBJ_FLAG_HIDDEN);
	} else {
		lv_obj_add_flag(countdown_bar, LV_OBJ_FLAG_HIDDEN);
	}
}

static void enter_state(enum app_state new_state)
{
	state = new_state;

	switch (state) {
	case APP_STATE_STAND:
	case APP_STATE_DEFUSED:
		set_led_color(LED_COLOR_GREEN);
		break;
	case APP_STATE_ZEROED:
		set_led_color(LED_COLOR_RED);
		break;
	case APP_STATE_COUNTDOWN:
	default:
		seconds_left = COUNTDOWN_SECONDS;
		second_beep_pattern(seconds_left);
		set_led_color(LED_COLOR_OFF);
		suppress_next_release_arm = false;
		break;
	}

	update_countdown_progress();
	update_defuse_progress(0U, false);
	update_screen();
}


int main(void)
{
	const struct device *display_dev;
	lv_obj_t *restart_button;
	lv_obj_t *button_label;
	uint32_t last_tick;
	uint32_t held_ms;
	uint32_t press_ts;
	uint32_t release_ts;
	uint32_t active_press_ts;
	bool press_evt;
	bool release_evt;
	bool release_used_for_arm;
	int ret;

	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev)) {
		LOG_ERR("Device not ready, aborting test");
		return 0;
	}

	if (gpio_is_ready_dt(&led_red)) {
		(void)gpio_pin_configure_dt(&led_red, GPIO_OUTPUT_INACTIVE);
	}
	if (gpio_is_ready_dt(&led_green)) {
		(void)gpio_pin_configure_dt(&led_green, GPIO_OUTPUT_INACTIVE);
	}
	if (gpio_is_ready_dt(&led_blue)) {
		(void)gpio_pin_configure_dt(&led_blue, GPIO_OUTPUT_INACTIVE);
	}

#ifdef CONFIG_LV_Z_ENCODER_INPUT
	lv_obj_t *arc;
	lv_group_t *arc_group;

	arc = lv_arc_create(lv_screen_active());
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

	btn_matrix = lv_buttonmatrix_create(lv_screen_active());
	lv_obj_align(btn_matrix, LV_ALIGN_CENTER, 0, 70);
	lv_buttonmatrix_set_map(btn_matrix, (const char **)btnm_map);
	lv_obj_set_size(btn_matrix, 100, 50);

	btn_matrix_group = lv_group_create();
	lv_group_add_obj(btn_matrix_group, btn_matrix);
	lv_indev_set_group(lvgl_input_get_indev(lvgl_keypad), btn_matrix_group);
#endif /* CONFIG_LV_Z_KEYPAD_INPUT */

	countdown_label = lv_label_create(lv_screen_active());
	lv_obj_set_style_text_align(countdown_label, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_align(countdown_label, LV_ALIGN_TOP_MID, 0, 0);

	hint_label = lv_label_create(lv_screen_active());
	lv_obj_set_style_text_align(hint_label, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_align(hint_label, LV_ALIGN_CENTER, 0, -2);

	countdown_bar = lv_bar_create(lv_screen_active());
	lv_obj_set_size(countdown_bar, 112, 6);
	lv_obj_align(countdown_bar, LV_ALIGN_CENTER, 0, 26);
	lv_bar_set_range(countdown_bar, 0, COUNTDOWN_SECONDS * 1000U);
	lv_obj_add_flag(countdown_bar, LV_OBJ_FLAG_HIDDEN);

	defuse_bar = lv_bar_create(lv_screen_active());
	lv_obj_set_size(defuse_bar, 112, 6);
	lv_obj_align(defuse_bar, LV_ALIGN_CENTER, 0, 26);
	lv_bar_set_range(defuse_bar, 0, DEFUSE_HOLD_MS);
	lv_obj_add_flag(defuse_bar, LV_OBJ_FLAG_HIDDEN);

	if (IS_ENABLED(CONFIG_LV_Z_POINTER_INPUT)) {
		restart_button = lv_button_create(lv_screen_active());
		lv_obj_set_size(restart_button, 130, 40);
		lv_obj_align(restart_button, LV_ALIGN_BOTTOM_MID, 0, -10);
		lv_obj_add_event_cb(restart_button, lv_btn_click_callback, LV_EVENT_CLICKED, NULL);

		button_label = lv_label_create(restart_button);
		lv_label_set_text(button_label, "Restart");
		lv_obj_center(button_label);
	}

	enter_state(APP_STATE_STAND);
	last_tick = k_uptime_get_32();
	active_press_ts = last_tick;

	lv_timer_handler();
	ret = display_blanking_off(display_dev);
	if (ret < 0 && ret != -ENOSYS) {
		LOG_ERR("Failed to turn blanking off (error %d)", ret);
		return 0;
	}

	while (1) {
		uint32_t now = k_uptime_get_32();
		release_used_for_arm = false;

		press_evt = button_press_evt;
		release_evt = button_release_evt;
		press_ts = button_press_ts;
		release_ts = button_release_ts;
		button_press_evt = false;
		button_release_evt = false;

		if (press_evt) {
			active_press_ts = press_ts;
		}

		if ((state == APP_STATE_STAND || state == APP_STATE_DEFUSED) && release_evt) {
			if (suppress_next_release_arm) {
				suppress_next_release_arm = false;
			} else {
				enter_state(APP_STATE_COUNTDOWN);
				last_tick = now;
				release_used_for_arm = true;
			}
		}

		if (state == APP_STATE_COUNTDOWN && button_down) {
			held_ms = now - active_press_ts;
			update_defuse_progress(held_ms, true);
			if (held_ms >= DEFUSE_HOLD_MS) {
				suppress_next_release_arm = true;
				enter_state(APP_STATE_DEFUSED);
			}
		}

		if (state == APP_STATE_COUNTDOWN && release_evt && !release_used_for_arm) {
			held_ms = release_ts - active_press_ts;
			update_defuse_progress(0U, false);
			if (held_ms < DEFUSE_HOLD_MS) {
				enter_state(APP_STATE_ZEROED);
			}
		}

		if (state != APP_STATE_COUNTDOWN && !button_down) {
			update_defuse_progress(0U, false);
		}

		if (state == APP_STATE_ZEROED && button_down) {
			held_ms = now - active_press_ts;
			if (held_ms >= RETURN_STAND_HOLD_MS) {
				enter_state(APP_STATE_STAND);
			}
		}

		if (state == APP_STATE_COUNTDOWN && (now - last_tick) >= 1000U) {
			last_tick += 1000U;
			if (seconds_left > 0U) {
				seconds_left--;
				if (seconds_left > 0U) {
					second_beep_pattern(seconds_left);
				} else {
					enter_state(APP_STATE_ZEROED);
					(void)beep_ms(4800U, LED_COLOR_RED);
					set_led_color(LED_COLOR_RED);
				}
				update_countdown_progress();
				update_screen();
			}
		}

		lv_timer_handler();
		k_sleep(K_MSEC(10));
	}
}
