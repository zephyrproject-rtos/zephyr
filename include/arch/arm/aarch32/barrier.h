/*
 * Copyright (c) 2022 Alexandre Mergnat <amergnat@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_AARCH32_BARRIER_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_AARCH32_BARRIER_H_

#ifndef _ASMLANGUAGE

#if defined(__clang__)

#define z_full_mb()	__builtin_arm_dsb(0xF)
#define z_read_mb()	__builtin_arm_dsb(0xF)
#define z_write_mb()	__builtin_arm_dsb(0xF)

#else

#define z_full_mb()	__asm__ volatile ("dsb 0xF" : : : "memory")
#define z_read_mb()	__asm__ volatile ("dsb 0xF" : : : "memory")
#define z_write_mb()	__asm__ volatile ("dsb 0xF" : : : "memory")

#endif /* __clang__ */

#endif /* !_ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_AARCH32_BARRIER_H_ */
