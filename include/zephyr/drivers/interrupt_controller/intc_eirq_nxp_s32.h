/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Driver for External interrupt/event controller in NXP S32 MCUs
 *
 */

#ifndef ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_EIRQ_NXP_S32_H_
#define ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_EIRQ_NXP_S32_H_

#include <Siul2_Icu_Ip.h>

/* Wrapper callback for EIRQ line */
typedef void (*eirq_nxp_s32_callback_t)(uint8_t pin, void *arg);

/**
 * @brief Unset EIRQ callback for line
 *
 * @param dev EIRQ device
 * @param line EIRQ line
 */
void eirq_nxp_s32_unset_callback(const struct device *dev, uint8_t line);

/**
 * @brief Set EIRQ callback for line
 *
 * @param dev  EIRQ device
 * @param line EIRQ line
 * @param cb   Callback
 * @param pin  GPIO pin
 * @param arg  Callback data
 *
 * @retval 0 on SUCCESS
 * @retval -EBUSY if callback for the line is already set
 */
int eirq_nxp_s32_set_callback(const struct device *dev, uint8_t line,
				eirq_nxp_s32_callback_t cb, uint8_t pin, void *arg);

/**
 * @brief Set edge event and enable interrupt for EIRQ line
 *
 * @param dev  EIRQ device
 * @param line EIRQ line
 * @param edge_type Type of edge event
 */
void eirq_nxp_s32_enable_interrupt(const struct device *dev, uint8_t line,
					Siul2_Icu_Ip_EdgeType edge_type);

/**
 * @brief Disable interrupt for EIRQ line
 *
 * @param dev  EIRQ device
 * @param line EIRQ line
 */
void eirq_nxp_s32_disable_interrupt(const struct device *dev, uint8_t line);

/**
 * @brief Get pending interrupt for EIRQ device
 *
 * @param dev EIRQ device
 * @return A mask contains pending flags
 */
uint32_t eirq_nxp_s32_get_pending(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_EIRQ_NXP_S32_H_ */
