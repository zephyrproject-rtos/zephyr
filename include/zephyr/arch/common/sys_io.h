/*
 * Copyright (c) 2015, Wind River Systems, Inc.
 * Copyright (c) 2017, Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Memory mapped registers I/O functions in non-arch-specific C code */

#ifndef ZEPHYR_INCLUDE_ARCH_COMMON_SYS_IO_H_
#define ZEPHYR_INCLUDE_ARCH_COMMON_SYS_IO_H_

#ifndef _ASMLANGUAGE

#include <zephyr/toolchain.h>
#include <zephyr/types.h>
#include <zephyr/sys/sys_io.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CONFIG_PLATFORM_HAS_CUSTOM_SYS_IO

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
	return *(volatile uint64_t *)addr;
}

static ALWAYS_INLINE void sys_write64(uint64_t data, mem_addr_t addr)
{
	*(volatile uint64_t *)addr = data;
}


#else

extern uint8_t platform_sysio_read8(mem_addr_t addr);
extern void platform_sysio_write8(uint8_t data, mem_addr_t addr);
extern uint16_t platform_sysio_read16(mem_addr_t addr);
extern void platform_sysio_write16(uint16_t data, mem_addr_t addr);
extern uint32_t platform_sysio_read32(mem_addr_t addr);
extern void platform_sysio_write32(uint32_t data, mem_addr_t addr);
extern uint64_t platform_sysio_read64(mem_addr_t addr);
extern void platform_sysio_write64(uint64_t data, mem_addr_t addr);

static ALWAYS_INLINE uint8_t sys_read8(mem_addr_t addr)
{
	return platform_sysio_read8(addr);
}

static ALWAYS_INLINE void sys_write8(uint8_t data, mem_addr_t addr)
{
	return platform_sysio_write8(data, addr);
}

static ALWAYS_INLINE uint16_t sys_read16(mem_addr_t addr)
{
	return platform_sysio_read16(addr);
}

static ALWAYS_INLINE void sys_write16(uint16_t data, mem_addr_t addr)
{
	return platform_sysio_write16(data, addr);
}

static ALWAYS_INLINE uint32_t sys_read32(mem_addr_t addr)
{
	return platform_sysio_read32(addr);
}

static ALWAYS_INLINE void sys_write32(uint32_t data, mem_addr_t addr)
{
	return platform_sysio_write32(data, addr);
}

static ALWAYS_INLINE uint64_t sys_read64(mem_addr_t addr)
{
	return platform_sysio_read64(addr);
}

static ALWAYS_INLINE void sys_write64(uint64_t data, mem_addr_t addr)
{
	return platform_sysio_write64(data, addr);
}

#endif /* CONFIG_PLATFORM_HAS_CUSTOM_SYS_IO */

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_COMMON_SYS_IO_H_ */
