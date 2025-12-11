/*
 * Copyright (c) 2024 Alexandre Bailon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef COAP_BUTTON_H
#define COAP_BUTTON_H

#include <zephyr/drivers/gpio.h>
#include <zephyr/data/json.h>

#define BTN_URI "sw"

#define BTN_MSG_STATE_OFF 0
#define BTN_MSG_STATE_ON  1

#define JSON_MAX_BTN 4

struct json_btn_state {
	int btn_id;
	int state;
};

struct json_btn_get {
	const char *device_id;
	struct json_btn_state btns[JSON_MAX_BTN];
	int count;
};

int button_init(const struct gpio_dt_spec *gpio);
int coap_btn_get_state(const char *addr, int led_id, int *state);

#ifdef CONFIG_OT_COAP_SAMPLE_SERVER
void coap_btn_reg_rsc(void);
#endif

#endif /* COAP_BUTTON_H */
