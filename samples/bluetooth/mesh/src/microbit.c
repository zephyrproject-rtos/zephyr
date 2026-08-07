/* microbit.c - BBC micro:bit specific hooks */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/bluetooth/mesh.h>
#include <zephyr/display/mb_display.h>
#include <zephyr/drivers/gpio.h>
#include "board.h"

static uint32_t oob_number;
static const struct gpio_dt_spec button =
	GPIO_DT_SPEC_GET(DT_NODELABEL(buttona), gpios);
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
static struct k_work *button_work;

static void button_pressed(const struct device *dev, struct gpio_callback *cb,
			   uint32_t pins)
{
	struct mb_display *disp = mb_display_get();

	/*
	 * CONFIG_MICROBIT_DISPLAY_STR_MAX needs adjustment if you
	 * change this.
	 */
	mb_display_print(disp, MB_DISPLAY_MODE_DEFAULT, 500, "%04u",
			 oob_number);

	if (button_work) {
		k_work_submit(button_work);
	}
}

static void configure_button(void)
{
	static struct gpio_callback button_cb;

	gpio_pin_configure_dt(&button, GPIO_INPUT);
	gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);

	gpio_init_callback(&button_cb, button_pressed, BIT(button.pin));

	gpio_add_callback(button.port, &button_cb);
}

void board_output_number(bt_mesh_output_action_t action, uint32_t number)
{
	struct mb_display *disp = mb_display_get();
	const struct mb_image arrow = MB_IMAGE({ 0, 0, 1, 0, 0 },
					       { 0, 1, 0, 0, 0 },
					       { 1, 1, 1, 1, 1 },
					       { 0, 1, 0, 0, 0 },
					       { 0, 0, 1, 0, 0 });

	oob_number = number;

	gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);

	mb_display_image(disp, MB_DISPLAY_MODE_DEFAULT, SYS_FOREVER_MS, &arrow,
			 1);
}

void board_prov_complete(void)
{
	struct mb_display *disp = mb_display_get();
	const struct mb_image smile = MB_IMAGE({ 0, 1, 0, 1, 0 },
					       { 0, 1, 0, 1, 0 },
					       { 0, 0, 0, 0, 0 },
					       { 1, 0, 0, 0, 1 },
					       { 0, 1, 1, 1, 0 });

	gpio_pin_interrupt_configure_dt(&button, GPIO_INT_DISABLE);

	mb_display_image(disp, MB_DISPLAY_MODE_DEFAULT, 10 * MSEC_PER_SEC,
			 &smile, 1);
}

int board_init(struct k_work *button_work_arg)
{
	struct mb_display *disp = mb_display_get();

	mb_display_image(disp, MB_DISPLAY_MODE_DEFAULT | MB_DISPLAY_FLAG_LOOP,
			 1 * MSEC_PER_SEC, onoff, ARRAY_SIZE(onoff));

	button_work = button_work_arg;

	configure_button();

	return 0;
}

void board_led_set(bool val)
{
	struct mb_display *disp = mb_display_get();

	mb_display_image(disp, MB_DISPLAY_MODE_DEFAULT, SYS_FOREVER_MS,
			 &onoff[val], 1);
}
