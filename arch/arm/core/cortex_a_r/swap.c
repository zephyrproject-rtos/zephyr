/*
 * Copyright (c) 2018 Linaro, Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <kernel_internal.h>

#include <errno.h>

/* The 'key' actually represents the BASEPRI register
 * prior to disabling interrupts via the BASEPRI mechanism.
 *
 * arch_swap() itself does not do much.
 */
int arch_swap(unsigned int key)
{
	/* store off key and return value */
	arch_current_thread()->arch.basepri = key;
	arch_current_thread()->arch.swap_return_value = -EAGAIN;

	z_arm_cortex_r_svc();
	irq_unlock(key);

	/* Context switch is performed here. Returning implies the
	 * thread has been context-switched-in again.
	 */
	return arch_current_thread()->arch.swap_return_value;
}
