/*
 * Copyright (c) 2026 BayLibre SAS
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/led/status_led.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(status_led_sample, LOG_LEVEL_INF);

#define LED0_NODE DT_ALIAS(led0)
#define SW0_NODE  DT_ALIAS(sw0)

#define LONG_PRESS_MS       1000
#define FAKE_ERROR_DELAY_MS 5000

enum sample_status {
	STATUS_WAITING,
	STATUS_ERROR,
};

static const struct status_led_anim sample_statuses[] = {
	/* Slow blink while waiting for the fake operation to complete */
	STATUS_LED_BLINK_ANIM(STATUS_WAITING, 1, 500, 500),
	/* Double flash until the error is cleared, over the waiting status */
	STATUS_LED_ANIM(STATUS_ERROR, 2, LED_ANIM_INFINITE, 100, 100, 100, 700),
};

STATUS_LED_DEFINE(sample_status_led, DEVICE_DT_GET(DT_PARENT(LED0_NODE)),
		  DT_NODE_CHILD_IDX(LED0_NODE), true, sample_statuses);

enum sample_state {
	STATE_IDLE,
	STATE_WAITING,
	STATE_ERROR,
};

enum button_press {
	SHORT_PRESS,
	LONG_PRESS,
};

K_MSGQ_DEFINE(press_msgq, sizeof(enum button_press), 4, 4);

static void button_input_cb(struct input_event *evt, void *user_data)
{
	static int64_t pressed_at;
	enum button_press press;

	ARG_UNUSED(user_data);

	if (evt->type != INPUT_EV_KEY || evt->code != DT_PROP(SW0_NODE, zephyr_code)) {
		return;
	}

	if (evt->value) {
		pressed_at = k_uptime_get();
		return;
	}

	press = k_uptime_get() - pressed_at >= LONG_PRESS_MS ? LONG_PRESS : SHORT_PRESS;
	if (k_msgq_put(&press_msgq, &press, K_NO_WAIT)) {
		LOG_WRN("Button press dropped");
	}
}

INPUT_CALLBACK_DEFINE(DEVICE_DT_GET(DT_PARENT(SW0_NODE)), button_input_cb, NULL);

static void status_set(enum sample_status status)
{
	int ret = status_led_set(&sample_status_led, status);

	if (ret) {
		LOG_ERR("Failed to set status %d: %d", status, ret);
	}
}

static void status_clear(enum sample_status status)
{
	int ret = status_led_clear(&sample_status_led, status);

	if (ret) {
		LOG_ERR("Failed to clear status %d: %d", status, ret);
	}
}

int main(void)
{
	enum sample_state state = STATE_IDLE;
	k_timepoint_t error_time = sys_timepoint_calc(K_FOREVER);
	enum button_press press;

	LOG_INF("Short press the button to start a fake operation");

	while (true) {
		if (k_msgq_get(&press_msgq, &press, sys_timepoint_timeout(error_time))) {
			LOG_INF("Fake error, long press the button to clear it");
			/* Keep the waiting status, reported again once the error is cleared */
			status_set(STATUS_ERROR);
			error_time = sys_timepoint_calc(K_FOREVER);
			state = STATE_ERROR;
			continue;
		}

		switch (state) {
		case STATE_IDLE:
			if (press == SHORT_PRESS) {
				LOG_INF("Waiting, short press the button to cancel");
				status_set(STATUS_WAITING);
				error_time = sys_timepoint_calc(K_MSEC(FAKE_ERROR_DELAY_MS));
				state = STATE_WAITING;
			}
			break;
		case STATE_WAITING:
			if (press == SHORT_PRESS) {
				LOG_INF("Canceled");
				status_clear(STATUS_WAITING);
				error_time = sys_timepoint_calc(K_FOREVER);
				state = STATE_IDLE;
			}
			break;
		case STATE_ERROR:
			if (press == LONG_PRESS) {
				LOG_INF("Error cleared, short press the button to cancel");
				status_clear(STATUS_ERROR);
				state = STATE_WAITING;
			}
			break;
		}
	}

	return 0;
}
