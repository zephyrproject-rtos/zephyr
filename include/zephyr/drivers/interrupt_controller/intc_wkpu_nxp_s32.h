/*
 * Copyright 2023-2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Driver for Wake-up interrupt/event controller in NXP S32 MCUs
 */

#ifndef ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_WKPU_NXP_S32_H_
#define ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_WKPU_NXP_S32_H_

/** NXP WKPU callback */
typedef void (*wkpu_nxp_s32_callback_t)(uint8_t pin, void *arg);

/**
 * @brief NXP WKPU pin activation type
 */
enum wkpu_nxp_s32_trigger {
	/** Interrupt triggered on rising edge */
	WKPU_NXP_S32_RISING_EDGE,
	/** Interrupt triggered on falling edge */
	WKPU_NXP_S32_FALLING_EDGE,
	/** Interrupt triggered on either edge */
	WKPU_NXP_S32_BOTH_EDGES,
};

/**
 * @brief Unset WKPU callback for line
 *
 * @param dev WKPU device
 * @param irq WKPU interrupt number
 */
void wkpu_nxp_s32_unset_callback(const struct device *dev, uint8_t irq);

/**
 * @brief Set WKPU callback for line
 *
 * @param dev  WKPU device
 * @param irq  WKPU interrupt number
 * @param pin  GPIO pin
 * @param cb   Callback
 * @param arg  Callback data
 *
 * @retval 0 on SUCCESS
 * @retval -EBUSY if callback for the line is already set
 */
int wkpu_nxp_s32_set_callback(const struct device *dev, uint8_t irq, uint8_t pin,
			      wkpu_nxp_s32_callback_t cb,  void *arg);

/**
 * @brief Set edge event and enable interrupt for WKPU line
 *
 * @param dev  WKPU device
 * @param irq  WKPU interrupt number
 * @param trigger pin activation trigger
 */
void wkpu_nxp_s32_enable_interrupt(const struct device *dev, uint8_t irq,
				   enum wkpu_nxp_s32_trigger trigger);

/**
 * @brief Disable interrupt for WKPU line
 *
 * @param dev  WKPU device
 * @param irq  WKPU interrupt number
 */
void wkpu_nxp_s32_disable_interrupt(const struct device *dev, uint8_t irq);

/**
 * @brief Get pending interrupt for WKPU device
 *
 * @param dev WKPU device
 * @return A bitmask containing pending interrupts
 */
uint64_t wkpu_nxp_s32_get_pending(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_WKPU_NXP_S32_H_ */
