/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/input/input.h>
#include <zephyr/input/input_renesas_ra_ctsu.h>
#include <zephyr/sys/printk.h>

#ifdef CONFIG_INPUT_RENESAS_RA_QE_TOUCH_CFG
#include "qe_touch_config.h"
#endif /* CONFIG_INPUT_RENESAS_RA_QE_TOUCH_CFG*/

#define BUTTON1 DT_NODELABEL(rtk0eg0019b01002bj_bt1)
#define BUTTON2 DT_NODELABEL(rtk0eg0019b01002bj_bt2)
#define BUTTON3 DT_NODELABEL(rtk0eg0019b01002bj_bt3)
#define SLIDER  DT_NODELABEL(rtk0eg0019b01002bj_slider)
#define WHEEL   DT_NODELABEL(rtk0eg0019b01002bj_wheel)

#define QE_TOUCH_CFG1 DT_CHILD(DT_NODELABEL(rtk0eg0019b01002bj_ctsu), group1)
#define QE_TOUCH_CFG2 DT_CHILD(DT_NODELABEL(rtk0eg0019b01002bj_ctsu), group2)
#define QE_TOUCH_CFG3 DT_CHILD(DT_NODELABEL(rtk0eg0019b01002bj_ctsu), group3)

#define NUM_ROWS DT_PROP_LEN(DT_PATH(zephyr_user), rtk0eg0019b01002bj_led_row_gpios)
#define NUM_COLS DT_PROP_LEN(DT_PATH(zephyr_user), rtk0eg0019b01002bj_led_col_gpios)

#define LED_MATRIX_ROWS(idx, nodeid)                                                               \
	GPIO_DT_SPEC_GET_BY_IDX(nodeid, rtk0eg0019b01002bj_led_row_gpios, idx)
#define LED_MATRIX_COLS(idx, nodeid)                                                               \
	GPIO_DT_SPEC_GET_BY_IDX(nodeid, rtk0eg0019b01002bj_led_col_gpios, idx)

#define BUTTON_LED_NUM (3U)

#define SLIDER_LED_NUM    (5U)
#define SLIDER_RESOLUTION (100)

#define WHEEL_LED_NUM           (8U)
#define WHEEL_RESOLUTION_DEGREE (360)

static const struct gpio_dt_spec led_row[NUM_ROWS] = {
	LISTIFY(NUM_ROWS, LED_MATRIX_ROWS, (,), DT_PATH(zephyr_user)),
};

static const struct gpio_dt_spec led_col[NUM_COLS] = {
	LISTIFY(NUM_COLS, LED_MATRIX_COLS, (,), DT_PATH(zephyr_user)),
};

static const unsigned int wheel_leds_lut[WHEEL_LED_NUM] = {0, 1, 2, 3, 4, 5, 6, 7};
static const unsigned int slider_leds_lut[SLIDER_LED_NUM] = {8, 9, 10, 11, 12};
static const unsigned int button_leds_lut[BUTTON_LED_NUM] = {13, 14, 15};

static void rtk0eg0019b01002bj_led_output(unsigned int led_idx, bool on)
{
	/* Refer to RTK0EG0022S01001BJ - Design Package for the Touch Application board layout */
	static const unsigned int led_map[NUM_ROWS * NUM_COLS][2] = {
		{0, 0}, {1, 0}, {2, 0}, {3, 0}, {0, 1}, {1, 1}, {2, 1}, {3, 1},
		{0, 3}, {1, 3}, {2, 3}, {3, 3}, {0, 2}, {1, 2}, {2, 2}, {3, 2},
	};
	const struct gpio_dt_spec *p_led_row = &led_row[led_map[led_idx][0]];
	const struct gpio_dt_spec *p_led_col = &led_col[led_map[led_idx][1]];

	gpio_pin_set_dt(p_led_row, on ? 1 : 0);
	gpio_pin_set_dt(p_led_col, on ? 1 : 0);
}

static void rtk0eg0019b01002bj_led_init(void)
{
	for (int i = 0; i < NUM_ROWS; i++) {
		gpio_pin_configure_dt(&led_row[i], GPIO_OUTPUT_INACTIVE);
	}

	for (int i = 0; i < NUM_COLS; i++) {
		gpio_pin_configure_dt(&led_col[i], GPIO_OUTPUT_INACTIVE);
	}
}

static void blink_leds(unsigned int duration_ms)
{
	for (int i = 0; i < 5; i++) {
		/* Turn all LEDs ON */
		for (int j = 0; j < NUM_ROWS * NUM_COLS; j++) {
			rtk0eg0019b01002bj_led_output(j, true);
		}
		k_msleep(duration_ms);

		/* Turn all LEDs OFF */
		for (int j = 0; j < NUM_ROWS * NUM_COLS; j++) {
			rtk0eg0019b01002bj_led_output(j, false);
		}
		k_msleep(duration_ms);
	}
}

static inline unsigned int wheel_get_current_step(unsigned int value)
{
	return (value * WHEEL_LED_NUM) / WHEEL_RESOLUTION_DEGREE;
}

static inline unsigned int slider_get_current_step(unsigned int value)
{
	return (value * SLIDER_LED_NUM) / SLIDER_RESOLUTION;
}

static void rtk0eg0019b01002bj_evt_handler(struct input_event *evt, void *user_data)
{
	unsigned int led_idx;

	/* Set all LEDs to OFF */
	for (int i = 0; i < NUM_ROWS * NUM_COLS; i++) {
		rtk0eg0019b01002bj_led_output(i, false);
	}

	switch (evt->code) {
	case INPUT_KEY_0: {
		led_idx = button_leds_lut[0];
		break;
	}
	case INPUT_KEY_1: {
		led_idx = button_leds_lut[1];
		break;
	}
	case INPUT_KEY_2: {
		led_idx = button_leds_lut[2];
		break;
	}
	case INPUT_ABS_WHEEL: {
		led_idx = wheel_get_current_step(evt->value) + wheel_leds_lut[0];
		break;
	}
	case INPUT_ABS_THROTTLE: {
		led_idx = slider_get_current_step(evt->value) + slider_leds_lut[0];
		break;
	}
	default:
		/* Unexpected event */
		return;
	}

	rtk0eg0019b01002bj_led_output(led_idx, true);
}

INPUT_CALLBACK_DEFINE_NAMED(DEVICE_DT_GET(BUTTON1), rtk0eg0019b01002bj_evt_handler, NULL, button1);
INPUT_CALLBACK_DEFINE_NAMED(DEVICE_DT_GET(BUTTON2), rtk0eg0019b01002bj_evt_handler, NULL, button2);
INPUT_CALLBACK_DEFINE_NAMED(DEVICE_DT_GET(BUTTON3), rtk0eg0019b01002bj_evt_handler, NULL, button3);
INPUT_CALLBACK_DEFINE_NAMED(DEVICE_DT_GET(SLIDER), rtk0eg0019b01002bj_evt_handler, NULL, slider);
INPUT_CALLBACK_DEFINE_NAMED(DEVICE_DT_GET(WHEEL), rtk0eg0019b01002bj_evt_handler, NULL, wheel);

int main(void)
{
#ifdef CONFIG_INPUT_RENESAS_RA_QE_TOUCH_CFG
	const struct device *qe_touch_cfg1 = DEVICE_DT_GET(QE_TOUCH_CFG1);
	const struct device *qe_touch_cfg2 = DEVICE_DT_GET(QE_TOUCH_CFG2);
	const struct device *qe_touch_cfg3 = DEVICE_DT_GET(QE_TOUCH_CFG3);
	int ret;
#endif
	rtk0eg0019b01002bj_led_init();

	/* Blink all leds 5 times, each time is 200ms */
	blink_leds(200);

#ifdef CONFIG_INPUT_RENESAS_RA_QE_TOUCH_CFG
	ret = renesas_ra_ctsu_group_configure(
		qe_touch_cfg1, (struct renesas_ra_ctsu_touch_cfg *)&g_qe_touch_instance_config01);
	if (ret < 0) {
		printk("Failed to configure QE Touch Group 1: %d\n", ret);
		return ret;
	}

	ret = renesas_ra_ctsu_group_configure(
		qe_touch_cfg2, (struct renesas_ra_ctsu_touch_cfg *)&g_qe_touch_instance_config02);
	if (ret < 0) {
		printk("Failed to configure QE Touch Group 2: %d\n", ret);
		return ret;
	}

	ret = renesas_ra_ctsu_group_configure(
		qe_touch_cfg3, (struct renesas_ra_ctsu_touch_cfg *)&g_qe_touch_instance_config03);
	if (ret < 0) {
		printk("Failed to configure QE Touch Group 3: %d\n", ret);
		return ret;
	}
#endif

	printk("rtk0eg0019b01002bj sample started\n");
	return 0;
}
