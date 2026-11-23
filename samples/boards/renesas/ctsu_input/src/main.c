/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/kernel.h>
#include <zephyr/input/input.h>
#include <zephyr/input/input_renesas_rx_ctsu.h>

#ifdef CONFIG_INPUT_RENESAS_RX_QE_TOUCH_CFG
#include "qe_touch_config.h"
#endif /* CONFIG_INPUT_RENESAS_RX_QE_TOUCH_CFG */

static const struct device *const test_touch_dev = DEVICE_DT_GET(DT_NODELABEL(ctsu));
#define BUTTON_1_IDX DT_NODE_CHILD_IDX(DT_NODELABEL(onboard_button_1))
#define BUTTON_2_IDX DT_NODE_CHILD_IDX(DT_NODELABEL(onboard_button_2))
#define SLIDER_IDX   DT_NODE_CHILD_IDX(DT_NODELABEL(onboard_slider))

#define STACKSIZE    1024
#define PRIORITY     7
#define LED0_NODE    DT_NODELABEL(testled0)
#define LED1_NODE    DT_NODELABEL(testled1)
#define LED2_NODE    DT_NODELABEL(testled2)
#define LED3_NODE    DT_NODELABEL(testled3)
#define NUM_TEST_LED 4

#define LED_ON  1
#define LED_OFF 0

enum {
	TOUCH_BUTTON_1 = 10,
	TOUCH_BUTTON_2 = 11,
	TOUCH_SLIDER = 1,
};

static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
static const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(LED2_NODE, gpios);
static const struct gpio_dt_spec led3 = GPIO_DT_SPEC_GET(LED3_NODE, gpios);
static const struct gpio_dt_spec *leds[] = {&led0, &led1, &led2, &led3};
static volatile int led_state;
static volatile int leds_on_off[NUM_TEST_LED] = {0};
static volatile int current_main_led;

static volatile int event_count[DT_CHILD_NUM_STATUS_OKAY(DT_NODELABEL(ctsu))] = {0};
static uint16_t last_code;
static volatile int32_t last_val[DT_CHILD_NUM_STATUS_OKAY(DT_NODELABEL(ctsu))];
static struct k_sem btn1_sem;
static struct k_sem btn2_sem;
static struct k_sem sldr_sem;
static struct k_sem render_sem;

static void sample_setup(void)
{
	gpio_pin_configure_dt(&led0, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&led1, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&led2, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&led3, GPIO_OUTPUT_INACTIVE);

	k_sem_init(&btn1_sem, 0, 1);
	k_sem_init(&btn2_sem, 0, 1);
	k_sem_init(&sldr_sem, 0, 1);
	k_sem_init(&render_sem, 0, 1);

#ifdef CONFIG_INPUT_RENESAS_RX_QE_TOUCH_CFG
	int ret = renesas_rx_ctsu_group_configure(
		test_touch_dev, (struct renesas_rx_ctsu_touch_cfg *)&g_qe_touch_instance_config01);

	if (ret < 0) {
		printk("Failed to configure QE Touch: %d\n", ret);
		return;
	}
#endif /* CONFIG_INPUT_RENESAS_RX_QE_TOUCH_CFG */
}

static void button1_task(void)
{
	while (1) {
		k_sem_take(&btn1_sem, K_FOREVER);
		if (led_state == 0) {
			/** Current leds are OFF, do nothing */
		} else {
			if (led_state >= 15) {
				led_state = 1;
			} else {
				led_state++;
			}
		}
		k_sem_give(&render_sem);
	}
}

static void button2_task(void)
{
	int prev_state = 0;

	while (1) {
		k_sem_take(&btn2_sem, K_FOREVER);
		if (led_state == 0) {
			/** Current led state is OFF */
			if (prev_state == 0) {
				/** First time power on */
				led_state = 1;
			} else {
				/** Restore prev_state */
				led_state = prev_state;
			}
		} else {
			prev_state = led_state;
			led_state = 0;
		}
		k_sem_give(&render_sem);
	}
}

static void slider_task(void)
{
	while (1) {
		k_sem_take(&sldr_sem, K_FOREVER);
		if (led_state == 0) {
			continue;
		}
		int curr_val = last_val[SLIDER_IDX];

		if (curr_val != TOUCH_OFF_VALUE) {
			if (curr_val == 100) {
				curr_val = 99;
			}
			current_main_led = 3 - (curr_val / 25);
		}
		k_sem_give(&render_sem);
	}
}

static void render_task(void)
{
	while (1) {
		k_sem_take(&render_sem, K_FOREVER);
		for (int i = 0; i < NUM_TEST_LED; i++) {
			if (led_state & BIT(i)) {
				gpio_pin_set_dt(leds[(current_main_led + i) % 4], LED_ON);
			} else {
				gpio_pin_set_dt(leds[(current_main_led + i) % 4], LED_OFF);
			}
		}
	}
}

static void test_touch_keys_cb_handler(struct input_event *evt, void *user_data)
{
	switch (evt->code) {
	case TOUCH_BUTTON_1:
		if (evt->value == 0) {
			printk("Button 1 released\n");
			if (last_val[BUTTON_1_IDX] != 0) {
				k_sem_give(&btn1_sem);
			}
		} else {
			event_count[BUTTON_1_IDX]++;
			printk("Button 1 pressed %d time(s)\n", event_count[BUTTON_1_IDX]);
		}
		last_val[BUTTON_1_IDX] = evt->value;
		break;

	case TOUCH_BUTTON_2:
		if (evt->value == 0) {
			printk("Button 2 released\n");
			if (last_val[BUTTON_2_IDX] != 0) {
				k_sem_give(&btn2_sem);
			}
		} else {
			event_count[BUTTON_2_IDX]++;
			printk("Button 2 pressed %d time(s)\n", event_count[BUTTON_2_IDX]);
		}
		last_val[BUTTON_2_IDX] = evt->value;
		break;

	case TOUCH_SLIDER:
		if (evt->value == TOUCH_OFF_VALUE) {
			printk("Slider released\n");
			if (last_val[SLIDER_IDX] != TOUCH_OFF_VALUE) {
			}
		} else {
			if (last_val[SLIDER_IDX] == TOUCH_OFF_VALUE) {
				event_count[SLIDER_IDX]++;
				printk("Slider pressed\n");
			}
			printk("Position: %d\n", evt->value);
		}
		k_sem_give(&sldr_sem);
		last_val[SLIDER_IDX] = evt->value;
		break;

	default:
		break;
	}
	last_code = evt->code;
}
INPUT_CALLBACK_DEFINE(test_touch_dev, test_touch_keys_cb_handler, NULL);

K_THREAD_DEFINE(button1_id, STACKSIZE, button1_task, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(button2_id, STACKSIZE, button2_task, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(slider_id, STACKSIZE, slider_task, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(render_id, STACKSIZE, render_task, NULL, NULL, NULL, PRIORITY, 0, 0);

int main(void)
{
	sample_setup();
	printk("On board CTSU components sample started!\n");
	printk("Press on touch nodes for sample\n");
	return 0;
}
