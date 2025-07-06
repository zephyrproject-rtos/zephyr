/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_XTENSA_THREAD_H_
#define ZEPHYR_INCLUDE_ARCH_XTENSA_THREAD_H_

#include <stdint.h>
#ifndef _ASMLANGUAGE

#ifdef CONFIG_XTENSA_MPU
#include <zephyr/arch/xtensa/mpu.h>
#endif

/* Xtensa doesn't use these structs, but Zephyr core requires they be
 * defined so they can be included in struct _thread_base.  Dummy
 * field exists for sizeof compatibility with C++.
 */

struct _callee_saved {
	char dummy;
};

typedef struct _callee_saved _callee_saved_t;

struct _thread_arch {
	uint32_t last_cpu;
#ifdef CONFIG_USERSPACE

#ifdef CONFIG_XTENSA_MMU
	uint32_t *ptables;
#endif

#ifdef CONFIG_XTENSA_MPU
	/* Pointer to the memory domain's MPU map. */
	struct xtensa_mpu_map *mpu_map;
#endif

	/* Initial privilege mode stack pointer when doing a system call.
	 * Un-set for surpervisor threads.
	 */
	uint8_t *psp;

	/* Stashed PS value used to restore PS when restoring from
	 * context switching or returning from non-nested interrupts.
	 */
	uint32_t return_ps;
#endif
};

typedef struct _thread_arch _thread_arch_t;

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_XTENSA_THREAD_H_ */
