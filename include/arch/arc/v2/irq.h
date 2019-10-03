/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARCv2 public interrupt handling
 *
 * ARCv2 kernel interrupt handling interface. Included by arc/arch.h.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARC_V2_IRQ_H_
#define ZEPHYR_INCLUDE_ARCH_ARC_V2_IRQ_H_

#include <arch/arc/v2/aux_regs.h>
#include <toolchain/common.h>
#include <irq.h>
#include <sys/util.h>
#include <sw_isr_table.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _ASMLANGUAGE
GTEXT(_irq_exit);
GTEXT(z_arch_irq_enable)
GTEXT(z_arch_irq_disable)
#else

extern void z_arch_irq_enable(unsigned int irq);
extern void z_arch_irq_disable(unsigned int irq);

extern void _irq_exit(void);
extern void z_irq_priority_set(unsigned int irq, unsigned int prio,
			      u32_t flags);
extern void _isr_wrapper(void);
extern void z_irq_spurious(void *unused);

/* Z_ISR_DECLARE will populate the .intList section with the interrupt's
 * parameters, which will then be used by gen_irq_tables.py to create
 * the vector table and the software ISR table. This is all done at
 * build-time.
 *
 * We additionally set the priority in the interrupt controller at
 * runtime.
 */
#define Z_ARCH_IRQ_CONNECT(irq_p, priority_p, isr_p, isr_param_p, flags_p) \
({ \
	Z_ISR_DECLARE(irq_p, 0, isr_p, isr_param_p); \
	z_irq_priority_set(irq_p, priority_p, flags_p); \
	irq_p; \
})

static ALWAYS_INLINE unsigned int z_arch_irq_lock(void)
{
	unsigned int key;

	__asm__ volatile("clri %0" : "=r"(key):: "memory");
	return key;
}

static ALWAYS_INLINE void z_arch_irq_unlock(unsigned int key)
{
	__asm__ volatile("seti %0" : : "ir"(key) : "memory");
}

static ALWAYS_INLINE bool z_arch_irq_unlocked(unsigned int key)
{
	/* ARC irq lock uses instruction "clri r0",
	 * r0 ==  {26’d0, 1’b1, STATUS32.IE, STATUS32.E[3:0] }
	 * bit4 is used to record IE (Interrupt Enable) bit
	 */
	return (key & 0x10)  ==  0x10;
}

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_ARC_V2_IRQ_H_ */
