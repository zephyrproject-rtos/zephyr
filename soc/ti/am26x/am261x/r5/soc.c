/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/fatal.h>
#include <zephyr/cache.h>

#include "soc.h"

#include <common/ctrl_partitions.h>
#define SOC_EARLY_BOOT_ATTR __attribute__((section(".soc_early_boot")))

#ifdef CONFIG_ARM_MPU
extern void z_arm_mpu_init(void);
extern void z_arm_configure_static_mpu_regions(void);
#endif /* CONFIG_ARM_MPU */

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

SOC_EARLY_BOOT_ATTR
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

SOC_EARLY_BOOT_ATTR
void soc_prep_hook(void)
{
#ifdef CONFIG_ARM_MPU
	z_arm_mpu_init();
	z_arm_configure_static_mpu_regions();
#endif /* CONFIG_ARM_MPU */
	cache_init();
}

void soc_early_init_hook(void)
{
	am26x_unlock_all_ctrl_partitions();
}
