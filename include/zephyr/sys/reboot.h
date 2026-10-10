/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Common target reboot functionality
 *
 * @details See subsys/os/Kconfig and the reboot help for details.
 */

#ifndef ZEPHYR_INCLUDE_SYS_REBOOT_H_
#define ZEPHYR_INCLUDE_SYS_REBOOT_H_

#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SYS_REBOOT_WARM 0
#define SYS_REBOOT_COLD 1

/**
 * @brief Reboot the system
 *
 * Reboot the system in the manner specified by @a type.  Not all architectures
 * or platforms support the various reboot types (SYS_REBOOT_COLD,
 * SYS_REBOOT_WARM).
 *
 * When called from a thread, this routine may block briefly in
 * sys_arch_reboot_prepare() before it locks interrupts, so it must not be
 * called with interrupts locked or a spinlock held. It may also be called
 * from ISR, exception or pre-kernel context, where it does not block.
 *
 * When successful, this routine does not return.
 */
FUNC_NORETURN void sys_reboot(int type);

/** @cond INTERNAL_HIDDEN */

/**
 * @brief Reset the system
 *
 * Provided by the architecture, SoC, board or firmware glue. Called by
 * sys_reboot() as its last step, with interrupts locked and, depending on the
 * configuration, caches and the system timer disabled. It must not block.
 *
 * @param type Reboot type passed to sys_reboot()
 */
void sys_arch_reboot(int type);

/**
 * @brief Prepare the system for a reboot
 *
 * Called by sys_reboot() before it locks interrupts, in the same context as
 * sys_reboot() itself: thread, ISR, exception or pre-kernel. Blocking is only
 * allowed in thread context, so an implementation must check k_is_in_isr()
 * and k_is_pre_kernel() before doing anything that may block.
 *
 * This is the only point in the reboot sequence where the scheduler can be
 * used. It is meant for work that has to be done before the reset but cannot
 * be done with interrupts locked, such as acquiring a lock that
 * sys_arch_reboot() itself depends on.
 *
 * The default implementation does nothing. Whoever provides sys_arch_reboot()
 * owns this weak function too and overrides it when needed.
 *
 * @param type Reboot type passed to sys_reboot()
 */
void sys_arch_reboot_prepare(int type);

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_REBOOT_H_ */
