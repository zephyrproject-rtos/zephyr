/*
 * Copyright (c) 2019 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PWM_PWM_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PWM_PWM_H_

/**
 * @name PWM polarity flags
 * The `PWM_POLARITY_*` flags are used with pwm_pin_set_cycles(),
 * pwm_pin_set_usec(), pwm_pin_set_nsec() or pwm_pin_configure_capture() to
 * specify the polarity of a PWM pin.
 * @{
 */
/** PWM pin normal polarity (active-high pulse). */
#define PWM_POLARITY_NORMAL	(0 << 0)

/** PWM pin inverted polarity (active-low pulse). */
#define PWM_POLARITY_INVERTED	(1 << 0)

/** @cond INTERNAL_HIDDEN */
#define PWM_POLARITY_MASK	0x1
/** @endcond */
/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PWM_PWM_H_ */
