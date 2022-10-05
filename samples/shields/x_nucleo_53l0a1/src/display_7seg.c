/*
 * Copyright (c) 2021 Titouan Christophe
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "display_7seg.h"

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

const uint8_t DISPLAY_OFF[4] = { CHAR_OFF, CHAR_OFF, CHAR_OFF, CHAR_OFF };
const uint8_t TEXT_Err[4] = { CHAR_E, CHAR_r, CHAR_r, CHAR_OFF };

static bool initialized;
static const struct device *const expander1 =
				DEVICE_DT_GET(DT_NODELABEL(expander1_x_nucleo_53l0a1));
static const struct device *const expander2 =
				DEVICE_DT_GET(DT_NODELABEL(expander2_x_nucleo_53l0a1));
static const uint8_t digits[16] = {
	CHAR_0,
	CHAR_1,
	CHAR_2,
	CHAR_3,
	CHAR_4,
	CHAR_5,
	CHAR_6,
	CHAR_7,
	CHAR_8,
	CHAR_9,
	CHAR_A,
	CHAR_b,
	CHAR_C,
	CHAR_d,
	CHAR_E,
	CHAR_F,
};

static const int max_positive_for_base[17] = {
	0,
	1,
	16,
	81,
	256,
	625,
	1296,
	2401,
	4096,
	6561,
	10000,
	14641,
	20736,
	28561,
	38416,
	50625,
	65536,
};

static const int max_negative_for_base[17] = {
	-0,
	-1,
	-8,
	-27,
	-64,
	-125,
	-216,
	-343,
	-512,
	-729,
	-1000,
	-1331,
	-1728,
	-2197,
	-2744,
	-3375,
	-4096,
};

static int init_display(void)
{
	int ret;

	for (int i = 0; i < 14; i++) {
		ret = gpio_pin_configure(expander1, i, GPIO_OUTPUT | GPIO_OUTPUT_INIT_HIGH);
		if (ret) {
			printk("Error in configure pin %d on expander1: %d", i, ret);
			return ret;
		}
		ret = gpio_pin_configure(expander2, i, GPIO_OUTPUT | GPIO_OUTPUT_INIT_HIGH);
		if (ret) {
			printk("Error in configure pin %d on expander2: %d", i, ret);
			return ret;
		}
	}

	initialized = true;
	return 0;
}


int display_chars(const uint8_t chars[4])
{
	int r;
	uint16_t port1, port2;

	if (!initialized) {
		r = init_display();
		if (r) {
			return r;
		}
	}

	port1 = ((chars[3] & 0x7f) << 7) | (chars[2] & 0x7f);
	port2 = ((chars[1] & 0x7f) << 7) | (chars[0] & 0x7f);

	/* Common anode: invert the output (LOW = led on) */
	r = gpio_port_set_masked(expander1, 0x3fff, port1 ^ 0x3fff);
	if (r) {
		return r;
	}
	return gpio_port_set_masked(expander2, 0x3fff, port2 ^ 0x3fff);
}

int display_number(int num, unsigned int base)
{
	int d, i, neg = 0;
	uint8_t representation[4] = { CHAR_E, CHAR_r, CHAR_r, CHAR_OFF };

	/* Unsupported base */
	if (base > 16) {
		return -EINVAL;
	}

	/* Number too large to be displayed */
	if (num <= max_negative_for_base[base] || num >= max_positive_for_base[base]) {
		return -EINVAL;
	}

	if (num < 0) {
		num = -num;
		representation[0] = CHAR_DASH;
		neg = 1;
	}

	for (i = 3; i >= neg; i--) {
		if (num > 0 || i == 3) {
			d = num % base;
			representation[i] = digits[d];
			num /= base;
		} else {
			representation[i] = CHAR_OFF;
		}
	}
	return display_chars(representation);
}
