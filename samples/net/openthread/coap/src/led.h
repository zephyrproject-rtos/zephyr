/*
 * Copyright (c) 2024 Alexandre Bailon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef COAP_LED_H
#define COAP_LED_H

#include <zephyr/data/json.h>

#define LED_URI "led"

#define LED_MSG_STATE_OFF    0
#define LED_MSG_STATE_ON     1
#define LED_MSG_STATE_TOGGLE 2

#define JSON_MAX_LED 4

struct json_led_state {
	int led_id;
	int state;
};

struct json_led_get {
	const char *device_id;
	struct json_led_state leds[JSON_MAX_LED];
	int count;
};

int coap_led_set_state(const char *addr, int led_id, int state);
int coap_led_get_state(const char *addr, int led_id, int *state);

#ifdef CONFIG_OT_COAP_SAMPLE_SERVER
void coap_led_reg_rsc(void);
#endif /* CONFIG_OT_COAP_SAMPLE_SERVER */

#endif /* COAP_LED_H */
