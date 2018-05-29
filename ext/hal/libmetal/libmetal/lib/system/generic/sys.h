/*
 * Copyright (c) 2015, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	generic/sys.h
 * @brief	Generic system primitives for libmetal.
 */

#ifndef __METAL_SYS__H__
#error "Include metal/sys.h instead of metal/generic/sys.h"
#endif

#ifndef __METAL_GENERIC_SYS__H__
#define __METAL_GENERIC_SYS__H__

#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "./@PROJECT_MACHINE@/sys.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef METAL_MAX_DEVICE_REGIONS
#define METAL_MAX_DEVICE_REGIONS 1
#endif

/** Structure of generic libmetal runtime state. */
struct metal_state {

	/** Common (system independent) data. */
	struct metal_common_state common;
};

#ifdef METAL_INTERNAL

/**
 * @brief restore interrupts to state before disable_global_interrupt()
 */
void sys_irq_restore_enable(void);

/**
 * @brief disable all interrupts
 */
void sys_irq_save_disable(void);

#endif /* METAL_INTERNAL */

#ifdef __cplusplus
}
#endif

#endif /* __METAL_GENERIC_SYS__H__ */
