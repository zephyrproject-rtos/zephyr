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

void __weak arch_icache_enable(void)
{
	csr_write(SOC_CVA6_CUSTOM_CSR_ICACHE, SOC_CVA6_CUSTOM_CSR_ICACHE_ENABLE);
}

void __weak arch_icache_disable(void)
{
	csr_write(SOC_CVA6_CUSTOM_CSR_ICACHE, SOC_CVA6_CUSTOM_CSR_ICACHE_DISABLE);
}

/* FIXME there is no common implementation for RISC-V, so we provide a SoC-level definition */
/* this prevents a linker error when the function is not defined */
void __weak arch_cache_init(void)
{
}
