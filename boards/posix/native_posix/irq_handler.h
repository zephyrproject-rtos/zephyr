/*
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NATIVE_POSIX_IRQ_HANDLER_H
#define _NATIVE_POSIX_IRQ_HANDLER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


void posix_irq_handler_im_from_sw(void);
void posix_sw_set_pending_IRQ(unsigned int IRQn);
void posix_sw_clear_pending_IRQ(unsigned int IRQn);


#ifdef __cplusplus
}
#endif

#endif /* _NATIVE_POSIX_IRQ_HANDLER_H */
