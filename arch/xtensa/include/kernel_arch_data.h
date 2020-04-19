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

#include <toolchain.h>
#include <linker/sections.h>
#include <arch/cpu.h>

#if !defined(_ASMLANGUAGE) && !defined(__ASSEMBLER__)
#include <kernel.h>            /* public kernel API */
#include <zephyr/types.h>
#include <sys/dlist.h>
#include <sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Bitmask definitions for the struct k_thread->flags bit field */

/* executing context is interrupt handler */
#define INT_ACTIVE (1 << 1)
/* executing context is exception handler */
#define EXC_ACTIVE (1 << 2)
/* thread uses floating point unit */
#define USE_FP 0x010

typedef struct __esf __esf_t;

#ifdef __cplusplus
}
#endif

#endif /*! _ASMLANGUAGE && ! __ASSEMBLER__ */

#endif /* ZEPHYR_ARCH_XTENSA_INCLUDE_KERNEL_ARCH_DATA_H_ */
