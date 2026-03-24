/*
 * Copyright (c) 2026 Arm Limited (or its affiliates). All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT arm_fvp_leds

#include <errno.h>
#include <zephyr/drivers/led.h>

/**
 * This file contains code to get S6LED 0-7 working on Arm FVPs.
 *
 * The LEDs are simple - they are just mapped to bits [7:0] of the SYS_LED register at 0x1C010008.
 */

#define S6LED_REG             DT_INST_REG_ADDR(0) /* Get LED Register location */
#define FVP_REGISTER_MAP_SIZE 0x1000              /* 4KB pages for register mapping */

/* Config struct */
struct led_fvp_config {
	size_t num_leds;
};

static uint8_t *s6led_ptr;

static int led_fvp_set_brightness(const struct device *dev, uint32_t led, uint8_t value)
{
	const struct led_fvp_config *config = dev->config;

	/* Sanity check: FVP should have 8 S6LEDs */
	if (config->num_leds != 8) {
		return -EINVAL;
	}

	/* Any brightness above 0 is ON */
	if (value > 0) {
		*s6led_ptr |= (0x1 << led); /* Set correct LED on */
	} else {
		*s6led_ptr &= ~(0x1 << led); /* Set correct LED off */
	}

	return 0;
}

static int led_fvp_on(const struct device *dev, uint32_t led)
{
	const struct led_fvp_config *config = dev->config;

	/* Sanity check: FVP should have 8 S6LEDs */
	if (config->num_leds != 8) {
		return -EINVAL;
	}

	led_fvp_set_brightness(dev, led, 255); /* Handle this in led_fvp_set_brightness() */

	return 0;
}

static int led_fvp_off(const struct device *dev, uint32_t led)
{
	const struct led_fvp_config *config = dev->config;

	/* Sanity check: FVP should have 8 S6LEDs */
	if (config->num_leds != 8) {
		return -EINVAL;
	}

	led_fvp_set_brightness(dev, led, 0); /* Handle this in led_fvp_set_brightness() */

	return 0;
}

static int led_fvp_init(const struct device *dev)
{
	/* Map the system registers, including the LED register */
	/* This function already handles proper alignment and offset */
	k_mem_map_phys_bare(&s6led_ptr, S6LED_REG, FVP_REGISTER_MAP_SIZE,
			    K_MEM_PERM_RW | K_MEM_CACHE_NONE);

	*s6led_ptr = 0x0; /* Clear out the LEDs */

	return 0;
}

static DEVICE_API(led, led_arm_fvp_api) = {
	.on = led_fvp_on,
	.off = led_fvp_off,
	.set_brightness = led_fvp_set_brightness,
};

#define LED_FVP_DEVICE(i)                                                                          \
	static const struct led_fvp_config led_fvp_config_##i = {                                  \
		.num_leds = DT_CHILD_NUM(DT_INST(i, arm_fvp_leds)),                                \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(i, &led_fvp_init, NULL, NULL, &led_fvp_config_##i, POST_KERNEL,      \
			      CONFIG_LED_INIT_PRIORITY, &led_arm_fvp_api);

DT_INST_FOREACH_STATUS_OKAY(LED_FVP_DEVICE)
