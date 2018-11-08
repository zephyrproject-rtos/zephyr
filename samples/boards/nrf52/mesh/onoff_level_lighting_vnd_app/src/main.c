/* Bluetooth: Mesh Generic OnOff, Generic Level, Lighting & Vendor Models
 *
 * Copyright (c) 2018 Vikrant More
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gpio.h>

#include "ble_mesh.h"
#include "device_composition.h"
#include "publisher.h"
#include "state_binding.h"
#include "transition.h"
#include "storage.h"

static bool reset;

struct device *led_device[4];
struct device *button_device[4];

static struct k_work button_work;

static void button_pressed(struct device *dev,
			   struct gpio_callback *cb, u32_t pins)
{
	k_work_submit(&button_work);
}

static void gpio_init(void)
{
	static struct gpio_callback button_cb[4];

	/* LEDs configuration & setting */

	led_device[0] = device_get_binding(LED0_GPIO_CONTROLLER);
	gpio_pin_configure(led_device[0], LED0_GPIO_PIN,
			   GPIO_DIR_OUT | GPIO_PUD_PULL_UP);
	gpio_pin_write(led_device[0], LED0_GPIO_PIN, 1);

	led_device[1] = device_get_binding(LED1_GPIO_CONTROLLER);
	gpio_pin_configure(led_device[1], LED1_GPIO_PIN,
			   GPIO_DIR_OUT | GPIO_PUD_PULL_UP);
	gpio_pin_write(led_device[1], LED1_GPIO_PIN, 1);

	led_device[2] = device_get_binding(LED2_GPIO_CONTROLLER);
	gpio_pin_configure(led_device[2], LED2_GPIO_PIN,
			   GPIO_DIR_OUT | GPIO_PUD_PULL_UP);
	gpio_pin_write(led_device[2], LED2_GPIO_PIN, 1);

	led_device[3] = device_get_binding(LED3_GPIO_CONTROLLER);
	gpio_pin_configure(led_device[3], LED3_GPIO_PIN,
			   GPIO_DIR_OUT | GPIO_PUD_PULL_UP);
	gpio_pin_write(led_device[3], LED3_GPIO_PIN, 1);

	/* Buttons configuration & setting */

	k_work_init(&button_work, publish);

	button_device[0] = device_get_binding(SW0_GPIO_CONTROLLER);
	gpio_pin_configure(button_device[0], SW0_GPIO_PIN,
			   (GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			    GPIO_PUD_PULL_UP |
			    GPIO_INT_DEBOUNCE | GPIO_INT_ACTIVE_LOW));
	gpio_init_callback(&button_cb[0], button_pressed, BIT(SW0_GPIO_PIN));
	gpio_add_callback(button_device[0], &button_cb[0]);
	gpio_pin_enable_callback(button_device[0], SW0_GPIO_PIN);

	button_device[1] = device_get_binding(SW1_GPIO_CONTROLLER);
	gpio_pin_configure(button_device[1], SW1_GPIO_PIN,
			   (GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			    GPIO_PUD_PULL_UP |
			    GPIO_INT_DEBOUNCE | GPIO_INT_ACTIVE_LOW));
	gpio_init_callback(&button_cb[1], button_pressed, BIT(SW1_GPIO_PIN));
	gpio_add_callback(button_device[1], &button_cb[1]);
	gpio_pin_enable_callback(button_device[1], SW1_GPIO_PIN);

	button_device[2] = device_get_binding(SW2_GPIO_CONTROLLER);
	gpio_pin_configure(button_device[2], SW2_GPIO_PIN,
			   (GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			    GPIO_PUD_PULL_UP |
			    GPIO_INT_DEBOUNCE | GPIO_INT_ACTIVE_LOW));
	gpio_init_callback(&button_cb[2], button_pressed, BIT(SW2_GPIO_PIN));
	gpio_add_callback(button_device[2], &button_cb[2]);
	gpio_pin_enable_callback(button_device[2], SW2_GPIO_PIN);

	button_device[3] = device_get_binding(SW3_GPIO_CONTROLLER);
	gpio_pin_configure(button_device[3], SW3_GPIO_PIN,
			   (GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			    GPIO_PUD_PULL_UP |
			    GPIO_INT_DEBOUNCE | GPIO_INT_ACTIVE_LOW));
	gpio_init_callback(&button_cb[3], button_pressed, BIT(SW3_GPIO_PIN));
	gpio_add_callback(button_device[3], &button_cb[3]);
	gpio_pin_enable_callback(button_device[3], SW3_GPIO_PIN);
}

static void light_default_var_init(void)
{
	gen_def_trans_time_srv_user_data.tt = 0x00;

	gen_power_onoff_srv_user_data.onpowerup = STATE_DEFAULT;

	light_lightness_srv_user_data.light_range_min = LIGHTNESS_MIN;
	light_lightness_srv_user_data.light_range_max = LIGHTNESS_MAX;
	light_lightness_srv_user_data.last = LIGHTNESS_MAX;
	light_lightness_srv_user_data.def = LIGHTNESS_MAX;

	light_ctl_srv_user_data.temp_range_min = TEMP_MIN;
	light_ctl_srv_user_data.temp_range_max = TEMP_MAX;
	light_ctl_srv_user_data.lightness_def = LIGHTNESS_MAX;
	light_ctl_srv_user_data.temp_def = TEMP_MIN;

	light_ctl_srv_user_data.lightness_temp_last =
		(u32_t) ((LIGHTNESS_MAX << 16) | TEMP_MIN);
}

static void light_default_status_init(void)
{
	u16_t lightness;

	lightness = (u16_t) (light_ctl_srv_user_data.lightness_temp_last >> 16);

	if (lightness) {
		gen_onoff_srv_root_user_data.onoff = STATE_ON;
	} else {
		gen_onoff_srv_root_user_data.onoff = STATE_OFF;
	}

	if (light_ctl_srv_user_data.lightness_temp_def) {
		light_ctl_srv_user_data.lightness_def = (u16_t)
			(light_ctl_srv_user_data.lightness_temp_def >> 16);

		light_ctl_srv_user_data.temp_def = (u16_t)
			(light_ctl_srv_user_data.lightness_temp_def);
	}

	light_lightness_srv_user_data.def =
		light_ctl_srv_user_data.lightness_def;

	light_ctl_srv_user_data.temp = light_ctl_srv_user_data.temp_def;

	switch (gen_power_onoff_srv_user_data.onpowerup) {
	case STATE_OFF:
		gen_onoff_srv_root_user_data.onoff = STATE_OFF;
		state_binding(ONOFF, ONOFF_TEMP);
		break;
	case STATE_DEFAULT:
		gen_onoff_srv_root_user_data.onoff = STATE_ON;
		state_binding(ONOFF, ONOFF_TEMP);
		break;
	case STATE_RESTORE:
		light_lightness_srv_user_data.last = (u16_t)
			(light_ctl_srv_user_data.lightness_temp_last >> 16);

		light_ctl_srv_user_data.temp =
			(u16_t) (light_ctl_srv_user_data.lightness_temp_last);

		state_binding(ONPOWERUP, ONOFF_TEMP);
		break;
	}

	default_tt = gen_def_trans_time_srv_user_data.tt;
}

/* unsolicitedly_publish_states_timer related things (Start) */

static void unsolicitedly_publish_states_work_handler(struct k_work *work)
{
	gen_onoff_publisher(&root_models[2]);
	gen_level_publisher(&root_models[4]);
	light_lightness_publisher(&root_models[11]);
	light_lightness_linear_publisher(&root_models[11]);
	light_ctl_publisher(&root_models[14]);

	gen_level_publisher(&s0_models[0]);
	light_ctl_temp_publisher(&s0_models[2]);
}

K_WORK_DEFINE(unsolicitedly_publish_states_work,
	      unsolicitedly_publish_states_work_handler);

static void unsolicitedly_publish_states_timer_handler(struct k_timer *dummy)
{
	k_work_submit(&unsolicitedly_publish_states_work);
}

K_TIMER_DEFINE(unsolicitedly_publish_states_timer,
	       unsolicitedly_publish_states_timer_handler, NULL);

/* unsolicitedly_publish_states_timer related things (End) */

/* save_lightness_temp_last_values_timer related things (Start) */

static void save_lightness_temp_last_values_timer_handler(struct k_timer *dummy)
{
	save_on_flash(LIGHTNESS_TEMP_LAST_STATE);
}

K_TIMER_DEFINE(save_lightness_temp_last_values_timer,
	       save_lightness_temp_last_values_timer_handler, NULL);

/* save_lightness_temp_last_values_timer related things (End) */

static void no_transition_work_handler(struct k_work *work)
{
	k_timer_start(&unsolicitedly_publish_states_timer, K_MSEC(5000), 0);

	/* If Lightness & Temperature values remains stable for
	 * 10 Seconds then & then only get stored on SoC flash.
	 */
	if (gen_power_onoff_srv_user_data.onpowerup == STATE_RESTORE) {
		k_timer_start(&save_lightness_temp_last_values_timer,
			      K_MSEC(10000), 0);
	}
}

K_WORK_DEFINE(no_transition_work, no_transition_work_handler);

void update_light_state(void)
{
	u8_t power, color;

	power = 100 * ((float) light_lightness_srv_user_data.actual / 65535);
	color = 100 * ((float) (gen_level_srv_s0_user_data.level + 32768)
		       / 65535);

	printk("power-> %d, color-> %d\n", power, color);

	if (gen_onoff_srv_root_user_data.onoff == STATE_ON) {
		/* LED1 On */
		gpio_pin_write(led_device[0], LED0_GPIO_PIN, 0);
	} else {
		/* LED1 Off */
		gpio_pin_write(led_device[0], LED0_GPIO_PIN, 1);
	}

	if (power < 50) {
		/* LED3 On */
		gpio_pin_write(led_device[2], LED2_GPIO_PIN, 0);
	} else {
		/* LED3 Off */
		gpio_pin_write(led_device[2], LED2_GPIO_PIN, 1);
	}

	if (color < 50) {
		/* LED4 On */
		gpio_pin_write(led_device[3], LED3_GPIO_PIN, 0);
	} else {
		/* LED4 Off */
		gpio_pin_write(led_device[3], LED3_GPIO_PIN, 1);
	}

	if (*ptr_counter == 0 || reset == false) {
		reset = true;
		k_work_submit(&no_transition_work);
	}
}

static void short_time_multireset_bt_mesh_unprovisioning(void)
{
	if (reset_counter >= 4) {
		reset_counter = 0;
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
	reset_counter = 0;
	save_on_flash(RESET_COUNTER);
	printk("Reset Counter set to Zero\n");
}

K_TIMER_DEFINE(reset_counter_timer, reset_counter_timer_handler, NULL);

void main(void)
{
	int err;

	light_default_var_init();

	gpio_init();

	printk("Initializing...\n");

	ps_settings_init();

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
	}

	light_default_status_init();

	update_light_state();

	randomize_publishers_TID();

	short_time_multireset_bt_mesh_unprovisioning();
	k_timer_start(&reset_counter_timer, K_MSEC(7000), 0);
}
