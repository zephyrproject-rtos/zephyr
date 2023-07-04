/*
 * Copyright (c) 2018 Linaro, Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <kernel_internal.h>

extern const int _k_neg_eagain;

/* The 'key' actually represents the BASEPRI register
 * prior to disabling interrupts via the BASEPRI mechanism.
 *
 * arch_swap() itself does not do much.
 */
int arch_swap(unsigned int key)
{
	/* store off key and return value */
	_current->arch.basepri = key;
	_current->arch.swap_return_value = _k_neg_eagain;

	z_arm_cortex_r_svc();
	irq_unlock(key);

	/* Context switch is performed here. Returning implies the
	 * thread has been context-switched-in again.
	 */
	return _current->arch.swap_return_value;
}
