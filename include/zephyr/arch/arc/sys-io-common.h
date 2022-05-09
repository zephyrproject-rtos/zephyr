/*
 * Copyright (c) 2021 Synopsys.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARC_SYS_IO_COMMON_H_
#define ZEPHYR_INCLUDE_ARCH_ARC_SYS_IO_COMMON_H_

#ifndef _ASMLANGUAGE

#include <zephyr/toolchain.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/arch/arc/v2/aux_regs.h>

#include <zephyr/types.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

static ALWAYS_INLINE uint8_t sys_read8(mem_addr_t addr)
{
	uint8_t value;

	compiler_barrier();
	value = *(volatile uint8_t *)addr;
	compiler_barrier();

	return value;
}

static ALWAYS_INLINE void sys_write8(uint8_t data, mem_addr_t addr)
{
	compiler_barrier();
	*(volatile uint8_t *)addr = data;
	compiler_barrier();
}

static ALWAYS_INLINE uint16_t sys_read16(mem_addr_t addr)
{
	uint16_t value;

	compiler_barrier();
	value = *(volatile uint16_t *)addr;
	compiler_barrier();

	return value;
}

static ALWAYS_INLINE void sys_write16(uint16_t data, mem_addr_t addr)
{
	compiler_barrier();
	*(volatile uint16_t *)addr = data;
	compiler_barrier();
}

static ALWAYS_INLINE uint32_t sys_read32(mem_addr_t addr)
{
	uint32_t value;

	compiler_barrier();
	value = *(volatile uint32_t *)addr;
	compiler_barrier();

	return value;
}

static ALWAYS_INLINE void sys_write32(uint32_t data, mem_addr_t addr)
{
	compiler_barrier();
	*(volatile uint32_t *)addr = data;
	compiler_barrier();
}

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_ARC_SYS_IO_COMMON_H_ */
