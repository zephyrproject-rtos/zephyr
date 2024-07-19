/*
 * Copyright (c) 2024 Linaro LTD
 * SPDX-License-Identifier: Apache-2.0
 */

/* This main is brought into the Rust application. */

#ifdef CONFIG_RUST

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

extern void rust_main(void);

const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

int main(void)
{
	rust_main();
	return 0;
}

#endif
