/*
 * Non-standard CVA6 cache management operations.
 *
 * Copyright(c) 2024, CISPA Helmholtz Center for Information Security
 * SPDX - License - Identifier : Apache-2.0
 */
#include <stddef.h>
#include <zephyr/arch/cache.h>

void arch_dcache_enable(void)
{
	__asm__ volatile("csrwi 0x7C1, 0x01");
}

void arch_dcache_disable(void)
{
	__asm__ volatile("csrwi 0x7C1, 0x00");
}
