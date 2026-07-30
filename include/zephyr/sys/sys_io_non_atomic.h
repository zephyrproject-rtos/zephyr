/*
 * Copyright (c) 2026 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Non-atomic ordered native-endian 64-bit memory-mapped I/O accessors.
 * @ingroup sys_io_apis
 *
 * The low and high words are placed according to the target endianness, consistent with the
 * native-endian :c:func:`sys_read32` and :c:func:`sys_write32` accessors.
 */

#ifndef ZEPHYR_INCLUDE_SYS_SYS_IO_NON_ATOMIC_H_
#define ZEPHYR_INCLUDE_SYS_SYS_IO_NON_ATOMIC_H_

#include <zephyr/sys/sys_io.h>

#ifdef __DOXYGEN__

/**
 * @brief Read a 64-bit value using low-word then high-word accesses
 *
 * This function performs two 32-bit reads. It first reads the low word, then reads the high word.
 * The low word is at @p addr on little-endian targets and at @p addr + 4 on big-endian targets.
 *
 * @param addr the base address of the memory mapped register
 *
 * @return the 64-bit value assembled from the two words
 *
 * @warning The two reads are not atomic. The caller must ensure that the addressed register
 *          supports non-atomic reads in this order.
 */
static inline uint64_t sys_read64_lo_hi(mm_reg_t addr);

/**
 * @brief Read a 64-bit value using high-word then low-word accesses
 *
 * This function performs two 32-bit reads. It first reads the high word, then reads the low word.
 * The high word is at @p addr + 4 on little-endian targets and at @p addr on big-endian targets.
 *
 * @param addr the base address of the memory mapped register
 *
 * @return the 64-bit value assembled from the two words
 *
 * @warning The two reads are not atomic. The caller must ensure that the addressed register
 *          supports non-atomic reads in this order.
 */
static inline uint64_t sys_read64_hi_lo(mm_reg_t addr);

/**
 * @brief Write a 64-bit value using low-word then high-word accesses
 *
 * This function performs two 32-bit writes. It first writes the low word, then writes the high
 * word. The low word is at @p addr on little-endian targets and at @p addr + 4 on big-endian
 * targets.
 *
 * @param data the 64-bit value to write
 * @param addr the base address of the memory mapped register
 *
 * @warning The two writes are not atomic. The caller must ensure that the addressed register
 *          supports non-atomic writes in this order.
 */
static inline void sys_write64_lo_hi(uint64_t data, mm_reg_t addr);

/**
 * @brief Write a 64-bit value using high-word then low-word accesses
 *
 * This function performs two 32-bit writes. It first writes the high word, then writes the low
 * word. The high word is at @p addr + 4 on little-endian targets and at @p addr on big-endian
 * targets.
 *
 * @param data the 64-bit value to write
 * @param addr the base address of the memory mapped register
 *
 * @warning The two writes are not atomic. The caller must ensure that the addressed register
 *          supports non-atomic writes in this order.
 */
static inline void sys_write64_hi_lo(uint64_t data, mm_reg_t addr);

#else

#include <zephyr/arch/cpu.h>

static ALWAYS_INLINE mem_addr_t z_sys_io64_lo_addr(mem_addr_t addr)
{
#ifdef CONFIG_BIG_ENDIAN
	return addr + sizeof(uint32_t);
#else
	return addr;
#endif
}

static ALWAYS_INLINE mem_addr_t z_sys_io64_hi_addr(mem_addr_t addr)
{
#ifdef CONFIG_BIG_ENDIAN
	return addr;
#else
	return addr + sizeof(uint32_t);
#endif
}

static ALWAYS_INLINE uint64_t sys_read64_lo_hi(mem_addr_t addr)
{
	uint32_t lo = sys_read32(z_sys_io64_lo_addr(addr));
	uint32_t hi = sys_read32(z_sys_io64_hi_addr(addr));

	return ((uint64_t)hi << 32) | lo;
}

static ALWAYS_INLINE uint64_t sys_read64_hi_lo(mem_addr_t addr)
{
	uint32_t hi = sys_read32(z_sys_io64_hi_addr(addr));
	uint32_t lo = sys_read32(z_sys_io64_lo_addr(addr));

	return ((uint64_t)hi << 32) | lo;
}

static ALWAYS_INLINE void sys_write64_lo_hi(uint64_t data, mem_addr_t addr)
{
	sys_write32((uint32_t)data, z_sys_io64_lo_addr(addr));
	sys_write32((uint32_t)(data >> 32), z_sys_io64_hi_addr(addr));
}

static ALWAYS_INLINE void sys_write64_hi_lo(uint64_t data, mem_addr_t addr)
{
	sys_write32((uint32_t)(data >> 32), z_sys_io64_hi_addr(addr));
	sys_write32((uint32_t)data, z_sys_io64_lo_addr(addr));
}

#endif /* __DOXYGEN__ */

#endif /* ZEPHYR_INCLUDE_SYS_SYS_IO_NON_ATOMIC_H_ */
