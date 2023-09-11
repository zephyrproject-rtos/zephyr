/*
 * Copyright (c) 2022 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Driver for Platform Level Interrupt Controller (PLIC)
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_RISCV_PLIC_H_
#define ZEPHYR_INCLUDE_DRIVERS_RISCV_PLIC_H_

#include <zephyr/device.h>

/**
 * @brief Install the ISR into the table
 *
 * @param irq Interrupt ID
 * @param routine Pointer to the ISR handler
 * @param param Pointer to the argument to be passed into the ISR handler
 */
void z_riscv_plic_isr_install(unsigned int irq, unsigned int priority,
			      void (*routine)(const void *), const void *param);

/**
 * @brief Install the ISR of an interrupt controller into the table
 *
 * See @ref z_riscv_plic_isr_install for more info
 *
 * @param dev Interrupt controller of the irq, the first instance will be used if this is NULL
 */
void z_riscv_plic_isr_intc_install(unsigned int irq,
				   unsigned int priority, void (*routine)(const void *),
				   const void *param, const struct device *dev);

/**
 * @brief Enable interrupt
 *
 * @param irq interrupt ID
 */
void riscv_plic_irq_enable(uint32_t irq, const struct device *dev);

/**
 * @brief Enable interrupt of an interrupt controller
 *
 * See @ref riscv_plic_irq_enable for more info
 *
 * @param dev Interrupt controller of the irq, the first instance will be used if this is NULL
 */
void riscv_plic_irq_intc_enable(const struct device *dev, uint32_t irq);

/**
 * @brief Disable interrupt
 *
 * @param irq interrupt ID
 */
void riscv_plic_irq_disable(uint32_t irq, const struct device *dev);

/**
 * @brief Disable interrupt of an interrupt controller
 *
 * See @ref riscv_plic_irq_disable for more info
 *
 * @param dev Interrupt controller of the irq, the first instance will be used if this is NULL
 */
void riscv_plic_irq_intc_disable(const struct device *dev, uint32_t irq);

/**
 * @brief Check if an interrupt is enabled
 *
 * @param irq interrupt ID
 * @return Returns true if interrupt is enabled, false otherwise
 */
int riscv_plic_irq_is_enabled(uint32_t irq, const struct device *dev);

/**
 * @brief Check if an interrupt of an interrupt controller is enabled
 *
 * See @ref riscv_plic_irq_is_enabled for more info
 *
 * @param dev Interrupt controller of the irq, the first instance will be used if this is NULL
 */
int riscv_plic_irq_intc_is_enabled(uint32_t irq, const struct device *dev);

/**
 * @brief Set interrupt priority
 *
 * @param irq interrupt ID
 * @param prio interrupt priority
 */
void riscv_plic_set_priority(uint32_t irq, uint32_t prio);

/**
 * @brief Set interrupt priority of an interrupt controller
 *
 * See @ref riscv_plic_set_priority for more info
 *
 * @param dev Interrupt controller of the irq, the first instance will be used if this is NULL
 */
void riscv_plic_intc_set_priority(uint32_t irq, uint32_t priority, const struct device *dev);

/**
 * @brief Get active interrupt ID
 *
 * @return Returns the ID of an active interrupt
 */
int riscv_plic_get_irq(void);

/**
 * @brief Get the interrupt controller of the active interrupt ID
 *
 * @return Returns the device struct of the interrupt controller
 */
const struct device *riscv_plic_get_dev(void);

#endif /* ZEPHYR_INCLUDE_DRIVERS_RISCV_PLIC_H_ */
