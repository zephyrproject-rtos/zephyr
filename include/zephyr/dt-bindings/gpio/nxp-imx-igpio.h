/*
 * Copyright (c) 2025 Feniex Industries Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_NXP_IMX_IGPIO_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_NXP_IMX_IGPIO_H_

/**
 * @name GPIO pull strength flags
 *
 * The pull strength flags are a Zephyr specific extension of the standard GPIO
 * flags specified by the Linux GPIO binding. Only applicable for NXP IMX
 * SoCs.
 *
 * The interface supports two different pull strengths:
 * `WEAK` - The lowest pull strength supported by the HW
 * `STRONG` - The highest pull strength supported by the HW
 *
 * @{
 */
/** @cond INTERNAL_HIDDEN */
#define NXP_IGPIO_PULL_STRENGTH_POS 8
#define NXP_IGPIO_PULL_STRENGTH_MASK (0x1U << NXP_IGPIO_PULL_STRENGTH_POS)
/** @endcond */

/** pull up/down strengths (only applies to CONFIG_SOC_SERIES_IMXRT10XX) */
#define NXP_IGPIO_PULL_WEAK (0x0U << NXP_IGPIO_PULL_STRENGTH_POS)
#define NXP_IGPIO_PULL_STRONG (0x1U << NXP_IGPIO_PULL_STRENGTH_POS)

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_NXP_IMX_IGPIO_H_ */
