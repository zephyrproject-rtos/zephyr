/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_X86_INTEL64_ALIGNMENT_H_
#define ZEPHYR_INCLUDE_ARCH_X86_INTEL64_ALIGNMENT_H_

#define ARCH_STACK_PTR_ALIGN 16UL

/* Thread object must be 16-byte aligned */

#define ARCH_DYNAMIC_OBJ_K_THREAD_ALIGNMENT     16

#endif  /* ZEPHYR_INCLUDE_ARCH_X86_INTEL64_ALIGNMENT_H_ */
