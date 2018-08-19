/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _KERNEL_ARCH_THREAD_H
#define _KERNEL_ARCH_THREAD_H

/* Vestigial boilerplate.  This must exist to it can be included in
 * kernel.h to define these structs to provide types for fields in the
 * Zephyr thread struct.  But we don't need that for this arch.
 */

struct _caller_saved { };
struct _callee_saved { };
struct _thread_arch { };

#endif /* _KERNEL_ARCH_THREAD_H */
