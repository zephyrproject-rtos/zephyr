/*
 * Copyright (c) 2019 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PWM_PWM_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PWM_PWM_H_

/**
 * @brief PWM Interface
 * @defgroup pwm_interface PWM Interface
 * @ingroup io_interfaces
 * @{
 */

/**
 * @name PWM period set helpers
 * The period cell in the PWM specifier needs to be provided in nanoseconds.
 * However, in some applications it is more convenient to use another scale.
 * @{
 */

/** Specify PWM period in nanoseconds */
#define PWM_NSEC(x)	(x)
/** Specify PWM period in microseconds */
#define PWM_USEC(x)	(PWM_NSEC(x) * 1000UL)
/** Specify PWM period in milliseconds */
#define PWM_MSEC(x)	(PWM_USEC(x) * 1000UL)
/** Specify PWM period in seconds */
#define PWM_SEC(x)	(PWM_MSEC(x) * 1000UL)
/** Specify PWM frequency in hertz */
#define PWM_HZ(x)	(PWM_SEC(1UL) / (x))
/** Specify PWM frequency in kilohertz */
#define PWM_KHZ(x)	(PWM_HZ((x) * 1000UL))

/** @} */

/**
 * @name PWM polarity flags
 * The `PWM_POLARITY_*` flags are used with pwm_set_cycles(), pwm_set()
 * or pwm_configure_capture() to specify the polarity of a PWM channel.
 *
 * The flags are on the lower 8bits of the pwm_flags_t
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

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PWM_PWM_H_ */
