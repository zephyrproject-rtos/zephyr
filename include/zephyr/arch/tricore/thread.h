/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief TriCore specific thread definitions
 */

#ifndef ZEPHYR_INCLUDE_ARCH_TRICORE_THREAD_H_
#define ZEPHYR_INCLUDE_ARCH_TRICORE_THREAD_H_

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>

/** @cond INTERNAL_HIDDEN */

struct _callee_saved {
	uint32_t pcxi;
#ifdef CONFIG_CPU_TC18
	/* Previous protection register set, restored with PCXI */
	uint32_t pprs;
#endif
};

typedef struct _callee_saved _callee_saved_t;

struct _thread_arch {
#if defined(CONFIG_USERSPACE)
	/* Top of the privileged stack used for system calls */
	uint32_t priv_stack_end;
#endif
	/* MPU protection register set (PSW.PRS) of the thread */
	uint8_t prs;
};

typedef struct _thread_arch _thread_arch_t;

/* Thread options bits holding the requested protection register set */
#define K_PROTECTION_SET_MASK GENMASK(7, 5)

/** @endcond */

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_TREAD_THREAD_H_ */
