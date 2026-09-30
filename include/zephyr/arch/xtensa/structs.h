/*
 * Copyright (c) Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_XTENSA_STRUCTS_H_
#define ZEPHYR_INCLUDE_ARCH_XTENSA_STRUCTS_H_

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
};

#endif /* ZEPHYR_INCLUDE_ARCH_XTENSA_STRUCTS_H_ */
