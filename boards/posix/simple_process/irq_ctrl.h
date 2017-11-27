/*
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SIMPLE_PROCESS_IRQ_CTRL_H
#define _SIMPLE_PROCESS_IRQ_CTRL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void hw_irq_ctrl_init(void);
void hw_irq_ctrl_cleanup(void);
void hw_irq_ctr_timer_triggered(void);

void set_currently_running_prio(int new);
int get_currently_running_prio(void);

void hw_irq_ctrl_prio_set(unsigned int irq, unsigned int prio);
uint8_t hw_irq_ctrl_get_prio(unsigned int irq);

int hw_irq_cont_get_highest_prio_irq(void);
uint32_t hw_irq_controller_get_current_lock(void);
uint32_t hw_irq_controller_change_lock(uint32_t new_lock);
uint64_t hw_irq_controller_get_irq_status(void);
void hw_irq_controller_disable_irq(unsigned int irq);
int hw_irq_controller_is_irq_enabled(unsigned int irq);
void hw_irq_controller_clear_irq(unsigned int irq);
void hw_irq_controller_enable_irq(unsigned int irq);
void hw_irq_controller_set_irq(unsigned int irq);
void hw_irq_irq_controller_raise_im(unsigned int irq);
void hw_irq_irq_controller_raise_im_from_sw(unsigned int irq);


#define N_IRQs 32

#ifdef __cplusplus
}
#endif

#endif /* _SIMPLE_PROCESS_IRQ_CTRL_H */
