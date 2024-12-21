/*
 * Copyright (c) 2024 Arduino SA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/led.h>

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   1000

/* Structure describing a color by its component values and name */
struct color_data {
	uint8_t r, g, b;
	const char *name;
};

/* The sequence of colors the RGB LED will display */
static const struct color_data color_sequence[] = {
	{ 0xFF, 0x00, 0x00, "Red" },
	{ 0x00, 0xFF, 0x00, "Green" },
	{ 0x00, 0x00, 0xFF, "Blue" },
	{ 0xFF, 0xFF, 0xFF, "White" },
	{ 0xFF, 0xFF, 0x00, "Yellow" },
	{ 0xFF, 0x00, 0xFF, "Purple" },
	{ 0x00, 0xFF, 0xFF, "Cyan" },
	{ 0xF4, 0x79, 0x20, "Orange" },
};

/*
 * A build error on this line means your board is unsupported.
 */
const struct device *led = DEVICE_DT_GET_ANY(issi_is31fl3194);

int main(void)
{
	int ret;
	int i = 0;

	if (!device_is_ready(led)) {
		return 0;
	}

	while (1) {
		ret = led_set_color(led, 0, 3, &(color_sequence[i].r));
		if (ret < 0) {
			return 0;
		}

		printk("LED color: %s\n", color_sequence[i].name);
		k_msleep(SLEEP_TIME_MS);
		i = (i + 1) % ARRAY_SIZE(color_sequence);
	}

	return 0;
}
