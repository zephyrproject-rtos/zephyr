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

#include <zephyr/arch/arc/v2/aux_regs.h>
#include <zephyr/toolchain/common.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util.h>
#include <zephyr/sw_isr_table.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ARC_MP_PRIMARY_CPU_ID 0

#ifndef _ASMLANGUAGE

extern void z_arc_firq_stack_set(void);
extern void arch_irq_enable(unsigned int irq);
extern void arch_irq_disable(unsigned int irq);
extern int arch_irq_is_enabled(unsigned int irq);
#ifdef CONFIG_TRACING_ISR
extern void sys_trace_isr_enter(void);
extern void sys_trace_isr_exit(void);
#endif

extern void z_irq_priority_set(unsigned int irq, unsigned int prio,
			      uint32_t flags);

/* Z_ISR_DECLARE will populate the .intList section with the interrupt's
 * parameters, which will then be used by gen_irq_tables.py to create
 * the vector table and the software ISR table. This is all done at
 * build-time.
 *
 * We additionally set the priority in the interrupt controller at
 * runtime.
 */
#define ARCH_IRQ_CONNECT(irq_p, priority_p, isr_p, isr_param_p, flags_p) \
{ \
	Z_ISR_DECLARE(irq_p, 0, isr_p, isr_param_p); \
	z_irq_priority_set(irq_p, priority_p, flags_p); \
}

/**
 * Configure a 'direct' static interrupt.
 *
 * When firq has no separate stack(CONFIG_ARC_FIRQ_STACK=N), it's not safe
 * to call C ISR handlers because sp will be switched to bank1's sp which
 * is undefined value.
 * So for this case, the priority cannot be set to 0 but next level 1
 *
 * When firq has separate stack (CONFIG_ARC_FIRQ_STACK=y) but at the same
 * time stack checking is enabled (CONFIG_ARC_STACK_CHECKING=y)
 * the stack checking can raise stack check exception as sp is switched to
 * firq's stack (bank1's sp). So for this case, the priority cannot be set
 * to 0 but next level 1.
 *
 * Note that for the above cases, if application still wants to use firq by
 * setting priority to 0. Application can call z_irq_priority_set again.
 * Then it's left to application to handle the details of firq
 *
 * See include/irq.h for details.
 * All arguments must be computable at build time.
 */
#define ARCH_IRQ_DIRECT_CONNECT(irq_p, priority_p, isr_p, flags_p) \
{ \
	Z_ISR_DECLARE(irq_p, ISR_FLAG_DIRECT, isr_p, NULL); \
	BUILD_ASSERT(priority_p || !IS_ENABLED(CONFIG_ARC_FIRQ) || \
	(IS_ENABLED(CONFIG_ARC_FIRQ_STACK) && \
	!IS_ENABLED(CONFIG_ARC_STACK_CHECKING)), \
	"irq priority cannot be set to 0 when CONFIG_ARC_FIRQ_STACK" \
	"is not configured or CONFIG_ARC_FIRQ_STACK " \
	"and CONFIG_ARC_STACK_CHECKING are configured together"); \
	z_irq_priority_set(irq_p, priority_p, flags_p); \
}


static inline void arch_isr_direct_header(void)
{
#ifdef CONFIG_TRACING_ISR
	sys_trace_isr_enter();
#endif
}

static inline void arch_isr_direct_footer(int maybe_swap)
{
	/* clear SW generated interrupt */
	if (z_arc_v2_aux_reg_read(_ARC_V2_ICAUSE) ==
	    z_arc_v2_aux_reg_read(_ARC_V2_AUX_IRQ_HINT)) {
		z_arc_v2_aux_reg_write(_ARC_V2_AUX_IRQ_HINT, 0);
	}
#ifdef CONFIG_TRACING_ISR
	sys_trace_isr_exit();
#endif
}

#define ARCH_ISR_DIRECT_HEADER() arch_isr_direct_header()
extern void arch_isr_direct_header(void);

#define ARCH_ISR_DIRECT_FOOTER(swap) arch_isr_direct_footer(swap)

#if defined(__CCAC__)
#define _ARC_DIRECT_ISR_FUNC_ATTRIBUTE		__interrupt__
#else
#define _ARC_DIRECT_ISR_FUNC_ATTRIBUTE		interrupt("ilink")
#endif

/*
 * Scheduling can not be done in direct isr. If required, please use kernel
 * aware interrupt handling
 */
#define ARCH_ISR_DIRECT_DECLARE(name) \
	static inline int name##_body(void); \
	__attribute__ ((_ARC_DIRECT_ISR_FUNC_ATTRIBUTE))void name(void) \
	{ \
		ISR_DIRECT_HEADER(); \
		name##_body(); \
		ISR_DIRECT_FOOTER(0); \
	} \
	static inline int name##_body(void)


/**
 *
 * @brief Disable all interrupts on the local CPU
 *
 * This routine disables interrupts.  It can be called from either interrupt or
 * thread level.  This routine returns an architecture-dependent
 * lock-out key representing the "interrupt disable state" prior to the call;
 * this key can be passed to irq_unlock() to re-enable interrupts.
 *
 * The lock-out key should only be used as the argument to the
 * irq_unlock() API.  It should never be used to manually re-enable
 * interrupts or to inspect or manipulate the contents of the source register.
 *
 * This function can be called recursively: it will return a key to return the
 * state of interrupt locking to the previous level.
 *
 * WARNINGS
 * Invoking a kernel routine with interrupts locked may result in
 * interrupts being re-enabled for an unspecified period of time.  If the
 * called routine blocks, interrupts will be re-enabled while another
 * thread executes, or while the system is idle.
 *
 * The "interrupt disable state" is an attribute of a thread.  Thus, if a
 * thread disables interrupts and subsequently invokes a kernel
 * routine that causes the calling thread to block, the interrupt
 * disable state will be restored when the thread is later rescheduled
 * for execution.
 *
 * @return An architecture-dependent lock-out key representing the
 * "interrupt disable state" prior to the call.
 */

static ALWAYS_INLINE unsigned int arch_irq_lock(void)
{
	unsigned int key;

	__asm__ volatile("clri %0" : "=r"(key):: "memory");
	return key;
}

static ALWAYS_INLINE void arch_irq_unlock(unsigned int key)
{
	__asm__ volatile("seti %0" : : "ir"(key) : "memory");
}

static ALWAYS_INLINE bool arch_irq_unlocked(unsigned int key)
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
