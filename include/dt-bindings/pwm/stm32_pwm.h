/*
 * Copyright (c) 2022 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PWM_STM32_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PWM_STM32_H_

/**
 * @name custom PWM complementary flags for output pins
 * This flag can be used with any of the `pwm_pin_set_*` API calls to indicate
 * that the PWM signal has to be routed to the complementary output channel.
 * This feature is only available on certain SoC families, refer to the
 * binding's documentation for more details.
 * The custom flags are on the upper 8bits of the pwm_flags_t
 * @{
 */
/** PWM complementary output pin is enabled */
#define PWM_STM32_COMPLEMENTARY	(1U << 8)

/** @cond INTERNAL_HIDDEN */
#define PWM_STM32_COMPLEMENTARY_MASK	0x100
/** @endcond */
/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PWM_STM32_H_ */
