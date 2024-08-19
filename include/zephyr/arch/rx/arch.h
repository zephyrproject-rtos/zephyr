/*
 * Copyright (c) 2021 KT-Elektronik, Klaucke und Partner GmbH
 * Copyright (c) 2024 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_RX_ARCH_H_
#define ZEPHYR_INCLUDE_ARCH_RX_ARCH_H_

/* Add include for DTS generated information */
#include <zephyr/devicetree.h>

#include <zephyr/arch/rx/thread.h>
#include <zephyr/arch/rx/misc.h>
#include <zephyr/arch/rx/arch_inlines.h>
#include <zephyr/arch/common/sys_bitops.h>
#include <zephyr/arch/common/sys_io.h>
#include <zephyr/arch/common/ffs.h>
#include <zephyr/sw_isr_table.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>
#include <zephyr/irq.h>

#define ARCH_STACK_PTR_ALIGN 4

#ifndef _ASMLANGUAGE

#ifdef __cplusplus
extern "C" {
#endif

#define IR_BASE_ADDRESS DT_REG_ADDR_BY_NAME(DT_NODELABEL(icu), IR)
#define REG(addr)       *((uint8_t *)(addr))

/* isr for undefined interrupts (results in a fatal error) */
void z_irq_spurious(const void *unused);
/* internal routine documented in C file, needed by IRQ_CONNECT() macro */
extern void z_irq_priority_set(uint32_t irq, uint32_t prio, uint32_t flags);

/* Z_ISR_DECLARE will populate the .intList section with the interrupt's
 * parameters, which will then be used by gen_irq_tables.py to create
 * the vector table and the software ISR table. This is all done at
 * build-time.
 *
 * We additionally set the priority in the interrupt controller at
 * runtime.
 */
#define ARCH_IRQ_CONNECT(irq_p, priority_p, isr_p, isr_param_p, flags_p)                           \
	{                                                                                          \
		Z_ISR_DECLARE(irq_p, 0, isr_p, isr_param_p);                                       \
		z_irq_priority_set(irq_p, priority_p, flags_p);                                    \
	}

#if CONFIG_TRACING_ISR
#define ARCH_ISR_DIRECT_HEADER()                                                                   \
	{                                                                                          \
		_kernel.cpus[0].nested++;                                                          \
		sys_trace_isr_enter()                                                              \
	}
#else
#define ARCH_ISR_DIRECT_HEADER()                                                                   \
	{                                                                                          \
		_kernel.cpus[0].nested++;                                                          \
	}
#endif

#if CONFIG_TRACING_ISR
#define ARCH_ISR_DIRECT_FOOTER(check_reschedule)                                                   \
	{                                                                                          \
		sys_trace_isr_exit();                                                              \
		irq_lock();                                                                        \
		if (check_reschedule && _kernel.cpus[0].nested == 1) {                             \
			z_rx_irq_exit();                                                           \
		}                                                                                  \
		_kernel.cpus[0].nested--;                                                          \
	}
#else
#define ARCH_ISR_DIRECT_FOOTER(check_reschedule)                                                   \
	{                                                                                          \
		irq_lock();                                                                        \
		if (check_reschedule && _kernel.cpus[0].nested == 1) {                             \
			z_rx_irq_exit();                                                           \
		}                                                                                  \
		_kernel.cpus[0].nested--;                                                          \
	}
#endif

struct esf {
};

typedef struct esf arch_esf_t;

static ALWAYS_INLINE unsigned int arch_irq_lock(void)
{
	uint32_t key;
	/* deactivate interrupts by clearing the PSW-i flag */
	__asm__ volatile("MVFC psw, %0\n"
			 "CLRPSW i"
			 : "=r"(key)
			 :
			 : "cc");
	/* return the value of the i-flag before clearing
	 * if irqs were locked already, it was 0 and calling
	 * arch_irq_unlock(key) will not actually unlock irqs, as this was a
	 * nested irq lock
	 */
	return key & BIT(16);
}

static inline void arch_irq_unlock(unsigned int key)
{
	if (key != 0) {
		/* re-activate interrupts by setting the PSW i-flag*/
		__asm__ volatile("SETPSW i" ::: "cc");
	}
}

static inline bool arch_irq_unlocked(unsigned int key)
{
	return key != 0;
}

static inline bool arch_is_in_isr(void)
{
	return _kernel.cpus[0].nested != 0U;
}

static ALWAYS_INLINE _cpu_t *arch_curr_cpu(void)
{
	return &_kernel.cpus[0];
}

static inline void trigger_irq(int irq)
{
	__ASSERT(irq < CONFIG_NUM_IRQS, "attempting to trigger invalid IRQ (%u)", irq);
	__ASSERT(irq >= CONFIG_GEN_IRQ_START_VECTOR, "attempting to trigger reserved IRQ (%u)",
		 irq);
	WRITE_BIT(REG(IR_BASE_ADDRESS + irq), 0, true);
}

#ifdef __cplusplus
}
#endif

#endif /* !_ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_RX_ARCH_H_ */
