/*
 * Copyright (c) 2022 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_TI_CC13XX_CC26XX_GPIO_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_TI_CC13XX_CC26XX_GPIO_H_

/**
 * @brief Enable GPIO pin debounce.
 *
 * The debounce flag is a Zephyr specific extension of the standard GPIO flags
 * specified by the Linux GPIO binding. Only applicable for Texas
 * Instruments CC1xx/CC26xx SoCs.
 */
#define CC13XX_CC26XX_GPIO_DEBOUNCE (1U << 8)

/**
 * @name GPIO drive strength flags
 *
 * The drive strength flags are a Zephyr specific extension of the standard GPIO
 * flags specified by the Linux GPIO binding. Only applicable for Texas
 * Instruments CC13xx/CC26xx SoCs.
 *
 * The interface supports two different drive strengths:
 * `DFLT` - The lowest drive strength supported by the HW
 * `ALT` - The highest drive strength supported by the HW
 *
 * @{
 */
/** @cond INTERNAL_HIDDEN */
#define CC13XX_CC26XX_GPIO_DS_POS 9
#define CC13XX_CC26XX_GPIO_DS_MASK (0x3U << CC13XX_CC26XX_GPIO_DS_POS)
/** @endcond */

/** Default drive strength. */
#define CC13XX_CC26XX_GPIO_DS_DFLT (0x0U << CC13XX_CC26XX_GPIO_DS_POS)

/** Alternative drive strength. */
#define CC13XX_CC26XX_GPIO_DS_ALT (0x3U << CC13XX_CC26XX_GPIO_DS_POS)

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_TI_CC13XX_CC26XX_GPIO_H_ */
