/*
 * Copyright (c) 2019-2020 Cobham Gaisler AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SPARC specific kernel interface header
 * This header contains the SPARC specific kernel interface.  It is
 * included by the generic kernel interface header (arch/cpu.h)
 */

#ifndef ZEPHYR_INCLUDE_ARCH_SPARC_ARCH_H_
#define ZEPHYR_INCLUDE_ARCH_SPARC_ARCH_H_

#include <zephyr/arch/sparc/thread.h>
#include <zephyr/arch/sparc/sparc.h>
#include <zephyr/arch/common/sys_bitops.h>
#include <zephyr/arch/common/sys_io.h>
#include <zephyr/arch/common/ffs.h>

#include <zephyr/irq.h>
#include <zephyr/sw_isr_table.h>
#include <soc.h>
#include <zephyr/devicetree.h>

/* stacks, for SPARC architecture stack shall be 8byte-aligned */
#define ARCH_STACK_PTR_ALIGN 8

/*
 * Software trap numbers.
 * Assembly usage: "ta SPARC_SW_TRAP_<TYPE>"
 */
#define SPARC_SW_TRAP_FLUSH_WINDOWS     0x03
#define SPARC_SW_TRAP_SET_PIL           0x09
#define SPARC_SW_TRAP_EXCEPT            0x0F

#ifndef _ASMLANGUAGE
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

#define STACK_ROUND_UP(x) ROUND_UP(x, ARCH_STACK_PTR_ALIGN)

/*
 * SOC specific function to translate from processor interrupt request level
 * (1..15) to logical interrupt source number. For example by probing the
 * interrupt controller.
 */
int z_sparc_int_get_source(int irl);
void z_irq_spurious(const void *unused);


#define ARCH_IRQ_CONNECT(irq_p, priority_p, isr_p, isr_param_p, flags_p) \
	{								 \
		Z_ISR_DECLARE(irq_p, 0, isr_p, isr_param_p);		 \
	}


static ALWAYS_INLINE unsigned int z_sparc_set_pil_inline(unsigned int newpil)
{
	register uint32_t oldpil __asm__ ("o0") = newpil;

	__asm__ volatile (
		"ta %1\nnop\n" :
		"=r" (oldpil) :
		"i" (SPARC_SW_TRAP_SET_PIL), "r" (oldpil) :
		"memory"
		);
	return oldpil;
}

static ALWAYS_INLINE unsigned int arch_irq_lock(void)
{
	return z_sparc_set_pil_inline(15);
}

static ALWAYS_INLINE void arch_irq_unlock(unsigned int key)
{
	z_sparc_set_pil_inline(key);
}

static ALWAYS_INLINE bool arch_irq_unlocked(unsigned int key)
{
	return key == 0;
}

static ALWAYS_INLINE void arch_nop(void)
{
	__asm__ volatile ("nop");
}

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

struct __esf {
	uint32_t out[8];
	uint32_t global[8];
	uint32_t psr;
	uint32_t pc;
	uint32_t npc;
	uint32_t wim;
	uint32_t tbr;
	uint32_t y;
};

typedef struct __esf z_arch_esf_t;

#define ARCH_EXCEPT(reason_p)						\
do {									\
	register uint32_t _g1 __asm__("g1") = reason_p;			\
									\
	__asm__ volatile (						\
		"ta %[vector]\n\t"					\
		:							\
		: [vector] "i" (SPARC_SW_TRAP_EXCEPT), "r" (_g1)	\
		: "memory"						\
	);								\
	CODE_UNREACHABLE;						\
} while (false)

#ifdef __cplusplus
}
#endif

#endif  /*_ASMLANGUAGE */

#endif  /* ZEPHYR_INCLUDE_ARCH_SPARC_ARCH_H_ */
