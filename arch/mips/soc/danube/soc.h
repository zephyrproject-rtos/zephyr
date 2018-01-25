/*
 * Copyright (c) 2017 Imagination Technologies Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the danube core
 */

#ifndef __DANUBE_SOC_H_
#define __DANUBE_SOC_H_

#ifndef _ASMLANGUAGE
#include <irq.h>

#define mips32_get_gp() \
(__extension__ ({ \
	register unsigned long __r; \
	__asm__ __volatile("move %0, $28" : "=d" (__r)); \
	__r; \
}))

#define MIPS_MACHINE_SOFT_IRQ       0  /* Machine Software Interrupt */
#define MIPS_MACHINE_TIMER_IRQ      7  /* Machine Timer Interrupt */

#define SOC_CAUSE_EXP_MASK			0x7C  /* Exception code Mask */

#if defined(CONFIG_MIPS_SOC_INTERRUPT_INIT)
void soc_interrupt_init(void);
#endif

#endif /* !_ASMLANGUAGE */

#endif /* __DANUBE_SOC_H_ */
