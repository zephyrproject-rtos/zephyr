/*
 * Copyright (c) Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_XTENSA_STRUCTS_H_
#define ZEPHYR_INCLUDE_ARCH_XTENSA_STRUCTS_H_

/* Per CPU architecture specifics */
struct _cpu_arch {
#if defined(CONFIG_XTENSA_LAZY_HIFI_SHARING)
	atomic_ptr_val_t hifi_owner; /* Owner of HiFi */
#if CONFIG_MP_MAX_NUM_CPUS > 1
	atomic_ptr_val_t save_hifi;  /* Save HiFi on IPI if match hifi_owner */
#endif
#elif defined(__cplusplus)
	/* An empty struct is not valid C, and compilers that accept it give
	 * it size 0 while C++ gives 1. Keep a byte so both languages agree.
	 */
	uint8_t dummy;
#endif
};

#endif /* ZEPHYR_INCLUDE_ARCH_XTENSA_STRUCTS_H_ */
