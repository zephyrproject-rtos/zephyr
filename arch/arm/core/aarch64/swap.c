/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <kernel_internal.h>

extern const int _k_neg_eagain;

int arch_swap(unsigned int key)
{
	_current->arch.swap_return_value = _k_neg_eagain;

	z_arm64_call_svc();
	irq_unlock(key);

	/* Context switch is performed here. Returning implies the
	 * thread has been context-switched-in again.
	 */
	return _current->arch.swap_return_value;
}
