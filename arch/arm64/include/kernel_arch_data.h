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
 * other definitions for the ARM Cortex-A/R/M processor architecture family.
 *
 * This file is also included by assembly language files which must #define
 * _ASMLANGUAGE before including this header file.  Note that kernel
 * assembly source files obtains structure offset values via "absolute symbols"
 * in the offsets.o module.
 */

#ifndef ZEPHYR_ARCH_ARM64_INCLUDE_KERNEL_ARCH_DATA_H_
#define ZEPHYR_ARCH_ARM64_INCLUDE_KERNEL_ARCH_DATA_H_

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/cpu.h>

#include <exception.h>

#ifndef _ASMLANGUAGE
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/sys/dlist.h>
#include <zephyr/sys/atomic.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct __esf _esf_t;
typedef struct __basic_sf _basic_sf_t;

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_ARM64_INCLUDE_KERNEL_ARCH_DATA_H_ */
