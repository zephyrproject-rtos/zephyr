/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <led_strip.h>

#include <string.h>

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_LED_STRIP_LEVEL
#include <logging/sys_log.h>

#include <zephyr.h>
#include <board.h>
#include <gpio.h>
#include <device.h>
#include <clock_control.h>

#define BLOCKING ((void *)1)

static int send_buf(u8_t *buf, size_t len)
{
	/* Address of OUTSET. OUTCLR is OUTSET + 4 */
	volatile u32_t *base = (u32_t *)(NRF_GPIO_BASE + 0x508);
	u32_t pin = BIT(CONFIG_WS2812B_SW_GPIO_PIN);
	struct device *clock;
	unsigned int key;
	/* Initilization of i is strictly not needed, but it avoids an
	 * uninitialized warning with the inline assembly.
	 */
	u32_t i = 0;

	clock = device_get_binding(CONFIG_CLOCK_CONTROL_NRF5_M16SRC_DRV_NAME);
	if (!clock) {
		SYS_LOG_ERR("Unable to get HF clock");
		return -EIO;
	}

	/* The inline assembly further below is designed to work only with
	 * the 16 MHz clock enabled.
	 */
	clock_control_on(clock, BLOCKING);
	key = irq_lock();

	while (len--) {
		u32_t b = *buf++;

		/* Generate signal out of the bits, MSB. 1-bit should be
		 * roughly 0.85us high, 0.4us low, whereas a 0-bit should be
		 * roughly 0.4us high, 0.85us low.
		 */
		__asm volatile ("movs %[i], #8\n" /* i = 8 */
				".start_bit:\n"

				/* OUTSET = BIT(LED_PIN) */
				"strb %[p], [%[r], #0]\n"

				/* if (b & 0x80) goto .long */
				"tst %[b], %[m]\n"
				"bne .long\n"

				/* 0-bit */
				"nop\nnop\n"
				/* OUTCLR = BIT(LED_PIN) */
				"strb %[p], [%[r], #4]\n"
				"nop\nnop\nnop\n"
				"b .next_bit\n"

				/* 1-bit */
				".long:\n"
				"nop\nnop\nnop\nnop\nnop\nnop\nnop\n"
				/* OUTCLR = BIT(LED_PIN) */
				"strb %[p], [%[r], #4]\n"

				".next_bit:\n"
				/* b <<= 1 */
				"lsl %[b], #1\n"
				/* i-- */
				"sub %[i], #1\n"
				/* if (i > 0) goto .start_bit */
				"bne .start_bit\n"
				:
				[i] "+r" (i)
				:
				[b] "l" (b),
				[m] "l" (0x80),
				[r] "l" (base),
				[p] "r" (pin)
				:);
	}

	irq_unlock(key);
	clock_control_off(clock, NULL);

	return 0;
}

static int ws2812b_sw_update_rgb(struct device *dev, struct led_rgb *pixels,
				 size_t num_pixels)
{
	u8_t *ptr = (u8_t *)pixels;
	size_t i;

	/* Convert from RGB to GRB format */
	for (i = 0; i < num_pixels; i++) {
		u8_t r = pixels[i].r;
		u8_t b = pixels[i].b;
		u8_t g = pixels[i].g;

		*ptr++ = g;
		*ptr++ = r;
		*ptr++ = b;
	}

	return send_buf((u8_t *)pixels, num_pixels * 3);
}

static int ws2812b_sw_update_channels(struct device *dev, u8_t *channels,
				      size_t num_channels)
{
	SYS_LOG_ERR("update_channels not implemented");
	return -ENOSYS;
}

static int ws2812b_sw_init(struct device *dev)
{
	struct device *gpio;

	gpio = device_get_binding(CONFIG_WS2812B_SW_GPIO_NAME);
	if (!gpio) {
		SYS_LOG_ERR("Unable to find %s", CONFIG_WS2812B_SW_GPIO_NAME);
		return -ENODEV;
	}

	gpio_pin_configure(gpio, CONFIG_WS2812B_SW_GPIO_PIN, GPIO_DIR_OUT);

	return 0;
}

static const struct led_strip_driver_api ws2812b_sw_api = {
	.update_rgb = ws2812b_sw_update_rgb,
	.update_channels = ws2812b_sw_update_channels,
};

DEVICE_AND_API_INIT(ws2812b_sw, CONFIG_WS2812B_SW_NAME, ws2812b_sw_init, NULL,
		    NULL, POST_KERNEL, CONFIG_LED_STRIP_INIT_PRIORITY,
		    &ws2812b_sw_api);
