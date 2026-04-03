/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include "ulp_lp_core_utils.h"

#define WAKEUP_PIN_NODE DT_ALIAS(wakeup_pin)

static const struct gpio_dt_spec wakeup_pin = GPIO_DT_SPEC_GET(WAKEUP_PIN_NODE, gpios);

int main(void)
{
	ulp_lp_core_wakeup_main_processor();

	gpio_pin_configure_dt(&wakeup_pin, GPIO_INPUT);
	gpio_pin_interrupt_configure_dt(&wakeup_pin, GPIO_INT_LEVEL_ACTIVE);

	ulp_lp_core_halt();

	return 0;
}
