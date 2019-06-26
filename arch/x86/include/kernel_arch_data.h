/*
 * Copyright (c) 2010-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Private kernel definitions (IA-32)
 *
 * This file contains private kernel structures definitions and various
 * other definitions for the Intel Architecture 32 bit (IA-32) processor
 * architecture.
 * The header include/kernel.h contains the public kernel interface
 * definitions, with include/arch/x86/arch.h supplying the
 * IA-32 specific portions of the public kernel interface.
 *
 * This file is also included by assembly language files which must #define
 * _ASMLANGUAGE before including this header file.  Note that kernel
 * assembly source files obtains structure offset values via "absolute symbols"
 * in the offsets.o module.
 */

/* this file is only meant to be included by kernel_structs.h */

#ifndef ZEPHYR_ARCH_X86_INCLUDE_KERNEL_ARCH_DATA_H_
#define ZEPHYR_ARCH_X86_INCLUDE_KERNEL_ARCH_DATA_H_

#include <toolchain.h>
#include <linker/sections.h>
#include <asm_inline.h>
#include <exception.h>
#include <kernel_arch_thread.h>
#include <sys/util.h>

#ifndef _ASMLANGUAGE
#include <kernel.h>
#include <kernel_internal.h>
#include <zephyr/types.h>
#include <sys/dlist.h>
#endif

/* Some configurations require that the stack/registers be adjusted before
 * z_thread_entry. See discussion in swap.S for z_x86_thread_entry_wrapper()
 */
#if defined(CONFIG_X86_IAMCU) || defined(CONFIG_DEBUG_INFO)
#define _THREAD_WRAPPER_REQUIRED
#endif


/* increase to 16 bytes (or more?) to support SSE/SSE2 instructions? */

#define STACK_ALIGN_SIZE 4

/* x86 Bitmask definitions for struct k_thread.thread_state */

/* executing context is interrupt handler */
#define _INT_ACTIVE (1 << 7)

/* executing context is exception handler */
#define _EXC_ACTIVE (1 << 6)

#define _INT_OR_EXC_MASK (_INT_ACTIVE | _EXC_ACTIVE)

/* end - states */

#if defined(CONFIG_LAZY_FP_SHARING) && defined(CONFIG_SSE)
#define _FP_USER_MASK (K_FP_REGS | K_SSE_REGS)
#elif defined(CONFIG_LAZY_FP_SHARING)
#define _FP_USER_MASK (K_FP_REGS)
#endif

/*
 * Exception/interrupt vector definitions: vectors 20 to 31 are reserved for
 * Intel; vectors 32 to 255 are user defined interrupt vectors.
 */

#define IV_DIVIDE_ERROR 0
#define IV_DEBUG 1
#define IV_NON_MASKABLE_INTERRUPT 2
#define IV_BREAKPOINT 3
#define IV_OVERFLOW 4
#define IV_BOUND_RANGE 5
#define IV_INVALID_OPCODE 6
#define IV_DEVICE_NOT_AVAILABLE 7
#define IV_DOUBLE_FAULT 8
#define IV_COPROC_SEGMENT_OVERRUN 9
#define IV_INVALID_TSS 10
#define IV_SEGMENT_NOT_PRESENT 11
#define IV_STACK_FAULT 12
#define IV_GENERAL_PROTECTION 13
#define IV_PAGE_FAULT 14
#define IV_RESERVED 15
#define IV_X87_FPU_FP_ERROR 16
#define IV_ALIGNMENT_CHECK 17
#define IV_MACHINE_CHECK 18
#define IV_SIMD_FP 19
#define IV_INTEL_RESERVED_END 31

/*
 * EFLAGS value to utilize for the initial context: IF=1.
 */

#define EFLAGS_INITIAL 0x00000200U

/* Enable paging and write protection */
#define CR0_PG_WP_ENABLE 0x80010000
/* Set the 5th bit in  CR4 */
#define CR4_PAE_ENABLE 0x00000020

#ifndef _ASMLANGUAGE

#include <sys/util.h>

#ifdef _THREAD_WRAPPER_REQUIRED
extern void z_x86_thread_entry_wrapper(k_thread_entry_t entry,
				      void *p1, void *p2, void *p3);
#endif /* _THREAD_WRAPPER_REQUIRED */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_X86_INCLUDE_KERNEL_ARCH_DATA_H_ */
