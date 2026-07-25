/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Perry Naseck, MIT Media Lab <pnaseck@media.mit.edu>
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Mock OOT fast GPIO backend for zephyr,gpio-emul.
 *
 * Minimal stub to test that the OOT include mechanism and
 * token-paste dispatch resolve correctly. Functions are empty;
 * only the symbols and types need to exist.
 */

#ifndef ZEPHYR_TEST_GPIO_FAST_ZEPHYR_GPIO_EMUL_H_
#define ZEPHYR_TEST_GPIO_FAST_ZEPHYR_GPIO_EMUL_H_

#include <zephyr/drivers/gpio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GPIO_FAST_OOT_BACKEND_ACTIVE 1

#define GPIO_FAST_SET_CYCLES_zephyr_gpio_emul        1234
#define GPIO_FAST_CLEAR_CYCLES_zephyr_gpio_emul      5678
#define GPIO_FAST_TOGGLE_CYCLES_zephyr_gpio_emul     91011
#define GPIO_FAST_GET_CYCLES_zephyr_gpio_emul        121314
#define GPIO_FAST_SET_INPUT_CYCLES_zephyr_gpio_emul  151617
#define GPIO_FAST_SET_OUTPUT_CYCLES_zephyr_gpio_emul 181920

/** @brief No RAM execution needed for test mock. */
#define GPIO_FAST_SEND_STREAM_ATTR_zephyr_gpio_emul

struct gpio_fast_spec_zephyr_gpio_emul {
	int dummy;
};

static ALWAYS_INLINE void gpio_fast_set_zephyr_gpio_emul(
	const struct gpio_fast_spec_zephyr_gpio_emul *spec)
{
	/** No-op for test mock */
}

static ALWAYS_INLINE void gpio_fast_clear_zephyr_gpio_emul(
	const struct gpio_fast_spec_zephyr_gpio_emul *spec)
{
	/** No-op for test mock */
}

static ALWAYS_INLINE void gpio_fast_toggle_zephyr_gpio_emul(
	const struct gpio_fast_spec_zephyr_gpio_emul *spec)
{
	/** No-op for test mock */
}

static ALWAYS_INLINE gpio_port_pins_t gpio_fast_get_zephyr_gpio_emul(
	const struct gpio_fast_spec_zephyr_gpio_emul *spec)
{
	/** No-op for test mock */
	return 0;
}

static ALWAYS_INLINE void gpio_fast_set_input_zephyr_gpio_emul(
	const struct gpio_fast_spec_zephyr_gpio_emul *spec)
{
	/** No-op for test mock */
}

static ALWAYS_INLINE void gpio_fast_set_output_zephyr_gpio_emul(
	const struct gpio_fast_spec_zephyr_gpio_emul *spec)
{
	/** No-op for test mock */
}

static inline int gpio_fast_configure_zephyr_gpio_emul(
	struct gpio_fast_spec_zephyr_gpio_emul *fast,
	const struct device *port, gpio_port_pins_t pin_mask,
	gpio_flags_t flags)
{
	/** No-op for test mock */
	return 0;
}

static inline int gpio_fast_pre_stream_zephyr_gpio_emul(
	const void *spec)
{
	/** No-op for test mock */
	return 0;
}

static inline int gpio_fast_post_stream_zephyr_gpio_emul(
	const void *spec)
{
	/** No-op for test mock */
	return 0;
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_TEST_GPIO_FAST_ZEPHYR_GPIO_EMUL_H_ */
