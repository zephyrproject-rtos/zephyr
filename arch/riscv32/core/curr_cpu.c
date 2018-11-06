/*
 * Copyright (c) 2018 SiFive Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <kernel_structs.h>

inline _cpu_t *_arch_curr_cpu(void)
{
	/* On hart init, the mscratch csr is set to the pointer for the
	 * _kernel.cpus[] struct for that hart */
	u32_t mscratch;

	__asm__ volatile("csrr %0, mscratch" : "=r" (mscratch));

	return (_cpu_t *) mscratch;
}

