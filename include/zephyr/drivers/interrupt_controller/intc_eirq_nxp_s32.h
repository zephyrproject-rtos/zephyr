/*
 * Copyright 2022, 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Driver for External interrupt/event controller in NXP S32 MCUs
 *
 */

#ifndef ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_EIRQ_NXP_S32_H_
#define ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_EIRQ_NXP_S32_H_

/** NXP SIUL2 EIRQ callback */
typedef void (*eirq_nxp_s32_callback_t)(uint8_t pin, void *arg);

/**
 * @brief NXP SIUL2 EIRQ pin activation type
 */
enum eirq_nxp_s32_trigger {
	/** Interrupt triggered on rising edge */
	EIRQ_NXP_S32_RISING_EDGE,
	/** Interrupt triggered on falling edge */
	EIRQ_NXP_S32_FALLING_EDGE,
	/** Interrupt triggered on either edge */
	EIRQ_NXP_S32_BOTH_EDGES,
};

/**
 * @brief Unset interrupt callback
 *
 * @param dev SIUL2 EIRQ device
 * @param irq interrupt number
 */
void eirq_nxp_s32_unset_callback(const struct device *dev, uint8_t irq);

/**
 * @brief Set callback for an interrupt associated with a given pin
 *
 * @param dev SIUL2 EIRQ device
 * @param irq interrupt number
 * @param pin GPIO pin associated with the interrupt
 * @param cb  callback to install
 * @param arg user data to include in callback
 *
 * @retval 0 on success
 * @retval -EBUSY if callback for the interrupt is already set
 */
int eirq_nxp_s32_set_callback(const struct device *dev, uint8_t irq, uint8_t pin,
				eirq_nxp_s32_callback_t cb, void *arg);

/**
 * @brief Enable interrupt on a given trigger event
 *
 * @param dev SIUL2 EIRQ device
 * @param irq interrupt number
 * @param trigger trigger event
 */
void eirq_nxp_s32_enable_interrupt(const struct device *dev, uint8_t irq,
				     enum eirq_nxp_s32_trigger trigger);

/**
 * @brief Disable interrupt
 *
 * @param dev SIUL2 EIRQ device
 * @param irq interrupt number
 */
void eirq_nxp_s32_disable_interrupt(const struct device *dev, uint8_t irq);

/**
 * @brief Get pending interrupts
 *
 * @param dev SIUL2 EIRQ device
 * @return A bitmask containing pending pending interrupts
 */
uint32_t eirq_nxp_s32_get_pending(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_EIRQ_NXP_S32_H_ */
