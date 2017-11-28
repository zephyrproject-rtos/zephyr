/*
 * Copyright (c) 2010-2014 Wind River Systems, Inc.
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief POSIX arch specific kernel interface header
 * This header contains the POSIX arch specific kernel interface.
 * It is included by the generic kernel interface header (include/arch/cpu.h)
 *
 * Starts as a cut down copy of the NIOS one
 */


#ifndef _ARCH_IFACE_H
#define _ARCH_IFACE_H

#include <irq.h>
#include <arch/posix/asm_inline.h>
#include <board_irq.h> /* Each board must define this */
#include <sw_isr_table.h>

#ifdef __cplusplus
extern "C" {
#endif

#define STACK_ALIGN 4
#define STACK_ALIGN_SIZE 4
#define OCTET_TO_SIZEOFUNIT(X) (X)
#define SIZEOFUNIT_TO_OCTET(X) (X)

#define _NANO_ERR_CPU_EXCEPTION (0)     /* Any unhandled exception */
#define _NANO_ERR_INVALID_TASK_EXIT (1) /* Invalid task exit */
#define _NANO_ERR_STACK_CHK_FAIL (2)    /* Stack corruption detected */
#define _NANO_ERR_ALLOCATION_FAIL (3)   /* Kernel Allocation Failure */
#define _NANO_ERR_SPURIOUS_INT (4)  /* Spurious interrupt */
#define _NANO_ERR_KERNEL_OOPS (5)       /* Kernel oops (fatal to thread) */
#define _NANO_ERR_KERNEL_PANIC (6)  /* Kernel panic (fatal to system) */

struct __esf {
	u32_t dummy; /*maybe we will want to add somethign someday*/
};

typedef struct __esf NANO_ESF;
extern const NANO_ESF _default_esf;

extern u32_t _timer_cycle_get_32(void);
#define _arch_k_cycle_get_32()  _timer_cycle_get_32()

#ifdef __cplusplus
}
#endif

#endif /* _ARCH_IFACE_H */
