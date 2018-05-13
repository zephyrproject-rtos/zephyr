/*
 * Copyright (c) 2018 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _DRIVERS_GPIO_SAM0_
#define _DRIVERS_GPIO_SAM0_

/* Shared structures between gpio_sam0.c and gpio_sam0_eic.c */

struct gpio_sam0_config {
	PortGroup *regs;
	u8_t id;
};

int eic_sam0_config(u8_t target, u8_t extint, int flags);
u8_t eic_sam0_get_target(struct device *port, u32_t pin);
int gpio_sam0_manage_callback(struct device *dev,
			      struct gpio_callback *callback, bool set);
int gpio_sam0_enable_callback(struct device *dev, int access_op, u32_t pin);
int gpio_sam0_disable_callback(struct device *dev, int access_op, u32_t pin);
u32_t gpio_sam0_get_pending_int(struct device *dev);

#endif
