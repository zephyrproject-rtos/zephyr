/*
 * Copyright (c) 2024-2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_RZ_EXT_IRQ_H_
#define ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_RZ_EXT_IRQ_H_

#define RZ_EXT_IRQ_TRIG_FALLING   0
#define RZ_EXT_IRQ_TRIG_RISING    1
#define RZ_EXT_IRQ_TRIG_BOTH_EDGE 2
#define RZ_EXT_IRQ_TRIG_LEVEL_LOW 3

/** RZ external interrupt callback */
typedef void (*intc_rz_ext_irq_callback_t)(void *arg);

/**
 * @brief Enable external interrupt for specified channel.
 *
 * @param dev: pointer to interrupt controller instance
 * @return 0 on success, or negative value on error
 */
int intc_rz_ext_irq_enable(const struct device *dev);

/**
 * @brief Disable external interrupt for specified channel.
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

/**
 * @brief Change trigger external interrupt type for specified channel.
 *
 * @param dev: pointer to interrupt controller instance
 * @param trig: trigger type to be changed
 * @return 0 on success, or negative value on error
 */
int intc_rz_ext_irq_set_type(const struct device *dev, uint8_t trig);

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_RZ_EXT_IRQ_H_ */
