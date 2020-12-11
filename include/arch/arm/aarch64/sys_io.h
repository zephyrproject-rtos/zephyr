/*
 * Copyright (c) 2015, Wind River Systems, Inc.
 * Copyright (c) 2017, Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* "Arch" bit manipulation functions in non-arch-specific C code (uses some
 * gcc builtins)
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_AARCH64_SYS_IO_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_AARCH64_SYS_IO_H_

#ifndef _ASMLANGUAGE

#include <zephyr/types.h>
#include <sys/sys_io.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Memory mapped registers I/O functions */

static ALWAYS_INLINE uint8_t sys_read8(mem_addr_t addr)
{
	uint8_t val = *(volatile uint8_t *)addr;

	__DMB();
	return val;
}

static ALWAYS_INLINE void sys_write8(uint8_t data, mem_addr_t addr)
{
	__DMB();
	*(volatile uint8_t *)addr = data;
}

static ALWAYS_INLINE uint16_t sys_read16(mem_addr_t addr)
{
	uint16_t val = *(volatile uint16_t *)addr;

	__DMB();
	return val;
}

static ALWAYS_INLINE void sys_write16(uint16_t data, mem_addr_t addr)
{
	__DMB();
	*(volatile uint16_t *)addr = data;
}

static ALWAYS_INLINE uint32_t sys_read32(mem_addr_t addr)
{
	uint32_t val = *(volatile uint32_t *)addr;

	__DMB();
	return val;
}

static ALWAYS_INLINE void sys_write32(uint32_t data, mem_addr_t addr)
{
	__DMB();
	*(volatile uint32_t *)addr = data;
}

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_AARCH64_SYS_IO_H_ */
