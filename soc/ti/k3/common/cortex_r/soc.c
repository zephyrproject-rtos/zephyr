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
#include <zephyr/devicetree.h>

#if defined(CONFIG_MULTI_LEVEL_INTERRUPTS) && DT_HAS_COMPAT_STATUS_OKAY(ti_sci_intr_out)
#include <zephyr/drivers/interrupt_controller/intc_ti_sci_intr.h>
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
#if defined(CONFIG_MULTI_LEVEL_INTERRUPTS) && DT_HAS_COMPAT_STATUS_OKAY(ti_sci_intr_out)
	ti_sci_intr_irq_priority_set(irq, prio, flags);
#else
	z_vim_irq_priority_set(irq, prio, flags);
#endif
}

void z_soc_irq_enable(unsigned int irq)
{
#if defined(CONFIG_MULTI_LEVEL_INTERRUPTS) && DT_HAS_COMPAT_STATUS_OKAY(ti_sci_intr_out)
	ti_sci_intr_irq_enable(irq);
#else
	z_vim_irq_enable(irq);
#endif
}

void z_soc_irq_disable(unsigned int irq)
{
#if defined(CONFIG_MULTI_LEVEL_INTERRUPTS) && DT_HAS_COMPAT_STATUS_OKAY(ti_sci_intr_out)
	ti_sci_intr_irq_disable(irq);
#else
	z_vim_irq_disable(irq);
#endif
}

int z_soc_irq_is_enabled(unsigned int irq)
{
#if defined(CONFIG_MULTI_LEVEL_INTERRUPTS) && DT_HAS_COMPAT_STATUS_OKAY(ti_sci_intr_out)
	return ti_sci_intr_irq_is_enabled(irq);
#else
	return z_vim_irq_is_enabled(irq);
#endif
}

void soc_early_init_hook(void)
{
	sys_cache_data_enable();
	sys_cache_instr_enable();

	k3_unlock_all_ctrl_partitions();
}
