/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _X86_64_ARCH_H
#define _X86_64_ARCH_H

#include <kernel_arch_func.h>
#include <arch/bits_portable.h>

#define STACK_ALIGN 8

typedef struct NANO_ESF NANO_ESF;
extern const NANO_ESF _default_esf;
void _SysFatalErrorHandler(unsigned int reason, const NANO_ESF *esf);
void _NanoFatalErrorHandler(unsigned int reason, const NANO_ESF *esf);

/* Existing code requires only these particular symbols be defined,
 * but doesn't put them in a global header.  Needs cleaner
 * cross-architecture standardization.  Implement only the minimal set
 * here.
 */
#define _NANO_ERR_STACK_CHK_FAIL 1
#define _NANO_ERR_KERNEL_OOPS    2
#define _NANO_ERR_KERNEL_PANIC   3

#endif /* _X86_64_ARCH_H */
