/*
 * Copyright (c) 2025, Microchip Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Private kernel definitions
 *
 * This file contains private kernel function/macro definitions and various
 * other definitions for the DSPIC processor architecture.
 */

#ifndef ZEPHYR_ARCH_DSPIC_INCLUDE_KERNEL_ARCH_FUNC_H_
#define ZEPHYR_ARCH_DSPIC_INCLUDE_KERNEL_ARCH_FUNC_H_

#ifndef _ASMLANGUAGE

#include <zephyr/toolchain.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/platform/hooks.h>
#include <xc.h>

#ifdef __cplusplus
extern "C" {
#endif
#if defined(CONFIG_BOARD_DSPIC33A_CURIOSITY_P33AK512MPS512)
#define IFS_COUNT 12
#elif defined(CONFIG_BOARD_DSPIC33A_CURIOSITY_P33AK128MC106)
#define IFS_COUNT 9
#endif
extern uint32_t vector_start;

/* dsPIC33A interrupt functionality initialization */
static ALWAYS_INLINE void z_dspic_interrupt_init(void)
{
	/**
	 * clear all the interrupts, set the interrupt flag status
	 * registers to zero.
	 */
	volatile uint32_t *int_enable_reg = (uint32_t *)DT_PROP(DT_NODELABEL(intc0), if_offset);
	(void)memset((void *)int_enable_reg, 0, (unsigned long)IFS_COUNT * sizeof(int));
	/* enable nested interrupts */
	INTCON1bits.NSTDIS = 0;
	/* enable global interrupts */
	INTCON1bits.GIE = 1;

	/**
	 * After a reset default values IVTBASEWR will be having value 1, IVTBASELK will be 0
	 * even though writing to these bits to make sure IVTBASE register is writable.
	 */
	PACCON1bits.IVTBASELK = 0;
	PACCON1bits.IVTBASEWR = 1;
	/* set the new base address */
	IVTBASE = (uint32_t)((void *)&vector_start);
	PACCON1bits.IVTBASEWR = 0;
	PACCON1bits.IVTBASELK = 1;
}

/* dsPIC33A fault initialization */
static ALWAYS_INLINE void z_dspic_fault_init(void)
{
	/* nothing to be done */
}

/**
 * Architecture specific kernel init function. Will initialize the
 * interrupt and exception handling. This function is called from
 * zephyr z_cstart() routine.
 */
static ALWAYS_INLINE void arch_kernel_init(void)
{
	z_dspic_interrupt_init();
	z_dspic_fault_init();
#ifdef CONFIG_SOC_PER_CORE_INIT_HOOK
	soc_per_core_init_hook();
#endif /* CONFIG_SOC_PER_CORE_INIT_HOOK */
}

static ALWAYS_INLINE void arch_thread_return_value_set(struct k_thread *thread, unsigned int value)
{
	thread->arch.swap_return_value = value;
}

int arch_swap(unsigned int key);

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_DSPIC_INCLUDE_KERNEL_ARCH_FUNC_H_ */
