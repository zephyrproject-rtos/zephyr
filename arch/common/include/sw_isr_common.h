/*
 * Copyright (c) 2023, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Private header for the software-managed ISR table's functions
 */

#ifndef ZEPHYR_ARCH_COMMON_INCLUDE_SW_ISR_COMMON_H_
#define ZEPHYR_ARCH_COMMON_INCLUDE_SW_ISR_COMMON_H_

#if !defined(_ASMLANGUAGE)
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Helper function used to compute the index in _sw_isr_table
 * based on passed IRQ.
 *
 * @param irq IRQ number in its zephyr format
 *
 * @return corresponding index in _sw_isr_table
 */
unsigned int z_get_sw_isr_table_idx(unsigned int irq);

/**
 * @brief Helper function used to get the parent interrupt controller device based on passed IRQ.
 *
 * @param irq IRQ number in its zephyr format
 *
 * @return corresponding interrupt controller device in _sw_isr_table
 */
const struct device *z_get_sw_isr_device_from_irq(unsigned int irq);

/**
 * @brief Helper function used to get the IRQN of the passed in parent interrupt
 * controller device.
 *
 * @param dev parent interrupt controller device
 *
 * @return IRQN of the interrupt controller
 */
unsigned int z_get_sw_isr_irq_from_device(const struct device *dev);

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_COMMON_INCLUDE_SW_ISR_COMMON_H_ */
