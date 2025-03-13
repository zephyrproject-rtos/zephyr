/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_RZ_EXT_IRQ_H_
#define ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_RZ_EXT_IRQ_H_

/** RZ external interrupt callback */
typedef void (*intc_rz_ext_irq_callback_t)(void *arg);

/**
 * @brief Enable external interrupt for specified channel at NVIC.
 *
 * @param dev: pointer to interrupt controller instance
 * @return 0 on success, or negative value on error
 */
int intc_rz_ext_irq_enable(const struct device *dev);

/**
 * @brief Disable external interrupt for specified channel at NVIC.
 *
 * @param dev: pointer to interrupt controller instance
 * @return 0 on success, or negative value on error
 */
int intc_rz_ext_irq_disable(const struct device *dev);

/**
 * @brief Updates the user callback
 *
 * @param dev: pointer to interrupt controller instance
 * @param cb: callback to set
 * @param arg: user data passed to callback
 * @return 0 on success, or negative value on error
 */
int intc_rz_ext_irq_set_callback(const struct device *dev, intc_rz_ext_irq_callback_t cb,
				 void *arg);

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_RZ_EXT_IRQ_H_ */
