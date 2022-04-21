/*
 * Copyright (c) 2022 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_NORDIC_NRF_GPIO_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_NORDIC_NRF_GPIO_H_

/**
 * @name GPIO drive strength flags
 *
 * The drive strength flags are a Zephyr specific extension of the standard GPIO
 * flags specified by the Linux GPIO binding. Only applicable for Nordic
 * Semiconductor nRF SoCs.
 *
 * The drive strength of individual pins can be configured
 * independently for when the pin output is low and high.
 *
 * The `GPIO_DS_*_LOW` enumerations define the drive strength of a pin
 * when output is low.

 * The `GPIO_DS_*_HIGH` enumerations define the drive strength of a pin
 * when output is high.
 *
 * The interface supports two different drive strengths:
 * `DFLT` - The lowest drive strength supported by the HW
 * `ALT` - The highest drive strength supported by the HW
 *
 * @{
 */
/** @cond INTERNAL_HIDDEN */
#define NRF_GPIO_DS_LOW_POS 9
#define NRF_GPIO_DS_LOW_MASK (0x1U << NRF_GPIO_DS_LOW_POS)
/** @endcond */

/** Default drive strength standard when GPIO pin output is low.
 */
#define NRF_GPIO_DS_DFLT_LOW (0x0U << NRF_GPIO_DS_LOW_POS)

/** Alternative drive strength when GPIO pin output is low.
 * For hardware that does not support configurable drive strength
 * use the default drive strength.
 */
#define NRF_GPIO_DS_ALT_LOW (0x1U << NRF_GPIO_DS_LOW_POS)

/** @cond INTERNAL_HIDDEN */
#define NRF_GPIO_DS_HIGH_POS 10
#define NRF_GPIO_DS_HIGH_MASK (0x1U << NRF_GPIO_DS_HIGH_POS)
/** @endcond */

/** Default drive strength when GPIO pin output is high.
 */
#define NRF_GPIO_DS_DFLT_HIGH (0x0U << NRF_GPIO_DS_HIGH_POS)

/** Alternative drive strength when GPIO pin output is high.
 * For hardware that does not support configurable drive strengths
 * use the default drive strength.
 */
#define NRF_GPIO_DS_ALT_HIGH (0x1U << NRF_GPIO_DS_HIGH_POS)

/** Combined default drive strength.
 */
#define NRF_GPIO_DS_DFLT (NRF_GPIO_DS_DFLT_LOW | NRF_GPIO_DS_DFLT_HIGH)

/** Combined alternative drive strength.
 */
#define NRF_GPIO_DS_ALT (NRF_GPIO_DS_ALT_LOW | NRF_GPIO_DS_ALT_HIGH)

/** @cond INTERNAL_HIDDEN */
#define NRF_GPIO_DS_MASK (NRF_GPIO_DS_LOW_MASK | NRF_GPIO_DS_HIGH_MASK)
/** @endcond */
/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_NORDIC_NRF_GPIO_H_ */
