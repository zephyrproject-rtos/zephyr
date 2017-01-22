/*
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <nanokernel.h>
#include <irq_offload.h>
#include <arch/xtensa/arch.h>
#include <xtensa_api.h>

/*
 * Xtensa core should support software interrupt in order to allow using
 * irq_offload feature
 */
#ifndef CONFIG_IRQ_OFFLOAD_INTNUM
#error "Please add entry for IRQ_OFFLOAD_INTNUM option to your arch/xtensa/soc/${XTENSA_CORE}/Kconfig file in order to use IRQ offload on this core."
#endif

static irq_offload_routine_t offload_routine;
static void *offload_param;

/* Called by ISR dispatcher */
void _irq_do_offload(void *unused)
{
	ARG_UNUSED(unused);
	offload_routine(offload_param);
}

void irq_offload(irq_offload_routine_t routine, void *parameter)
{
	IRQ_CONNECT(CONFIG_IRQ_OFFLOAD_INTNUM, XCHAL_EXCM_LEVEL,
		_irq_do_offload, NULL, 0);
	_arch_irq_disable(CONFIG_IRQ_OFFLOAD_INTNUM);
	offload_routine = routine;
	offload_param = parameter;
	_xt_set_intset(1 << CONFIG_IRQ_OFFLOAD_INTNUM);
	/*
	 * Enable the software interrupt, in case it is disabled, so that IRQ
	 * offload is serviced.
	 */
	_arch_irq_enable(CONFIG_IRQ_OFFLOAD_INTNUM);
}
