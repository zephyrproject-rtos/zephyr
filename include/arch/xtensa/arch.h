/*
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Xtensa specific kernel interface header
 * This header contains the Xtensa specific kernel interface.  It is included
 * by the generic kernel interface header (include/arch/cpu.h)
 */

#ifndef _ARCH_IFACE_H
#define _ARCH_IFACE_H

#include <irq.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(_ASMLANGUAGE) && !defined(__ASSEMBLER__)
#include <stdint.h>
#include <sw_isr_table.h>
#include <arch/xtensa/xtensa_irq.h>

#define STACK_ALIGN 16
#define OCTET_TO_SIZEOFUNIT(X) (X)
#define SIZEOFUNIT_TO_OCTET(X) (X)

#define _NANO_ERR_HW_EXCEPTION (0)      /* MPU/Bus/Usage fault */
#define _NANO_ERR_INVALID_TASK_EXIT (1) /* Invalid task exit */
#define _NANO_ERR_STACK_CHK_FAIL (2)    /* Stack corruption detected */
#define _NANO_ERR_ALLOCATION_FAIL (3)   /* Kernel Allocation Failure */

/* Xtensa GPRs are often designated by two different names */
#define sys_define_gpr_with_alias(name1, name2) union { uint32_t name1, name2; }

#include <arch/xtensa/exc.h>

#define find_lsb_set __builtin_ffs
#define find_msb_set __builtin_clz

/* internal routine documented in C file, needed by IRQ_CONNECT() macro */
extern void _irq_priority_set(uint32_t irq, uint32_t prio, uint32_t flags);


/**
 * Configure a static interrupt.
 *
 * All arguments must be computable by the compiler at build time; if this
 * can't be done use irq_connect_dynamic() instead.
 *
 * Internally this function does a few things:
 *
 * 1. The enum statement has no effect but forces the compiler to only
 * accept constant values for the irq_p parameter, very important as the
 * numerical IRQ line is used to create a named section.
 *
 * 2. An instance of _isr_table_entry is created containing the ISR and its
 * parameter. If you look at how _sw_isr_table is created, each entry in the
 * array is in its own section named by the IRQ line number. What we are doing
 * here is to override one of the default entries (which points to the
 * spurious IRQ handler) with what was supplied here.
 *
 * 3. The priority level for the interrupt is configured by a call to
 * _irq_priority_set()
 *
 * @param irq_p IRQ line number
 * @param priority_p Interrupt priority
 * @param isr_p Interrupt service routine
 * @param isr_param_p ISR parameter
 * @param flags_p IRQ options
 *
 * @return The vector assigned to this interrupt
 */
#define _ARCH_IRQ_CONNECT(irq_p, priority_p, isr_p, isr_param_p, flags_p) \
({ \
	enum { IRQ = irq_p }; \
	static struct _isr_table_entry \
		_CONCAT(_isr_irq, irq_p) \
		__attribute__ ((used)) \
		__attribute__ ((section(\
			STRINGIFY(_CONCAT(.gnu.linkonce.d.isr_irq, irq_p)))\
		)) = {isr_param_p, isr_p}; \
	_irq_priority_set(irq_p, priority_p, flags_p); \
	irq_p; \
})
#endif /* !defined(_ASMLANGUAGE) && !defined(__ASSEMBLER__)  */
#ifdef __cplusplus
}
#endif

#endif /* _ARCH_IFACE_H */
