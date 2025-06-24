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
 * definitions, with include/arch/x86/ia32/arch.h supplying the
 * IA-32 specific portions of the public kernel interface.
 *
 * This file is also included by assembly language files which must #define
 * _ASMLANGUAGE before including this header file.  Note that kernel
 * assembly source files obtains structure offset values via "absolute symbols"
 * in the offsets.o module.
 */

/* this file is only meant to be included by kernel_structs.h */

#ifndef ZEPHYR_ARCH_X86_INCLUDE_IA32_KERNEL_ARCH_DATA_H_
#define ZEPHYR_ARCH_X86_INCLUDE_IA32_KERNEL_ARCH_DATA_H_

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <ia32/exception.h>
#include <zephyr/sys/util.h>

#ifndef _ASMLANGUAGE
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/sys/dlist.h>
#endif

/* Some configurations require that the stack/registers be adjusted before
 * z_thread_entry. See discussion in swap.S for z_x86_thread_entry_wrapper()
 */
#if defined(CONFIG_X86_DEBUG_INFO)
#define _THREAD_WRAPPER_REQUIRED
#endif

#if defined(CONFIG_LAZY_FPU_SHARING) && defined(CONFIG_X86_SSE)
#define _FP_USER_MASK (K_FP_REGS | K_SSE_REGS)
#elif defined(CONFIG_LAZY_FPU_SHARING)
#define _FP_USER_MASK (K_FP_REGS)
#endif

#ifndef _ASMLANGUAGE

#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _THREAD_WRAPPER_REQUIRED
extern void z_x86_thread_entry_wrapper(k_thread_entry_t entry,
				      void *p1, void *p2, void *p3);
#endif /* _THREAD_WRAPPER_REQUIRED */

#ifdef CONFIG_THREAD_LOCAL_STORAGE
extern void z_x86_tls_update_gdt(struct k_thread *thread);
#endif

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_X86_INCLUDE_IA32_KERNEL_ARCH_DATA_H_ */
