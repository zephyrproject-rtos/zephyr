/*
 * Copyright (c) 2025 Dima Nikiforov <vnikiforov@berkeley.edu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief FESVR HTIF Reboot Support
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/htif.h>


#if defined(CONFIG_UART_HTIF)
// Ensure HTIF is ready before writing
static inline void htif_wait_for_ready(void) {
    while (tohost != 0) {
        if (fromhost != 0) {
            fromhost = 0;  // Acknowledge any pending responses
        }
    }
}
#endif

void sys_arch_reboot(int type)
{
	int success = 0;
	ARG_UNUSED(type);

	#if defined(CONFIG_UART_HTIF)
    k_mutex_lock(&htif_lock, K_FOREVER);
    htif_wait_for_ready();
    tohost = (uint64_t)((success << 1) | 1); // HTIF Exit Command
    k_mutex_unlock(&htif_lock);
    while (1);  // Halt execution
	#endif
}
