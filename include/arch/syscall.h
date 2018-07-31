/* syscall.h - automatically selects the correct syscall.h file to include */

/*
 * Copyright (c) 1997-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ARCHSYSCALL_H__
#define __ARCHSYSCALL_H__

#if defined(CONFIG_X86)
#include <arch/x86/syscall.h>
#elif defined(CONFIG_ARM)
#include <arch/arm/syscall.h>
#elif defined(CONFIG_ARC)
#include <arch/arc/syscall.h>
#endif

#endif /* __ARCHSYSCALL_H__ */
