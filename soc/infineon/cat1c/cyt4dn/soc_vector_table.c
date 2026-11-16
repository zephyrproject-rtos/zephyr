/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
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

#if defined(CONFIG_CPU_CORTEX_M0PLUS)
extern void ifx_srom_syscall_isr(void);

/*
 * NvicMux0 and NvicMux1 carry the SROM syscall handlers from the boot ROM
 * vector table (0x40 and 0x44), NvicMux2 the trampoline into NvicMux0.
 * soc_early_init_hook() checks the ROM entries against this table.
 */
#define IFX_SROM_IRQ0_HANDLER 0x00000049U
#define IFX_SROM_IRQ1_HANDLER 0x00000281U
#endif

const uintptr_t __irq_vector_table _irq_vector_table[CPU_IRQS] = {
#if defined(CONFIG_CPU_CORTEX_M0PLUS)
	((uintptr_t)IFX_SROM_IRQ0_HANDLER),
	((uintptr_t)IFX_SROM_IRQ1_HANDLER),
	((uintptr_t)ifx_srom_syscall_isr),
#else
	((uintptr_t)_isr_wrapper),
	((uintptr_t)_isr_wrapper),
	((uintptr_t)_isr_wrapper),
#endif
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
