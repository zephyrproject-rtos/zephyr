/*
 * Copyright (c) BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_RISCV_STRUCTS_H_
#define ZEPHYR_INCLUDE_RISCV_STRUCTS_H_

/* Per CPU architecture specifics */
struct _cpu_arch {
#ifdef CONFIG_USERSPACE
	unsigned long user_exc_sp;
	unsigned long user_exc_tmp0;
	unsigned long user_exc_tmp1;
#endif
#if defined(CONFIG_SMP) || (CONFIG_MP_MAX_NUM_CPUS > 1)
	unsigned long hartid;
	bool online;
#endif
#ifdef CONFIG_FPU_SHARING
	atomic_ptr_val_t fpu_owner;
	uint32_t fpu_state;
#endif
#if defined(CONFIG_CPP) && !defined(CONFIG_USERSPACE) &&                                           \
	!(defined(CONFIG_SMP) || (CONFIG_MP_MAX_NUM_CPUS > 1)) && !defined(CONFIG_FPU_SHARING)
	/* Empty struct has size 0 in C, size 1 in C++. Force them to be the same. */
	uint8_t unused_cpp_size_compatibility;
#endif
};

#if defined(CONFIG_CPP)
BUILD_ASSERT(sizeof(struct _cpu_arch) >= 1);
#endif

#endif /* ZEPHYR_INCLUDE_RISCV_STRUCTS_H_ */
