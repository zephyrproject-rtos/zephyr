/*
 * Copyright (c) 2025 ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_ENE_KB106X_GPIO_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_ENE_KB106X_GPIO_H_

/**
 * @name GPIO pin voltage flags
 *
 * The voltage flags are a Zephyr specific extension of the standard GPIO flags
 * specified by the Linux GPIO binding for use with the ENE KB106x SoCs.
 *
 * @{
 *  Note: Bits 15 down to 8 are reserved for SoC specific flags.
 */

/** @cond INTERNAL_HIDDEN */
#define ENE_GPIO_VOLTAGE_POS  8
#define ENE_GPIO_VOLTAGE_MASK (1U << ENE_GPIO_VOLTAGE_POS)

#define ENE_GPIO_DRIVING_POS  9
#define ENE_GPIO_DRIVING_MASK (1U << ENE_GPIO_DRIVING_POS)
/** @endcond */

/** Set pin at the default voltage level (3.3V) */
#define ENE_GPIO_VOLTAGE_DEFAULT (0U << ENE_GPIO_VOLTAGE_POS)
/** Set pin voltage level at 1.8 V */
#define ENE_GPIO_VOLTAGE_1P8     (1U << ENE_GPIO_VOLTAGE_POS)

/** Set pin at the default driving current (4mA) */
#define ENE_GPIO_DRIVING_DEFAULT (0U << ENE_GPIO_DRIVING_POS)
/** Set pin driving current at 16mA */
#define ENE_GPIO_DRIVING_16MA    (1U << ENE_GPIO_DRIVING_POS)

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_ENE_KB106X_GPIO_H_ */
