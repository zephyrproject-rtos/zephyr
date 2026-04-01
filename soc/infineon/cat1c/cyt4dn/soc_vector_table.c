/*
 * Copyright (c) 2026 Linumiz
 * Copyright (c) 2026 Infineon Technologies AG
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/kernel.h>

#define CPU_IRQS 16	/* M0P and M7 has 8 NVIC Lines and 8 Internal IRQs */

/*
 * Hand-crafted hardware NVIC vector table for CAT1C.
 *
 * gen_isr_tables.py cannot generate this table because it sizes its
 * output to CONFIG_NUM_IRQS (6368 on M7, 3986 on M0+), which counts
 * all L1+L2 IRQ slots in the multi-level interrupt framework. The
 * Cortex-M hardware NVIC vector table only has 16 entries here
 * (8 NvicMux + 8 Internal/software IRQs); the L2 system interrupts
 * are dispatched in software via _sw_isr_table from inside
 * _isr_wrapper, not via the hardware vector table.
 *
 * CONFIG_GEN_IRQ_VECTOR_TABLE is therefore set to n in
 * Kconfig.defconfig, and this file provides the 16 hardware slots
 * directly.
 */

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
