/*
 * Copyright (c) 2018 ispace, inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief SPARC memory mapped register I/O operations
 */

#ifndef ZEPHYR_INCLUDE_ARCH_SPARC_ASM_INLINE_GCC_H_
#define ZEPHYR_INCLUDE_ARCH_SPARC_ASM_INLINE_GCC_H_

#ifndef _ASMLANGUAGE

#include <toolchain.h>
#include <zephyr/types.h>
#include <sys/sys_io.h>

#ifdef __cplusplus
extern "C" {
#endif

static ALWAYS_INLINE u8_t sys_read8(mm_reg_t addr)
{
	u8_t ret;

	__asm__ volatile("ldub [%1], %0;\n\t"
			 : "=r"(ret)
			 : "r"(addr)
			 : "memory");

	return ret;
}

static ALWAYS_INLINE void sys_write8(u8_t data, mm_reg_t addr)
{
	__asm__ volatile("stb %0, [%1];\n\t"
			 : /* no output */
			 : "r"(data), "r"(addr)
			 : "memory");
}

static ALWAYS_INLINE u16_t sys_read16(mm_reg_t addr)
{
	u16_t ret;

	__asm__ volatile("lduh [%1], %0;\n\t"
			 : "=r"(ret)
			 : "r"(addr)
			 : "memory");

	return ret;
}

static ALWAYS_INLINE void sys_write16(u16_t data, mm_reg_t addr)
{
	__asm__ volatile("sth %0, [%1];\n\t"
			 : /* no output */
			 : "r"(data), "r"(addr)
			 : "memory");
}

static ALWAYS_INLINE u32_t sys_read32(mm_reg_t addr)
{
	u32_t ret;

	__asm__ volatile("ld [%1], %0;\n\t"
			 : "=r"(ret)
			 : "r"(addr)
			 : "memory");

	return ret;
}

static ALWAYS_INLINE void sys_write32(u32_t data, mm_reg_t addr)
{
	__asm__ volatile("st %0, [%1];\n\t"
			 : /* no output */
			 : "r"(data), "r"(addr)
			 : "memory");
}

/* Memory bit manipulation functions */

static ALWAYS_INLINE void sys_set_bit(mem_addr_t addr, unsigned int bit)
{
	sys_write32(sys_read32(addr) | (1 << bit), addr);
}

static ALWAYS_INLINE void sys_clear_bit(mem_addr_t addr, unsigned int bit)
{
	sys_write32(sys_read32(addr) & ~(1 << bit), addr);
}

static ALWAYS_INLINE int sys_test_bit(mem_addr_t addr, unsigned int bit)
{
	return sys_read32(addr) & (1 << bit);
}

static ALWAYS_INLINE
void sys_bitfield_set_bit(mem_addr_t addr, unsigned int bit)
{
	mem_addr_t pos = addr + (bit >> 3);

	sys_write8(sys_read8(pos) | (1 << (bit & 7)), pos);
}

static ALWAYS_INLINE
void sys_bitfield_clear_bit(mem_addr_t addr, unsigned int bit)
{
	mem_addr_t pos = addr + (bit >> 3);

	sys_write8(sys_read8(pos) & ~(1 << (bit & 7)), pos);
}

static ALWAYS_INLINE
int sys_bitfield_test_bit(mem_addr_t addr, unsigned int bit)
{
	mem_addr_t pos = addr + (bit >> 3);

	return sys_read8(pos) & (1 << (bit & 7));
}

static ALWAYS_INLINE
int sys_test_and_set_bit(mem_addr_t addr, unsigned int bit)
{
	int ret;

	ret = sys_test_bit(addr, bit);
	sys_set_bit(addr, bit);

	return ret;
}

static ALWAYS_INLINE
int sys_test_and_clear_bit(mem_addr_t addr, unsigned int bit)
{
	int ret;

	ret = sys_test_bit(addr, bit);
	sys_clear_bit(addr, bit);

	return ret;
}

static ALWAYS_INLINE
int sys_bitfield_test_and_set_bit(mem_addr_t addr, unsigned int bit)
{
	int ret;

	ret = sys_bitfield_test_bit(addr, bit);
	sys_bitfield_set_bit(addr, bit);

	return ret;
}

static ALWAYS_INLINE
int sys_bitfield_test_and_clear_bit(mem_addr_t addr, unsigned int bit)
{
	int ret;

	ret = sys_bitfield_test_bit(addr, bit);
	sys_bitfield_clear_bit(addr, bit);

	return ret;
}

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_SPARC_ASM_INLINE_GCC_H_ */
