/*
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <irq_offload.h>
#include <arch/xtensa/arch.h>
#include <arch/xtensa/xtensa_api.h>

/*
 * Xtensa core should support software interrupt in order to allow using
 * irq_offload feature
 */

static irq_offload_routine_t offload_routine;
static const void *offload_param;

/* Called by ISR dispatcher */
void z_irq_do_offload(const void *unused)
{
	ARG_UNUSED(unused);
	offload_routine(offload_param);
}

void arch_irq_offload(irq_offload_routine_t routine, const void *parameter)
{
	IRQ_CONNECT(CONFIG_IRQ_OFFLOAD_INTNUM, XCHAL_EXCM_LEVEL,
		z_irq_do_offload, NULL, 0);
	arch_irq_disable(CONFIG_IRQ_OFFLOAD_INTNUM);
	offload_routine = routine;
	offload_param = parameter;
	z_xt_set_intset(BIT(CONFIG_IRQ_OFFLOAD_INTNUM));
	/*
	 * Enable the software interrupt, in case it is disabled, so that IRQ
	 * offload is serviced.
	 */
	arch_irq_enable(CONFIG_IRQ_OFFLOAD_INTNUM);
}
