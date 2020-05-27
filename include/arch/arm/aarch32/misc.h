/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM AArch32 public kernel miscellaneous
 *
 * ARM AArch32-specific kernel miscellaneous interface. Included by arm/arch.h.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_AARCH32_MISC_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_AARCH32_MISC_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE
extern uint32_t z_timer_cycle_get_32(void);

static inline uint32_t arch_k_cycle_get_32(void)
{
	return z_timer_cycle_get_32();
}

static ALWAYS_INLINE void arch_nop(void)
{
	__asm__ volatile("nop");
}

#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_AARCH32_MISC_H_ */
