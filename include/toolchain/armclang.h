/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_TOOLCHAIN_ARMCLANG_H_
#define ZEPHYR_INCLUDE_TOOLCHAIN_ARMCLANG_H_


#include <toolchain/llvm.h>

/*
 * To reuse as much as possible from the llvm.h header we only redefine the
 * __GENERIC_SECTION and Z_GENERIC_SECTION macros here to include the `used` keyword.
 */
#undef __GENERIC_SECTION
#undef Z_GENERIC_SECTION

#define __GENERIC_SECTION(segment) __attribute__((section(STRINGIFY(segment)), used))
#define Z_GENERIC_SECTION(segment) __GENERIC_SECTION(segment)

/* Memory Barrier buitin functions */
#ifdef CONFIG_ARM
#define __HAS_BUILTIN_MEMORY_BARRIER 1
#define arch_isb()		__builtin_arm_isb(0xF)
#define arch_mb()		__builtin_arm_dsb(0xF)
#define arch_rmb()		__builtin_arm_dsb(0xF)
#define arch_wmb()		__builtin_arm_dsb(0xF)
#define arch_smp_mb()	__builtin_arm_dmb(0xF)
#define arch_smp_rmb()	__builtin_arm_dmb(0xF)
#define arch_smp_wmb()	__builtin_arm_dmb(0xF)
#endif /* CONFIG_ARM */

#endif /* ZEPHYR_INCLUDE_TOOLCHAIN_ARMCLANG_H_ */
