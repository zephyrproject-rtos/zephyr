/*
 * Copyright (c) 2024 Javad Rahimipetroudi <javad.rahimipetroudi@mind.be>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/led.h>

LOG_MODULE_REGISTER(tlc59731_, LOG_LEVEL_DBG);

#define RED   0
#define GREEN 1
#define BLUE  2

#define HIGH  1

#define TEST_DELAY 1000 /* ms */
#define NUM_COLORS 3    /* NUMBER of LEDs in a RGB LED */

/* The devicetree node identifier for the chip-select alias ('rgb_led_cs') */
#define LED0_CS_NODE DT_ALIAS(rgb_led_cs)
/* The devicetree node identifier for RGB LED controller alias ('rgbl-led') */
#define LED_CNTRL    DT_ALIAS(rgb_led)

const struct device *tlc59731 = DEVICE_DT_GET(LED_CNTRL);
static const struct gpio_dt_spec led_cs = GPIO_DT_SPEC_GET(LED0_CS_NODE, gpios);

/**
 * @brief Test RGB LED Brightness Levels
 *
 * @return int
 */
void test_rgb_brightness(void)
{
	uint8_t rgb = 0;
	uint8_t level = 0;

	while (rgb < NUM_COLORS) {
		while (level < 100) {
			led_set_brightness(tlc59731, rgb, level++);
			k_msleep(4);
		}

		k_msleep(200);
		while (level > 0) {
			led_set_brightness(tlc59731, rgb, level--);
			k_msleep(4);
		}

		rgb++;
		level = 0x00;
	}
}
void set_colors(uint8_t *colors, uint8_t r, uint8_t g, uint8_t b)
{
	colors[0] = r;
	colors[1] = g;
	colors[2] = b;
}
void test_rgb_set_color(void)
{
	uint8_t colors[] = {213, 184, 87};

	led_set_color(tlc59731, 0, NUM_COLORS, colors);
	k_msleep(TEST_DELAY);

	set_colors(colors, 219, 28, 104);
	led_set_color(tlc59731, 0, NUM_COLORS, colors);
	k_msleep(TEST_DELAY);

	set_colors(colors, 34, 219, 28);
	led_set_color(tlc59731, 0, NUM_COLORS, colors);
	k_msleep(TEST_DELAY);

	set_colors(colors, 255, 128, 0);
	led_set_color(tlc59731, 0, NUM_COLORS, colors);
	k_msleep(TEST_DELAY);
	/* Turn all  off */
	set_colors(colors, 0, 0, 0);
	led_set_color(tlc59731, 0, NUM_COLORS, colors);
	k_msleep(TEST_DELAY);
}

void test_led_on_off(void)
{
	/* Turn on RED, GREEN, BLUE */
	led_on(tlc59731, RED);
	k_msleep(TEST_DELAY);

	led_on(tlc59731, GREEN);
	k_msleep(TEST_DELAY);

	led_on(tlc59731, BLUE);
	k_msleep(TEST_DELAY);

	/* Turn off BLUE, GREEN, RED */
	led_off(tlc59731, BLUE);
	k_msleep(TEST_DELAY);

	led_off(tlc59731, GREEN);
	k_msleep(TEST_DELAY);

	led_off(tlc59731, RED);
	k_msleep(TEST_DELAY);
}


int main(void)
{
	int fret = 0;

	fret = gpio_pin_configure_dt(&led_cs, GPIO_OUTPUT_ACTIVE);
	if (fret < 0) {
		printf("Error conf CS pin\r\n");
		return fret;
	}

	/* Enable RGB chip select on the board */
	fret = gpio_pin_set_dt(&led_cs, HIGH);
	if (fret < 0) {
		return fret;
	}

	k_msleep(5);

	if (tlc59731) {
		LOG_DBG("Found LED controller %s", tlc59731->name);
	} else if (!device_is_ready(tlc59731)) {
		LOG_ERR("LED device %s is not ready", tlc59731->name);
		return 0;
	}

	LOG_INF("Found LED device %s", tlc59731->name);

	while (1) {
		test_led_on_off();
		test_rgb_brightness();
		test_rgb_set_color();
	}

	return 0;
}
