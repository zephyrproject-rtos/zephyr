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
#define SLEEP_TIME_MS 1000
#define BRIGHTNESS_SLEEP_TIME_MS 5

#define MAX_CHANNELS 4
/* Structure describing a color by its component values and name */
struct color_data {
	uint8_t r, g, b;
	const char *name;
};

static const char * const led_colors[] = {
	"WHITE", "RED", "GREEN", "BLUE", "AMBER", "VIOLET", "YELLOW", "IR"
};

/*
 * A build error on these lines means your board is unsupported.
 */
const struct device *led4_dev = DEVICE_DT_GET_ANY(issi_is31fl3194);
const struct device *led7_dev = DEVICE_DT_GET_ANY(issi_is31fl3197);

uint8_t rgb_mapping[3] = {0, 0, 0}; /* R G B */

/*
 * Generate RGB color mapping
 */
int generate_rgb_color_mapping(const struct device *dev)
{
	const struct led_info *info;
	int rgb_led;
	uint8_t colors_found;

	/* Find the Red, Green, and blue color indexes */
	for (rgb_led = 0;; rgb_led++) {
		colors_found = 0;
		printk("Check led:%u for RGB colors\n", rgb_led);
		if (led_get_info(dev, rgb_led, &info) < 0) {
			printk("End of LED list\n");
			return -1;
		}
		/* Generate color mapping table for this device */
		for (uint8_t i = 0; i < info->num_colors; i++) {
			switch (info->color_mapping[i]) {
			case LED_COLOR_ID_RED:
				rgb_mapping[0] = i;
				colors_found |= 0x1;
				break;
			case LED_COLOR_ID_GREEN:
				rgb_mapping[1] = i;
				colors_found |= 0x2;
				break;
			case LED_COLOR_ID_BLUE:
				rgb_mapping[2] = i;
				colors_found |= 0x4;
				break;
			default:
				printk("Unknown color: %x found in index %d\n",
				       info->color_mapping[i], i);
				/* Ignore */
				break;
			}
		}

		if (colors_found == 0x07) {
			break;
		}
	}

	printk("Color indexes: LED:%u R:%u G:%u B:%u\n", rgb_led, rgb_mapping[0],
		rgb_mapping[1], rgb_mapping[2]);
	return rgb_led;
}

/*
 * Set one color (R or G or B) to all on and then fade it down to off
 */
void fade_one_color(const struct device *dev, uint8_t led, uint8_t num_colors,
		    uint8_t color_index, const char *color_name)
{
	uint8_t colors[MAX_CHANNELS] = {0, 0, 0, 0};
	int color = 0xff;

	printk("%s\n", color_name);

	/* Make sure all of the leds are off */
	if (led_set_color(dev, led, num_colors, colors) < 0) {
		return;
	}

	for (;;) {
		colors[color_index] = color;
		if (led_set_color(dev, led, num_colors, colors) < 0) {
			return;
		}
		if (color == 0) {
			break;
		}
		k_msleep(SLEEP_TIME_MS);
		color >>= 2;
	}
}


/*
 * Set all channels at the same time
 */
void fade_all_channels(const struct device *dev, uint8_t channel_count)
{
	uint8_t colors[MAX_CHANNELS] = {0, 0, 0, 0};
	int color = 0xff;
	int i;

	/* Make sure all of the leds are off */
	if (channel_count > MAX_CHANNELS) {
		printk("Warning: channel count %u > Max channels %u\n", channel_count,
		       MAX_CHANNELS);
		channel_count = MAX_CHANNELS;

	}
	if (led_write_channels(dev, 0, channel_count, colors) < 0) {
		return;
	}

	for (;;) {
		for (i = 0; i < channel_count; i++) {
			colors[i] = color;
		}

		if (led_write_channels(dev, 0, channel_count, colors) < 0) {
			return;
		}
		if (color == 0) {
			break;
		}
		k_msleep(SLEEP_TIME_MS);
		color >>= 2;
	}
}

int main(void)
{
	int err;
	uint8_t device_channel_count = 0;
	uint8_t color_id;
	int rgb_led;
	const struct led_info *info;
	const struct device *led_dev = led4_dev;

	if (led_dev == NULL) {
		led_dev = led7_dev;
	}

	if (!led_dev) {
		printk("No device with compatible issi,is31fl3194 or issi,is31fl3197 found\n");
		return 0;
	}

	if (!device_is_ready(led_dev)) {
		printk("LED device %s not ready\n", led_dev->name);
		return 0;
	}

	/* Enumerate all of the LEDS and print information */
	printk("\nLED List\n");
	for (uint32_t led = 0;; led++) {
		err = led_get_info(led_dev, led, &info);
		if (err < 0) {
			printk("End of LED List\n");
			break;
		}
		printk("%u: label:%s index:%u num colors:%u\n", led, info->label,
			info->index, info->num_colors);
		device_channel_count += info->num_colors;
		for (uint8_t color_index = 0; color_index < info->num_colors; color_index++) {
			printk("\t%u: ", info->color_mapping[color_index]);
			color_id = info->color_mapping[color_index];
			fade_one_color(led_dev, led, info->num_colors, color_index,
				       (color_id < LED_COLOR_ID_MAX) ?
					led_colors[color_id] : "** unknown **");
		}
	}

	/* Lets fade all of the channels at the same time */
	printk("Fade all %u channels\n", device_channel_count);
	fade_all_channels(led_dev, device_channel_count);

	/* Check for RGB LED */
	rgb_led = generate_rgb_color_mapping(led_dev);
	if (rgb_led < 0) {
		printk("No RGB LED found\n");
	} else {
		/* Lets try to verify the color indexes */
		printk("RGB Colors found, now Fading: ");
		led_get_info(led_dev, 0, &info);
		fade_one_color(led_dev, rgb_led, info->num_colors, rgb_mapping[0], "RED");
		fade_one_color(led_dev, rgb_led, info->num_colors, rgb_mapping[1], "GREEN");
		fade_one_color(led_dev, rgb_led, info->num_colors, rgb_mapping[2], "BLUE");
	}

	printk("Test completed\n");
	return 0;
}
