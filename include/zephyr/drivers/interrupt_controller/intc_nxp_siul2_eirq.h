/*
 * Copyright 2022, 2024-2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Driver for NXP SIUL2 external interrupt/event controller.
 *
 */

#ifndef ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_NXP_SIUL2_EIRQ_H_
#define ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_NXP_SIUL2_EIRQ_H_

/** NXP SIUL2 EIRQ callback */
typedef void (*nxp_siul2_eirq_callback_t)(uint8_t pin, void *arg);

/**
 * @brief NXP SIUL2 EIRQ pin activation type
 */
enum nxp_siul2_eirq_trigger {
	/** Interrupt triggered on rising edge */
	NXP_SIUl2_EIRQ_RISING_EDGE,
	/** Interrupt triggered on falling edge */
	NXP_SIUl2_EIRQ_FALLING_EDGE,
	/** Interrupt triggered on either edge */
	NXP_SIUl2_EIRQ_BOTH_EDGES,
};

/**
 * @brief Unset interrupt callback
 *
 * @param dev SIUL2 EIRQ device
 * @param irq interrupt number
 */
void nxp_siul2_eirq_unset_callback(const struct device *dev, uint8_t irq);

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
int nxp_siul2_eirq_set_callback(const struct device *dev, uint8_t irq, uint8_t pin,
				nxp_siul2_eirq_callback_t cb, void *arg);

/**
 * @brief Enable interrupt on a given trigger event
 *
 * @param dev SIUL2 EIRQ device
 * @param irq interrupt number
 * @param trigger trigger event
 */
void nxp_siul2_eirq_enable_interrupt(const struct device *dev, uint8_t irq,
				     enum nxp_siul2_eirq_trigger trigger);

/**
 * @brief Disable interrupt
 *
 * @param dev SIUL2 EIRQ device
 * @param irq interrupt number
 */
void nxp_siul2_eirq_disable_interrupt(const struct device *dev, uint8_t irq);

/**
 * @brief Get pending interrupts
 *
 * @param dev SIUL2 EIRQ device
 * @return A bitmask containing pending pending interrupts
 */
uint32_t nxp_siul2_eirq_get_pending(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_NXP_SIUL2_EIRQ_H_ */
