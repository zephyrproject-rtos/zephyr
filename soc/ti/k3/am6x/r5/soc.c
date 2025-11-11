/* Copyright (C) 2023 BeagleBoard.org Foundation
 * Copyright (C) 2023 S Prashanth
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/fatal.h>

#include "soc.h"
#include <common/ctrl_partitions.h>

unsigned int z_soc_irq_get_active(void)
{
	return z_vim_irq_get_active();
}

void z_soc_irq_eoi(unsigned int irq)
{
	z_vim_irq_eoi(irq);
}

void z_soc_irq_init(void)
{
	z_vim_irq_init();
}

void z_soc_irq_priority_set(unsigned int irq, unsigned int prio, uint32_t flags)
{
	/* Configure interrupt type and priority */
	z_vim_irq_priority_set(irq, prio, flags);
}

void z_soc_irq_enable(unsigned int irq)
{
	/* Enable interrupt */
	z_vim_irq_enable(irq);
}

void z_soc_irq_disable(unsigned int irq)
{
	/* Disable interrupt */
	z_vim_irq_disable(irq);
}

int z_soc_irq_is_enabled(unsigned int irq)
{
	/* Check if interrupt is enabled */
	return z_vim_irq_is_enabled(irq);
}

void soc_early_init_hook(void)
{
	k3_unlock_all_ctrl_partitions();
}
