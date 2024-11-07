/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "button_handler.h"
#include "button_assignments.h"

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/shell/shell.h>
#include <zephyr/zbus/zbus.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "macros_common.h"
#include "nrf5340_audio_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(button_handler, CONFIG_MODULE_BUTTON_HANDLER_LOG_LEVEL);

ZBUS_CHAN_DEFINE(button_chan, struct button_msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
		 ZBUS_MSG_INIT(0));

/* How many buttons does the module support. Increase at memory cost */
#define BUTTONS_MAX 5
#define BASE_10	    10

/* Only allow one button msg at a time, as a mean of debounce */
K_MSGQ_DEFINE(button_queue, sizeof(struct button_msg), 1, 4);

static bool debounce_is_ongoing;
static struct gpio_callback btn_callback[BUTTONS_MAX];

/* clang-format off */
const static struct btn_config btn_cfg[] = {
	{
		.btn_name = STRINGIFY(BUTTON_VOLUME_DOWN),
		.btn_pin = BUTTON_VOLUME_DOWN,
		.btn_cfg_mask = DT_GPIO_FLAGS(DT_ALIAS(sw0), gpios),
	},
	{
		.btn_name = STRINGIFY(BUTTON_VOLUME_UP),
		.btn_pin = BUTTON_VOLUME_UP,
		.btn_cfg_mask = DT_GPIO_FLAGS(DT_ALIAS(sw1), gpios),
	},
	{
		.btn_name = STRINGIFY(BUTTON_PLAY_PAUSE),
		.btn_pin = BUTTON_PLAY_PAUSE,
		.btn_cfg_mask = DT_GPIO_FLAGS(DT_ALIAS(sw2), gpios),
	},
	{
		.btn_name = STRINGIFY(BUTTON_4),
		.btn_pin = BUTTON_4,
		.btn_cfg_mask = DT_GPIO_FLAGS(DT_ALIAS(sw3), gpios),
	},
	{
		.btn_name = STRINGIFY(BUTTON_5),
		.btn_pin = BUTTON_5,
		.btn_cfg_mask = DT_GPIO_FLAGS(DT_ALIAS(sw4), gpios),
	}
};
/* clang-format on */

static const struct device *gpio_53_dev;

/**@brief Simple debouncer for buttons
 *
 * @note Needed as low-level driver debounce is not
 * implemented in Zephyr for nRF53 yet
 */
static void on_button_debounce_timeout(struct k_timer *timer)
{
	debounce_is_ongoing = false;
}

K_TIMER_DEFINE(button_debounce_timer, on_button_debounce_timeout, NULL);

/** @brief Find the index of a button from the pin number
 */
static int pin_to_btn_idx(uint8_t btn_pin, uint32_t *pin_idx)
{
	for (uint8_t i = 0; i < ARRAY_SIZE(btn_cfg); i++) {
		if (btn_pin == btn_cfg[i].btn_pin) {
			*pin_idx = i;
			return 0;
		}
	}

	LOG_WRN("Button idx not found");
	return -ENODEV;
}

/** @brief Convert from mask to pin
 *
 * @note: Will check that a single bit and a single bit only is set in the mask.
 */
static int pin_msk_to_pin(uint32_t pin_msk, uint32_t *pin_out)
{
	if (!pin_msk) {
		LOG_ERR("Mask is empty");
		return -EACCES;
	}

	if (pin_msk & (pin_msk - 1)) {
		LOG_ERR("Two or more buttons set in mask");
		return -EACCES;
	}

	*pin_out = 0;

	while (pin_msk) {
		pin_msk = pin_msk >> 1;
		(*pin_out)++;
	}

	/* Deduct 1 for zero indexing */
	(*pin_out)--;

	return 0;
}

static void button_publish_thread(void)
{
	int ret;
	struct button_msg msg;

	while (1) {
		k_msgq_get(&button_queue, &msg, K_FOREVER);

		ret = zbus_chan_pub(&button_chan, &msg, K_NO_WAIT);
		if (ret) {
			LOG_ERR("Failed to publish button msg, ret: %d", ret);
		}
	}
}

K_THREAD_DEFINE(button_publish, CONFIG_BUTTON_PUBLISH_STACK_SIZE, button_publish_thread, NULL, NULL,
		NULL, K_PRIO_PREEMPT(CONFIG_BUTTON_PUBLISH_THREAD_PRIO), 0, 0);

/*  ISR triggered by GPIO when assigned button(s) are pushed */
static void button_isr(const struct device *port, struct gpio_callback *cb, uint32_t pin_msk)
{
	int ret;
	struct button_msg msg;

	if (debounce_is_ongoing) {
		LOG_DBG("Btn debounce in action");
		return;
	}

	uint32_t btn_pin = 0;
	uint32_t btn_idx = 0;

	ret = pin_msk_to_pin(pin_msk, &btn_pin);
	ERR_CHK(ret);

	ret = pin_to_btn_idx(btn_pin, &btn_idx);
	ERR_CHK(ret);

	LOG_DBG("Pushed button idx: %d pin: %d name: %s", btn_idx, btn_pin,
		btn_cfg[btn_idx].btn_name);

	msg.button_pin = btn_pin;
	msg.button_action = BUTTON_PRESS;

	ret = k_msgq_put(&button_queue, (void *)&msg, K_NO_WAIT);
	if (ret == -EAGAIN) {
		LOG_WRN("Btn msg queue full");
	}

	debounce_is_ongoing = true;
	k_timer_start(&button_debounce_timer, K_MSEC(CONFIG_BUTTON_DEBOUNCE_MS), K_NO_WAIT);
}

int button_pressed(gpio_pin_t button_pin, bool *button_pressed)
{
	int ret;

	if (!device_is_ready(gpio_53_dev)) {
		return -ENODEV;
	}

	if (button_pressed == NULL) {
		return -EINVAL;
	}

	ret = gpio_pin_get(gpio_53_dev, button_pin);
	switch (ret) {
	case 0:
		*button_pressed = false;
		break;
	case 1:
		*button_pressed = true;
		break;
	default:
		return ret;
	}

	return 0;
}

int button_handler_init(void)
{
	int ret;

	if (ARRAY_SIZE(btn_cfg) == 0) {
		LOG_WRN("No buttons assigned");
		return -EINVAL;
	}

	gpio_53_dev = DEVICE_DT_GET(DT_NODELABEL(gpio0));

	if (!device_is_ready(gpio_53_dev)) {
		LOG_ERR("Device driver not ready.");
		return -ENODEV;
	}

	for (uint8_t i = 0; i < ARRAY_SIZE(btn_cfg); i++) {
		ret = gpio_pin_configure(gpio_53_dev, btn_cfg[i].btn_pin,
					 GPIO_INPUT | btn_cfg[i].btn_cfg_mask);
		if (ret) {
			return ret;
		}

		gpio_init_callback(&btn_callback[i], button_isr, BIT(btn_cfg[i].btn_pin));

		ret = gpio_add_callback(gpio_53_dev, &btn_callback[i]);
		if (ret) {
			return ret;
		}

		ret = gpio_pin_interrupt_configure(gpio_53_dev, btn_cfg[i].btn_pin,
						   GPIO_INT_EDGE_TO_INACTIVE);
		if (ret) {
			return ret;
		}
	}

	return 0;
}
