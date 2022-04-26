/*
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Xtensa specific kernel interface header
 * This header contains the Xtensa specific kernel interface.  It is included
 * by the generic kernel interface header (include/arch/cpu.h)
 */

#ifndef ZEPHYR_INCLUDE_ARCH_XTENSA_ARCH_H_
#define ZEPHYR_INCLUDE_ARCH_XTENSA_ARCH_H_

#include <zephyr/irq.h>

#include <zephyr/devicetree.h>
#if !defined(_ASMLANGUAGE) && !defined(__ASSEMBLER__)
#include <zephyr/types.h>
#include <zephyr/toolchain.h>
#include <zephyr/arch/common/sys_bitops.h>
#include <zephyr/arch/common/sys_io.h>
#include <zephyr/arch/common/ffs.h>
#include <zephyr/sw_isr_table.h>
#include <zephyr/arch/xtensa/thread.h>
#include <zephyr/arch/xtensa/irq.h>
#include <xtensa/config/core.h>
#include <zephyr/arch/common/addr_types.h>
#include <zephyr/arch/xtensa/gdbstub.h>

#ifdef CONFIG_KERNEL_COHERENCE
#define ARCH_STACK_PTR_ALIGN XCHAL_DCACHE_LINESIZE
#else
#define ARCH_STACK_PTR_ALIGN 16
#endif

/* Xtensa GPRs are often designated by two different names */
#define sys_define_gpr_with_alias(name1, name2) union { uint32_t name1, name2; }

#include <zephyr/arch/xtensa/exc.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void xtensa_arch_except(int reason_p);

#define ARCH_EXCEPT(reason_p) do { \
	xtensa_arch_except(reason_p); \
} while (false)

/* internal routine documented in C file, needed by IRQ_CONNECT() macro */
extern void z_irq_priority_set(uint32_t irq, uint32_t prio, uint32_t flags);

#define ARCH_IRQ_CONNECT(irq_p, priority_p, isr_p, isr_param_p, flags_p) \
{ \
	Z_ISR_DECLARE(irq_p, flags_p, isr_p, isr_param_p); \
}

/* Spurious interrupt handler. Throws an error if called */
extern void z_irq_spurious(const void *unused);

#define XTENSA_ERR_NORET

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

#ifdef __cplusplus
}
#endif

#endif /* !defined(_ASMLANGUAGE) && !defined(__ASSEMBLER__)  */

#endif /* ZEPHYR_INCLUDE_ARCH_XTENSA_ARCH_H_ */
