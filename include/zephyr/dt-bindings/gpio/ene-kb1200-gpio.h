/*
 * Copyright (c) 2024 ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_ENE_KB1200_GPIO_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_ENE_KB1200_GPIO_H_

/**
 * @name GPIO pin voltage flags
 *
 * The voltage and driving flags are a Zephyr specific extension of the standard GPIO flags
 * specified by the Linux GPIO binding for use with the ENE KB1200 SoC.
 *
 * @{
 *  Note: Bits 15 down to 8 are reserved for SoC specific flags.
 */

/** @cond INTERNAL_HIDDEN */
#define KB1200_GPIO_VOLTAGE_POS         8
#define KB1200_GPIO_VOLTAGE_MASK        (1U << KB1200_GPIO_VOLTAGE_POS)

#define KB1200_GPIO_DRIVING_POS         9
#define KB1200_GPIO_DRIVING_MASK        (1U << KB1200_GPIO_DRIVING_POS)
/** @endcond */

/** Set pin at the default voltage level (3.3V) */
#define KB1200_GPIO_VOLTAGE_DEFAULT     (0U << KB1200_GPIO_VOLTAGE_POS)
/** Set pin voltage level at 1.8 V */
#define KB1200_GPIO_VOLTAGE_1P8	        (1U << KB1200_GPIO_VOLTAGE_POS)

/** Set pin at the default driving current (4mA) */
#define KB1200_GPIO_DRIVING_DEFAULT     (0U << KB1200_GPIO_DRIVING_POS)
/** Set pin driving current at 16mA */
#define KB1200_GPIO_DRIVING_16MA        (1U << KB1200_GPIO_DRIVING_POS)


/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_ENE_KB1200_GPIO_H_ */
