/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/misc/espressif_rmt/rmt_tx.h>
#include "led_strip_encoder.h"

#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)

#if DT_NODE_HAS_PROP(ZEPHYR_USER_NODE, rmt_output_gpios)
const struct gpio_dt_spec rmt_gpio = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), rmt_output_gpios);
#else
#error "Unsupported board: see README and check /zephyr,user node"
#endif

static const struct device *const rmt_dev = DEVICE_DT_GET_ONE(espressif_esp32_rmt);

#define RMT_LED_STRIP_RESOLUTION_HZ 10000000 /* 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution) */
#define EXAMPLE_LED_NUMBERS         24
#define EXAMPLE_CHASE_SPEED_MS      10

static uint8_t led_strip_pixels[EXAMPLE_LED_NUMBERS * 3];

/**
 * @brief Simple helper function, converting HSV color space to RGB color space.
 *
 * Wiki: https://en.wikipedia.org/wiki/HSL_and_HSV
 */
void led_strip_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b)
{
    h %= 360; /* h -> [0,360] */
    uint32_t rgb_max = v * 2.55f;
    uint32_t rgb_min = rgb_max * (100 - s) / 100.0f;

    uint32_t i = h / 60;
    uint32_t diff = h % 60;

    /* RGB adjustment amount by hue */
    uint32_t rgb_adj = (rgb_max - rgb_min) * diff / 60;

    switch (i) {
    case 0:
        *r = rgb_max;
        *g = rgb_min + rgb_adj;
        *b = rgb_min;
        break;
    case 1:
        *r = rgb_max - rgb_adj;
        *g = rgb_max;
        *b = rgb_min;
        break;
    case 2:
        *r = rgb_min;
        *g = rgb_max;
        *b = rgb_min + rgb_adj;
        break;
    case 3:
        *r = rgb_min;
        *g = rgb_max - rgb_adj;
        *b = rgb_max;
        break;
    case 4:
        *r = rgb_min + rgb_adj;
        *g = rgb_min;
        *b = rgb_max;
        break;
    default:
        *r = rgb_max;
        *g = rgb_min;
        *b = rgb_max - rgb_adj;
        break;
    }
}

int main(void)
{
	uint32_t red = 0;
	uint32_t green = 0;
	uint32_t blue = 0;
	uint16_t hue = 0;
	uint16_t start_rgb = 0;
	int rc;

	if (!device_is_ready(rmt_dev)) {
		printk("RMT device %s is not ready\n", rmt_dev->name);
		return -EINVAL;
	}
	
	printk("Create RMT TX channel\n");
	rmt_channel_handle_t led_chan = NULL;
	rmt_tx_channel_config_t tx_chan_config = {
		.clk_src = RMT_CLK_SRC_DEFAULT, /* select source clock */
		.gpio_num = rmt_gpio.pin,
		.mem_block_symbols = 64, /* increase the block size can make the LED less flickering */
		.resolution_hz = RMT_LED_STRIP_RESOLUTION_HZ,
		.trans_queue_depth = 4, /* set the number of transactions that can be pending in the background */
	};
	rc = rmt_new_tx_channel(&tx_chan_config, &led_chan);
	if (rc) {
		printk("RMT TX channel creation failed\n");
		return rc;
	}

	printk("Install led strip encoder\n");
	rmt_encoder_handle_t led_encoder = NULL;
	led_strip_encoder_config_t encoder_config = {
		.resolution = RMT_LED_STRIP_RESOLUTION_HZ,
	};
	rc = rmt_new_led_strip_encoder(&encoder_config, &led_encoder);
	if (rc) {
		printk("Unable to create encoder\n");
		return rc;
	}

	printk("Enable RMT TX channel\n");
	rc = rmt_enable(led_chan);
	if (rc) {
		printk("Unable to enable RMT TX channel\n");
		return rc;
	}

	printk("Start LED rainbow chase\n");
	rmt_transmit_config_t tx_config = {
		.loop_count = 0, /* no transfer loop */
	};
	while (1) {
		for (int i = 0; i < 3; i++) {
			for (int j = i; j < EXAMPLE_LED_NUMBERS; j += 3) {
				/* Build RGB pixels */
				hue = j * 360 / EXAMPLE_LED_NUMBERS + start_rgb;
				led_strip_hsv2rgb(hue, 100, 100, &red, &green, &blue);
				led_strip_pixels[j * 3 + 0] = green;
				led_strip_pixels[j * 3 + 1] = blue;
				led_strip_pixels[j * 3 + 2] = red;
			}
			/* Flush RGB values to LEDs */
			rc = rmt_transmit(led_chan, led_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config);
			if (rc) {
				printk("Unable to transmit data over TX channel\n");
				return rc;
			}
			rc = rmt_tx_wait_all_done(led_chan, K_FOREVER);
			if (rc) {
				printk("Waiting until all done failed\n");
				return rc;
			}
			k_sleep(K_MSEC(EXAMPLE_CHASE_SPEED_MS));
			memset(led_strip_pixels, 0, sizeof(led_strip_pixels));
			rc = rmt_transmit(led_chan, led_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config);
			if (rc) {
				printk("Unable to transmit data over TX channel\n");
				return rc;
			}
			rc = rmt_tx_wait_all_done(led_chan, K_FOREVER);
			if (rc) {
				printk("Waiting until all done failed\n");
				return rc;
			}
			k_sleep(K_MSEC(EXAMPLE_CHASE_SPEED_MS));
		}
		start_rgb += 60;
	}
}
