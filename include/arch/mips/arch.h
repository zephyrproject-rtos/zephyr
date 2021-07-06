/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_MIPS_ARCH_H_
#define ZEPHYR_INCLUDE_ARCH_MIPS_ARCH_H_

#include <arch/mips/thread.h>
#include <arch/common/sys_bitops.h>
#include <arch/common/sys_io.h>
#include <arch/common/ffs.h>

#include <irq.h>
#include <devicetree.h>

#define ARCH_STACK_PTR_ALIGN 8

#ifndef _ASMLANGUAGE
#include <mips/cpu.h>
#include <sys/util.h>
#include <sw_isr_table.h>

#ifdef __cplusplus
extern "C" {
#endif

void arch_irq_enable(unsigned int irq);
void arch_irq_disable(unsigned int irq);
int arch_irq_is_enabled(unsigned int irq);
void z_irq_spurious(const void *unused);

/**
 * Configure a static interrupt.
 *
 * All arguments must be computable by the compiler at build time.
 *
 * @param irq_p IRQ line number
 * @param priority_p Interrupt priority
 * @param isr_p Interrupt service routine
 * @param isr_param_p ISR parameter
 * @param flags_p IRQ options
 *
 * @return The vector assigned to this interrupt
 */

#define ARCH_IRQ_CONNECT(irq_p, priority_p, isr_p, isr_param_p, flags_p) \
	{								 \
		Z_ISR_DECLARE(irq_p, 0, isr_p, isr_param_p);		 \
	}

static ALWAYS_INLINE unsigned int arch_irq_lock(void)
{
	if (mips_getsr() & SR_IE) {
		mips_bicsr(SR_IE);
		return 1;
	}
	return 0;
}

static ALWAYS_INLINE void arch_irq_unlock(unsigned int key)
{
	if (key) {
		mips_bissr(SR_IE);
	} else {
		mips_bicsr(SR_IE);
	}
}

static ALWAYS_INLINE bool arch_irq_unlocked(unsigned int key)
{
	return key != 0;
}

static ALWAYS_INLINE void arch_nop(void)
{
	__asm__ volatile ("nop");
}

extern uint32_t z_timer_cycle_get_32(void);

static inline uint32_t arch_k_cycle_get_32(void)
{
	return z_timer_cycle_get_32();
}

#include <mips/cpu.h>

struct linkctx;

struct __esf {
	reg_t r[31];
	reg_t epc;
	reg_t badvaddr;
	reg_t hi;
	reg_t lo;
	/* This field is for future extension */
	struct linkctx *link;
	/* Status at the point of the exception.  This may not be restored
	 * on return from exception if running under an RTOS */
	uint32_t status;
	/* These fields should be considered read-only */
	uint32_t cause;
	uint32_t badinstr;
	uint32_t badpinstr;
};

typedef struct __esf z_arch_esf_t;

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_MIPS_ARCH_H_ */
