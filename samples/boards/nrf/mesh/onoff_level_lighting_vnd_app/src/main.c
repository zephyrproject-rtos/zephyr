/* Bluetooth: Mesh Generic OnOff, Generic Level, Lighting & Vendor Models
 *
 * Copyright (c) 2018 Vikrant More
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>

#include "app_gpio.h"
#include "ble_mesh.h"
#include "device_composition.h"
#include "no_transition_work_handler.h"
#include "state_binding.h"
#include "storage.h"
#include "transition.h"

static bool reset;

static void light_default_var_init(void)
{
	ctl->tt = 0x00;

	ctl->onpowerup = STATE_DEFAULT;

	ctl->light->range_min = LIGHTNESS_MIN;
	ctl->light->range_max = LIGHTNESS_MAX;
	ctl->light->last = LIGHTNESS_MAX;
	ctl->light->def = LIGHTNESS_MAX;
	ctl->light->target = ctl->light->def;

	ctl->temp->range_min = TEMP_MIN;
	ctl->temp->range_max = TEMP_MAX;
	ctl->temp->def = TEMP_MAX;
	ctl->temp->target = ctl->temp->def;

	ctl->duv->def = DELTA_UV_DEF;
	ctl->duv->target = ctl->duv->def;
}

/* This function should only get call after execution of settings_load() */
static void light_default_status_init(void)
{
	/* Retrieve Range of Lightness */
	if (ctl->light->range) {
		ctl->light->range_max = (uint16_t) (ctl->light->range >> 16);
		ctl->light->range_min = (uint16_t) ctl->light->range;
	}

	/* Retrieve Range of Temperature */
	if (ctl->temp->range) {
		ctl->temp->range_max = (uint16_t) (ctl->temp->range >> 16);
		ctl->temp->range_min = (uint16_t) ctl->temp->range;
	}

	ctl->light->last = constrain_lightness(ctl->light->last);
	ctl->light->def = constrain_lightness(ctl->light->def);
	ctl->light->target = constrain_lightness(ctl->light->target);

	ctl->temp->def = constrain_temperature(ctl->temp->def);
	ctl->temp->target = constrain_temperature(ctl->temp->target);

	ctl->temp->current = ctl->temp->def;
	ctl->duv->current = ctl->duv->def;

	switch (ctl->onpowerup) {
	case STATE_OFF:
		ctl->light->current = 0U;
		break;
	case STATE_DEFAULT:
		if (ctl->light->def == 0U) {
			ctl->light->current = ctl->light->last;
		} else {
			ctl->light->current = ctl->light->def;
		}
		break;
	case STATE_RESTORE:
		ctl->light->current = ctl->light->target;
		ctl->temp->current = ctl->temp->target;
		ctl->duv->current = ctl->duv->target;
		break;
	}

	ctl->light->target = ctl->light->current;
	ctl->temp->target = ctl->temp->current;
	ctl->duv->target = ctl->duv->current;
}

void update_vnd_led_gpio(void)
{
#ifndef ONE_LED_ONE_BUTTON_BOARD
	gpio_pin_set_dt(&led_device[1], vnd_user_data.current == STATE_ON);
#endif
}

void update_led_gpio(void)
{
	uint8_t power, color;

	power = 100 * ((float) ctl->light->current / 65535);
	color = 100 * ((float) (ctl->temp->current - ctl->temp->range_min) /
		       (ctl->temp->range_max - ctl->temp->range_min));

	printk("power-> %d, color-> %d\n", power, color);

	gpio_pin_set_dt(&led_device[0], ctl->light->current);
#ifndef ONE_LED_ONE_BUTTON_BOARD
	gpio_pin_set_dt(&led_device[2], power < 50);
	gpio_pin_set_dt(&led_device[3], color < 50);
#endif
}

void update_light_state(void)
{
	update_led_gpio();

	if (ctl->transition->counter == 0 || reset == false) {
		reset = true;
		k_work_submit(&no_transition_work);
	}
}

static void short_time_multireset_bt_mesh_unprovisioning(void)
{
	if (reset_counter >= 4U) {
		reset_counter = 0U;
		printk("BT Mesh reset\n");
		bt_mesh_reset();
	} else {
		printk("Reset Counter -> %d\n", reset_counter);
		reset_counter++;
	}

	save_on_flash(RESET_COUNTER);
}

static void reset_counter_timer_handler(struct k_timer *dummy)
{
	reset_counter = 0U;
	save_on_flash(RESET_COUNTER);
	printk("Reset Counter set to Zero\n");
}

K_TIMER_DEFINE(reset_counter_timer, reset_counter_timer_handler, NULL);

void main(void)
{
	int err;

	light_default_var_init();

	app_gpio_init();

	printk("Initializing...\n");

	ps_settings_init();

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	bt_ready();

	light_default_status_init();

	update_light_state();

	short_time_multireset_bt_mesh_unprovisioning();
	k_timer_start(&reset_counter_timer, K_MSEC(7000), K_NO_WAIT);
}
