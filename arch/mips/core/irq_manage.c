/*
 * Copyright (c) 2017 Imagination Technologies Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <kernel_structs.h>
#include <toolchain.h>
#include <arch/cpu.h>
#include <irq.h>
#include <sw_isr_table.h>
#include <logging/kernel_event_logger.h>
#include <ksched.h>

/* used in isr.S */
void *irq_args[8];

void _irq_spurious(void *unused)
{
	u32_t cause;

	ARG_UNUSED(unused);

	cause = mips32_getcr() & SOC_CAUSE_EXP_MASK;

	printk("Spurious interrupt detected! IRQ: %d\n", (int)cause);
	_NanoFatalErrorHandler(_NANO_ERR_SPURIOUS_INT, &_default_esf);
}

void _enter_irq(int ipending)
{
	int index;

	_kernel.nested++;

#ifdef CONFIG_IRQ_OFFLOAD
	_irq_do_offload();
#endif

	while (ipending) {
		struct _isr_table_entry *ite;

#ifdef CONFIG_KERNEL_EVENT_LOGGER_INTERRUPT
		_sys_k_event_logger_interrupt();
#endif

		index = find_lsb_set(ipending) - 1;
		ipending &= ~(1 << index);

		ite = &_sw_isr_table[index];
		ite->isr(ite->arg);
	}

	_kernel.nested--;
#ifdef CONFIG_STACK_SENTINEL
	_check_stack_sentinel();
#endif
}
