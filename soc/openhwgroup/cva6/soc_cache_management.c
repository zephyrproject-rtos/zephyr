/*
 * Non-standard CVA6 cache management operations.
 *
 * Copyright(c) 2024, CISPA Helmholtz Center for Information Security
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stddef.h>
#include <zephyr/arch/cache.h>
#include <zephyr/arch/riscv/csr.h>
#include <zephyr/kernel.h>

#include "cva6.h"

void __weak arch_dcache_enable(void)
{
	csr_write(SOC_CVA6_CUSTOM_CSR_DCACHE, SOC_CVA6_CUSTOM_CSR_DCACHE_ENABLE);
}

void __weak arch_dcache_disable(void)
{
	csr_write(SOC_CVA6_CUSTOM_CSR_DCACHE, SOC_CVA6_CUSTOM_CSR_DCACHE_DISABLE);
}

int __weak arch_dcache_flush_all(void)
{
	arch_dcache_disable();
	arch_dcache_enable();

	return 0;
}

int __weak arch_dcache_invd_all(void)
{
	return arch_dcache_flush_all();
}

int __weak arch_dcache_flush_and_invd_all(void)
{
	return arch_dcache_flush_all();
}

/* FIXME currently not supported by all CVA6 - overwrite at board or SoC level */
int __weak arch_dcache_flush_range(void *addr, size_t size)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(size);

	return arch_dcache_flush_all();
}

int __weak arch_dcache_invd_range(void *addr, size_t size)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(size);

	return arch_dcache_flush_all();
}

int __weak arch_dcache_flush_and_invd_range(void *addr, size_t size)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(size);

	return arch_dcache_flush_all();
}

void __weak arch_icache_enable(void)
{
	csr_write(SOC_CVA6_CUSTOM_CSR_ICACHE, SOC_CVA6_CUSTOM_CSR_ICACHE_ENABLE);
}

void __weak arch_icache_disable(void)
{
	csr_write(SOC_CVA6_CUSTOM_CSR_ICACHE, SOC_CVA6_CUSTOM_CSR_ICACHE_DISABLE);
}

int __weak arch_icache_flush_all(void)
{
	arch_icache_disable();
	arch_icache_enable();

	return 0;
}

int __weak arch_icache_invd_all(void)
{
	return arch_icache_flush_all();
}

int __weak arch_icache_flush_and_invd_all(void)
{
	return arch_icache_flush_all();
}

/* FIXME currently not supported by all CVA6 - overwrite at board or SoC level */
int __weak arch_icache_flush_range(void *addr, size_t size)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(size);

	return arch_icache_flush_all();
}

int __weak arch_icache_invd_range(void *addr, size_t size)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(size);

	return arch_icache_flush_all();
}

int __weak arch_icache_flush_and_invd_range(void *addr, size_t size)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(size);

	return arch_icache_flush_all();
}

/* FIXME there is no common implementation for RISC-V, so we provide a SoC-level definition */
/* this prevents a linker error when the function is not defined */
void __weak arch_cache_init(void)
{
}
