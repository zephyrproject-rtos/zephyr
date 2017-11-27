/*
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SIMPLE_PROCESS_IRQ_HANDLER_H
#define _SIMPLE_PROCESS_IRQ_HANDLER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


void pb_irq_handler_im_from_sw(void);
void pb_sw_set_pending_IRQ(unsigned int IRQn);
void pb_sw_clear_pending_IRQ(unsigned int IRQn);

#ifdef __cplusplus
}
#endif

#endif /* _SIMPLE_PROCESS_IRQ_HANDLER_H */
