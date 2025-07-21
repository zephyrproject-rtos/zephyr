/*
 * Copyright (c) 2025 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/fatal.h>
#include <zephyr/cache.h>

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
	z_vim_irq_priority_set(irq, prio, flags);
}

void z_soc_irq_enable(unsigned int irq)
{
	z_vim_irq_enable(irq);
}

void z_soc_irq_disable(unsigned int irq)
{
	z_vim_irq_disable(irq);
}

int z_soc_irq_is_enabled(unsigned int irq)
{
	return z_vim_irq_is_enabled(irq);
}

void cache_init(void)
{
#ifdef CONFIG_ICACHE
	arch_icache_disable();
	arch_icache_invd_all();
	arch_icache_enable();
#endif /* CONFIG_ICACHE */
#ifdef CONFIG_DCACHE
	arch_dcache_disable();
	arch_dcache_invd_all();
	arch_dcache_enable();
#endif /* CONFIG_DCACHE */
}

void soc_early_init_hook(void)
{
	cache_init();
	am26x_unlock_all_ctrl_partitions();
}
