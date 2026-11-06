/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Renesas RH850 specific kernel interface header
 *
 * This header contains the Renesas RH850 specific kernel interface.  It is
 * included by the kernel interface architecture-abstraction header
 * (include/zephyr/arch/cpu.h).
 */

#ifndef ZEPHYR_INCLUDE_ARCH_RH850_ARCH_H_
#define ZEPHYR_INCLUDE_ARCH_RH850_ARCH_H_

/* Add include for DTS generated information */
#include <zephyr/arch/exception.h>
#include <zephyr/devicetree.h>
#include <zephyr/arch/rh850/thread.h>
#include <zephyr/arch/rh850/misc.h>
#include <zephyr/arch/rh850/arch_inlines.h>
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

#define REG(addr) *((volatile uint8_t *)(addr))

/* isr for undefined interrupts (results in a fatal error) */
void z_irq_spurious(const void *unused);

#ifndef CONFIG_RH850_CUSTOM_INTERRUPT_CONTROLLER
extern void rh850_irq_enable(unsigned int irq);
extern void rh850_irq_disable(unsigned int irq);
extern int rh850_irq_is_enabled(unsigned int irq);
extern void rh850_irq_priority_set(unsigned int irq, unsigned int prio, uint32_t flags);

#define arch_irq_enable(irq)                    rh850_irq_enable(irq)
#define arch_irq_disable(irq)                   rh850_irq_disable(irq)
#define arch_irq_is_enabled(irq)                rh850_irq_is_enabled(irq)
#define arch_irq_priority_set(irq, prio, flags) rh850_irq_priority_set(irq, prio, flags)

#else
/*
 * When a custom interrupt controller is specified,
 * map the architecture interrupt control functions to the SoC layer interrupt
 * control functions.
 */

extern void z_soc_irq_enable(unsigned int irq);
extern void z_soc_irq_disable(unsigned int irq);
extern int z_soc_irq_is_enabled(unsigned int irq);
extern void z_soc_irq_priority_set(unsigned int irq, unsigned int prio, unsigned int flags);

#define arch_irq_enable(irq)                    z_soc_irq_enable(irq)
#define arch_irq_disable(irq)                   z_soc_irq_disable(irq)
#define arch_irq_is_enabled(irq)                z_soc_irq_is_enabled(irq)
#define arch_irq_priority_set(irq, prio, flags) z_soc_irq_priority_set(irq, prio, flags)
#endif

extern void z_rh850_isr_enter(void);
extern void z_rh850_isr_exit(void);

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
		arch_irq_priority_set(irq_p, priority_p, flags_p);                                 \
	}

static ALWAYS_INLINE unsigned int arch_irq_lock(void)
{
	unsigned int status;

	__asm__ volatile("stsr 5, %0, 0\n" /* Save PSW state */
			 "di\n"
			 : "=r"(status)
			 :
			 : "memory");

	return (status & (1U << 5)); /* Return PSW.ID before setting as key */
}

static ALWAYS_INLINE void arch_irq_unlock(unsigned int key)
{
	volatile unsigned int k = key;

	if (0U == (k & (1U << 5))) {
		__asm__ volatile("ei" : : : "memory"); /* Enable acknowledgment of EI interrrupts */
	}
}

static ALWAYS_INLINE bool arch_irq_unlocked(unsigned int key)
{
	volatile unsigned int k = key;

	return (0U == (k & (1U << 5)));
}

static ALWAYS_INLINE uint32_t rh850_get_core_id(void)
{
	uint32_t peid;

	__asm__ volatile("stsr 0, %0, 2\n" : "=r"(peid) : : "memory");

	return (peid & 0x1FU); /* bits [4:0] is PEID */
}

static ALWAYS_INLINE _cpu_t *arch_curr_cpu(void)
{
#if defined(CONFIG_SMP)
	uint32_t core = rh850_get_core_id();

	__ASSERT(core < CONFIG_MP_MAX_NUM_CPUS, "Invalid core id %u", core);

	return &_kernel.cpus[core];
#else
	return &_kernel.cpus[0];
#endif
}

/** Implementation of @ref arch_cpu_irqs_are_enabled. */
static ALWAYS_INLINE bool arch_cpu_irqs_are_enabled(void)
{
	uint32_t psw;

	__asm__ volatile("stsr 5, %0, 0\n" : "=r"(psw) : : "memory");

	return (psw & BIT(5)) == 0; /* bits [5] is ID */
}

#ifdef __cplusplus
}
#endif

#endif /* !_ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_RH850_ARCH_H_ */
