/* microbit.c - BBC micro:bit specific hooks */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>

#include <zephyr/display/mb_display.h>

#include <zephyr/bluetooth/mesh.h>

#include "board.h"

static uint32_t oob_number;
static const struct device *gpio;

static void button_pressed(const struct device *dev, struct gpio_callback *cb,
			   uint32_t pins)
{
	struct mb_display *disp = mb_display_get();

	mb_display_print(disp, MB_DISPLAY_MODE_DEFAULT, 500,
			 "OOB Number: %u", oob_number);
}

static void configure_button(void)
{
	static struct gpio_callback button_cb;

	gpio = device_get_binding(DT_GPIO_LABEL(DT_ALIAS(sw0), gpios));

	gpio_pin_configure(gpio, DT_GPIO_PIN(DT_ALIAS(sw0), gpios),
			   DT_GPIO_FLAGS(DT_ALIAS(sw0), gpios) | GPIO_INPUT);

	gpio_init_callback(&button_cb, button_pressed,
			   BIT(DT_GPIO_PIN(DT_ALIAS(sw0), gpios)));

	gpio_pin_interrupt_configure(gpio, DT_GPIO_PIN(DT_ALIAS(sw0), gpios),
				     GPIO_INT_EDGE_TO_ACTIVE);

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

	gpio_pin_interrupt_configure(gpio, DT_GPIO_PIN(DT_ALIAS(sw0), gpios),
				     GPIO_INT_EDGE_TO_ACTIVE);

	mb_display_image(disp, MB_DISPLAY_MODE_DEFAULT, SYS_FOREVER_MS, &arrow,
			 1);
}

void board_prov_complete(void)
{
	struct mb_display *disp = mb_display_get();
	struct mb_image arrow = MB_IMAGE({ 0, 1, 0, 1, 0 },
					 { 0, 1, 0, 1, 0 },
					 { 0, 0, 0, 0, 0 },
					 { 1, 0, 0, 0, 1 },
					 { 0, 1, 1, 1, 0 });


	gpio_pin_interrupt_configure(gpio, DT_GPIO_PIN(DT_ALIAS(sw0), gpios),
				     GPIO_INT_DISABLE);

	mb_display_image(disp, MB_DISPLAY_MODE_DEFAULT, 10 * MSEC_PER_SEC,
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
			 1 * MSEC_PER_SEC, blink, ARRAY_SIZE(blink));

	configure_button();
}
