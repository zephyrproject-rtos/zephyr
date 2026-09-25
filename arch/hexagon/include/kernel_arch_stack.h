/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_HEXAGON_INCLUDE_KERNEL_ARCH_STACK_H_
#define ZEPHYR_ARCH_HEXAGON_INCLUDE_KERNEL_ARCH_STACK_H_

/* Stack object alignment (function-like macro per Zephyr convention) */
#define ARCH_THREAD_STACK_OBJ_ALIGN(size)  ARCH_STACK_PTR_ALIGN
#define ARCH_KERNEL_STACK_OBJ_ALIGN        ARCH_STACK_PTR_ALIGN

/* Stack size alignment */
#define ARCH_THREAD_STACK_SIZE_ALIGN(size)  ROUND_UP(size, ARCH_STACK_PTR_ALIGN)

/* Reserved space in stack objects */
#define ARCH_THREAD_STACK_RESERVED 0
#define ARCH_KERNEL_STACK_RESERVED 0

#endif /* ZEPHYR_ARCH_HEXAGON_INCLUDE_KERNEL_ARCH_STACK_H_ */
