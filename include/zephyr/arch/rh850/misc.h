/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Renesas RH850 public kernel miscellaneous
 *
 *  Renesas RH850-specific kernel miscellaneous interface. Included by arch/rh850/arch.h.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_RH850_MISC_H_
#define ZEPHYR_INCLUDE_ARCH_RH850_MISC_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE

extern uint32_t sys_clock_cycle_get_32(void);

static inline uint32_t arch_k_cycle_get_32(void)
{
	return sys_clock_cycle_get_32();
}

extern uint64_t sys_clock_cycle_get_64(void);

static inline uint64_t arch_k_cycle_get_64(void)
{
	return sys_clock_cycle_get_64();
}

static ALWAYS_INLINE void arch_nop(void)
{
	__asm__ volatile("nop");
}

#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_RH850_MISC_H_ */
