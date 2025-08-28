/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_NXP_S32_GPIO_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_NXP_S32_GPIO_H_

/**
 * @brief NXP S32 GPIO specific flags
 *
 * The driver flags are encoded in the 8 upper bits of @ref gpio_dt_flags_t as
 * follows:
 *
 * - Bit 8: Interrupt controller to which the respective GPIO interrupt is routed.
 *
 * @ingroup gpio_interface_ext
 * @{
 */

/** @cond INTERNAL_HIDDEN */
#define NXP_S32_GPIO_INT_CONTROLLER_POS		8
#define NXP_S32_GPIO_INT_CONTROLLER_MASK	(0x1U << NXP_S32_GPIO_INT_CONTROLLER_POS)
/** @endcond */

/**
 * @name NXP S32 GPIO interrupt controller routing flags
 * @brief NXP S32 GPIO interrupt controller routing flags
 * @{
 */

/** Interrupt routed to the WKPU controller */
#define NXP_S32_GPIO_INT_WKPU	(0x1U << NXP_S32_GPIO_INT_CONTROLLER_POS)

/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_NXP_S32_GPIO_H_ */
