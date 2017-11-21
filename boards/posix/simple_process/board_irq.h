/*
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _BOARD_IRQ_H
#define _BOARD_IRQ_H


#ifdef __cplusplus
extern "C" {
#endif

unsigned int board_irq_lock(void);
void board_irq_unlock(unsigned int key);
void board_irq_full_unlock(void);

/* _ARCH_IRQ_CONNECT , _ARCH_IRQ_DIRECT_CONNECT
 * _ARCH_ISR_DIRECT_DECLARE and so forth would need to be defined here
 * for boards which do support interrupts
 */

#ifdef __cplusplus
}
#endif

#endif /* _BOARD_IRQ_H */
