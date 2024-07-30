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


#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/irq.h>
#include <zephyr/sw_isr_table.h>
#include <ksched.h>
#include <kswap.h>
#include <zephyr/tracing/tracing.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

FUNC_NORETURN void z_irq_spurious(const void *unused)
{
	ARG_UNUSED(unused);
	LOG_ERR("Spurious interrupt detected! ipending: %x",
		z_nios2_creg_read(NIOS2_CR_IPENDING));
	z_nios2_fatal_error(K_ERR_SPURIOUS_IRQ, NULL);
}


void arch_irq_enable(unsigned int irq)
{
	uint32_t ienable;
	unsigned int key;

	key = irq_lock();

	ienable = z_nios2_creg_read(NIOS2_CR_IENABLE);
	ienable |= BIT(irq);
	z_nios2_creg_write(NIOS2_CR_IENABLE, ienable);

	irq_unlock(key);
};



void arch_irq_disable(unsigned int irq)
{
	uint32_t ienable;
	unsigned int key;

	key = irq_lock();

	ienable = z_nios2_creg_read(NIOS2_CR_IENABLE);
	ienable &= ~BIT(irq);
	z_nios2_creg_write(NIOS2_CR_IENABLE, ienable);

	irq_unlock(key);
};

int arch_irq_is_enabled(unsigned int irq)
{
	uint32_t ienable;

	ienable = z_nios2_creg_read(NIOS2_CR_IENABLE);
	return ienable & BIT(irq);
}

/**
 * @brief Interrupt demux function
 *
 * Given a bitfield of pending interrupts, execute the appropriate handler
 *
 * @param ipending Bitfield of interrupts
 */
void _enter_irq(uint32_t ipending)
{
	int index;

	_kernel.cpus[0].nested++;

#ifdef CONFIG_IRQ_OFFLOAD
	z_irq_do_offload();
#endif

	while (ipending) {
		struct _isr_table_entry *ite;

#ifdef CONFIG_TRACING_ISR
		sys_trace_isr_enter();
#endif

		index = find_lsb_set(ipending) - 1;
		ipending &= ~BIT(index);

		ite = &_sw_isr_table[index];

		ite->isr(ite->arg);
#ifdef CONFIG_TRACING_ISR
		sys_trace_isr_exit();
#endif
	}

	_kernel.cpus[0].nested--;
#ifdef CONFIG_STACK_SENTINEL
	z_check_stack_sentinel();
#endif
}

#ifdef CONFIG_DYNAMIC_INTERRUPTS
int arch_irq_connect_dynamic(unsigned int irq, unsigned int priority,
			     void (*routine)(const void *parameter),
			     const void *parameter, uint32_t flags)
{
	ARG_UNUSED(flags);
	ARG_UNUSED(priority);

	z_isr_install(irq, routine, parameter);
	return irq;
}
#endif /* CONFIG_DYNAMIC_INTERRUPTS */
