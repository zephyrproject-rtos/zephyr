/*
 * Copyright (c) 2022 Alexandre Mergnat <amergnat@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_AARCH32_BARRIER_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_AARCH32_BARRIER_H_

#ifndef _ASMLANGUAGE

#if defined (__GNUC__)

#define z_full_mb()	__asm__ volatile ("dsb 0xF":::"memory")
#define z_read_mb()	__asm__ volatile ("dsb 0xF":::"memory")
#define z_write_mb()	__asm__ volatile ("dsb 0xF":::"memory")

#elif defined (__clang__)

#define z_full_mb()	__builtin_arm_dsb(0xF)
#define z_read_mb()	__builtin_arm_dsb(0xF)
#define z_write_mb()	__builtin_arm_dsb(0xF)

#else

#error The ARM32 architecture does not have a memory barrier implementation

#endif /* __GNUC__ */

#endif /* !_ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_AARCH32_BARRIER_H_ */
