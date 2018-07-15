/* Bluetooth: Mesh Generic OnOff, Generic Level, Lighting & Vendor Models
 *
 * Copyright (c) 2018 Vikrant More
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <board.h>
#include <gpio.h>

#include "common.h"
#include "ble_mesh.h"
#include "device_composition.h"
#include "publisher.h"

struct device *led_device[4];
struct device *button_device[4];

static struct k_work button_work;

static void button_pressed(struct device *dev,
			   struct gpio_callback *cb, uint32_t pins)
{
	k_work_submit(&button_work);
}

static void gpio_init(void)
{
	static struct gpio_callback button_cb[4];

	/* LEDs configiuratin & setting */

	led_device[0] = device_get_binding(LED0_GPIO_PORT);
	gpio_pin_configure(led_device[0], LED0_GPIO_PIN,
			   GPIO_DIR_OUT | GPIO_PUD_PULL_UP);
	gpio_pin_write(led_device[0], LED0_GPIO_PIN, 1);

	led_device[1] = device_get_binding(LED1_GPIO_PORT);
	gpio_pin_configure(led_device[1], LED1_GPIO_PIN,
			   GPIO_DIR_OUT | GPIO_PUD_PULL_UP);
	gpio_pin_write(led_device[1], LED1_GPIO_PIN, 1);

	led_device[2] = device_get_binding(LED2_GPIO_PORT);
	gpio_pin_configure(led_device[2], LED2_GPIO_PIN,
			   GPIO_DIR_OUT | GPIO_PUD_PULL_UP);
	gpio_pin_write(led_device[2], LED2_GPIO_PIN, 1);

	led_device[3] = device_get_binding(LED3_GPIO_PORT);
	gpio_pin_configure(led_device[3], LED3_GPIO_PIN,
			   GPIO_DIR_OUT | GPIO_PUD_PULL_UP);
	gpio_pin_write(led_device[3], LED3_GPIO_PIN, 1);

	/* Buttons configiuratin & setting */

	k_work_init(&button_work, publish);

	button_device[0] = device_get_binding(SW0_GPIO_NAME);
	gpio_pin_configure(button_device[0], SW0_GPIO_PIN,
			   (GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			    GPIO_PUD_PULL_UP |
			    GPIO_INT_DEBOUNCE | GPIO_INT_ACTIVE_LOW));
	gpio_init_callback(&button_cb[0], button_pressed, BIT(SW0_GPIO_PIN));
	gpio_add_callback(button_device[0], &button_cb[0]);
	gpio_pin_enable_callback(button_device[0], SW0_GPIO_PIN);

	button_device[1] = device_get_binding(SW1_GPIO_NAME);
	gpio_pin_configure(button_device[1], SW1_GPIO_PIN,
			   (GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			    GPIO_PUD_PULL_UP |
			    GPIO_INT_DEBOUNCE | GPIO_INT_ACTIVE_LOW));
	gpio_init_callback(&button_cb[1], button_pressed, BIT(SW1_GPIO_PIN));
	gpio_add_callback(button_device[1], &button_cb[1]);
	gpio_pin_enable_callback(button_device[1], SW1_GPIO_PIN);

	button_device[2] = device_get_binding(SW2_GPIO_NAME);
	gpio_pin_configure(button_device[2], SW2_GPIO_PIN,
			   (GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			    GPIO_PUD_PULL_UP |
			    GPIO_INT_DEBOUNCE | GPIO_INT_ACTIVE_LOW));
	gpio_init_callback(&button_cb[2], button_pressed, BIT(SW2_GPIO_PIN));
	gpio_add_callback(button_device[2], &button_cb[2]);
	gpio_pin_enable_callback(button_device[2], SW2_GPIO_PIN);

	button_device[3] = device_get_binding(SW3_GPIO_NAME);
	gpio_pin_configure(button_device[3], SW3_GPIO_PIN,
			   (GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			    GPIO_PUD_PULL_UP |
			    GPIO_INT_DEBOUNCE | GPIO_INT_ACTIVE_LOW));
	gpio_init_callback(&button_cb[3], button_pressed, BIT(SW3_GPIO_PIN));
	gpio_add_callback(button_device[3], &button_cb[3]);
	gpio_pin_enable_callback(button_device[3], SW3_GPIO_PIN);
}

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
}

void main(void)
{
	int err;

	light_default_status_init();

	gpio_init();

	update_light_state();

	printk("Initializing...\n");

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
	}

	randomize_publishers_TID();
}
