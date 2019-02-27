/*
 * Copyright (c) 2019 Synopsys.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARCv2 atomic lock (only for ARC EM)
 *
 */
#ifndef ZEPHYR_INCLUDE_ARCH_ARC_V2_ATOMIC_LOCK_H_
#define ZEPHYR_INCLUDE_ARCH_ARC_V2_ATOMIC_LOCK_H_

#ifdef CONFIG_ARC_EX_ATOMIC
#include <stddef.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif
__syscall unsigned int arc_atomic_lock(void);

static inline unsigned int z_impl_arc_atomic_lock(void)
{
	unsigned int key;

	__asm__ volatile("clri %0" : "=r"(key):: "memory");
	return key;
}

__syscall void arc_atomic_unlock(unsigned int key);

static inline void z_impl_arc_atomic_unlock(unsigned int key)
{
	__asm__ volatile("seti %0" : : "ir"(key) : "memory");
}

#ifdef __cplusplus
}
#endif

#include <syscalls/atomic_lock.h>

#endif
#endif /* ZEPHYR_INCLUDE_ARCH_ARC_V2_ATOMIC_LOCK_H_ */
