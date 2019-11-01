/*
 * Copyright (c) 2019 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_X86_INCLUDE_KERNEL_ARCH_THREAD_H_
#define ZEPHYR_ARCH_X86_INCLUDE_KERNEL_ARCH_THREAD_H_

#ifdef CONFIG_X86_64
#include <intel64/kernel_arch_thread.h>
#else
#include <ia32/kernel_arch_thread.h>
#endif

#endif /* ZEPHYR_ARCH_X86_INCLUDE_KERNEL_ARCH_THREAD_H_ */
