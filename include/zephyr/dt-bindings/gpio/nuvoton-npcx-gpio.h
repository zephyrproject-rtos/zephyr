/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_NUVOTON_NPCX_GPIO_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_NUVOTON_NPCX_GPIO_H_

/**
 * @name GPIO pin voltage flags
 *
 * The voltage flags are a Zephyr specific extension of the standard GPIO flags
 * specified by the Linux GPIO binding for use with the Nuvoton NPCX SoCs.
 *
 * @{
 */

/** @cond INTERNAL_HIDDEN */
#define NPCX_GPIO_VOLTAGE_POS	11
#define NPCX_GPIO_VOLTAGE_MASK	(1U << NPCX_GPIO_VOLTAGE_POS)
/** @endcond */

/** Set pin at the default voltage level (3.3V) */
#define NPCX_GPIO_VOLTAGE_DEFAULT	(0U << NPCX_GPIO_VOLTAGE_POS)
/** Set pin voltage level at 1.8 V */
#define NPCX_GPIO_VOLTAGE_1P8		(1U << NPCX_GPIO_VOLTAGE_POS)

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_NUVOTON_NPCX_GPIO_H_ */
