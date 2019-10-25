/*
 * Copyright (c) 2010-2014 Wind River Systems, Inc.
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief POSIX arch specific kernel interface header
 * This header contains the POSIX arch specific kernel interface.
 * It is included by the generic kernel interface header (include/arch/cpu.h)
 *
 */


#ifndef ZEPHYR_INCLUDE_ARCH_POSIX_ARCH_H_
#define ZEPHYR_INCLUDE_ARCH_POSIX_ARCH_H_

/* Add include for DTS generated information */
#include <generated_dts_board.h>

#include <toolchain.h>
#include <irq.h>
#include <arch/posix/asm_inline.h>
#include <board_irq.h> /* Each board must define this */
#include <sw_isr_table.h>
#include <posix_soc_if.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_64BIT
#define STACK_ALIGN 8
#define STACK_ALIGN_SIZE 8
#else
#define STACK_ALIGN 4
#define STACK_ALIGN_SIZE 4
#endif

struct __esf {
	u32_t dummy; /*maybe we will want to add something someday*/
};

typedef struct __esf z_arch_esf_t;

extern u32_t z_timer_cycle_get_32(void);

static inline u32_t z_arch_k_cycle_get_32(void)
{
	return z_timer_cycle_get_32();
}

static ALWAYS_INLINE void z_arch_nop(void)
{
	__asm__ volatile("nop");
}

static ALWAYS_INLINE bool z_arch_irq_unlocked(unsigned int key)
{
	return key == false;
}

static ALWAYS_INLINE unsigned int z_arch_irq_lock(void)
{
	return posix_irq_lock();
}


static ALWAYS_INLINE void z_arch_irq_unlock(unsigned int key)
{
	posix_irq_unlock(key);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_POSIX_ARCH_H_ */
