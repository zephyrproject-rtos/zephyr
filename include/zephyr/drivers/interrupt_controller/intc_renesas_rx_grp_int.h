/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Renesas RX group interrupt controller header file
 */

#ifndef ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_RENESAS_RX_GRP_INT_H_
#define ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_RENESAS_RX_GRP_INT_H_

#include "platform.h"

/**
 * @brief Enables or disables a group interrupt for a given interrupt vector.
 *
 * @param dev RX group interrupt device.
 * @param vector The interrupt vector (bsp_int_src_t) to be controlled.
 * @param set A boolean indicating enable or disable.
 *
 * @retval 0 If successful.
 * @retval -EINVAL if the interrupt control operation fails.
 */
int rx_grp_intc_set_grp_int(const struct device *dev, bsp_int_src_t vector, bool set);

/**
 * @brief Enables or disables a specific group interrupt source by setting or clearing the
 * corresponding bit (vector_num) in the group interrupt register.
 *
 * @param dev RX group interrupt device.
 * @param vector_num Index of the interrupt source (0–31) within the group.
 * @param set A boolean indicating enable or disable.
 *
 * @retval 0 If successful.
 * @retval -EINVAL if the interrupt control operation fails.
 */
int rx_grp_intc_set_gen(const struct device *dev, uint8_t vector_num, bool set);

/**
 * @brief Registers a callback function for a specific group interrupt source (vector). When the
 * interrupt is triggered, the provided callback is executed with the associated context.
 *
 * @param dev RX group interrupt device.
 * @param vector Interrupt source to attach the callback to.
 * @param callback Function to be called when the interrupt occurs.
 * @param context Pointer to user-defined data passed to the callback.
 *
 * @retval 0 if successful.
 * @retval -EINVAL if the callback registration fails.
 */
int rx_grp_intc_set_callback(const struct device *dev, bsp_int_src_t vector, bsp_int_cb_t callback,
			     void *context);

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_RENESAS_RX_GRP_INT_H_ */
