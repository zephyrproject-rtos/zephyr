/*
 * Copyright (c) 2025, Microchip Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/irq.h>
#include <zephyr/pm/pm.h>
#include <zephyr/offsets.h>
#include <kernel_arch_swap.h>
#include "kswap.h"

int arch_swap(unsigned int key)
{
#ifdef CONFIG_INSTRUMENT_THREAD_SWITCHING
	z_thread_mark_switched_out();
#endif

	/* store off key and return value */
	_current->arch.cpu_level = key;
	_current->arch.swap_return_value = -EAGAIN;

	/*Check if swap is needed*/
	if (_kernel.ready_q.cache != _current) {
		z_dspic_do_swap();
	}

#ifdef CONFIG_INSTRUMENT_THREAD_SWITCHING
	z_thread_mark_switched_in();
#endif

	/* This arch has only one SP and dosent use any kernel call style ABI
	 * Which means a return will pollute the next stacks working reg (w0-w4)
	 * Adding a hack here to return w0 without modification
	 * TODO: analyse the use of return value from arch_swap
	 *       Make sure the return value is not being intrepreted
	 *       wrongly
	 */
	irq_unlock(key);
	register int result __asm__("w0");
	return result;

	/* Context switch is performed here. Returning implies the
	 * thread has been context-switched-in again.
	 */
}
