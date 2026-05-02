/*
 * Copyright (c) 2021 KT-Elektronik, Klaucke und Partner GmbH
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Renesas RX public kernel miscellaneous
 *
 *  Renesas RX-specific kernel miscellaneous interface. Included by arch/rx/arch.h.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_RX_MISC_H_
#define ZEPHYR_INCLUDE_ARCH_RX_MISC_H_

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
	__asm__ volatile("nop;");
}

#define arch_brk()  __asm__ volatile("brk;")
#define arch_wait() __asm__ volatile("wait;")

#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_RX_MISC_H_ */
