/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Private kernel definitions (XTENSA)
 *
 * This file contains private kernel structures definitions and various
 * other definitions for the XTENSA processors family architecture.
 *
 * This file is also included by assembly language files which must #define
 * _ASMLANGUAGE before including this header file.  Note that kernel
 * assembly source files obtains structure offset values via "absolute symbols"
 * in the offsets.o module.
 */

#ifndef ZEPHYR_ARCH_XTENSA_INCLUDE_KERNEL_ARCH_DATA_H_
#define ZEPHYR_ARCH_XTENSA_INCLUDE_KERNEL_ARCH_DATA_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <toolchain.h>
#include <linker/sections.h>
#include <arch/cpu.h>
#include <kernel_arch_thread.h>

#if !defined(_ASMLANGUAGE) && !defined(__ASSEMBLER__)
#include <kernel.h>            /* public kernel API */
#include <kernel_internal.h>
#include <zephyr/types.h>
#include <misc/dlist.h>
#include <misc/util.h>

/* Bitmask definitions for the struct k_thread->flags bit field */

/* executing context is interrupt handler */
#define INT_ACTIVE (1 << 1)
/* executing context is exception handler */
#define EXC_ACTIVE (1 << 2)
/* thread uses floating point unit */
#define USE_FP 0x010

typedef struct __esf __esf_t;

struct _kernel_arch {
};

typedef struct _kernel_arch _kernel_arch_t;

#endif /*! _ASMLANGUAGE && ! __ASSEMBLER__ */

#ifdef CONFIG_USE_SWITCH
void xtensa_switch(void *switch_to, void **switched_from);
#define _arch_switch xtensa_switch
#endif

/* stacks */
#define STACK_ROUND_UP(x) ROUND_UP(x, STACK_ALIGN_SIZE)
#define STACK_ROUND_DOWN(x) ROUND_DOWN(x, STACK_ALIGN_SIZE)

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_ARCH_XTENSA_INCLUDE_KERNEL_ARCH_DATA_H_ */

