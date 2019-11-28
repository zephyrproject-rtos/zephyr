/* syscall.h - automatically selects the correct syscall.h file to include */

/*
 * Copyright (c) 1997-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_SYSCALL_H_
#define ZEPHYR_INCLUDE_ARCH_SYSCALL_H_

#if defined(CONFIG_X86) && !defined(CONFIG_X86_64)
#include <arch/x86/ia32/syscall.h>
#elif defined(CONFIG_ARM)
#include <arch/arm/syscall.h>
#elif defined(CONFIG_ARC)
#include <arch/arc/syscall.h>
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_SYSCALL_H_ */
