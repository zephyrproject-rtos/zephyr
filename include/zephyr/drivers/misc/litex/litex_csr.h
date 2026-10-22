/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2018 - 2019 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief LiteX control and status register access helpers.
 * @ingroup litex_csr
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MISC_LITEX_LITEX_CSR_H_
#define ZEPHYR_INCLUDE_DRIVERS_MISC_LITEX_LITEX_CSR_H_

#include <zephyr/devicetree.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/sys/sys_io.h>

#ifndef _ASMLANGUAGE
/**
 * @defgroup litex_csr LiteX control and status registers
 * @ingroup misc_interfaces
 *
 * Access CSRs using the configured subregister width and ordering.
 * Each subregister occupies a 4-byte address slot.
 *
 * @{
 */

BUILD_ASSERT(CONFIG_LITEX_CSR_DATA_WIDTH == 8 || CONFIG_LITEX_CSR_DATA_WIDTH == 32,
	     "CONFIG_LITEX_CSR_DATA_WIDTH must be 8 or 32 bits");

/** Number of data bytes in a CSR subregister (1 or 4). */
#define LITEX_CSR_DW_BYTES     (CONFIG_LITEX_CSR_DATA_WIDTH/8)
/** Address stride between consecutive CSR subregisters, in bytes. */
#define LITEX_CSR_OFFSET_BYTES 4

/**
 * @brief Calculate the number of subregisters needed for a CSR.
 *
 * @param csr_bytes Size of the CSR in bytes; must be greater than zero.
 *
 * @return Number of subregisters needed to hold the CSR.
 */
static inline size_t litex_num_subregs(size_t csr_bytes)
{
	return (csr_bytes - 1) / LITEX_CSR_DW_BYTES + 1;
}

/**
 * @brief Read an 8-bit CSR.
 *
 * @param addr Address of the first CSR subregister, aligned to 4 bytes.
 *
 * @return Value read from the CSR.
 */
static inline uint8_t litex_read8(mem_addr_t addr)
{
	return sys_read8(addr);
}

/**
 * @brief Read a 16-bit CSR.
 *
 * @param addr Address of the first CSR subregister, aligned to 4 bytes.
 *
 * @return Value read from the CSR.
 */
static inline uint16_t litex_read16(mem_addr_t addr)
{
#if CONFIG_LITEX_CSR_DATA_WIDTH == 8
#ifdef CONFIG_LITEX_CSR_ORDERING_BIG
	return (sys_read8(addr) << 8)
		| sys_read8(addr + 0x4);
#else
	return sys_read8(addr)
		| (sys_read8(addr + 0x4) << 8);
#endif
#else /* CONFIG_LITEX_CSR_DATA_WIDTH == 8 */
	return sys_read16(addr);
#endif
}

/**
 * @brief Read a 32-bit CSR.
 *
 * @param addr Address of the first CSR subregister, aligned to 4 bytes.
 *
 * @return Value read from the CSR.
 */
static inline uint32_t litex_read32(mem_addr_t addr)
{
#if CONFIG_LITEX_CSR_DATA_WIDTH == 8
#ifdef CONFIG_LITEX_CSR_ORDERING_BIG
	return (sys_read8(addr) << 24)
		| (sys_read8(addr + 0x4) << 16)
		| (sys_read8(addr + 0x8) << 8)
		| sys_read8(addr + 0xc);
#else
	return sys_read8(addr)
		| (sys_read8(addr + 0x4) << 8)
		| (sys_read8(addr + 0x8) << 16)
		| (sys_read8(addr + 0xc) << 24);
#endif
#else /* CONFIG_LITEX_CSR_DATA_WIDTH == 8 */
	return sys_read32(addr);
#endif
}

/**
 * @brief Read a 64-bit CSR.
 *
 * @param addr Address of the first CSR subregister, aligned to 4 bytes.
 *
 * @return Value read from the CSR.
 */
static inline uint64_t litex_read64(mem_addr_t addr)
{
#if CONFIG_LITEX_CSR_DATA_WIDTH == 8
#ifdef CONFIG_LITEX_CSR_ORDERING_BIG
	return (((uint64_t)sys_read8(addr)) << 56)
		| ((uint64_t)sys_read8(addr + 0x4) << 48)
		| ((uint64_t)sys_read8(addr + 0x8) << 40)
		| ((uint64_t)sys_read8(addr + 0xc) << 32)
		| ((uint64_t)sys_read8(addr + 0x10) << 24)
		| ((uint64_t)sys_read8(addr + 0x14) << 16)
		| ((uint64_t)sys_read8(addr + 0x18) << 8)
		| (uint64_t)sys_read8(addr + 0x1c);
#else
	return (uint64_t)sys_read8(addr)
		| ((uint64_t)sys_read8(addr + 0x4) << 8)
		| ((uint64_t)sys_read8(addr + 0x8) << 16)
		| ((uint64_t)sys_read8(addr + 0xc) << 24)
		| ((uint64_t)sys_read8(addr + 0x10) << 32)
		| ((uint64_t)sys_read8(addr + 0x14) << 40)
		| ((uint64_t)sys_read8(addr + 0x18) << 48)
		| ((uint64_t)sys_read8(addr + 0x1c) << 56);
#endif
#else /* CONFIG_LITEX_CSR_DATA_WIDTH == 8 */
#ifdef CONFIG_LITEX_CSR_ORDERING_BIG
	return ((uint64_t)sys_read32(addr) << 32) | (uint64_t)sys_read32(addr + 0x4);
#else
	return sys_read64(addr);
#endif
#endif
}

/**
 * @brief Write an 8-bit CSR.
 *
 * @param value Value to write to the CSR.
 * @param addr Address of the first CSR subregister, aligned to 4 bytes.
 */
static inline void litex_write8(uint8_t value, mem_addr_t addr)
{
	sys_write8(value, addr);
}

/**
 * @brief Write a 16-bit CSR.
 *
 * @param value Value to write to the CSR.
 * @param addr Address of the first CSR subregister, aligned to 4 bytes.
 */
static inline void litex_write16(uint16_t value, mem_addr_t addr)
{
#if CONFIG_LITEX_CSR_DATA_WIDTH == 8
#ifdef CONFIG_LITEX_CSR_ORDERING_BIG
	sys_write8(value >> 8, addr);
	sys_write8(value, addr + 0x4);
#else
	sys_write8(value, addr);
	sys_write8(value >> 8, addr + 0x4);
#endif
#else /* CONFIG_LITEX_CSR_DATA_WIDTH == 8 */
	sys_write16(value, addr);
#endif
}

/**
 * @brief Write a 32-bit CSR.
 *
 * @param value Value to write to the CSR.
 * @param addr Address of the first CSR subregister, aligned to 4 bytes.
 */
static inline void litex_write32(uint32_t value, mem_addr_t addr)
{
#if CONFIG_LITEX_CSR_DATA_WIDTH == 8
#ifdef CONFIG_LITEX_CSR_ORDERING_BIG
	sys_write8(value >> 24, addr);
	sys_write8(value >> 16, addr + 0x4);
	sys_write8(value >> 8, addr + 0x8);
	sys_write8(value, addr + 0xC);
#else
	sys_write8(value, addr);
	sys_write8(value >> 8, addr + 0x4);
	sys_write8(value >> 16, addr + 0x8);
	sys_write8(value >> 24, addr + 0xC);
#endif
#else /* CONFIG_LITEX_CSR_DATA_WIDTH == 8 */
	sys_write32(value, addr);
#endif
}

/**
 * @brief Write a 64-bit CSR.
 *
 * @param value Value to write to the CSR.
 * @param addr Address of the first CSR subregister, aligned to 4 bytes.
 */
static inline void litex_write64(uint64_t value, mem_addr_t addr)
{
#if CONFIG_LITEX_CSR_DATA_WIDTH == 8
#ifdef CONFIG_LITEX_CSR_ORDERING_BIG
	sys_write8(value >> 56, addr);
	sys_write8(value >> 48, addr + 0x4);
	sys_write8(value >> 40, addr + 0x8);
	sys_write8(value >> 32, addr + 0xC);
	sys_write8(value >> 24, addr + 0x10);
	sys_write8(value >> 16, addr + 0x14);
	sys_write8(value >> 8, addr + 0x18);
	sys_write8(value, addr + 0x1C);
#else
	sys_write8(value, addr);
	sys_write8(value >> 8, addr + 0x4);
	sys_write8(value >> 16, addr + 0x8);
	sys_write8(value >> 24, addr + 0xC);
	sys_write8(value >> 32, addr + 0x10);
	sys_write8(value >> 40, addr + 0x14);
	sys_write8(value >> 48, addr + 0x18);
	sys_write8(value >> 56, addr + 0x1C);
#endif
#else /* CONFIG_LITEX_CSR_DATA_WIDTH == 8 */
#ifdef CONFIG_LITEX_CSR_ORDERING_BIG
	sys_write32(value >> 32, addr);
	sys_write32(value, addr + 0x4);
#else
	sys_write64(value, addr);
#endif
#endif /* CONFIG_LITEX_CSR_DATA_WIDTH == 8 */
}

/**
 * @brief Write a CSR of the specified size.
 *
 * @param addr Address of the first CSR subregister, aligned to 4 bytes.
 * @param size CSR size in bytes: 1, 2, or 4. Other values cause no write.
 * @param value Value to write; only the least significant @p size bytes are used.
 */
static inline void litex_write(mem_addr_t addr, uint8_t size, uint32_t value)
{
	switch (size) {
	case 1:
		litex_write8(value, addr);
		break;
	case 2:
		litex_write16(value, addr);
		break;
	case 4:
		litex_write32(value, addr);
		break;
	default:
		break;
	}
}

/**
 * @brief Read a CSR of the specified size.
 *
 * @param addr Address of the first CSR subregister, aligned to 4 bytes.
 * @param size CSR size in bytes: 1, 2, or 4.
 *
 * @return Value read from the CSR, zero-extended to 32 bits.
 * @retval 0 Unsupported size.
 */
static inline uint32_t litex_read(mem_addr_t addr, uint32_t size)
{
	switch (size) {
	case 1:
		return litex_read8(addr);
	case 2:
		return litex_read16(addr);
	case 4:
		return litex_read32(addr);
	default:
		return 0;
	}
}

/**
 * @brief Write consecutive 32-bit CSR words.
 *
 * The buffer stores the least significant word at index zero. With big-endian
 * CSR ordering, the highest buffer index is accessed at the lowest CSR address.
 *
 * @param addr Address of the first CSR subregister, aligned to 4 bytes.
 * @param[in] buf Caller-owned buffer containing at least @p cnt words to write.
 *            Must remain valid until this function returns; may be NULL if @p cnt is zero.
 * @param cnt Number of 32-bit words to write.
 */
static inline void litex_write32_array(mem_addr_t addr, uint32_t *buf, size_t cnt)
{
	size_t i;

	for (i = 0; i < cnt; i++) {
#ifdef CONFIG_LITEX_CSR_ORDERING_BIG
		litex_write32(buf[cnt - 1 - i], addr);
#else
		litex_write32(buf[i], addr);
#endif
		addr += litex_num_subregs(sizeof(uint32_t)) * LITEX_CSR_OFFSET_BYTES;
	}
}

/**
 * @brief Read consecutive 32-bit CSR words.
 *
 * The buffer stores the least significant word at index zero. With big-endian
 * CSR ordering, the highest buffer index is accessed at the lowest CSR address.
 *
 * @param addr Address of the first CSR subregister, aligned to 4 bytes.
 * @param[out] buf Caller-owned buffer with space for at least @p cnt words read.
 *            Must remain valid until this function returns; may be NULL if @p cnt is zero.
 * @param cnt Number of 32-bit words to read.
 */
static inline void litex_read32_array(mem_addr_t addr, uint32_t *buf, size_t cnt)
{
	size_t i;

	for (i = 0; i < cnt; i++) {
#ifdef CONFIG_LITEX_CSR_ORDERING_BIG
		buf[cnt - 1 - i] = litex_read32(addr);
#else
		buf[i] = litex_read32(addr);
#endif
		addr += litex_num_subregs(sizeof(uint32_t)) * LITEX_CSR_OFFSET_BYTES;
	}
}

/** @} */

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_DRIVERS_MISC_LITEX_LITEX_CSR_H_ */
