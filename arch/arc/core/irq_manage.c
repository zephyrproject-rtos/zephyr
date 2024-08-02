/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARCv2 interrupt management
 *
 *
 * Interrupt management:
 *
 * - enabling/disabling
 *
 * An IRQ number passed to the @a irq parameters found in this file is a
 * number from 16 to last IRQ number on the platform.
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/sw_isr_table.h>
#include <zephyr/irq.h>
#include <zephyr/sys/printk.h>


/*
 * storage space for the interrupt stack of fast_irq
 */
#if defined(CONFIG_ARC_FIRQ_STACK)
#if defined(CONFIG_SMP)
K_KERNEL_STACK_ARRAY_DEFINE(_firq_interrupt_stack, CONFIG_MP_MAX_NUM_CPUS,
			    CONFIG_ARC_FIRQ_STACK_SIZE);
#else
K_KERNEL_STACK_DEFINE(_firq_interrupt_stack, CONFIG_ARC_FIRQ_STACK_SIZE);
#endif

/**
 * @brief Set the stack pointer for firq handling
 */
void z_arc_firq_stack_set(void)
{
#ifdef CONFIG_SMP
	char *firq_sp = K_KERNEL_STACK_BUFFER(
		  _firq_interrupt_stack[z_arc_v2_core_id()]) +
		  CONFIG_ARC_FIRQ_STACK_SIZE;
#else
	char *firq_sp = K_KERNEL_STACK_BUFFER(_firq_interrupt_stack) +
		  CONFIG_ARC_FIRQ_STACK_SIZE;
#endif

/* the z_arc_firq_stack_set must be called when irq diasbled, as
 * it can be called not only in the init phase but also other places
 */
	unsigned int key = arch_irq_lock();

	__asm__ volatile (
/* only ilink will not be banked, so use ilink as channel
 * between 2 banks
 */
	"mov %%ilink, %0\n\t"
	"lr %0, [%1]\n\t"
	"or %0, %0, %2\n\t"
	"kflag %0\n\t"
	"mov %%sp, %%ilink\n\t"
/* switch back to bank0, use ilink to avoid the pollution of
 * bank1's gp regs.
 */
	"lr %%ilink, [%1]\n\t"
	"and %%ilink, %%ilink, %3\n\t"
	"kflag %%ilink\n\t"
	:
	: "r"(firq_sp), "i"(_ARC_V2_STATUS32),
	  "i"(_ARC_V2_STATUS32_RB(1)),
	  "i"(~_ARC_V2_STATUS32_RB(7))
	);
	arch_irq_unlock(key);
}
#endif

/*
 * ARC CPU interrupt controllers hierarchy.
 *
 * Single-core (UP) case:
 *
 *   --------------------------
 *   |  CPU core 0            |
 *   --------------------------
 *   |  core 0 (private)      |
 *   |  interrupt controller  |
 *   --------------------------
 *               |
 *      [internal interrupts]
 *      [external interrupts]
 *
 *
 * Multi-core (SMP) case:
 *
 *   --------------------------               --------------------------
 *   |  CPU core 0            |               |  CPU core 1            |
 *   --------------------------               --------------------------
 *   |  core 0 (private)      |               |  core 1 (private)      |
 *   |  interrupt controller  |               |  interrupt controller  |
 *   --------------------------               --------------------------
 *     |    |      |                                |     |      |
 *     |    | [core 0 private internal interrupts]  |     |   [core 1 private internal interrupts]
 *     |    |                                       |     |
 *     |    |                                       |     |
 *     |   -------------------------------------------    |
 *     |   |     IDU (Interrupt Distribution Unit)   |    |
 *     |   -------------------------------------------    |
 *     |                       |                          |
 *     |          [common (shared) interrupts]            |
 *     |                                                  |
 *     |                                                  |
 *   [core 0 private external interrupts]               [core 1 private external interrupts]
 *
 *
 *
 *  The interrupts are grouped in HW in the same order - firstly internal interrupts
 *  (with lowest line numbers in IVT), than common interrupts (if present), than external
 *  interrupts (with highest line numbers in IVT).
 *
 *  NOTE: in case of SMP system we currently support in Zephyr only private internal and common
 *  interrupts, so the core-private external interrupts are currently not supported for SMP.
 */

/**
 * @brief Enable an interrupt line
 *
 * Clear possible pending interrupts on the line, and enable the interrupt
 * line. After this call, the CPU will receive interrupts for the specified
 * @a irq.
 */
void arch_irq_enable(unsigned int irq);

/**
 * @brief Disable an interrupt line
 *
 * Disable an interrupt line. After this call, the CPU will stop receiving
 * interrupts for the specified @a irq.
 */
void arch_irq_disable(unsigned int irq);

/**
 * @brief Return IRQ enable state
 *
 * @param irq IRQ line
 * @return interrupt enable state, true or false
 */
int arch_irq_is_enabled(unsigned int irq);

#ifdef CONFIG_ARC_CONNECT

#define IRQ_NUM_TO_IDU_NUM(id)		((id) - ARC_CONNECT_IDU_IRQ_START)
#define IRQ_IS_COMMON(id)		((id) >= ARC_CONNECT_IDU_IRQ_START)

void arch_irq_enable(unsigned int irq)
{
	if (IRQ_IS_COMMON(irq)) {
		z_arc_connect_idu_set_mask(IRQ_NUM_TO_IDU_NUM(irq), 0x0);
	} else {
		z_arc_v2_irq_unit_int_enable(irq);
	}
}

void arch_irq_disable(unsigned int irq)
{
	if (IRQ_IS_COMMON(irq)) {
		z_arc_connect_idu_set_mask(IRQ_NUM_TO_IDU_NUM(irq), 0x1);
	} else {
		z_arc_v2_irq_unit_int_disable(irq);
	}
}

int arch_irq_is_enabled(unsigned int irq)
{
	if (IRQ_IS_COMMON(irq)) {
		return !z_arc_connect_idu_read_mask(IRQ_NUM_TO_IDU_NUM(irq));
	} else {
		return z_arc_v2_irq_unit_int_enabled(irq);
	}
}
#else
void arch_irq_enable(unsigned int irq)
{
	z_arc_v2_irq_unit_int_enable(irq);
}

void arch_irq_disable(unsigned int irq)
{
	z_arc_v2_irq_unit_int_disable(irq);
}

int arch_irq_is_enabled(unsigned int irq)
{
	return z_arc_v2_irq_unit_int_enabled(irq);
}
#endif /* CONFIG_ARC_CONNECT */

/**
 * @internal
 *
 * @brief Set an interrupt's priority
 *
 * Lower values take priority over higher values. Special case priorities are
 * expressed via mutually exclusive flags.

 * The priority is verified if ASSERT_ON is enabled; max priority level
 * depends on CONFIG_NUM_IRQ_PRIO_LEVELS.
 */

void z_irq_priority_set(unsigned int irq, unsigned int prio, uint32_t flags)
{
	ARG_UNUSED(flags);

	__ASSERT(prio < CONFIG_NUM_IRQ_PRIO_LEVELS,
		 "invalid priority %d for irq %d", prio, irq);
/* 0 -> CONFIG_NUM_IRQ_PRIO_LEVELS allocated to secure world
 * left prio levels allocated to normal world
 */
#if defined(CONFIG_ARC_SECURE_FIRMWARE)
	prio = prio < ARC_N_IRQ_START_LEVEL ?
		prio : (ARC_N_IRQ_START_LEVEL - 1);
#elif defined(CONFIG_ARC_NORMAL_FIRMWARE)
	prio = prio < ARC_N_IRQ_START_LEVEL ?
		 ARC_N_IRQ_START_LEVEL : prio;
#endif
	z_arc_v2_irq_unit_prio_set(irq, prio);
}

/**
 * @brief Spurious interrupt handler
 *
 * Installed in all dynamic interrupt slots at boot time. Throws an error if
 * called.
 */

void z_irq_spurious(const void *unused)
{
	ARG_UNUSED(unused);
	z_fatal_error(K_ERR_SPURIOUS_IRQ, NULL);
}

#ifdef CONFIG_DYNAMIC_INTERRUPTS
int arch_irq_connect_dynamic(unsigned int irq, unsigned int priority,
			     void (*routine)(const void *parameter),
			     const void *parameter, uint32_t flags)
{
	z_isr_install(irq, routine, parameter);
	z_irq_priority_set(irq, priority, flags);
	return irq;
}
#endif /* CONFIG_DYNAMIC_INTERRUPTS */
