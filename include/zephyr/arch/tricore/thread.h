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
};

typedef struct _callee_saved _callee_saved_t;

struct _thread_arch {
	/* empty */
};

typedef struct _thread_arch _thread_arch_t;

/** @endcond */

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_TREAD_THREAD_H_ */
