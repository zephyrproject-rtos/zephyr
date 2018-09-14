/*
 * Copyright (c) 2013-2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Private kernel definitions (ARM)
 *
 * This file contains private kernel structures definitions and various
 * other definitions for the ARM Cortex-M3 processor architecture.
 *
 * This file is also included by assembly language files which must #define
 * _ASMLANGUAGE before including this header file.  Note that kernel
 * assembly source files obtains structure offset values via "absolute symbols"
 * in the offsets.o module.
 */

#ifndef ZEPHYR_ARCH_ARM_INCLUDE_KERNEL_ARCH_DATA_H_
#define ZEPHYR_ARCH_ARM_INCLUDE_KERNEL_ARCH_DATA_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <toolchain.h>
#include <linker/sections.h>
#include <arch/cpu.h>
#include <kernel_arch_thread.h>

#ifndef _ASMLANGUAGE
#include <kernel.h>
#include <kernel_internal.h>
#include <zephyr/types.h>
#include <misc/dlist.h>
#include <atomic.h>
#endif

#ifndef _ASMLANGUAGE

typedef struct __esf _esf_t;

#endif /* _ASMLANGUAGE */

/* stacks */

#define STACK_ROUND_UP(x) ROUND_UP(x, STACK_ALIGN_SIZE)
#define STACK_ROUND_DOWN(x) ROUND_DOWN(x, STACK_ALIGN_SIZE)

#ifdef CONFIG_CPU_CORTEX_M
#include <cortex_m/stack.h>
#include <cortex_m/exc.h>
#endif

#ifndef _ASMLANGUAGE

struct _kernel_arch {
	/* empty */
};

typedef struct _kernel_arch _kernel_arch_t;

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_ARCH_ARM_INCLUDE_KERNEL_ARCH_DATA_H_ */
