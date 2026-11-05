/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/drivers/led.h>

#include "canopen_internal.h"

#define LED_TICK_MS 200U
#define LED_RUN     BIT(0)
#define LED_ERR     BIT(1)
#define LED_UNSET   0xFFU

enum led_pattern {
	LED_OFF,
	LED_ON,
	LED_BLINK,
	LED_FLASH1,
	LED_FLASH2,
	LED_FLASH3,
	LED_FLASH4,
};

static uint8_t led_length(uint8_t pattern)
{
	if (pattern == LED_BLINK) {
		return 2U;
	}

	if (pattern >= LED_FLASH1) {
		return (2U * (pattern - LED_FLASH1 + 1U)) + 4U;
	}

	return 1U;
}

static bool led_lit(uint8_t pattern, uint8_t phase)
{
	if (pattern == LED_ON) {
		return true;
	}

	if (pattern == LED_BLINK) {
		return phase == 0U;
	}

	if (pattern >= LED_FLASH1) {
		return (phase < (2U * (pattern - LED_FLASH1 + 1U))) && ((phase % 2U) == 0U);
	}

	return false;
}

static uint8_t led_run_pattern(const struct canopen_node *node)
{
	switch (node->nmt_state) {
	case CANOPEN_NMT_PRE_OPERATIONAL:
		return LED_BLINK;
	case CANOPEN_NMT_OPERATIONAL:
		return LED_ON;
	case CANOPEN_NMT_STOPPED:
		return LED_FLASH1;
	default:
		return LED_OFF;
	}
}

static uint8_t led_err_pattern(struct canopen_node *node)
{
	if (node->can_state_last == CAN_STATE_BUS_OFF) {
		return LED_ON;
	}

	if (canopen_emcy_active(node, CANOPEN_EMCY_HEARTBEAT)) {
		return LED_FLASH2;
	}

	if (node->sync.timed_out) {
		return LED_FLASH3;
	}

	if (canopen_emcy_active(node, CANOPEN_EMCY_RPDO_TIMEOUT)) {
		return LED_FLASH4;
	}

	if ((node->can_state_last == CAN_STATE_ERROR_WARNING) ||
	    (node->can_state_last == CAN_STATE_ERROR_PASSIVE)) {
		return LED_FLASH1;
	}

	return LED_OFF;
}

static void led_set(const struct led_dt_spec *spec, bool on)
{
	if (spec != NULL) {
		(void)(on ? led_on_dt(spec) : led_off_dt(spec));
	}
}

static void leds_apply(struct canopen_node *node)
{
	struct canopen_leds *leds = &node->leds;
	bool err = led_lit(leds->err, leds->err_phase);
	bool run = led_lit(leds->run, leds->run_phase) && !(node->cfg->led_bicolor && err);
	uint8_t on = (run ? LED_RUN : 0U) | (err ? LED_ERR : 0U);
	uint8_t changed = on ^ leds->on;

	if ((changed & LED_RUN) != 0U) {
		led_set(node->cfg->led_run, run);
	}

	if ((changed & LED_ERR) != 0U) {
		led_set(node->cfg->led_err, err);
	}

	leds->on = on;
}

int canopen_leds_init(struct canopen_node *node)
{
	const struct canopen_node_config *cfg = node->cfg;

	if (((cfg->led_run != NULL) && !led_is_ready_dt(cfg->led_run)) ||
	    ((cfg->led_err != NULL) && !led_is_ready_dt(cfg->led_err))) {
		return -ENODEV;
	}

	led_set(cfg->led_run, false);
	led_set(cfg->led_err, false);
	node->leds.run = LED_UNSET;
	node->leds.err = LED_UNSET;
	node->leds.on = 0U;
	node->leds.next = sys_timepoint_calc(K_FOREVER);

	return 0;
}

void canopen_leds_process(struct canopen_node *node)
{
	struct canopen_leds *leds = &node->leds;
	uint8_t run = led_run_pattern(node);
	uint8_t err = led_err_pattern(node);
	bool changed = false;

	if ((node->cfg->led_run == NULL) && (node->cfg->led_err == NULL)) {
		return;
	}

	if (run != leds->run) {
		leds->run = run;
		leds->run_phase = 0U;
		changed = true;
	}

	if (err != leds->err) {
		leds->err = err;
		leds->err_phase = 0U;
		changed = true;
	}

	if (!changed && sys_timepoint_expired(leds->next)) {
		leds->run_phase = (leds->run_phase + 1U) % led_length(leds->run);
		leds->err_phase = (leds->err_phase + 1U) % led_length(leds->err);
		changed = true;
	}

	if (changed) {
		leds->next = sys_timepoint_calc(K_MSEC(LED_TICK_MS));
		leds_apply(node);
	}
}

k_timepoint_t canopen_leds_deadline(struct canopen_node *node)
{
	const struct canopen_leds *leds = &node->leds;

	if ((led_length(leds->run) == 1U) && (led_length(leds->err) == 1U)) {
		return sys_timepoint_calc(K_FOREVER);
	}

	return leds->next;
}
