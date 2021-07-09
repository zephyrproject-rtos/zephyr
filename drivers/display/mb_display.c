/*
 * Copyright (c) 2017 Intel Corporation
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
#define LED_ROW1_GPIO_PIN   13
#define LED_ROW1_GPIO_PORT  DT_LABEL(DT_NODELABEL(gpio0))

/* Onboard LED Row 2 */
#define LED_ROW2_GPIO_PIN   14
#define LED_ROW2_GPIO_PORT  DT_LABEL(DT_NODELABEL(gpio0))

/* Onboard LED Row 3 */
#define LED_ROW3_GPIO_PIN   15
#define LED_ROW3_GPIO_PORT  DT_LABEL(DT_NODELABEL(gpio0))

/* Onboard LED Column 1 */
#define LED_COL1_GPIO_PIN   4
#define LED_COL1_GPIO_PORT  DT_LABEL(DT_NODELABEL(gpio0))

/* Onboard LED Column 2 */
#define LED_COL2_GPIO_PIN   5
#define LED_COL2_GPIO_PORT  DT_LABEL(DT_NODELABEL(gpio0))

/* Onboard LED Column 3 */
#define LED_COL3_GPIO_PIN   6
#define LED_COL3_GPIO_PORT  DT_LABEL(DT_NODELABEL(gpio0))

/* Onboard LED Column 4 */
#define LED_COL4_GPIO_PIN   7
#define LED_COL4_GPIO_PORT  DT_LABEL(DT_NODELABEL(gpio0))

/* Onboard LED Column 5 */
#define LED_COL5_GPIO_PIN   8
#define LED_COL5_GPIO_PORT  DT_LABEL(DT_NODELABEL(gpio0))

/* Onboard LED Column 6 */
#define LED_COL6_GPIO_PIN   9
#define LED_COL6_GPIO_PORT  DT_LABEL(DT_NODELABEL(gpio0))

/* Onboard LED Column 7 */
#define LED_COL7_GPIO_PIN   10
#define LED_COL7_GPIO_PORT  DT_LABEL(DT_NODELABEL(gpio0))

/* Onboard LED Column 8 */
#define LED_COL8_GPIO_PIN   11
#define LED_COL8_GPIO_PORT  DT_LABEL(DT_NODELABEL(gpio0))

/* Onboard LED Column 9 */
#define LED_COL9_GPIO_PIN   12
#define LED_COL9_GPIO_PORT  DT_LABEL(DT_NODELABEL(gpio0))

static const struct device *mb_dev;

struct x_y {
	uint8_t x:4,
	     y:4;
};

/* Where the X,Y coordinates of each row/col are found.
 * The top left corner has the coordinates 0,0.
 */
static const struct x_y map[DISPLAY_ROWS][DISPLAY_COLS] = {
	{{0, 0}, {2, 0}, {4, 0}, {4, 3}, {3, 3}, {2, 3}, {1, 3}, {0, 3}, {1, 2} },
	{{4, 2}, {0, 2}, {2, 2}, {1, 0}, {3, 0}, {3, 4}, {1, 4}, {0, 0}, {0, 0} },
	{{2, 4}, {4, 4}, {0, 4}, {0, 1}, {1, 1}, {2, 1}, {3, 1}, {4, 1}, {3, 2} },
};

/* Mask of all the column bits */
const uint32_t col_mask[GPIO_PORTS] = {
	(((~0UL) << LED_COL1_GPIO_PIN) & ((~0UL) >> (31 - LED_COL9_GPIO_PIN))),
};

#define GET_PIXEL(img, x, y) ((img)->row[y] & BIT(x))

/* Precalculate all three rows of an image and start the rendering. */
void mb_start_image(const struct mb_image *img,
		    uint32_t rows[DISPLAY_ROWS][GPIO_PORTS])
{
	int row, col;

	for (row = 0; row < DISPLAY_ROWS; row++) {
		rows[row][0] = 0U;

		for (col = 0; col < DISPLAY_COLS; col++) {
			if (GET_PIXEL(img, map[row][col].x, map[row][col].y)) {
				rows[row][0] |= BIT(LED_COL1_GPIO_PIN + col);
			}
		}

		rows[row][0] = ~rows[row][0] & col_mask[0];
		rows[row][0] |= BIT(LED_ROW1_GPIO_PIN + row);
	}
}

#define ROW_PIN(n) (LED_ROW1_GPIO_PIN + (n))

void mb_update_pins(uint8_t cur, const uint32_t val[GPIO_PORTS])
{
	uint32_t pin, prev = (cur + 2) % 3;

	/* Disable the previous row */
	gpio_pin_set_raw(mb_dev, ROW_PIN(prev), 0);

	/* Set the column pins to their correct values */
	for (pin = LED_COL1_GPIO_PIN; pin <= LED_COL9_GPIO_PIN; pin++) {
		gpio_pin_set_raw(mb_dev, pin, !!(val[0] & BIT(pin)));
	}

	/* Enable the new row */
	gpio_pin_set_raw(mb_dev, ROW_PIN(cur), 1);
}

static int mb_display_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	mb_dev = device_get_binding(DT_LABEL(DT_NODELABEL(gpio0)));

	__ASSERT(mb_dev, "No GPIO device found");

	gpio_pin_configure(mb_dev, LED_ROW1_GPIO_PIN, GPIO_OUTPUT);
	gpio_pin_configure(mb_dev, LED_ROW2_GPIO_PIN, GPIO_OUTPUT);
	gpio_pin_configure(mb_dev, LED_ROW3_GPIO_PIN, GPIO_OUTPUT);
	gpio_pin_configure(mb_dev, LED_COL1_GPIO_PIN, GPIO_OUTPUT);
	gpio_pin_configure(mb_dev, LED_COL2_GPIO_PIN, GPIO_OUTPUT);
	gpio_pin_configure(mb_dev, LED_COL3_GPIO_PIN, GPIO_OUTPUT);
	gpio_pin_configure(mb_dev, LED_COL4_GPIO_PIN, GPIO_OUTPUT);
	gpio_pin_configure(mb_dev, LED_COL5_GPIO_PIN, GPIO_OUTPUT);
	gpio_pin_configure(mb_dev, LED_COL6_GPIO_PIN, GPIO_OUTPUT);
	gpio_pin_configure(mb_dev, LED_COL7_GPIO_PIN, GPIO_OUTPUT);
	gpio_pin_configure(mb_dev, LED_COL8_GPIO_PIN, GPIO_OUTPUT);
	gpio_pin_configure(mb_dev, LED_COL9_GPIO_PIN, GPIO_OUTPUT);

	return 0;
}

SYS_INIT(mb_display_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
