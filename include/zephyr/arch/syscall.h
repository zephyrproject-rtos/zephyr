/* syscall.h - automatically selects the correct syscall.h file to include */

/*
 * Copyright (c) 1997-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_SYSCALL_H_
#define ZEPHYR_INCLUDE_ARCH_SYSCALL_H_

#if defined(CONFIG_X86)
#if defined(CONFIG_X86_64)
#include <zephyr/arch/x86/intel64/syscall.h>
#else
#include <zephyr/arch/x86/ia32/syscall.h>
#endif
#elif defined(CONFIG_ARM64)
#include <zephyr/arch/arm64/syscall.h>
#elif defined(CONFIG_ARM)
#include <zephyr/arch/arm/syscall.h>
#elif defined(CONFIG_ARC)
#include <zephyr/arch/arc/syscall.h>
#elif defined(CONFIG_RISCV)
#include <zephyr/arch/riscv/syscall.h>
#elif defined(CONFIG_XTENSA)
#include <zephyr/arch/xtensa/syscall.h>
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_SYSCALL_H_ */
