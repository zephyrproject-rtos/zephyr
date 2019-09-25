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

#ifdef __cplusplus
extern "C" {
#endif

#define STACK_ALIGN 4
#define STACK_ALIGN_SIZE 4

struct __esf {
	u32_t dummy; /*maybe we will want to add something someday*/
};

typedef struct __esf z_arch_esf_t;

extern u32_t z_timer_cycle_get_32(void);
#define z_arch_k_cycle_get_32()  z_timer_cycle_get_32()

/**
 * @brief Explicitly nop operation.
 */
static ALWAYS_INLINE void arch_nop(void)
{
	__asm__ volatile("nop");
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_POSIX_ARCH_H_ */
