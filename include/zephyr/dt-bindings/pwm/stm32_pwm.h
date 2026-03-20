/*
 * Copyright (c) 2022 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PWM_STM32_PWM_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PWM_STM32_PWM_H_

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
#define STM32_PWM_COMPLEMENTARY	(1U << 8)
/**
 * @deprecated Use the PWM complementary `STM32_PWM_COMPLEMENTARY` flag instead.
 */
#define PWM_STM32_COMPLEMENTARY	(1U << 8)

/** @cond INTERNAL_HIDDEN */
#define STM32_PWM_COMPLEMENTARY_MASK	0x100
/** @endcond */
/** @} */


/**
 * @name custom PWM flags for input capture prescaler
 * These flags can be used with the `pwm_configure_capture` API call to set the input capture
 * prescaler (ICxPSC in TIMx_CCMRx). Only useful with PWM_CAPTURE_TYPE_PERIOD.
 * @{
 */
 /** @cond INTERNAL_HIDDEN */
#define STM32_PWM_CAPTURE_PSC_POS  9
#define STM32_PWM_CAPTURE_PSC_MASK (0x3 << STM32_PWM_CAPTURE_PSC_POS)
/** @endcond */

 /** PWM input capture prescaler is set to 1 (default) */
#define STM32_PWM_CAPTURE_PSC_1 (0U << STM32_PWM_CAPTURE_PSC_POS)

/** PWM input capture prescaler is set to 2 */
#define STM32_PWM_CAPTURE_PSC_2 (1U << STM32_PWM_CAPTURE_PSC_POS)

/** PWM input capture prescaler is set to 4 */
#define STM32_PWM_CAPTURE_PSC_4 (2U << STM32_PWM_CAPTURE_PSC_POS)

/** PWM input capture prescaler is set to 8 */
#define STM32_PWM_CAPTURE_PSC_8 (3U << STM32_PWM_CAPTURE_PSC_POS)

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PWM_STM32_PWM_H_ */
