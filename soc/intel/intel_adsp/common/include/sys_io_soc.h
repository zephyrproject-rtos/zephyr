/*
 * Copyright (c) 2015, Wind River Systems, Inc.
 * Copyright (c) 2017, Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_INTEL_ADSP_INCLUDE_SYS_IO_SOC_H_
#define ZEPHYR_SOC_INTEL_ADSP_INCLUDE_SYS_IO_SOC_H_

#ifndef _ASMLANGUAGE

#include <zephyr/toolchain.h>
#include <zephyr/types.h>
#include <zephyr/sys/sys_io.h>

#if defined(CONFIG_INTEL_ADSP_HIFI_64BIT_MMIO)
#include <xtensa/tie/xt_hifi2.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @cond INTERNAL_HIDDEN
 * Memory mapped registers I/O functions in non-arch-specific C code.
 */

static ALWAYS_INLINE uint8_t sys_read8(mem_addr_t addr)
{
	return *(volatile uint8_t *)addr;
}

static ALWAYS_INLINE void sys_write8(uint8_t data, mem_addr_t addr)
{
	*(volatile uint8_t *)addr = data;
}

static ALWAYS_INLINE uint16_t sys_read16(mem_addr_t addr)
{
	return *(volatile uint16_t *)addr;
}

static ALWAYS_INLINE void sys_write16(uint16_t data, mem_addr_t addr)
{
	*(volatile uint16_t *)addr = data;
}

static ALWAYS_INLINE uint32_t sys_read32(mem_addr_t addr)
{
	return *(volatile uint32_t *)addr;
}

static ALWAYS_INLINE void sys_write32(uint32_t data, mem_addr_t addr)
{
	*(volatile uint32_t *)addr = data;
}

static ALWAYS_INLINE uint64_t sys_read64(mem_addr_t addr)
{
#if defined(CONFIG_INTEL_ADSP_HIFI_64BIT_MMIO)
	return (uint64_t)AE_L64_I((const ae_int64 *)addr, 0);
#else
	return *(volatile uint64_t *)addr;
#endif
}

static ALWAYS_INLINE void sys_write64(uint64_t data, mem_addr_t addr)
{
#if defined(CONFIG_INTEL_ADSP_HIFI_64BIT_MMIO)
	AE_S64_I((ae_int64)data, (ae_int64 *)addr, 0);
#else
	*(volatile uint64_t *)addr = data;
#endif
}

/**
 * @endcond
 */

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_SOC_INTEL_ADSP_INCLUDE_SYS_IO_SOC_H_ */
