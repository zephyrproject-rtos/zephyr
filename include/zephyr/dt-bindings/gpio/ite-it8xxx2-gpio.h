/*
 * Copyright (c) 2022 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_ITE_IT8XXX2_GPIO_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_ITE_IT8XXX2_GPIO_H_

/**
 * @name GPIO pin voltage flags
 *
 * The voltage flags are a Zephyr specific extension of the standard GPIO flags
 * specified by the Linux GPIO binding for use with the ITE IT8xxx2 SoCs.
 *
 * @{
 */

/** @cond INTERNAL_HIDDEN */
#define IT8XXX2_GPIO_VOLTAGE_POS	11
#define IT8XXX2_GPIO_VOLTAGE_MASK	(3U << IT8XXX2_GPIO_VOLTAGE_POS)
/** @endcond */

/** Set pin at the default voltage level */
#define IT8XXX2_GPIO_VOLTAGE_DEFAULT	(0U << IT8XXX2_GPIO_VOLTAGE_POS)
/** Set pin voltage level at 1.8 V */
#define IT8XXX2_GPIO_VOLTAGE_1P8	(1U << IT8XXX2_GPIO_VOLTAGE_POS)
/** Set pin voltage level at 3.3 V */
#define IT8XXX2_GPIO_VOLTAGE_3P3	(2U << IT8XXX2_GPIO_VOLTAGE_POS)
/** Set pin voltage level at 5.0 V */
#define IT8XXX2_GPIO_VOLTAGE_5P0	(3U << IT8XXX2_GPIO_VOLTAGE_POS)

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_ITE_IT8XXX2_GPIO_H_ */
