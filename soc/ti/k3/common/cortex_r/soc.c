/* Copyright (C) 2023 BeagleBoard.org Foundation
 * Copyright (C) 2023 S Prashanth
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/fatal.h>

#include "soc.h"
#include "ctrl_partitions.h"
#include <zephyr/cache.h>

#if defined(CONFIG_TI_SCI_INTR)
#include <zephyr/drivers/interrupt_controller/intc_ti_sci_intr.h>
#include <zephyr/irq_multilevel.h>
#endif

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
#if defined(CONFIG_TI_SCI_INTR)
	if (irq_get_level(irq) > 1U) {
		ti_sci_intr_irq_priority_set(irq, prio, flags);
		return;
	}
#endif
	z_vim_irq_priority_set(irq, prio, flags);
}

void z_soc_irq_enable(unsigned int irq)
{
#if defined(CONFIG_TI_SCI_INTR)
	if (irq_get_level(irq) > 1U) {
		ti_sci_intr_irq_enable(irq);
		return;
	}
#endif
	z_vim_irq_enable(irq);
}

void z_soc_irq_disable(unsigned int irq)
{
#if defined(CONFIG_TI_SCI_INTR)
	if (irq_get_level(irq) > 1U) {
		ti_sci_intr_irq_disable(irq);
		return;
	}
#endif
	z_vim_irq_disable(irq);
}

int z_soc_irq_is_enabled(unsigned int irq)
{
#if defined(CONFIG_TI_SCI_INTR)
	if (irq_get_level(irq) > 1U) {
		return ti_sci_intr_irq_is_enabled(irq);
	}
#endif
	return z_vim_irq_is_enabled(irq);
}

void soc_early_init_hook(void)
{
	sys_cache_data_enable();
	sys_cache_instr_enable();

	k3_unlock_all_ctrl_partitions();
}
