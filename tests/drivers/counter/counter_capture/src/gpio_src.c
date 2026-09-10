/*
 * Copyright (c) 2026 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Trigger-source backend for real hardware: the capture input is driven by a
 * GPIO that is physically wired (loopback) to the counter's capture pin.
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/devicetree.h>

#include "trigger_src.h"

#define CAPTURE_COUNT DT_PROP_LEN(DT_NODELABEL(counter_loopback_0), test_counter_captures)

/* clang-format off */
static const struct gpio_dt_spec capture_tester_gpios[] = {
	DT_FOREACH_PROP_ELEM_SEP(
		DT_PATH(zephyr_user),
		capture_tester_gpios,
		GPIO_DT_SPEC_GET_BY_IDX, (,))
};
/* clang-format on */

BUILD_ASSERT(ARRAY_SIZE(capture_tester_gpios) == CAPTURE_COUNT,
	     "capture_tester_gpios and test-counter-captures size mismatch");

int trigger_src_setup(size_t idx)
{
	if (!gpio_is_ready_dt(&capture_tester_gpios[idx])) {
		return -ENODEV;
	}

	return gpio_pin_configure_dt(&capture_tester_gpios[idx], GPIO_OUTPUT_LOW);
}

int trigger_src_get(size_t idx)
{
	return gpio_pin_get_dt(&capture_tester_gpios[idx]);
}

int trigger_src_set(size_t idx, int level)
{
	return gpio_pin_set_dt(&capture_tester_gpios[idx], level);
}
