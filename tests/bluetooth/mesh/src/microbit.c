/* microbit.c - BBC micro:bit specific hooks */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gpio.h>
#include <board.h>

#include <display/mb_display.h>

#include <bluetooth/mesh.h>

#include "board.h"

static u32_t oob_number;
static struct device *gpio;

static void button_pressed(struct device *dev, struct gpio_callback *cb,
			   uint32_t pins)
{
	struct mb_display *disp = mb_display_get();

	mb_display_print(disp, MB_DISPLAY_MODE_DEFAULT, K_MSEC(500),
			 "OOB Number: %u", oob_number);
}

static void configure_button(void)
{
	static struct gpio_callback button_cb;

	gpio = device_get_binding(SW0_GPIO_NAME);

	gpio_pin_configure(gpio, SW0_GPIO_PIN,
			   (GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			    GPIO_INT_ACTIVE_LOW));

	gpio_init_callback(&button_cb, button_pressed, BIT(SW0_GPIO_PIN));

	gpio_add_callback(gpio, &button_cb);
}

void board_output_number(bt_mesh_output_action_t action, uint32_t number)
{
	struct mb_display *disp = mb_display_get();
	struct mb_image arrow = MB_IMAGE({ 0, 0, 1, 0, 0 },
					 { 0, 1, 0, 0, 0 },
					 { 1, 1, 1, 1, 1 },
					 { 0, 1, 0, 0, 0 },
					 { 0, 0, 1, 0, 0 });

	oob_number = number;

	gpio_pin_enable_callback(gpio, SW0_GPIO_PIN);

	mb_display_image(disp, MB_DISPLAY_MODE_DEFAULT, K_FOREVER, &arrow, 1);
}

void board_prov_complete(void)
{
	struct mb_display *disp = mb_display_get();
	struct mb_image arrow = MB_IMAGE({ 0, 1, 0, 1, 0 },
					 { 0, 1, 0, 1, 0 },
					 { 0, 0, 0, 0, 0 },
					 { 1, 0, 0, 0, 1 },
					 { 0, 1, 1, 1, 0 });


	gpio_pin_disable_callback(gpio, SW0_GPIO_PIN);

	mb_display_image(disp, MB_DISPLAY_MODE_DEFAULT, K_SECONDS(10),
			 &arrow, 1);
}

void board_init(void)
{
	struct mb_display *disp = mb_display_get();
	static struct mb_image blink[] = {
		MB_IMAGE({ 1, 1, 1, 1, 1 },
			 { 1, 1, 1, 1, 1 },
			 { 1, 1, 1, 1, 1 },
			 { 1, 1, 1, 1, 1 },
			 { 1, 1, 1, 1, 1 }),
		MB_IMAGE({ 0, 0, 0, 0, 0 },
			 { 0, 0, 0, 0, 0 },
			 { 0, 0, 0, 0, 0 },
			 { 0, 0, 0, 0, 0 },
			 { 0, 0, 0, 0, 0 })
	};

	mb_display_image(disp, MB_DISPLAY_MODE_DEFAULT | MB_DISPLAY_FLAG_LOOP,
			 K_SECONDS(1), blink, ARRAY_SIZE(blink));

	configure_button();
}
