/*
 * Copyright (C) 2025-2026 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_INTC_MCHP_AIC_G1_H_
#define ZEPHYR_DRIVERS_INTC_MCHP_AIC_G1_H_

/*
 * Microchip G1 AIC Driver Interface Functions
 */

/**
 * @brief Get active interrupt ID.
 *
 * @return Returns the ID of an active interrupt.
 */
unsigned int z_aic_irq_get_active(void);

/**
 * @brief Signal end-of-interrupt.
 *
 * @param irq interrupt ID.
 */
void z_aic_irq_eoi(unsigned int irq);

/**
 * @brief Interrupt controller initialization.
 */
void z_aic_irq_init(void);

/**
 * @brief Configure priority, irq type for the interrupt ID.
 *
 * @param irq interrupt ID.
 * @param prio interrupt priority.
 * @param flags interrupt flags.
 */
void z_aic_irq_priority_set(unsigned int irq, unsigned int prio, unsigned int flags);

/**
 * @brief Enable Interrupt.
 *
 * @param irq interrupt ID.
 */
void z_aic_irq_enable(unsigned int irq);

/**
 * @brief Disable Interrupt.
 *
 * @param irq interrupt ID.
 */
void z_aic_irq_disable(unsigned int irq);

/**
 * @brief Check if an interrupt is enabled.
 *
 * @param irq interrupt ID.
 *
 * @retval 0 If interrupt is disabled.
 * @retval 1 If interrupt is enabled.
 */
int z_aic_irq_is_enabled(unsigned int irq);

#endif /* ZEPHYR_DRIVERS_INTC_MCHP_AIC_G1_H_ */
