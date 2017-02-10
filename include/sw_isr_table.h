/*
 * Copyright (c) 2014, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Software-managed ISR table
 *
 * Data types for a software-managed ISR table, with a parameter per-ISR.
 */

#ifndef _SW_ISR_TABLE__H_
#define _SW_ISR_TABLE__H_

#include <arch/cpu.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(_ASMLANGUAGE)
/*
 * Note the order: arg first, then ISR. This allows a table entry to be
 * loaded arg -> r0, isr -> r3 in _isr_wrapper with one ldmia instruction,
 * on ARM Cortex-M (Thumb2).
 */
struct _isr_table_entry {
	void *arg;
	void (*isr)(void *);
};

/* The software ISR table itself, an array of these structures indexed by the
 * irq line
 */
extern struct _isr_table_entry _sw_isr_table[];

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* _SW_ISR_TABLE__H_ */
