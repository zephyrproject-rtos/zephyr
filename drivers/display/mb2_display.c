/*
 * Copyright (c) 2020 Lingao Meng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * References:
 *
 * https://www.microbit.co.uk/device/screen
 * https://lancaster-university.github.io/microbit-docs/ubit/display/
 */

#include <zephyr.h>
#include <init.h>
#include <drivers/gpio.h>
#include <device.h>
#include <string.h>
#include <sys/printk.h>

#include <display/mb_display.h>

#include "mb_display.h"

/* Onboard LED Row 1 */
#define LED_ROW1_GPIO_PIN   21
#define LED_ROW1_GPIO_PORT  DT_LABEL(DT_NODELABEL(gpio0))

/* Onboard LED Row 2 */
#define LED_ROW2_GPIO_PIN   22
#define LED_ROW2_GPIO_PORT  DT_LABEL(DT_NODELABEL(gpio0))

/* Onboard LED Row 3 */
#define LED_ROW3_GPIO_PIN   15
#define LED_ROW3_GPIO_PORT  DT_LABEL(DT_NODELABEL(gpio0))

/* Onboard LED Row 4 */
#define LED_ROW4_GPIO_PIN   24
#define LED_ROW4_GPIO_PORT  DT_LABEL(DT_NODELABEL(gpio0))

/* Onboard LED Row 5 */
#define LED_ROW5_GPIO_PIN   19
#define LED_ROW5_GPIO_PORT  DT_LABEL(DT_NODELABEL(gpio0))

/* Onboard LED Column 1 */
#define LED_COL1_GPIO_PIN   28
#define LED_COL1_GPIO_PORT  DT_LABEL(DT_NODELABEL(gpio0))

/* Onboard LED Column 2 */
#define LED_COL2_GPIO_PIN   11
#define LED_COL2_GPIO_PORT  DT_LABEL(DT_NODELABEL(gpio0))

/* Onboard LED Column 3 */
#define LED_COL3_GPIO_PIN   31
#define LED_COL3_GPIO_PORT  DT_LABEL(DT_NODELABEL(gpio0))

/* Onboard LED Column 4 */
#define LED_COL4_GPIO_PIN   5
#define LED_COL4_GPIO_PORT  DT_LABEL(DT_NODELABEL(gpio1))

/* Onboard LED Column 5 */
#define LED_COL5_GPIO_PIN   30
#define LED_COL5_GPIO_PORT  DT_LABEL(DT_NODELABEL(gpio0))

static const struct device *mb2_dev[GPIO_PORTS];

/* Mask of all the column bits */
const uint32_t col_mask[GPIO_PORTS] = {
	(BIT(LED_COL1_GPIO_PIN) | BIT(LED_COL2_GPIO_PIN) |
	 BIT(LED_COL3_GPIO_PIN) | BIT(LED_COL5_GPIO_PIN)),

	(BIT(LED_COL4_GPIO_PIN)),
};

#define GET_PIXEL(img, x, y) ((img)->row[y] & BIT(x))

static const uint8_t row_pins[DISPLAY_ROWS] = {
	LED_ROW1_GPIO_PIN,
	LED_ROW2_GPIO_PIN,
	LED_ROW3_GPIO_PIN,
	LED_ROW4_GPIO_PIN,
	LED_ROW5_GPIO_PIN,
};

#define ROW_PIN(n)  (row_pins[n])

static const uint8_t col_pins[DISPLAY_COLS][GPIO_PORTS] = {
	{ LED_COL1_GPIO_PIN, 0 },
	{ LED_COL2_GPIO_PIN, 0 },
	{ LED_COL3_GPIO_PIN, 0 },
	{ LED_COL4_GPIO_PIN, 1 },
	{ LED_COL5_GPIO_PIN, 0 },
};

#define COL_PIN(n)  (col_pins[n][0])
#define COL_PORT(n) (mb2_dev[col_pins[n][1]])

/* Precalculate all five rows of an image and start the rendering. */
void mb_start_image(const struct mb_image *img,
		    uint32_t rows[DISPLAY_ROWS][GPIO_PORTS])
{
	int row, col;

	for (row = 0; row < DISPLAY_ROWS; row++) {
		memcpy(rows[row], col_mask, sizeof(col_mask));

		for (col = 0; col < DISPLAY_COLS; col++) {
			if (GET_PIXEL(img, col, row)) {
				rows[row][col_pins[col][1]] ^= BIT(COL_PIN(col));
			}
		}
	}
}

void mb_update_pins(uint8_t cur, const uint32_t val[GPIO_PORTS])
{
	int i;
	uint32_t prev = (cur + 4) % 5;

	/* Disable the previous row */
	gpio_pin_set_raw(mb2_dev[0], ROW_PIN(prev), 0);

	/* Set the column pins to their correct values */
	for (i = 0; i < DISPLAY_COLS; i++) {
		gpio_pin_set_raw(COL_PORT(i), COL_PIN(i),
				 !!(val[col_pins[i][1]] & BIT(COL_PIN(i))));
	}

	/* Enable the new row */
	gpio_pin_set_raw(mb2_dev[0], ROW_PIN(cur), 1);
}

static int mb2_display_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	int i;

	mb2_dev[0] = device_get_binding(DT_LABEL(DT_NODELABEL(gpio0)));
	mb2_dev[1] = device_get_binding(DT_LABEL(DT_NODELABEL(gpio1)));

	__ASSERT(dev, "No GPIO device found");

	for (i = 0; i < ARRAY_SIZE(row_pins); i++) {
		gpio_pin_configure(mb2_dev[0], ROW_PIN(i), GPIO_OUTPUT);
	}

	for (i = 0; i < ARRAY_SIZE(col_pins); i++) {
		gpio_pin_configure(COL_PORT(i), COL_PIN(i), GPIO_OUTPUT);
	}

	return 0;
}

SYS_INIT(mb2_display_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
