/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Software interrupts utility code - ARM64 implementation
 */

#include <zephyr/kernel.h>
#include <zephyr/irq_offload.h>
#include <exc.h>

void arch_irq_offload(irq_offload_routine_t routine, const void *parameter)
{
	register const void *x0 __asm__("x0") = routine;
	register const void *x1 __asm__("x1") = parameter;

	__asm__ volatile ("svc %[svid]"
			  :
			  : [svid] "i" (_SVC_CALL_IRQ_OFFLOAD),
			    "r" (x0), "r" (x1));
}
