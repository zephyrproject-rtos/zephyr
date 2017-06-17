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

#ifndef _kernel_arch_data_h_
#define _kernel_arch_data_h_

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
#include <nano_internal.h>

struct _kernel_arch {
	/* nothing for now */
};

typedef struct _kernel_arch _kernel_arch_t;

extern char _interrupt_stack[CONFIG_ISR_STACK_SIZE];

#endif /* _ASMLANGUAGE */

#endif /* _kernel_arch_data_h_ */
