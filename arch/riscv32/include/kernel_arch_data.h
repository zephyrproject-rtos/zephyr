/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Private kernel definitions
 *
 * This file contains private kernel structures definitions and various
 * other definitions for the RISCV32 processor architecture.
 */

#ifndef ZEPHYR_ARCH_RISCV32_INCLUDE_KERNEL_ARCH_DATA_H_
#define ZEPHYR_ARCH_RISCV32_INCLUDE_KERNEL_ARCH_DATA_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <toolchain.h>
#include <linker/sections.h>
#include <arch/cpu.h>
#include <kernel_arch_thread.h>

#ifndef _ASMLANGUAGE
#include <kernel.h>
#include <zephyr/types.h>
#include <misc/util.h>
#include <misc/dlist.h>
#include <kernel_internal.h>

struct _kernel_arch {
	/* nothing for now */
};

typedef struct _kernel_arch _kernel_arch_t;

extern K_THREAD_STACK_DEFINE(_interrupt_stack, CONFIG_ISR_STACK_SIZE);

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_RISCV32_INCLUDE_KERNEL_ARCH_DATA_H_ */
