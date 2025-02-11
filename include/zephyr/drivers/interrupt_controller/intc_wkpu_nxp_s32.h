/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Driver for Wake-up interrupt/event controller in NXP S32 MCUs
 */

#ifndef ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_WKPU_NXP_S32_H_
#define ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_WKPU_NXP_S32_H_

#include <Wkpu_Ip.h>

/* Wrapper callback for WKPU line */
typedef void (*wkpu_nxp_s32_callback_t)(uint8_t pin, void *arg);

/**
 * @brief Unset WKPU callback for line
 *
 * @param dev WKPU device
 * @param line WKPU line
 */
void wkpu_nxp_s32_unset_callback(const struct device *dev, uint8_t line);

/**
 * @brief Set WKPU callback for line
 *
 * @param dev  WKPU device
 * @param line WKPU line
 * @param cb   Callback
 * @param pin  GPIO pin
 * @param arg  Callback data
 *
 * @retval 0 on SUCCESS
 * @retval -EBUSY if callback for the line is already set
 */
int wkpu_nxp_s32_set_callback(const struct device *dev, uint8_t line,
			      wkpu_nxp_s32_callback_t cb, uint8_t pin, void *arg);

/**
 * @brief Set edge event and enable interrupt for WKPU line
 *
 * @param dev  WKPU device
 * @param line WKPU line
 * @param edge_type Type of edge event
 */
void wkpu_nxp_s32_enable_interrupt(const struct device *dev, uint8_t line,
				   Wkpu_Ip_EdgeType edge_type);

/**
 * @brief Disable interrupt for WKPU line
 *
 * @param dev  WKPU device
 * @param line WKPU line
 */
void wkpu_nxp_s32_disable_interrupt(const struct device *dev, uint8_t line);

/**
 * @brief Get pending interrupt for WKPU device
 *
 * @param dev WKPU device
 * @return A mask contains pending flags
 */
uint64_t wkpu_nxp_s32_get_pending(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_WKPU_NXP_S32_H_ */
