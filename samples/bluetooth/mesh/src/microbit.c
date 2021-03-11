/* microbit.c - BBC micro:bit specific hooks */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <bluetooth/mesh.h>
#include <display/mb_display.h>
#include <drivers/gpio.h>
#include "board.h"

static uint32_t oob_number;
static const struct device *gpio;
static const struct mb_image onoff[] = {
	MB_IMAGE({ 0, 0, 0, 0, 0 },
		 { 0, 0, 0, 0, 0 },
		 { 0, 0, 0, 0, 0 },
		 { 0, 0, 0, 0, 0 },
		 { 0, 0, 0, 0, 0 }),
	MB_IMAGE({ 1, 1, 1, 1, 1 },
		 { 1, 1, 1, 1, 1 },
		 { 1, 1, 1, 1, 1 },
		 { 1, 1, 1, 1, 1 },
		 { 1, 1, 1, 1, 1 }),
};
static struct k_work *button;

static void button_pressed(const struct device *dev, struct gpio_callback *cb,
			   uint32_t pins)
{
	struct mb_display *disp = mb_display_get();

	mb_display_print(disp, MB_DISPLAY_MODE_DEFAULT, 500, "%04u",
			 oob_number);

	if (button) {
		k_work_submit(button);
	}
}

static void configure_button(void)
{
	static struct gpio_callback button_cb;

	gpio = device_get_binding(DT_GPIO_LABEL(DT_ALIAS(sw0), gpios));

	gpio_pin_configure(gpio, DT_GPIO_PIN(DT_ALIAS(sw0), gpios),
			   GPIO_INPUT | DT_GPIO_FLAGS(DT_ALIAS(sw0), gpios));
	gpio_pin_interrupt_configure(gpio, DT_GPIO_PIN(DT_ALIAS(sw0), gpios),
				     GPIO_INT_EDGE_TO_ACTIVE);

	gpio_init_callback(&button_cb, button_pressed,
			   BIT(DT_GPIO_PIN(DT_ALIAS(sw0), gpios)));

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
	struct mb_image smile = MB_IMAGE({ 0, 1, 0, 1, 0 },
					 { 0, 1, 0, 1, 0 },
					 { 0, 0, 0, 0, 0 },
					 { 1, 0, 0, 0, 1 },
					 { 0, 1, 1, 1, 0 });


	gpio_pin_interrupt_configure(gpio, DT_GPIO_PIN(DT_ALIAS(sw0), gpios),
				     GPIO_INT_DISABLE);

	mb_display_image(disp, MB_DISPLAY_MODE_DEFAULT, 10 * MSEC_PER_SEC,
			 &smile, 1);
}

int board_init(struct k_work *button_work)
{
	struct mb_display *disp = mb_display_get();

	mb_display_image(disp, MB_DISPLAY_MODE_DEFAULT | MB_DISPLAY_FLAG_LOOP,
			 1 * MSEC_PER_SEC, onoff, ARRAY_SIZE(onoff));

	button = button_work;

	configure_button();

	return 0;
}

void board_led_set(bool val)
{
	struct mb_display *disp = mb_display_get();

	mb_display_image(disp, MB_DISPLAY_MODE_DEFAULT, SYS_FOREVER_MS,
			 &onoff[val], 1);
}
