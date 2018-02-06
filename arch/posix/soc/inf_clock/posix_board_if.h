/*
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _POSIX_CORE_BOARD_PROVIDED_IF_H
#define _POSIX_CORE_BOARD_PROVIDED_IF_H

#include "zephyr/types.h"

/*
 * This file lists the functions the posix "inf_clock" soc
 * expect the the board to provide
 *
 * All functions listed here must be provided by the implementation of the board
 *
 * See soc_irq.h for more
 */

#ifdef __cplusplus
extern "C" {
#endif

void posix_irq_handler(void);
void posix_exit(int exit_code);
u64_t posix_get_hw_cycle(void);

#if defined(CONFIG_ARCH_HAS_CUSTOM_BUSY_WAIT)
void k_busy_wait(u32_t usec_to_wait);
#endif

#ifdef __cplusplus
}
#endif

#endif /* _POSIX_CORE_BOARD_PROVIDED_IF_H */
