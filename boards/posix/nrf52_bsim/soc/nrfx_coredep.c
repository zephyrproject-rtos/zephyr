/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <kernel_internal.h>

/*
 * Replacement for the nrfx nrfx_coredep_delay_us()
 * which busy waits for the given number of microseconds.
 *
 * This function will replace at *link* time the
 * nrfx one which had been marked as weak.
 */
void nrfx_coredep_delay_us(uint32_t time_us)
{
	if (time_us == 0) {
		return;
	}
	arch_busy_wait(time_us);
}
