/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Gerson Fernando Budke
 * SPDX-FileCopyrightText: Copyright (c) 2026 Perry Naseck, MIT Media Lab <pnaseck@media.mit.edu>
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Generic (non-cycle-accurate) fast GPIO fallback.
 *
 * Wraps the standard GPIO API. Will NOT meet timing requirements
 * for protocols like WS2812, but enables builds for slower protocols
 * such as 1-Wire or servo control.
 *
 * This header is always included. Consumers opt into the generic backend
 * per-node by setting fast-gpio-backend = "generic" in their DT node.
 *
 * All types, functions, and constants are namespaced with the
 * _generic suffix for consistency with vendor backends. Generic
 * aliases (gpio_fast_set, etc.) are provided at the bottom.
 */

#ifndef ZEPHYR_DRIVERS_GPIO_FAST_GENERIC_H_
#define ZEPHYR_DRIVERS_GPIO_FAST_GENERIC_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Defined so drivers/applications can detect the generic fallback
 *        and decide whether the timing is acceptable for their use case.
 *        Also used in gpio_fast.c to conditionally compile the generic
 *        gpio_fast_configure() implementation.
 */
#define GPIO_FAST_GENERIC_FALLBACK 1

/*
 * Operation cost estimates for generic fallback (in CPU cycles).
 *
 * These are worst-case estimates for the full GPIO driver API call
 * chain: indirect function call, driver config lookup, register
 * access, and etc. Actual costs vary widely by driver.
 */
/** @brief Worst-case CPU cycles for gpio_fast_set() generic fallback. */
#define GPIO_FAST_SET_CYCLES_generic        50
/** @brief Worst-case CPU cycles for gpio_fast_clear() generic fallback. */
#define GPIO_FAST_CLEAR_CYCLES_generic      50
/** @brief Worst-case CPU cycles for gpio_fast_toggle() generic fallback. */
#define GPIO_FAST_TOGGLE_CYCLES_generic     60
/** @brief Worst-case CPU cycles for gpio_fast_get() generic fallback. */
#define GPIO_FAST_GET_CYCLES_generic        50
/** @brief Worst-case CPU cycles for gpio_fast_set_input() generic fallback. */
#define GPIO_FAST_SET_INPUT_CYCLES_generic  60
/** @brief Worst-case CPU cycles for gpio_fast_set_output() generic fallback. */
#define GPIO_FAST_SET_OUTPUT_CYCLES_generic 60

/** @brief No RAM execution needed for generic fallback. */
#define GPIO_FAST_SEND_STREAM_ATTR_generic

/**
 * @brief Fast GPIO spec for generic fallback.
 *
 * Stores the port device pointer and pin mask for delegation
 * to the standard GPIO API.
 */
struct gpio_fast_spec_generic {
	/** GPIO port device */
	const struct device *port;
	/** Pin bitmask */
	gpio_port_pins_t pin_mask;
};

/** @brief Set all masked pins HIGH via standard GPIO API. */
static ALWAYS_INLINE void gpio_fast_set_generic(
	const struct gpio_fast_spec_generic *spec)
{
	gpio_port_set_bits_raw(spec->port, spec->pin_mask);
}

/** @brief Set all masked pins LOW via standard GPIO API. */
static ALWAYS_INLINE void gpio_fast_clear_generic(
	const struct gpio_fast_spec_generic *spec)
{
	gpio_port_clear_bits_raw(spec->port, spec->pin_mask);
}

/** @brief Toggle all masked pins via standard GPIO API. */
static ALWAYS_INLINE void gpio_fast_toggle_generic(
	const struct gpio_fast_spec_generic *spec)
{
	gpio_port_toggle_bits(spec->port, spec->pin_mask);
}

/** @brief Read raw physical state of masked pins via standard GPIO API. */
static ALWAYS_INLINE gpio_port_pins_t gpio_fast_get_generic(
	const struct gpio_fast_spec_generic *spec)
{
	gpio_port_value_t val = 0;

	gpio_port_get_raw(spec->port, &val);
	return val & spec->pin_mask;
}

/** @brief Switch masked pins to input via standard GPIO API. */
static ALWAYS_INLINE void gpio_fast_set_input_generic(
	const struct gpio_fast_spec_generic *spec)
{
	gpio_port_set_masked_raw(spec->port, spec->pin_mask, 0);
}

/** @brief Switch masked pins to output via standard GPIO API. */
static ALWAYS_INLINE void gpio_fast_set_output_generic(
	const struct gpio_fast_spec_generic *spec)
{
	gpio_port_set_masked_raw(spec->port, spec->pin_mask,
				 spec->pin_mask);
}

int gpio_fast_configure_generic(struct gpio_fast_spec_generic *fast,
				const struct device *port,
				gpio_port_pins_t pin_mask,
				gpio_flags_t flags);
int gpio_fast_pre_stream_generic(const void *spec);
int gpio_fast_post_stream_generic(const void *spec);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_GPIO_FAST_GENERIC_H_ */
