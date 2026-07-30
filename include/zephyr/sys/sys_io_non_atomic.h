/*
 * Copyright (c) 2026 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Non-atomic ordered 64-bit memory-mapped I/O accessors.
 * @ingroup sys_io_apis
 */

#ifndef ZEPHYR_INCLUDE_SYS_SYS_IO_NON_ATOMIC_H_
#define ZEPHYR_INCLUDE_SYS_SYS_IO_NON_ATOMIC_H_

#include <zephyr/sys/sys_io.h>

#ifdef __DOXYGEN__

/**
 * @brief Read a 64-bit value using low-word then high-word accesses
 *
 * This function performs two 32-bit reads. It first reads the low word from @p addr, then reads the
 * high word from @p addr + 4.
 *
 * @param addr the address of the low word of the memory mapped register
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
 * This function performs two 32-bit reads. It first reads the high word from @p addr + 4, then
 * reads the low word from @p addr.
 *
 * @param addr the address of the low word of the memory mapped register
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
 * This function performs two 32-bit writes. It first writes the low word to @p addr, then writes
 * the high word to @p addr + 4.
 *
 * @param data the 64-bit value to write
 * @param addr the address of the low word of the memory mapped register
 *
 * @warning The two writes are not atomic. The caller must ensure that the addressed register
 *          supports non-atomic writes in this order.
 */
static inline void sys_write64_lo_hi(uint64_t data, mm_reg_t addr);

/**
 * @brief Write a 64-bit value using high-word then low-word accesses
 *
 * This function performs two 32-bit writes. It first writes the high word to @p addr + 4, then
 * writes the low word to @p addr.
 *
 * @param data the 64-bit value to write
 * @param addr the address of the low word of the memory mapped register
 *
 * @warning The two writes are not atomic. The caller must ensure that the addressed register
 *          supports non-atomic writes in this order.
 */
static inline void sys_write64_hi_lo(uint64_t data, mm_reg_t addr);

#else

#include <zephyr/arch/cpu.h>

static ALWAYS_INLINE uint64_t sys_read64_lo_hi(mem_addr_t addr)
{
	uint32_t low = sys_read32(addr);
	uint32_t high = sys_read32(addr + sizeof(uint32_t));

	return ((uint64_t)high << 32) | low;
}

static ALWAYS_INLINE uint64_t sys_read64_hi_lo(mem_addr_t addr)
{
	uint32_t high = sys_read32(addr + sizeof(uint32_t));
	uint32_t low = sys_read32(addr);

	return ((uint64_t)high << 32) | low;
}

static ALWAYS_INLINE void sys_write64_lo_hi(uint64_t data, mem_addr_t addr)
{
	sys_write32((uint32_t)data, addr);
	sys_write32((uint32_t)(data >> 32), addr + sizeof(uint32_t));
}

static ALWAYS_INLINE void sys_write64_hi_lo(uint64_t data, mem_addr_t addr)
{
	sys_write32((uint32_t)(data >> 32), addr + sizeof(uint32_t));
	sys_write32((uint32_t)data, addr);
}

#endif /* __DOXYGEN__ */

#endif /* ZEPHYR_INCLUDE_SYS_SYS_IO_NON_ATOMIC_H_ */
