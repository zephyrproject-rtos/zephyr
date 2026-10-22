/*
 * Copyright (c) 2017 Oticon A/S
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BOARDS_POSIX_NATIVE_SIM_IRQ_HANDLER_H
#define BOARDS_POSIX_NATIVE_SIM_IRQ_HANDLER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void posix_sw_set_pending_IRQ(unsigned int IRQn);
void posix_sw_clear_pending_IRQ(unsigned int IRQn);

#ifdef __cplusplus
}
#endif

#endif /* BOARDS_POSIX_NATIVE_SIM_IRQ_HANDLER_H */
