/*
 * Copyright (c) 2017 Oticon A/S
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief API to the native simulator - native interrupt controller
 */

#ifndef NATIVE_SIMULATOR_NATIVE_SRC_IRQ_CTRL_H
#define NATIVE_SIMULATOR_NATIVE_SRC_IRQ_CTRL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void hw_irq_ctrl_set_cur_prio(int new);
int hw_irq_ctrl_get_cur_prio(void);

void hw_irq_ctrl_prio_set(unsigned int irq, unsigned int prio);
uint8_t hw_irq_ctrl_get_prio(unsigned int irq);

int hw_irq_ctrl_get_highest_prio_irq(void);
uint32_t hw_irq_ctrl_get_current_lock(void);
uint32_t hw_irq_ctrl_change_lock(uint32_t new_lock);
uint64_t hw_irq_ctrl_get_irq_status(void);
void hw_irq_ctrl_disable_irq(unsigned int irq);
int hw_irq_ctrl_is_irq_enabled(unsigned int irq);
void hw_irq_ctrl_clear_irq(unsigned int irq);
void hw_irq_ctrl_enable_irq(unsigned int irq);
void hw_irq_ctrl_set_irq(unsigned int irq);
void hw_irq_ctrl_raise_im(unsigned int irq);
void hw_irq_ctrl_raise_im_from_sw(unsigned int irq);

#define N_IRQS 32

#ifdef __cplusplus
}
#endif

#endif /* NATIVE_SIMULATOR_NATIVE_SRC_IRQ_CTRL_H */
