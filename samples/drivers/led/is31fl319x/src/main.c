/*
 * Copyright (c) 2024 Arduino SA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/led.h>
#include <zephyr/dt-bindings/led/led.h>

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
const struct device *led_dev = DEVICE_DT_GET_ANY(issi_is31fl3194);

uint8_t rgb_mapping[3] = {0, 0, 0}; /* R G B */

/*
 * Forward references
 */
extern bool generate_rgb_color_mapping(const struct device *dev,  uint32_t led);
extern bool output_rgb_color(const struct device *dev,  uint32_t led, const uint8_t *colors);
extern void fade_one_channel(const struct device *dev,  uint8_t rgb_color_index, const char *name);

/*
 * Main
 */
int main(void)
{
	int i = 0;

	if (!device_is_ready(led_dev)) {
		return 0;
	}

	if (!generate_rgb_color_mapping(led_dev, 0)) {
		printk("Failed to generate RGB Color mapping\n");
		return 0;
	}

	/* Lets try to verify the color indexes */
	fade_one_channel(led_dev, 0, "Red");
	fade_one_channel(led_dev, 1, "Green");
	fade_one_channel(led_dev, 2, "Blue");

	while (1) {
		/* Map the colors */
		if (!output_rgb_color(led_dev, 0, &(color_sequence[i].r))) {
			printk("Call to led_set_color failed\n");
			return 0;
		}

		printk("LED color: %s\n", color_sequence[i].name);
		k_msleep(SLEEP_TIME_MS);
		i = (i + 1) % ARRAY_SIZE(color_sequence);
	}

	return 0;
}

/*
 * Generate RGB color mapping
 */
bool generate_rgb_color_mapping(const struct device *dev,  uint32_t led)
{
	const struct led_info *info;
	int i;

	/* Find the Red, Green, and blue color indexes */
	if (led_get_info(dev, led, &info) < 0) {
		printk("Failed to retrieve LED info\n");
		return false;
	}
	/* Generate color mapping table for this device */
	for (i = 0; i < info->num_colors; i++) {
		switch (info->color_mapping[i]) {
		case LED_COLOR_ID_RED:
			rgb_mapping[0] = i;
			break;
		case LED_COLOR_ID_GREEN:
			rgb_mapping[1] = i;
			break;
		case LED_COLOR_ID_BLUE:
			rgb_mapping[2] = i;
			break;
		default:
			printk("Unknown color: %x found in index %d\n", info->color_mapping[i], i);
			/* Ignore */
			break;
		}
	}

	printk("Color indexes: R:%u G:%u B:%u\n", rgb_mapping[0],
		rgb_mapping[1], rgb_mapping[2]);
	return true;
}

/*
 * Set one color (R or G or B) to all on and then fade it down to off
 */
void fade_one_channel(const struct device *dev,  uint8_t rgb_color_index, const char *name)
{
	uint8_t colors[3] = {0, 0, 0};
	int color = 0xff;

	/* Make sure all of the leds are off */
	if (led_set_color(dev, 0, 3, colors) < 0) {
		return;
	}

	printk("Test fading %s\n", name);
	for (;;) {
		led_set_channel(dev, rgb_mapping[rgb_color_index], color);
		if (color == 0) {
			break;
		}
		k_msleep(SLEEP_TIME_MS);
		color >>= 2;
	}
}

/*
 * Map the RGB color to the mapping order and output to the LED.
 */
bool output_rgb_color(const struct device *dev,  uint32_t led, const uint8_t *colors)
{
	uint8_t mapped_colors[3];

	for (uint8_t i = 0; i < 3; i++) {
		mapped_colors[rgb_mapping[i]] = colors[i];
	}

	return led_set_color(dev, led, 3, mapped_colors) >= 0;
}
