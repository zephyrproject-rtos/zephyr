/*
 * Copyright (c) 2024 GARDENA GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_SILABS_SI32_HD_GPIO_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_SILABS_SI32_HD_GPIO_H_

/**
 * @name GPIO drive strength flags
 *
 * The drive strength flags are a Zephyr specific extension of the standard GPIO flags specified by
 * the Linux GPIO binding. Only applicable for Silicon Labs SiM3 SoCs.
 *
 * The interface supports two different drive strengths:
 * `DFLT` - The lowest drive strength supported by the HW
 * `ALT` - The highest drive strength supported by the HW
 *
 * @{
 */
/** @cond INTERNAL_HIDDEN */
#define SI32_HD_GPIO_DS_POS 9
/** @endcond */

/** Enable port bank current limit on this pin. */
#define SI32_GPIO_DS_ENABLE_CURRENT_LIMIT (0x0U << SI32_HD_GPIO_DS_POS)

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_SILABS_SI32_HD_GPIO_H_ */
