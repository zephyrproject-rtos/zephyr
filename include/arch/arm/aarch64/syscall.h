/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM specific syscall header
 *
 * This header contains the ARM specific syscall interface.  It is
 * included by the syscall interface architecture-abstraction header
 * (include/arch/aarch64/syscall.h)
 */

#ifndef ZEPHYR_INCLUDE_ARCH_AARCH64_ARM_SYSCALL_H_
#define ZEPHYR_INCLUDE_ARCH_AARCH64_ARM_SYSCALL_H_

#define _SVC_CALL_CONTEXT_SWITCH	0
#define _SVC_CALL_IRQ_OFFLOAD		1
#define _SVC_CALL_RUNTIME_EXCEPT	2

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_AARCH64_SYSCALL_H_ */
