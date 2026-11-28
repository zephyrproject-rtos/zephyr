/*
 * Copyright (c) 2026 Linumiz
 * Copyright (c) 2026 Infineon Technologies AG
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/kernel.h>

#define CPU_IRQS 16	/* M0P and M4 has 8 NVIC Lines and 8 Internal IRQs */

const uintptr_t __irq_vector_table _irq_vector_table[CPU_IRQS] = {
	((uintptr_t)_isr_wrapper),
	((uintptr_t)_isr_wrapper),
	((uintptr_t)_isr_wrapper),
	((uintptr_t)_isr_wrapper),
	((uintptr_t)_isr_wrapper),
	((uintptr_t)_isr_wrapper),
	((uintptr_t)_isr_wrapper),
	((uintptr_t)_isr_wrapper),
	((uintptr_t)_isr_wrapper),
	((uintptr_t)_isr_wrapper),
	((uintptr_t)_isr_wrapper),
	((uintptr_t)_isr_wrapper),
	((uintptr_t)_isr_wrapper),
	((uintptr_t)_isr_wrapper),
	((uintptr_t)_isr_wrapper),
	((uintptr_t)_isr_wrapper),
};
