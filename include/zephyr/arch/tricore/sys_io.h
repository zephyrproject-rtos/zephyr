/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Meta
 * SPDX-License-Identifier: Apache-2.0
 */

/* Memory mapped registers I/O functions for TriCore */

#ifndef ZEPHYR_INCLUDE_ARCH_TRICORE_SYS_IO_H_
#define ZEPHYR_INCLUDE_ARCH_TRICORE_SYS_IO_H_

#ifndef _ASMLANGUAGE

#include <zephyr/toolchain.h>
#include <zephyr/types.h>
#include <zephyr/sys/sys_io.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @cond INTERNAL_HIDDEN
 *
 * Each accessor performs one load or store of its declared width. Accesses to peripheral space
 * must be naturally aligned.
 */

static ALWAYS_INLINE uint8_t sys_read8(mem_addr_t addr)
{
	uint8_t val;

	__asm__ volatile("ld.bu %0, %1" : "=d"(val) : "m"(*(uint8_t *)addr));
	return val;
}

static ALWAYS_INLINE void sys_write8(uint8_t data, mem_addr_t addr)
{
	__asm__ volatile("st.b %0, %1" : "=m"(*(uint8_t *)addr) : "d"(data));
}

static ALWAYS_INLINE uint16_t sys_read16(mem_addr_t addr)
{
	uint16_t val;

	__asm__ volatile("ld.hu %0, %1" : "=d"(val) : "m"(*(uint16_t *)addr));
	return val;
}

static ALWAYS_INLINE void sys_write16(uint16_t data, mem_addr_t addr)
{
	__asm__ volatile("st.h %0, %1" : "=m"(*(uint16_t *)addr) : "d"(data));
}

static ALWAYS_INLINE uint32_t sys_read32(mem_addr_t addr)
{
	uint32_t val;

	__asm__ volatile("ld.w %0, %1" : "=d"(val) : "m"(*(uint32_t *)addr));
	return val;
}

static ALWAYS_INLINE void sys_write32(uint32_t data, mem_addr_t addr)
{
	__asm__ volatile("st.w %0, %1" : "=m"(*(uint32_t *)addr) : "d"(data));
}

static ALWAYS_INLINE uint64_t sys_read64(mem_addr_t addr)
{
	uint64_t val;

	__asm__ volatile("ld.d %A0, %1" : "=d"(val) : "m"(*(uint64_t *)addr));
	return val;
}

static ALWAYS_INLINE void sys_write64(uint64_t data, mem_addr_t addr)
{
	__asm__ volatile("st.d %0, %A1" : "=m"(*(uint64_t *)addr) : "d"(data));
}

/**
 * @endcond
 */

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_TRICORE_SYS_IO_H_ */
