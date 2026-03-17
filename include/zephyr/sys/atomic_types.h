/* Copyright (c) 1997-2015, Wind River Systems, Inc.
 * Copyright (c) 2021 Intel Corporation
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_ATOMIC_TYPES_H_
#define ZEPHYR_INCLUDE_SYS_ATOMIC_TYPES_H_

/* Detect C11 stdatomic support (C mode only, not C++) */
#if !defined(__cplusplus) && defined(__STDC_VERSION__) && \
	(__STDC_VERSION__ >= 201112L) && !defined(__STDC_NO_ATOMICS__)
#define ZEPHYR_HAS_C11_STDATOMIC 1
#include <stdatomic.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef long atomic_t;
typedef atomic_t atomic_val_t;
typedef void *atomic_ptr_t;
typedef atomic_ptr_t atomic_ptr_val_t;

/**
 * @brief Atomic unsigned char type.
 *
 * Named atomic_uchar_t to avoid conflicts with C11 stdatomic.h's atomic_uchar.
 *
 * Three-tier type system:
 * - Tier 3 (Fallback): On platforms without native byte-sized atomics
 *   (RISC-V, Xtensa, Cortex-M0/M0+, RX, ARC), falls back to atomic_t
 *   and uses word-sized atomic operations.
 * - Tier 1 (C11): When C11 stdatomic is available, uses atomic_uchar
 *   from <stdatomic.h> for true atomic semantics.
 * - Tier 2 (GCC Builtins): In C++ mode or without C11 support, uses
 *   unsigned char with GCC __atomic_* builtins.
 */
#if defined(CONFIG_RISCV) || defined(CONFIG_XTENSA) || \
	defined(CONFIG_CPU_CORTEX_M0) || defined(CONFIG_CPU_CORTEX_M0PLUS) || \
	defined(CONFIG_RX) || defined(CONFIG_ARC)
/* Tier 3: These architectures lack native byte-sized atomics, use word type */
typedef atomic_t atomic_uchar_t;
#elif defined(ZEPHYR_HAS_C11_STDATOMIC)
/* Tier 1: C11 stdatomic available - use atomic_uchar from <stdatomic.h> */
typedef atomic_uchar atomic_uchar_t;
#else
/* Tier 2: C++ mode or no C11 support - use unsigned char with GCC builtins */
typedef unsigned char atomic_uchar_t;
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_ATOMIC_TYPES_H_ */
