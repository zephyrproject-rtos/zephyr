/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Nios II C-domain interrupt management code for use with Internal
 * Interrupt Controller (IIC)
 */


#include <kernel.h>
#include <kernel_structs.h>
#include <arch/cpu.h>
#include <irq.h>
#include <sw_isr_table.h>
#include <ksched.h>
#include <kswap.h>
#include <debug/tracing.h>
#include <logging/log.h>
LOG_MODULE_DECLARE(os);

FUNC_NORETURN void z_irq_spurious(void *unused)
{
	ARG_UNUSED(unused);
	LOG_ERR("Spurious interrupt detected! ipending: %x",
		z_nios2_creg_read(NIOS2_CR_IPENDING));
	z_nios2_fatal_error(K_ERR_SPURIOUS_IRQ, NULL);
}


void z_arch_irq_enable(unsigned int irq)
{
	u32_t ienable;
	unsigned int key;

	key = irq_lock();

	ienable = z_nios2_creg_read(NIOS2_CR_IENABLE);
	ienable |= BIT(irq);
	z_nios2_creg_write(NIOS2_CR_IENABLE, ienable);

	irq_unlock(key);
};



void z_arch_irq_disable(unsigned int irq)
{
	u32_t ienable;
	unsigned int key;

	key = irq_lock();

	ienable = z_nios2_creg_read(NIOS2_CR_IENABLE);
	ienable &= ~BIT(irq);
	z_nios2_creg_write(NIOS2_CR_IENABLE, ienable);

	irq_unlock(key);
};


/**
 * @brief Interrupt demux function
 *
 * Given a bitfield of pending interrupts, execute the appropriate handler
 *
 * @param ipending Bitfield of interrupts
 */
void _enter_irq(u32_t ipending)
{
	int index;

#ifdef CONFIG_EXECUTION_BENCHMARKING
	extern void read_timer_start_of_isr(void);
	read_timer_start_of_isr();
#endif

	_kernel.nested++;

#ifdef CONFIG_IRQ_OFFLOAD
	z_irq_do_offload();
#endif

	while (ipending) {
		struct _isr_table_entry *ite;

		sys_trace_isr_enter();

		index = find_lsb_set(ipending) - 1;
		ipending &= ~BIT(index);

		ite = &_sw_isr_table[index];

#ifdef CONFIG_EXECUTION_BENCHMARKING
		extern void read_timer_end_of_isr(void);
		read_timer_end_of_isr();
#endif
		ite->isr(ite->arg);
		sys_trace_isr_exit();
	}

	_kernel.nested--;
#ifdef CONFIG_STACK_SENTINEL
	z_check_stack_sentinel();
#endif
}

#ifdef CONFIG_DYNAMIC_INTERRUPTS
int z_arch_irq_connect_dynamic(unsigned int irq, unsigned int priority,
			      void (*routine)(void *parameter), void *parameter,
			      u32_t flags)
{
	ARG_UNUSED(flags);
	ARG_UNUSED(priority);

	z_isr_install(irq, routine, parameter);
	return irq;
}
#endif /* CONFIG_DYNAMIC_INTERRUPTS */
