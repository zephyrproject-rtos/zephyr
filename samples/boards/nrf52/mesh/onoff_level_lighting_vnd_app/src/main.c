/* Bluetooth: Mesh Generic OnOff, Generic Level, Lighting & Vendor Models
 *
 * Copyright (c) 2018 Vikrant More
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/gpio.h>

#include "app_gpio.h"
#include "ble_mesh.h"
#include "device_composition.h"
#include "no_transition_work_handler.h"
#include "state_binding.h"
#include "storage.h"
#include "transition.h"

#if defined(CONFIG_MCUMGR)
#include <mgmt/smp_bt.h>
#include "smp_svr.h"
#endif

static bool reset;

static void light_default_var_init(void)
{
	ctl->tt = 0x00;

	ctl->onpowerup = STATE_DEFAULT;

	ctl->light->range_min = LIGHTNESS_MIN;
	ctl->light->range_max = LIGHTNESS_MAX;
	ctl->light->last = LIGHTNESS_MAX;
	ctl->light->def = LIGHTNESS_MAX;

	ctl->temp->range_min = TEMP_MIN;
	ctl->temp->range_max = TEMP_MAX;
	ctl->temp->def = TEMP_MAX;

	ctl->duv->def = DELTA_UV_DEF;

	ctl->light_temp_def = (u32_t) ((LIGHTNESS_MAX << 16) | TEMP_MAX);
	ctl->light_temp_last_tgt = (u32_t) ((LIGHTNESS_MAX << 16) | TEMP_MAX);
}

/* This function should only get call after reading persistent data storage */
static void light_default_status_init(void)
{
	u16_t light_def;

	/* Retrieve Range of Lightness */
	if (ctl->light->range) {
		ctl->light->range_max = (u16_t) (ctl->light->range >> 16);
		ctl->light->range_min = (u16_t) ctl->light->range;
	}

	/* Retrieve Range of Temperature */
	if (ctl->temp->range) {
		ctl->temp->range_max = (u16_t) (ctl->temp->range >> 16);
		ctl->temp->range_min = (u16_t) ctl->temp->range;
	}

	/* Retrieve Default Lightness Value */
	light_def = (u16_t) (ctl->light_temp_def >> 16);
	ctl->light->def = constrain_lightness(light_def);

	/* Retrieve Default Temperature Value */
	ctl->temp->def = (u16_t) ctl->light_temp_def;

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
		ctl->light->current = (u16_t) (ctl->light_temp_last_tgt >> 16);
		ctl->temp->current = (u16_t) ctl->light_temp_last_tgt;
		break;
	}

	default_tt = ctl->tt;

	ctl->light->target = ctl->light->current;
	ctl->temp->target = ctl->temp->current;
	ctl->duv->target = ctl->duv->current;
}

void update_led_gpio(void)
{
	u8_t power, color;

	power = 100 * ((float) ctl->light->current / 65535);
	color = 100 * ((float) (ctl->temp->current - ctl->temp->range_min) /
		       (ctl->temp->range_max - ctl->temp->range_min));

	printk("power-> %d, color-> %d\n", power, color);

	if (ctl->light->current) {
		/* LED1 On */
		gpio_pin_write(led_device[0], DT_ALIAS_LED0_GPIOS_PIN, 0);
	} else {
		/* LED1 Off */
		gpio_pin_write(led_device[0], DT_ALIAS_LED0_GPIOS_PIN, 1);
	}

	if (power < 50) {
		/* LED3 On */
		gpio_pin_write(led_device[2], DT_ALIAS_LED2_GPIOS_PIN, 0);
	} else {
		/* LED3 Off */
		gpio_pin_write(led_device[2], DT_ALIAS_LED2_GPIOS_PIN, 1);
	}

	if (color < 50) {
		/* LED4 On */
		gpio_pin_write(led_device[3], DT_ALIAS_LED3_GPIOS_PIN, 0);
	} else {
		/* LED4 Off */
		gpio_pin_write(led_device[3], DT_ALIAS_LED3_GPIOS_PIN, 1);
	}
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

#if defined(CONFIG_MCUMGR)
	smp_svr_init();
#endif

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

#if defined(CONFIG_MCUMGR)
	/* Initialize the Bluetooth mcumgr transport. */
	smp_bt_register();

	k_timer_start(&smp_svr_timer, K_NO_WAIT, K_MSEC(1000));
#endif
}
