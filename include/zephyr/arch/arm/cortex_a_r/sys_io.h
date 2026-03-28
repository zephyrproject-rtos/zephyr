/*
 * Copyright (c) 2015, Wind River Systems, Inc.
 * Copyright (c) 2017, Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* "Arch" bit manipulation functions in non-arch-specific C code (uses some
 * gcc builtins)
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_A_R_SYS_IO_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_A_R_SYS_IO_H_

#ifndef _ASMLANGUAGE

#include <zephyr/types.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/barrier.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Memory mapped registers I/O functions */

static ALWAYS_INLINE uint8_t sys_read8(mem_addr_t addr)
{
	uint8_t val;

	__asm__ volatile("ldrb %0, [%1]" : "=r" (val) : "r" (addr));

	barrier_dmem_fence_full();
	return val;
}

static ALWAYS_INLINE void sys_write8(uint8_t data, mem_addr_t addr)
{
	barrier_dmem_fence_full();
	__asm__ volatile("strb %0, [%1]" : : "r" (data), "r" (addr));
}

static ALWAYS_INLINE uint16_t sys_read16(mem_addr_t addr)
{
	uint16_t val;

	__asm__ volatile("ldrh %0, [%1]" : "=r" (val) : "r" (addr));

	barrier_dmem_fence_full();
	return val;
}

static ALWAYS_INLINE void sys_write16(uint16_t data, mem_addr_t addr)
{
	barrier_dmem_fence_full();
	__asm__ volatile("strh %0, [%1]" : : "r" (data), "r" (addr));
}

static ALWAYS_INLINE uint32_t sys_read32(mem_addr_t addr)
{
	uint32_t val;

	__asm__ volatile("ldr %0, [%1]" : "=r" (val) : "r" (addr));

	barrier_dmem_fence_full();
	return val;
}

static ALWAYS_INLINE void sys_write32(uint32_t data, mem_addr_t addr)
{
	barrier_dmem_fence_full();
	__asm__ volatile("str %0, [%1]" : : "r" (data), "r" (addr));
}

static ALWAYS_INLINE uint64_t sys_read64(mem_addr_t addr)
{
	uint64_t val;

	__asm__ volatile("ldrd %Q0, %R0, [%1]" : "=r" (val) : "r" (addr));

	barrier_dmem_fence_full();
	return val;
}

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_A_R_SYS_IO_H_ */
