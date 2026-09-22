/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief TriCore specific syscall header
 *
 * Trap numbers used with the syscall instruction by the kernel for
 * context switch, exception and IRQ offload dispatch.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_TRICORE_SYSCALL_H_
#define ZEPHYR_INCLUDE_ARCH_TRICORE_SYSCALL_H_

/** @cond INTERNAL_HIDDEN */

/* Syscall ids used */
#define TRICORE_SYSCALL_CALL        0
#define TRICORE_SYSCALL_SWITCH      1
#define TRICORE_SYSCALL_EXCEPT      2
#define TRICORE_SYSCALL_IRQ_OFFLOAD 3

/** @endcond */

#endif /* ZEPHYR_INCLUDE_ARCH_TRICORE_SYSCALL_H_ */
