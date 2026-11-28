/*
 * Copyright (c) Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_XTENSA_STRUCTS_H_
#define ZEPHYR_INCLUDE_ARCH_XTENSA_STRUCTS_H_

#include <xtensa/config/core-isa.h>

#if XCHAL_HAVE_NMI
#define XTENSA_HPINT_MAX_LEVEL XCHAL_NMILEVEL
#else
#define XTENSA_HPINT_MAX_LEVEL XCHAL_NUM_INTLEVELS
#endif

/* Interrupt level above XCHAL_EXCM_LEVEL that runs on its own stack (not the debug level) */
#define XTENSA_HPINT_LEVEL(l) (((l) > XCHAL_EXCM_LEVEL) && ((l) != XCHAL_DEBUGLEVEL))

/* Number of such levels */
#if (XCHAL_DEBUGLEVEL > XCHAL_EXCM_LEVEL) && (XCHAL_DEBUGLEVEL <= XTENSA_HPINT_MAX_LEVEL)
#define XTENSA_HPINT_LEVELS (XTENSA_HPINT_MAX_LEVEL - XCHAL_EXCM_LEVEL - 1)
#else
#define XTENSA_HPINT_LEVELS (XTENSA_HPINT_MAX_LEVEL - XCHAL_EXCM_LEVEL)
#endif

/* Per CPU architecture specifics */
struct _cpu_arch {
#if defined(CONFIG_XTENSA_LAZY_CP_SHARING)
	atomic_ptr_val_t cp_owner; /* Owner of the coprocessor */
#if CONFIG_MP_MAX_NUM_CPUS > 1
	atomic_ptr_val_t save_cp;  /* Save on IPI if match cp_owner */
#endif
#elif defined(__cplusplus)
	/* An empty struct is not valid C, and compilers that accept it give
	 * it size 0 while C++ gives 1. Keep a byte so both languages agree.
	 */
	uint8_t dummy;
#endif
#if XTENSA_HPINT_LEVELS > 0
	/* One stack per interrupt level above XCHAL_EXCM_LEVEL */
	uint8_t hpint_stacks[XTENSA_HPINT_LEVELS][CONFIG_XTENSA_HPINT_STACK_SIZE] __aligned(16);
#endif
};

#endif /* ZEPHYR_INCLUDE_ARCH_XTENSA_STRUCTS_H_ */
